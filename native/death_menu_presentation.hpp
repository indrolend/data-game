#pragma once

#include "Game.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

struct DeathMenuRect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

struct DeathMenuRow {
    const char* label = "";
    int selection = -1;
    float centerX = 0.0f;
    float baselineY = 0.0f;
    float textScale = 1.0f;
    DeathMenuRect hit;
};

struct DeathMenuPresentation {
    bool active = false;
    float uiScale = 1.0f;
    float canvasWidth = 1280.0f;
    float canvasHeight = 720.0f;
    float fade = 0.0f;
    float awaken = 0.0f;
    float pulse = 0.5f;
    int selection = 0;
    std::array<DeathMenuRow, 2> rows{};
};

inline DeathMenuPresentation makeDeathMenuPresentation(
        const GameState& state, float viewportWidth, float viewportHeight) {
    DeathMenuPresentation result;
    result.active = state.dead;
    result.uiScale = std::max(0.55f, std::min(1.8f,
        std::min(viewportWidth / 1280.0f, viewportHeight / 720.0f)));
    result.canvasWidth = viewportWidth / result.uiScale;
    result.canvasHeight = viewportHeight / result.uiScale;
    result.fade = clampf(state.cinematic.overlayFade, 0.0f, 1.0f);
    result.awaken = clampf(state.cinematic.restartAwaken, 0.0f, 1.0f);
    result.pulse = 0.5f + 0.5f * std::sin(state.time * 1.5f);
    result.selection = std::max(0, std::min(1, state.cinematic.deathChoice));

    const float centerX = result.canvasWidth * 0.5f;
    const float centerY = result.canvasHeight * 0.56f;
    const float againScale = 2.6f + result.awaken * 0.35f;
    const auto makeRow = [centerX](const char* label, int selection,
                                   float baselineY, float textScale) {
        DeathMenuRow row;
        row.label = label;
        row.selection = selection;
        row.centerX = centerX;
        row.baselineY = baselineY;
        row.textScale = textScale;
        const float textWidth = static_cast<float>(std::strlen(label)) * 6.0f * textScale;
        row.hit = {centerX - textWidth * 0.5f - 34.0f, baselineY - 18.0f,
            textWidth + 68.0f, 7.0f * textScale + 36.0f};
        return row;
    };
    result.rows[0] = makeRow("Again?", 0, centerY, againScale);
    result.rows[1] = makeRow("Quit", 1, centerY + 58.0f, 1.55f);
    return result;
}

inline int deathMenuItemAt(const DeathMenuPresentation& menu, float viewportX, float viewportY) {
    if (!menu.active) return -1;
    const float x = viewportX / menu.uiScale;
    const float y = viewportY / menu.uiScale;
    for (const auto& row : menu.rows) {
        if (x >= row.hit.x && x <= row.hit.x + row.hit.w &&
            y >= row.hit.y && y <= row.hit.y + row.hit.h) return row.selection;
    }
    return -1;
}
