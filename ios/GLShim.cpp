#include "GLShim.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <functional>
#include <map>

namespace {

using Mat = std::array<float, 16>; // column-major, as in OpenGL

Mat identity() {
    Mat m{};
    m[0] = m[5] = m[10] = m[15] = 1.0f;
    return m;
}

Mat multiply(const Mat& a, const Mat& b) {
    Mat r{};
    for (int col = 0; col < 4; ++col)
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) sum += a[k * 4 + row] * b[col * 4 + k];
            r[col * 4 + row] = sum;
        }
    return r;
}

struct Light {
    bool enabled = false;
    float diffuse[3] = {0, 0, 0};
    float position[4] = {0, 0, 1, 0}; // eye space
    float constant = 1.0f, linear = 0.0f, quadratic = 0.0f;
};

struct State {
    Mat modelview = identity(), projection = identity();
    std::vector<Mat> modelviewStack, projectionStack;
    GLenum matrixMode = GL_MODELVIEW;
    float normalMatrix[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    bool normalMatrixDirty = false;

    float color[4] = {1, 1, 1, 1};
    float normal[3] = {0, 0, 1};
    float texcoord[2] = {0, 0};

    bool lighting = false, fog = false, blend = false, depthTest = false, cull = false;
    bool texture2D = false, stencilTest = false;
    bool depthWrite = true;
    bool colorWrite = true;
    GLenum cullFace = GL_BACK;
    bool lineMode = false;
    bool stencilEqual = false;
    float modelAmbient[3] = {0.2f, 0.2f, 0.2f};
    Light lights[8];
    float fogColor[3] = {0, 0, 0};
    float fogDensity = 1.0f;
    GLuint boundTexture = 0;

    GLenum primitive = 0;
    bool inBegin = false;
    std::vector<glshim::Vertex> pending;

    glshim::Frame frame;
    std::vector<std::function<void()>>* recording = nullptr;
};

State g;
std::map<GLuint, std::vector<std::function<void()>>> displayLists;
std::vector<glshim::Texture> textures(1);
GLuint nextList = 1;

Mat& currentMatrix() { return g.matrixMode == GL_PROJECTION ? g.projection : g.modelview; }
std::vector<Mat>& currentStack() { return g.matrixMode == GL_PROJECTION ? g.projectionStack : g.modelviewStack; }
void matrixChanged() { if (g.matrixMode == GL_MODELVIEW) g.normalMatrixDirty = true; }

void updateNormalMatrix() {
    // Cofactor matrix of the modelview 3x3 == det * inverse-transpose.
    const Mat& m = g.modelview;
    const float a = m[0], b = m[4], c = m[8];
    const float d = m[1], e = m[5], f = m[9];
    const float h = m[2], i = m[6], j = m[10];
    const float c00 = e * j - f * i, c01 = f * h - d * j, c02 = d * i - e * h;
    const float c10 = c * i - b * j, c11 = a * j - c * h, c12 = b * h - a * i;
    const float c20 = b * f - c * e, c21 = c * d - a * f, c22 = a * e - b * d;
    const float det = a * c00 + b * c01 + c * c02;
    const float s = det < 0.0f ? -1.0f : 1.0f;
    const float n[9] = {c00, c01, c02, c10, c11, c12, c20, c21, c22};
    for (int k = 0; k < 9; ++k) g.normalMatrix[k] = n[k] * s;
    g.normalMatrixDirty = false;
}

glshim::Texture* boundTexture() {
    return g.boundTexture < textures.size() ? &textures[g.boundTexture] : nullptr;
}

void emitVertex(float x, float y, float z) {
    if (g.normalMatrixDirty) updateNormalMatrix();
    const Mat& mv = g.modelview;
    const float ex = mv[0] * x + mv[4] * y + mv[8] * z + mv[12];
    const float ey = mv[1] * x + mv[5] * y + mv[9] * z + mv[13];
    const float ez = mv[2] * x + mv[6] * y + mv[10] * z + mv[14];
    float r = g.color[0], gg = g.color[1], b = g.color[2];
    if (g.lighting) {
        const float* nm = g.normalMatrix;
        float nx = nm[0] * g.normal[0] + nm[3] * g.normal[1] + nm[6] * g.normal[2];
        float ny = nm[1] * g.normal[0] + nm[4] * g.normal[1] + nm[7] * g.normal[2];
        float nz = nm[2] * g.normal[0] + nm[5] * g.normal[1] + nm[8] * g.normal[2];
        const float nl = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (nl > 1e-8f) { nx /= nl; ny /= nl; nz /= nl; }
        float lr = g.modelAmbient[0], lg = g.modelAmbient[1], lb = g.modelAmbient[2];
        for (const Light& light : g.lights) {
            if (!light.enabled) continue;
            float lx = light.position[0], ly = light.position[1], lz = light.position[2], atten = 1.0f;
            if (light.position[3] != 0.0f) {
                lx = lx / light.position[3] - ex;
                ly = ly / light.position[3] - ey;
                lz = lz / light.position[3] - ez;
                const float dist = std::sqrt(lx * lx + ly * ly + lz * lz);
                atten = 1.0f / std::max(1e-4f, light.constant + light.linear * dist + light.quadratic * dist * dist);
                if (dist > 1e-8f) { lx /= dist; ly /= dist; lz /= dist; }
            } else {
                const float len = std::sqrt(lx * lx + ly * ly + lz * lz);
                if (len > 1e-8f) { lx /= len; ly /= len; lz /= len; }
            }
            const float ndotl = std::max(0.0f, nx * lx + ny * ly + nz * lz) * atten;
            lr += light.diffuse[0] * ndotl;
            lg += light.diffuse[1] * ndotl;
            lb += light.diffuse[2] * ndotl;
        }
        r *= lr; gg *= lg; b *= lb;
    }
    if (g.fog) {
        const float d = g.fogDensity * std::fabs(ez);
        const float f = std::exp(-d * d);
        r = r * f + g.fogColor[0] * (1.0f - f);
        gg = gg * f + g.fogColor[1] * (1.0f - f);
        b = b * f + g.fogColor[2] * (1.0f - f);
    }
    const Mat& p = g.projection;
    glshim::Vertex v;
    v.x = p[0] * ex + p[4] * ey + p[8] * ez + p[12];
    v.y = p[1] * ex + p[5] * ey + p[9] * ez + p[13];
    v.z = p[2] * ex + p[6] * ey + p[10] * ez + p[14];
    v.w = p[3] * ex + p[7] * ey + p[11] * ez + p[15];
    v.r = std::min(1.0f, std::max(0.0f, r));
    v.g = std::min(1.0f, std::max(0.0f, gg));
    v.b = std::min(1.0f, std::max(0.0f, b));
    v.a = g.color[3];
    v.u = g.texcoord[0];
    v.v = g.texcoord[1];
    g.pending.push_back(v);
}

void appendTriangle(const glshim::Vertex& a, const glshim::Vertex& b, const glshim::Vertex& c,
                    std::uint32_t texture) {
    glshim::Frame& f = g.frame;
    const int cull = g.cull && g.cullFace == GL_BACK ? 1 : 0;
    const bool newBatch = f.batches.empty() || f.batches.back().texture != texture ||
        f.batches.back().blend != g.blend || f.batches.back().depthTest != g.depthTest ||
        f.batches.back().depthWrite != g.depthWrite || f.batches.back().cull != cull;
    if (newBatch) {
        glshim::Batch batch;
        batch.first = static_cast<std::uint32_t>(f.vertices.size());
        batch.texture = texture;
        batch.blend = g.blend;
        batch.depthTest = g.depthTest;
        batch.depthWrite = g.depthWrite;
        batch.cull = cull;
        f.batches.push_back(batch);
    }
    f.vertices.push_back(a);
    f.vertices.push_back(b);
    f.vertices.push_back(c);
    f.batches.back().count += 3;
}

template <typename Fn>
bool record(Fn&& fn) {
    if (!g.recording) return false;
    g.recording->push_back(std::forward<Fn>(fn));
    return true;
}

GLenum lightIndex(GLenum light) { return light - GL_LIGHT0; }

} // namespace

namespace glshim {

void beginFrame(int width, int height) {
    float clear[4];
    std::memcpy(clear, g.frame.clear, sizeof(clear));
    g = State{};
    std::memcpy(g.frame.clear, clear, sizeof(clear));
    g.lights[0].diffuse[0] = g.lights[0].diffuse[1] = g.lights[0].diffuse[2] = 1.0f;
    g.frame.viewportWidth = width;
    g.frame.viewportHeight = height;
}

const Frame& frame() { return g.frame; }

const Texture* texture(std::uint32_t id) { return id < textures.size() ? &textures[id] : nullptr; }

} // namespace glshim

void glClearColor(GLfloat r, GLfloat g_, GLfloat b, GLfloat a) {
    g.frame.clear[0] = r; g.frame.clear[1] = g_; g.frame.clear[2] = b; g.frame.clear[3] = a;
}
void glClear(GLbitfield) {}
void glClearStencil(GLint) {}
void glViewport(GLint, GLint, GLsizei, GLsizei) {}
void glFlush() {}

void glEnable(GLenum cap) {
    if (record([=] { glEnable(cap); })) return;
    switch (cap) {
        case GL_LIGHTING: g.lighting = true; break;
        case GL_FOG: g.fog = true; break;
        case GL_BLEND: g.blend = true; break;
        case GL_DEPTH_TEST: g.depthTest = true; break;
        case GL_CULL_FACE: g.cull = true; break;
        case GL_TEXTURE_2D: g.texture2D = true; break;
        case GL_STENCIL_TEST: g.stencilTest = true; break;
        default:
            if (cap >= GL_LIGHT0 && cap <= GL_LIGHT7) g.lights[lightIndex(cap)].enabled = true;
    }
}
void glDisable(GLenum cap) {
    if (record([=] { glDisable(cap); })) return;
    switch (cap) {
        case GL_LIGHTING: g.lighting = false; break;
        case GL_FOG: g.fog = false; break;
        case GL_BLEND: g.blend = false; break;
        case GL_DEPTH_TEST: g.depthTest = false; break;
        case GL_CULL_FACE: g.cull = false; break;
        case GL_TEXTURE_2D: g.texture2D = false; break;
        case GL_STENCIL_TEST: g.stencilTest = false; break;
        default:
            if (cap >= GL_LIGHT0 && cap <= GL_LIGHT7) g.lights[lightIndex(cap)].enabled = false;
    }
}
void glBlendFunc(GLenum, GLenum) {} // renderer only uses SRC_ALPHA / ONE_MINUS_SRC_ALPHA
void glDepthMask(GLboolean flag) {
    if (record([=] { glDepthMask(flag); })) return;
    g.depthWrite = flag != GL_FALSE;
}
void glColorMask(GLboolean r, GLboolean gr, GLboolean b, GLboolean a) {
    g.colorWrite = r || gr || b || a;
}
void glCullFace(GLenum mode) { g.cullFace = mode; }
void glPolygonMode(GLenum, GLenum mode) { g.lineMode = mode == GL_LINE; }
void glLineWidth(GLfloat) {}
// The shim has no stencil buffer: stencil-tested draws (directional shadow
// receivers) are dropped rather than drawn unmasked.
void glStencilFunc(GLenum func, GLint, GLuint) { g.stencilEqual = func == GL_EQUAL; }
void glStencilOp(GLenum, GLenum, GLenum) {}
void glStencilMask(GLuint) {}

void glMatrixMode(GLenum mode) { g.matrixMode = mode; }
void glLoadIdentity() { currentMatrix() = identity(); matrixChanged(); }
void glPushMatrix() { currentStack().push_back(currentMatrix()); }
void glPopMatrix() {
    auto& stack = currentStack();
    if (stack.empty()) return;
    currentMatrix() = stack.back();
    stack.pop_back();
    matrixChanged();
}
void glMultMatrixf(const GLfloat* m) {
    Mat other;
    std::memcpy(other.data(), m, sizeof(float) * 16);
    currentMatrix() = multiply(currentMatrix(), other);
    matrixChanged();
}
void glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
    Mat m = identity();
    m[12] = x; m[13] = y; m[14] = z;
    glMultMatrixf(m.data());
}
void glScalef(GLfloat x, GLfloat y, GLfloat z) {
    Mat m = identity();
    m[0] = x; m[5] = y; m[10] = z;
    glMultMatrixf(m.data());
}
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    const float len = std::sqrt(x * x + y * y + z * z);
    if (len < 1e-8f) return;
    x /= len; y /= len; z /= len;
    const float rad = angle * 3.14159265358979323846f / 180.0f;
    const float c = std::cos(rad), s = std::sin(rad), t = 1.0f - c;
    Mat m = identity();
    m[0] = t * x * x + c;     m[4] = t * x * y - s * z; m[8] = t * x * z + s * y;
    m[1] = t * x * y + s * z; m[5] = t * y * y + c;     m[9] = t * y * z - s * x;
    m[2] = t * x * z - s * y; m[6] = t * y * z + s * x; m[10] = t * z * z + c;
    glMultMatrixf(m.data());
}
void glOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) {
    Mat m = identity();
    m[0] = static_cast<float>(2.0 / (r - l));
    m[5] = static_cast<float>(2.0 / (t - b));
    m[10] = static_cast<float>(-2.0 / (f - n));
    m[12] = static_cast<float>(-(r + l) / (r - l));
    m[13] = static_cast<float>(-(t + b) / (t - b));
    m[14] = static_cast<float>(-(f + n) / (f - n));
    glMultMatrixf(m.data());
}
void glFrustum(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) {
    Mat m{};
    m[0] = static_cast<float>(2.0 * n / (r - l));
    m[5] = static_cast<float>(2.0 * n / (t - b));
    m[8] = static_cast<float>((r + l) / (r - l));
    m[9] = static_cast<float>((t + b) / (t - b));
    m[10] = static_cast<float>(-(f + n) / (f - n));
    m[11] = -1.0f;
    m[14] = static_cast<float>(-2.0 * f * n / (f - n));
    glMultMatrixf(m.data());
}

void glBegin(GLenum mode) {
    if (record([=] { glBegin(mode); })) return;
    g.primitive = mode;
    g.inBegin = true;
    g.pending.clear();
}
void glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
    if (record([=] { glVertex3f(x, y, z); })) return;
    if (g.inBegin) emitVertex(x, y, z);
}
void glVertex2f(GLfloat x, GLfloat y) { glVertex3f(x, y, 0.0f); }
void glEnd() {
    if (record([] { glEnd(); })) return;
    g.inBegin = false;
    const glshim::Texture* tex = g.texture2D ? boundTexture() : nullptr;
    const bool texturedOk = !g.texture2D || (tex && tex->valid);
    const bool drawable = !g.lineMode && g.colorWrite && !(g.stencilTest && g.stencilEqual) && texturedOk;
    const std::uint32_t texId = g.texture2D ? g.boundTexture : 0;
    const auto& v = g.pending;
    const std::size_t n = v.size();
    if (drawable) {
        switch (g.primitive) {
            case GL_TRIANGLES:
                for (std::size_t i = 0; i + 2 < n; i += 3) appendTriangle(v[i], v[i + 1], v[i + 2], texId);
                break;
            case GL_QUADS:
                for (std::size_t i = 0; i + 3 < n; i += 4) {
                    appendTriangle(v[i], v[i + 1], v[i + 2], texId);
                    appendTriangle(v[i], v[i + 2], v[i + 3], texId);
                }
                break;
            case GL_TRIANGLE_STRIP:
                for (std::size_t i = 0; i + 2 < n; ++i) {
                    if (i % 2 == 0) appendTriangle(v[i], v[i + 1], v[i + 2], texId);
                    else appendTriangle(v[i + 1], v[i], v[i + 2], texId);
                }
                break;
            case GL_TRIANGLE_FAN:
                for (std::size_t i = 1; i + 1 < n; ++i) appendTriangle(v[0], v[i], v[i + 1], texId);
                break;
            default: break; // points / line loops are debug-only in the renderer
        }
    }
    g.pending.clear();
}
void glNormal3f(GLfloat x, GLfloat y, GLfloat z) {
    if (record([=] { glNormal3f(x, y, z); })) return;
    g.normal[0] = x; g.normal[1] = y; g.normal[2] = z;
}
void glColor4f(GLfloat r, GLfloat gr, GLfloat b, GLfloat a) {
    if (record([=] { glColor4f(r, gr, b, a); })) return;
    g.color[0] = r; g.color[1] = gr; g.color[2] = b; g.color[3] = a;
}
void glColor3f(GLfloat r, GLfloat gr, GLfloat b) { glColor4f(r, gr, b, 1.0f); }
void glTexCoord2f(GLfloat s, GLfloat t) {
    if (record([=] { glTexCoord2f(s, t); })) return;
    g.texcoord[0] = s; g.texcoord[1] = t;
}

void glLightfv(GLenum light, GLenum pname, const GLfloat* p) {
    if (light < GL_LIGHT0 || light > GL_LIGHT7) return;
    Light& l = g.lights[lightIndex(light)];
    if (pname == GL_DIFFUSE) {
        l.diffuse[0] = p[0]; l.diffuse[1] = p[1]; l.diffuse[2] = p[2];
    } else if (pname == GL_POSITION) {
        // GL transforms light positions by the modelview current at call time.
        const Mat& m = g.modelview;
        for (int row = 0; row < 4; ++row)
            l.position[row] = m[row] * p[0] + m[4 + row] * p[1] + m[8 + row] * p[2] + m[12 + row] * p[3];
    }
}
void glLightf(GLenum light, GLenum pname, GLfloat param) {
    if (light < GL_LIGHT0 || light > GL_LIGHT7) return;
    Light& l = g.lights[lightIndex(light)];
    if (pname == GL_CONSTANT_ATTENUATION) l.constant = param;
    else if (pname == GL_LINEAR_ATTENUATION) l.linear = param;
    else if (pname == GL_QUADRATIC_ATTENUATION) l.quadratic = param;
}
void glLightModelfv(GLenum pname, const GLfloat* p) {
    if (pname == GL_LIGHT_MODEL_AMBIENT) for (int i = 0; i < 3; ++i) g.modelAmbient[i] = p[i];
}
void glFogfv(GLenum pname, const GLfloat* p) {
    if (pname == GL_FOG_COLOR) for (int i = 0; i < 3; ++i) g.fogColor[i] = p[i];
}
void glFogf(GLenum pname, GLfloat param) { if (pname == GL_FOG_DENSITY) g.fogDensity = param; }
void glFogi(GLenum, GLint) {} // only GL_EXP2 is used

GLuint glGenLists(GLsizei range) {
    const GLuint first = nextList;
    nextList += static_cast<GLuint>(std::max(1, range));
    return first;
}
void glNewList(GLuint list, GLenum) {
    auto& ops = displayLists[list];
    ops.clear();
    g.recording = &ops;
}
void glEndList() { g.recording = nullptr; }
void glCallList(GLuint list) {
    if (record([=] { glCallList(list); })) return;
    const auto it = displayLists.find(list);
    if (it == displayLists.end()) return;
    for (const auto& op : it->second) op();
}

void glGenTextures(GLsizei n, GLuint* out) {
    for (GLsizei i = 0; i < n; ++i) {
        textures.emplace_back();
        out[i] = static_cast<GLuint>(textures.size() - 1);
    }
}
void glBindTexture(GLenum, GLuint texture) { g.boundTexture = texture; }
void glTexParameteri(GLenum, GLenum pname, GLint param) {
    glshim::Texture* t = boundTexture();
    if (!t) return;
    if (pname == GL_TEXTURE_WRAP_S) t->clamp = param == GL_CLAMP;
    else if (pname == GL_TEXTURE_MAG_FILTER) t->linear = param != GL_NEAREST;
}
void glPixelStorei(GLenum, GLint) {}

namespace {
void storePixels(glshim::Texture& t, int xo, int yo, int w, int h, GLenum format, const void* pixels) {
    if (!pixels || w <= 0 || h <= 0 || t.width <= 0 || t.height <= 0) return;
    const int channels = format == GL_RGB ? 3 : 4;
    const auto* src = static_cast<const std::uint8_t*>(pixels);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            const int dx = xo + x, dy = yo + y;
            if (dx < 0 || dy < 0 || dx >= t.width || dy >= t.height) continue;
            const std::uint8_t* s = src + (static_cast<std::size_t>(y) * w + x) * channels;
            std::uint8_t* d = &t.rgba[(static_cast<std::size_t>(dy) * t.width + dx) * 4];
            d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = channels == 4 ? s[3] : 255;
        }
    t.valid = true;
    ++t.version;
}
} // namespace

void glTexImage2D(GLenum, GLint, GLint, GLsizei width, GLsizei height, GLint, GLenum format,
                  GLenum, const void* pixels) {
    glshim::Texture* t = boundTexture();
    if (!t || t == &textures[0]) return;
    t->width = width;
    t->height = height;
    t->rgba.assign(static_cast<std::size_t>(std::max(0, width)) * std::max(0, height) * 4, 0);
    t->valid = false;
    ++t->version;
    storePixels(*t, 0, 0, width, height, format, pixels);
}
void glTexSubImage2D(GLenum, GLint, GLint xo, GLint yo, GLsizei w, GLsizei h, GLenum format, GLenum,
                     const void* pixels) {
    glshim::Texture* t = boundTexture();
    if (t && t != &textures[0]) storePixels(*t, xo, yo, w, h, format, pixels);
}
// Framebuffer read-back (door data-mosh) has no shim equivalent; textures fed
// only by it stay invalid and their draws are skipped.
void glCopyTexImage2D(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint) {}
void glCopyTexSubImage2D(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei) {}
