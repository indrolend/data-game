#pragma once

// Platform-neutral iPhone touch mapping onto the shared Game::setTouchControls
// contract. Held controls (move stick, vacuum, sprint) stay asserted for every
// frame a finger is down; edge actions (jump, melee, shoot, camera) fire once
// per press. Multiple fingers are tracked independently by touch id.

#include <cmath>
#include <cstdint>
#include <vector>

namespace ios_touch {

enum class Button { Vacuum, Jump, Melee, Shoot, Sprint, Camera, Count };

struct Insets { float left = 0, top = 0, right = 0, bottom = 0; };

struct Circle { float x = 0, y = 0, r = 0; };

struct Output {
    float moveX = 0, moveZ = 0;
    float lookX = 0, lookY = 0;
    bool vacuumHeld = false, sprintHeld = false;
    bool jumpPressed = false, meleePressed = false, shootPressed = false, cameraPressed = false;
};

class TouchControls {
public:
    static constexpr float StickRadius = 58.0f;
    static constexpr float Deadzone = 0.12f;
    static constexpr float SprintThreshold = 0.92f;
    static constexpr float LookScale = 1.8f;

    void setLayout(float width, float height, Insets insets) {
        width_ = width; height_ = height; insets_ = insets;
        const float right = width - insets.right, bottom = height - insets.bottom;
        buttons_[static_cast<int>(Button::Vacuum)] = {right - 76, bottom - 96, 44};
        buttons_[static_cast<int>(Button::Jump)] = {right - 168, bottom - 60, 32};
        buttons_[static_cast<int>(Button::Melee)] = {right - 76, bottom - 202, 32};
        buttons_[static_cast<int>(Button::Shoot)] = {right - 168, bottom - 160, 32};
        buttons_[static_cast<int>(Button::Sprint)] = {right - 252, bottom - 108, 28};
        buttons_[static_cast<int>(Button::Camera)] = {right - 40, insets.top + 44, 24};
        if (!stickActive()) stickBase_ = defaultStickBase();
    }

    void touchBegan(std::uintptr_t id, float x, float y) {
        Touch t{id, Role::Look, Button::Count, x, y};
        for (int i = 0; i < static_cast<int>(Button::Count); ++i) {
            const Circle& c = buttons_[i];
            const float dx = x - c.x, dy = y - c.y;
            const float hit = c.r + 10.0f;
            if (dx * dx + dy * dy <= hit * hit) {
                t.role = Role::Button;
                t.button = static_cast<Button>(i);
                switch (t.button) {
                    case Button::Jump: jump_ = true; break;
                    case Button::Melee: melee_ = true; break;
                    case Button::Shoot: shoot_ = true; break;
                    case Button::Camera: camera_ = true; break;
                    default: break;
                }
                touches_.push_back(t);
                return;
            }
        }
        if (!stickActive() && inStickZone(x, y)) {
            t.role = Role::Stick;
            stickBase_ = clampToZone(x, y);
            stickThumb_ = stickBase_;
        }
        touches_.push_back(t);
    }

    void touchMoved(std::uintptr_t id, float x, float y) {
        for (Touch& t : touches_) {
            if (t.id != id) continue;
            if (t.role == Role::Stick) {
                stickThumb_ = {x, y};
            } else if (t.role == Role::Look) {
                lookX_ += (x - t.x) * LookScale;
                lookY_ += (y - t.y) * LookScale;
            }
            t.x = x; t.y = y;
            return;
        }
    }

    void touchEnded(std::uintptr_t id) {
        for (std::size_t i = 0; i < touches_.size(); ++i) {
            if (touches_[i].id != id) continue;
            if (touches_[i].role == Role::Stick) stickBase_ = defaultStickBase();
            touches_.erase(touches_.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
    }

    void releaseAll() {
        touches_.clear();
        stickBase_ = defaultStickBase();
        jump_ = melee_ = shoot_ = camera_ = false;
        lookX_ = lookY_ = 0.0f;
    }

    // Held state is sampled, one-shot edges and accumulated look are consumed.
    Output poll() {
        Output out;
        float sx = 0, sy = 0;
        if (stickActive()) {
            sx = (stickThumb_.x - stickBase_.x) / StickRadius;
            sy = (stickThumb_.y - stickBase_.y) / StickRadius;
            const float mag = std::sqrt(sx * sx + sy * sy);
            if (mag > 1.0f) { sx /= mag; sy /= mag; }
            const float clamped = mag > 1.0f ? 1.0f : mag;
            if (clamped < Deadzone) sx = sy = 0.0f;
            else if (clamped >= SprintThreshold) out.sprintHeld = true;
        }
        out.moveX = sx;
        out.moveZ = -sy;
        out.lookX = lookX_; out.lookY = lookY_;
        for (const Touch& t : touches_) {
            if (t.role != Role::Button) continue;
            if (t.button == Button::Vacuum) out.vacuumHeld = true;
            if (t.button == Button::Sprint) out.sprintHeld = true;
        }
        out.jumpPressed = jump_; out.meleePressed = melee_;
        out.shootPressed = shoot_; out.cameraPressed = camera_;
        lookX_ = lookY_ = 0.0f;
        jump_ = melee_ = shoot_ = camera_ = false;
        return out;
    }

    bool stickActive() const {
        for (const Touch& t : touches_) if (t.role == Role::Stick) return true;
        return false;
    }
    bool buttonHeld(Button b) const {
        for (const Touch& t : touches_) if (t.role == Role::Button && t.button == b) return true;
        return false;
    }
    const Circle& button(Button b) const { return buttons_[static_cast<int>(b)]; }
    Circle stickBase() const { return {stickBase_.x, stickBase_.y, StickRadius}; }
    Circle stickThumb() const { return {stickThumb_.x, stickThumb_.y, 24.0f}; }
    bool stickVisibleActive() const { return stickActive(); }
    float width() const { return width_; }
    float height() const { return height_; }

private:
    enum class Role { Look, Stick, Button };
    struct Touch { std::uintptr_t id; Role role; Button button; float x, y; };
    struct Point { float x = 0, y = 0; };

    bool inStickZone(float x, float y) const {
        const float safeW = width_ - insets_.left - insets_.right;
        const float safeH = height_ - insets_.top - insets_.bottom;
        return x < insets_.left + safeW * 0.45f && y > insets_.top + safeH * 0.35f;
    }
    Point clampToZone(float x, float y) const {
        const float minX = insets_.left + StickRadius * 0.6f;
        const float maxY = height_ - insets_.bottom - StickRadius * 0.6f;
        return {x < minX ? minX : x, y > maxY ? maxY : y};
    }
    Point defaultStickBase() const {
        return {insets_.left + 28.0f + StickRadius + 12.0f, height_ - insets_.bottom - 28.0f - StickRadius - 36.0f};
    }

    float width_ = 0, height_ = 0;
    Insets insets_;
    Circle buttons_[static_cast<int>(Button::Count)];
    std::vector<Touch> touches_;
    Point stickBase_, stickThumb_;
    float lookX_ = 0, lookY_ = 0;
    bool jump_ = false, melee_ = false, shoot_ = false, camera_ = false;
};

} // namespace ios_touch
