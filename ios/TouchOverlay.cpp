#include "TouchOverlay.hpp"

#include "BitmapFont.hpp"
#include "GLShim.hpp"

#include <cmath>
#include <cstring>

namespace ios_touch {
namespace {

void disc(const Circle& c, float r, float g, float b, float a) {
    constexpr int segments = 28;
    glColor4f(r, g, b, a);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(c.x, c.y);
    for (int i = 0; i <= segments; ++i) {
        const float angle = static_cast<float>(i) * 6.2831853f / segments;
        glVertex2f(c.x + std::cos(angle) * c.r, c.y + std::sin(angle) * c.r);
    }
    glEnd();
}

void ring(const Circle& c, float r, float g, float b, float a) {
    disc(c, r, g, b, a);
    Circle inner = c;
    inner.r = c.r - 2.5f;
    disc(inner, 0.02f, 0.03f, 0.04f, 0.55f);
}

void label(const char* text, float cx, float cy, float scale, float r, float g, float b) {
    const std::size_t n = std::strlen(text);
    const float advance = 6.0f * scale;
    float pen = cx - (static_cast<float>(n) * advance - scale) * 0.5f;
    const float top = cy - 3.5f * scale;
    glColor4f(r, g, b, 0.95f);
    for (std::size_t i = 0; i < n; ++i, pen += advance) {
        const auto rows = bitmapGlyph(text[i]);
        for (int row = 0; row < 7; ++row)
            for (int col = 0; col < 5; ++col)
                if (rows[row] & (1u << (4 - col))) {
                    const float x = pen + col * scale, y = top + row * scale;
                    glBegin(GL_QUADS);
                    glVertex2f(x, y); glVertex2f(x + scale, y);
                    glVertex2f(x + scale, y + scale); glVertex2f(x, y + scale);
                    glEnd();
                }
    }
}

} // namespace

void drawOverlay(const TouchControls& controls) {
    if (controls.width() <= 0.0f || controls.height() <= 0.0f) return;
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING); glDisable(GL_FOG);
    glDisable(GL_CULL_FACE); glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glOrtho(0, controls.width(), controls.height(), 0, -1, 1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    const Circle base = controls.stickBase();
    ring(base, 0.35f, 0.85f, 0.95f, controls.stickVisibleActive() ? 0.45f : 0.22f);
    disc(controls.stickVisibleActive() ? controls.stickThumb() : Circle{base.x, base.y, 24.0f},
         0.55f, 0.95f, 1.0f, controls.stickVisibleActive() ? 0.75f : 0.35f);

    struct Item { Button button; const char* text; float r, g, b; };
    const Item items[] = {
        {Button::Vacuum, "VAC", 0.95f, 0.30f, 0.80f}, {Button::Jump, "JUMP", 0.60f, 0.95f, 0.35f},
        {Button::Melee, "HIT", 0.95f, 0.75f, 0.25f}, {Button::Shoot, "FIRE", 0.35f, 0.85f, 0.95f},
        {Button::Sprint, "RUN", 0.80f, 0.80f, 0.90f}, {Button::Camera, "CAM", 0.80f, 0.80f, 0.90f}};
    for (const Item& item : items) {
        const Circle& c = controls.button(item.button);
        const bool held = controls.buttonHeld(item.button);
        ring(c, item.r, item.g, item.b, held ? 0.85f : 0.38f);
        label(item.text, c.x, c.y, c.r > 40 ? 3.0f : 2.0f, 1, 1, 1);
    }
    glDepthMask(GL_TRUE); glDisable(GL_BLEND);
}

} // namespace ios_touch
