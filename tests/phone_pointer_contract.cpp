#include <cassert>
#include <cstdio>

#include "parent_bridge.hpp"

namespace {

const PhoneDisplayMenuRow& selectionRow(const PhoneDisplayMenuLayout& layout, int selection) {
    const auto* row = phoneDisplayRowForSelection(layout, selection);
    assert(row != nullptr);
    return *row;
}

int hitCenter(const ParentBridge& bridge, const PhoneDisplayMenuRow& row) {
    return bridge.menuItemAt(row.hit.x + row.hit.w * 0.5f, row.hit.y + row.hit.h * 0.5f);
}

} // namespace

int main() {
    ParentBridge bridge;
    bridge.startRallyFixture();
    bridge.setPaused(true);
    bridge.tick();

    const auto pause = makePhoneDisplayMenuLayout(bridge.state());
    assert(pause.selectableCount == 3);
    const auto& resume = selectionRow(pause, 0);
    const auto& settings = selectionRow(pause, 1);
    assert(resume.label == "Resume" && settings.label == "Settings");
    assert(hitCenter(bridge, resume) == resume.selectableIndex);
    assert(hitCenter(bridge, settings) == settings.selectableIndex);
    assert(bridge.menuItemAt(2.0f, 2.0f) == -1);
    // A selectable row with an empty hit rectangle must never match (0, 0).
    PhoneDisplayMenuLayout empty_hit_layout;
    PhoneDisplayMenuRow hidden_row;
    hidden_row.selectable = true;
    hidden_row.visible = false;
    addPhoneDisplayRow(empty_hit_layout, hidden_row);
    assert(phoneDisplayItemAt(empty_hit_layout, 0.0f, 0.0f) == -1);
    assert(phoneDisplayItemAt(empty_hit_layout, 1.0f, 1.0f) == -1);

    bridge.setMenuSelection(settings.selectableIndex);
    assert(bridge.state().hud.menuSelection == settings.selectableIndex);
    bridge.setMenuSelection(999);
    assert(bridge.state().hud.menuSelection == settings.selectableIndex);
    assert(bridge.menuConfirm());
    bridge.setMenuSelection(0);
    assert(bridge.menuConfirm());

    const auto controls = makePhoneDisplayMenuLayout(bridge.state());
    bool checkedNonselectable = false;
    bool checkedPeek = false;
    bool checkedClipped = false;
    for (int i = 0; i < controls.rowCount; ++i) {
        const auto& row = controls.rows[i];
        if (!checkedNonselectable && row.visible && !row.selectable && !row.peek) {
            assert(bridge.menuItemAt(row.visual.x + row.visual.w * 0.5f,
                                     row.visual.y + row.visual.h * 0.5f) == -1);
            checkedNonselectable = true;
        }
        if (!checkedPeek && row.peek) {
            assert(bridge.menuItemAt(row.visual.x + row.visual.w * 0.5f,
                                     row.visual.y + row.visual.h * 0.5f) == -1);
            checkedPeek = true;
        }
        if (!checkedClipped && !row.visible) {
            assert(bridge.menuItemAt(row.visual.x + row.visual.w * 0.5f,
                                     row.visual.y + row.visual.h * 0.5f) == -1);
            checkedClipped = true;
        }
    }
    assert(checkedNonselectable && checkedPeek && checkedClipped);

    assert(bridge.menuBack());
    assert(bridge.menuBack());
    bridge.setMenuSelection(0);
    assert(bridge.state().hud.menuSelection == 0);
    assert(bridge.menuConfirm());
    assert(!bridge.state().uiPaused);
    bridge.setMenuSelection(1);
    assert(bridge.state().hud.menuSelection == 0);
    assert(bridge.menuItemAt(resume.hit.x + resume.hit.w * 0.5f,
                             resume.hit.y + resume.hit.h * 0.5f) == -1);

    std::puts("RESULT: PASS native phone hit rectangles, selection mutation, clipping, and inactive guards");
    return 0;
}
