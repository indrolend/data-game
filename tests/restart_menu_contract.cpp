#include <cassert>
#include <cstdio>
#include <string>

#include "parent_bridge.hpp"

namespace {
int hitCenter(const ParentBridge& bridge, const DeathMenuPresentation& menu, int row) {
    const auto& hit = menu.rows[row].hit;
    return bridge.deathMenuItemAt(
        (hit.x + hit.w * 0.5f) * menu.uiScale,
        (hit.y + hit.h * 0.5f) * menu.uiScale,
        menu.canvasWidth * menu.uiScale,
        menu.canvasHeight * menu.uiScale);
}
}

int main() {
    ParentBridge bridge;
    bridge.startDeathFixture();
    assert(bridge.state().dead);
    assert(!bridge.state().started);

    const auto menu = bridge.deathMenuPresentation(1280.0f, 720.0f);
    assert(menu.active && menu.selection == 0);
    assert(std::string(menu.rows[0].label) == "Again?");
    assert(std::string(menu.rows[1].label) == "Quit");
    assert(hitCenter(bridge, menu, 0) == 0);
    assert(hitCenter(bridge, menu, 1) == 1);
    assert(bridge.deathMenuItemAt(4.0f, 4.0f, 1280.0f, 720.0f) == -1);

    bridge.menuMoveSelection(1);
    assert(bridge.state().cinematic.deathChoice == 1);
    bridge.menuMoveSelection(1);
    assert(bridge.state().cinematic.deathChoice == 1);
    bridge.menuMoveSelection(-1);
    assert(bridge.state().cinematic.deathChoice == 0);
    bridge.setMenuSelection(7);
    assert(bridge.state().cinematic.deathChoice == 0);
    assert(bridge.menuConfirm());
    assert(!bridge.state().dead && bridge.state().started);
    assert(bridge.state().cinematic.restartAwaken > 0.99f);

    bridge.startDeathFixture();
    assert(bridge.state().dead);
    bridge.setMenuSelection(1);
    assert(bridge.menuConfirm());
    assert(!bridge.state().dead && !bridge.state().started);
    assert(bridge.state().cinematic.introActive);

    std::puts("RESULT: PASS native restart menu geometry, bounded selection, restart, and quit");
    return 0;
}
