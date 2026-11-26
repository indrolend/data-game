#pragma once

#include "DeveloperCodec.hpp"
#include "PhoneMenuModel.hpp"

enum class DeveloperMenuAction : unsigned char {
    None,
    OpenStatus,
    OpenRendering,
    OpenEnvironment,
    OpenPlaytest,
    OpenConsole,
    StateShow,
    CollidersToggle,
    BatteryFull,
    SoulSpawn,
    EnemiesToggle,
    ShadowModeCycle,
    ShadowDepthMapToggle,
    ShadowGeometryToggle,
    LightingShow,
    ShadowShow,
    ShadowReset,
    LightingReset,
    EnvironmentShow,
    EnvironmentCatalog,
    RoomNext,
    RoomReroll,
    PlaytestRally,
    PlaytestTraversal,
    PlaytestRooms,
    Back
};

struct DeveloperMenuEntry {
    const char* label = "";
    DeveloperMenuAction action = DeveloperMenuAction::None;
};

inline int developerMenuItemCount(DeveloperCodecState::Page page) {
    switch (page) {
        case DeveloperCodecState::Page::Root: return 5;
        case DeveloperCodecState::Page::Status: return 6;
        case DeveloperCodecState::Page::Rendering: return 8;
        case DeveloperCodecState::Page::Environment: return 5;
        case DeveloperCodecState::Page::Playtest: return 4;
    }
    return 0;
}

inline DeveloperMenuEntry developerMenuEntry(DeveloperCodecState::Page page, int selection) {
    static constexpr std::array<DeveloperMenuEntry, 5> Root{{
        {"STATUS", DeveloperMenuAction::OpenStatus},
        {"RENDERING", DeveloperMenuAction::OpenRendering},
        {"ENVIRONMENT", DeveloperMenuAction::OpenEnvironment},
        {"PLAYTEST LABS", DeveloperMenuAction::OpenPlaytest},
        {"COMMAND CONSOLE", DeveloperMenuAction::OpenConsole}
    }};
    static constexpr std::array<DeveloperMenuEntry, 6> Status{{
        {"STATE SNAPSHOT", DeveloperMenuAction::StateShow},
        {"COLLIDERS: TOGGLE", DeveloperMenuAction::CollidersToggle},
        {"BATTERY: FULL", DeveloperMenuAction::BatteryFull},
        {"SOUL: SPAWN", DeveloperMenuAction::SoulSpawn},
        {"ENEMIES: TOGGLE", DeveloperMenuAction::EnemiesToggle},
        {"BACK", DeveloperMenuAction::Back}
    }};
    static constexpr std::array<DeveloperMenuEntry, 8> Rendering{{
        {"SHADOW MODE: CYCLE", DeveloperMenuAction::ShadowModeCycle},
        {"DEPTH MAP: TOGGLE", DeveloperMenuAction::ShadowDepthMapToggle},
        {"GEOMETRY ONLY: TOGGLE", DeveloperMenuAction::ShadowGeometryToggle},
        {"LIGHTING STATUS", DeveloperMenuAction::LightingShow},
        {"SHADOW STATUS", DeveloperMenuAction::ShadowShow},
        {"RESET SHADOWS", DeveloperMenuAction::ShadowReset},
        {"RESET LIGHTING", DeveloperMenuAction::LightingReset},
        {"BACK", DeveloperMenuAction::Back}
    }};
    static constexpr std::array<DeveloperMenuEntry, 5> Environment{{
        {"CURRENT ROOM", DeveloperMenuAction::EnvironmentShow},
        {"ROOM CATALOG", DeveloperMenuAction::EnvironmentCatalog},
        {"NEXT ROOM", DeveloperMenuAction::RoomNext},
        {"REROLL ROOM", DeveloperMenuAction::RoomReroll},
        {"BACK", DeveloperMenuAction::Back}
    }};
    static constexpr std::array<DeveloperMenuEntry, 4> Playtest{{
        {"RALLY LAB", DeveloperMenuAction::PlaytestRally},
        {"TRAVERSAL LAB", DeveloperMenuAction::PlaytestTraversal},
        {"ROOM INSPECTOR", DeveloperMenuAction::PlaytestRooms},
        {"BACK", DeveloperMenuAction::Back}
    }};

    if (selection < 0 || selection >= developerMenuItemCount(page)) return {};
    switch (page) {
        case DeveloperCodecState::Page::Root: return Root[selection];
        case DeveloperCodecState::Page::Status: return Status[selection];
        case DeveloperCodecState::Page::Rendering: return Rendering[selection];
        case DeveloperCodecState::Page::Environment: return Environment[selection];
        case DeveloperCodecState::Page::Playtest: return Playtest[selection];
    }
    return {};
}

inline const char* developerMenuPageTitle(DeveloperCodecState::Page page) {
    switch (page) {
        case DeveloperCodecState::Page::Root: return "DEVELOPER";
        case DeveloperCodecState::Page::Status: return "STATUS";
        case DeveloperCodecState::Page::Rendering: return "RENDERING";
        case DeveloperCodecState::Page::Environment: return "ENVIRONMENT";
        case DeveloperCodecState::Page::Playtest: return "PLAYTEST LABS";
    }
    return "DEVELOPER";
}

inline PhoneMenuPageViewModel makeDeveloperPhonePage(const DeveloperCodecState& codec) {
    PhoneMenuPageViewModel page;
    page.title = developerMenuPageTitle(codec.page);
    page.navigationHint = "ARROWS / W S / CLICK: NAVIGATE  ENTER: SELECT  TAB: CONSOLE";

    for (int i = 0; i < developerMenuItemCount(codec.page); ++i) {
        addPhoneMenuItem(page, developerMenuEntry(codec.page, i).label, PhoneMenuAction::None);
    }

    return page;
}
