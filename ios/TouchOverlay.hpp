#pragma once

#include "TouchControls.hpp"

namespace ios_touch {
// Draws the stick/buttons through the same GL entry points as the game
// renderer, in point coordinates, after DesktopRenderer::draw().
void drawOverlay(const TouchControls& controls);
}
