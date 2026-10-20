// Renders the real Game state through the real DesktopRenderer, compiled
// against the iOS GL shim, then rasterises the shim triangle stream on the CPU
// to prove the frame contains the game world rather than a uniform clear.
#include "DesktopRenderer.hpp"
#include "GLShim.hpp"
#include "TouchOverlay.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <vector>

namespace {

struct Image {
    int w, h;
    std::vector<float> rgb, depth;
};

Image rasterize(const glshim::Frame& frame) {
    Image img{frame.viewportWidth, frame.viewportHeight, {}, {}};
    img.rgb.resize(static_cast<std::size_t>(img.w) * img.h * 3);
    img.depth.assign(static_cast<std::size_t>(img.w) * img.h, 1e9f);
    for (std::size_t i = 0; i < img.rgb.size(); i += 3) {
        img.rgb[i] = frame.clear[0]; img.rgb[i + 1] = frame.clear[1]; img.rgb[i + 2] = frame.clear[2];
    }
    for (const glshim::Batch& batch : frame.batches) {
        if (batch.texture) continue; // textured quads are covered by the Metal path
        for (std::uint32_t t = batch.first; t + 2 < batch.first + batch.count; t += 3) {
            const glshim::Vertex* v = &frame.vertices[t];
            if (v[0].w <= 1e-4f || v[1].w <= 1e-4f || v[2].w <= 1e-4f) continue;
            float sx[3], sy[3], sz[3];
            for (int k = 0; k < 3; ++k) {
                sx[k] = (v[k].x / v[k].w * 0.5f + 0.5f) * img.w;
                sy[k] = (1.0f - (v[k].y / v[k].w * 0.5f + 0.5f)) * img.h;
                sz[k] = v[k].z / v[k].w;
            }
            const float area = (sx[1] - sx[0]) * (sy[2] - sy[0]) - (sx[2] - sx[0]) * (sy[1] - sy[0]);
            if (std::fabs(area) < 1e-6f) continue;
            if (batch.cull && area > 0) continue; // y flipped: CCW in NDC is area < 0 here
            const int x0 = std::max(0, static_cast<int>(std::floor(std::min({sx[0], sx[1], sx[2]}))));
            const int x1 = std::min(img.w - 1, static_cast<int>(std::ceil(std::max({sx[0], sx[1], sx[2]}))));
            const int y0 = std::max(0, static_cast<int>(std::floor(std::min({sy[0], sy[1], sy[2]}))));
            const int y1 = std::min(img.h - 1, static_cast<int>(std::ceil(std::max({sy[0], sy[1], sy[2]}))));
            for (int y = y0; y <= y1; ++y)
                for (int x = x0; x <= x1; ++x) {
                    const float px = x + 0.5f, py = y + 0.5f;
                    const float w0 = ((sx[1] - px) * (sy[2] - py) - (sx[2] - px) * (sy[1] - py)) / area;
                    const float w1 = ((sx[2] - px) * (sy[0] - py) - (sx[0] - px) * (sy[2] - py)) / area;
                    const float w2 = 1.0f - w0 - w1;
                    if (w0 < 0 || w1 < 0 || w2 < 0) continue;
                    const float z = w0 * sz[0] + w1 * sz[1] + w2 * sz[2];
                    if (z < -1.0f || z > 1.0f) continue;
                    const std::size_t p = static_cast<std::size_t>(y) * img.w + x;
                    if (batch.depthTest && z > img.depth[p]) continue;
                    const float a = batch.blend ? w0 * v[0].a + w1 * v[1].a + w2 * v[2].a : 1.0f;
                    const float col[3] = {w0 * v[0].r + w1 * v[1].r + w2 * v[2].r,
                                          w0 * v[0].g + w1 * v[1].g + w2 * v[2].g,
                                          w0 * v[0].b + w1 * v[1].b + w2 * v[2].b};
                    for (int c = 0; c < 3; ++c) img.rgb[p * 3 + c] = img.rgb[p * 3 + c] * (1 - a) + col[c] * a;
                    if (batch.depthWrite) img.depth[p] = z;
                }
        }
    }
    return img;
}

bool check(bool ok, const char* name) {
    if (!ok) std::fprintf(stderr, "IOS_GL_SHIM_FAIL %s\n", name);
    return ok;
}

} // namespace

int main() {
    DesktopRenderer renderer;
    renderer.setAssetRoot(std::filesystem::path(DB_TEST_MODEL_ROOT));
    constexpr int W = 390, H = 844;
    renderer.resize(W, H);

    Game game;
    game.reset();
    for (int i = 0; i < 90; ++i) {
        game.setTouchControls(0, 0.8f, 0, 0, false, false, false, false, false, false);
        game.update(1.0f / 60.0f);
    }

    glshim::beginFrame(W, H);
    renderer.draw(game.state());
    const glshim::Frame& frame = glshim::frame();

    bool ok = true;
    ok &= check(frame.vertices.size() > 300, "real scene emits geometry");
    ok &= check(!frame.batches.empty(), "batches recorded");
    std::size_t counted = 0;
    for (const auto& b : frame.batches) counted += b.count;
    ok &= check(counted == frame.vertices.size(), "batch counts cover the vertex stream");
    bool finite = true;
    for (const auto& v : frame.vertices)
        finite &= std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) && std::isfinite(v.w) &&
                  std::isfinite(v.r) && std::isfinite(v.g) && std::isfinite(v.b);
    ok &= check(finite, "vertices finite");

    const Image img = rasterize(frame);
    std::set<int> colors;
    int nonBackground = 0;
    for (std::size_t p = 0; p < img.rgb.size(); p += 3) {
        const float dr = img.rgb[p] - frame.clear[0], dg = img.rgb[p + 1] - frame.clear[1], db = img.rgb[p + 2] - frame.clear[2];
        if (std::fabs(dr) + std::fabs(dg) + std::fabs(db) > 0.03f) ++nonBackground;
        colors.insert((static_cast<int>(img.rgb[p] * 15) << 8) | (static_cast<int>(img.rgb[p + 1] * 15) << 4) |
                      static_cast<int>(img.rgb[p + 2] * 15));
    }
    const double coverage = static_cast<double>(nonBackground) / (img.w * img.h);
    std::printf("vertices=%zu batches=%zu coverage=%.3f colors=%zu\n", frame.vertices.size(),
                frame.batches.size(), coverage, colors.size());
    ok &= check(coverage > 0.10, "world covers a meaningful part of the frame");
    ok &= check(colors.size() > 12, "frame is not near-uniform");

    if (const char* dump = std::getenv("DB_IOS_SHIM_PPM")) {
        if (FILE* f = std::fopen(dump, "wb")) {
            std::fprintf(f, "P6\n%d %d\n255\n", img.w, img.h);
            for (float v : img.rgb) std::fputc(static_cast<int>(std::min(1.0f, std::max(0.0f, v)) * 255), f);
            std::fclose(f);
        }
    }

    // The control overlay is drawn through the same shim.
    ios_touch::TouchControls controls;
    controls.setLayout(W, H, {0, 47, 0, 34});
    const std::size_t before = frame.vertices.size();
    ios_touch::drawOverlay(controls);
    ok &= check(glshim::frame().vertices.size() > before, "touch overlay emits geometry");

    if (!ok) return 1;
    std::puts("IOS_GL_SHIM_OK");
    return 0;
}
