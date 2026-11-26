#include "DeveloperPhoneMenu.hpp"
#include "PhoneDisplayLayout.hpp"

#include <cassert>

int main() {
    DeveloperCodecState codec;
    const std::array<DeveloperCodecState::Page, 5> pages{{
        DeveloperCodecState::Page::Root,
        DeveloperCodecState::Page::Status,
        DeveloperCodecState::Page::Rendering,
        DeveloperCodecState::Page::Environment,
        DeveloperCodecState::Page::Playtest
    }};

    for (const auto page : pages) {
        codec.page = page;
        const auto model = makeDeveloperPhonePage(codec);
        const auto layout = makePhoneDisplayMenuLayout(model, 0.0f);
        assert(model.selectableCount == developerMenuItemCount(page));
        assert(layout.selectableCount == developerMenuItemCount(page));
        assert(model.elementCount <= PhoneMenuPageViewModel::MaxElements);
        for (int i = 0; i < model.selectableCount; ++i) {
            assert(phoneMenuElementForSelection(model, i) != nullptr);
            assert(phoneDisplayRowForSelection(layout, i) != nullptr);
            assert(developerMenuEntry(page, i).action != DeveloperMenuAction::None);
        }
        assert(developerMenuEntry(page, -1).action == DeveloperMenuAction::None);
        assert(developerMenuEntry(page, model.selectableCount).action == DeveloperMenuAction::None);
    }

    assert(developerMenuEntry(DeveloperCodecState::Page::Root, 0).action == DeveloperMenuAction::OpenStatus);
    assert(developerMenuEntry(DeveloperCodecState::Page::Rendering, 7).action == DeveloperMenuAction::Back);
    assert(developerMenuEntry(DeveloperCodecState::Page::Environment, 4).action == DeveloperMenuAction::Back);
    assert(developerMenuEntry(DeveloperCodecState::Page::Playtest, 3).action == DeveloperMenuAction::Back);
    return 0;
}
