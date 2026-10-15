#pragma once

#include "RoomEnvironment.hpp"

namespace music_director {

enum class MenuTheme : unsigned char { Title, Pensive, Count };
enum class StageTheme : unsigned char { Original, Turnt, Wacky, Boss, Count };

constexpr MenuTheme menuTheme(bool attractMode, bool submenuOpen) noexcept {
    return attractMode || submenuOpen ? MenuTheme::Pensive : MenuTheme::Title;
}

constexpr StageTheme stageTheme(
    const room_environment::RoomEnvironmentPlan& room,
    int roomIndex,
    bool bossActive = false) noexcept {
    if (bossActive) return StageTheme::Boss;
    if (roomIndex > 2 && roomIndex % 6 == 0) return StageTheme::Wacky;
    if (room.setting == room_environment::RoomSetting::City) return StageTheme::Turnt;
    return StageTheme::Original;
}

} // namespace music_director
