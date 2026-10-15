#include "MusicDirector.hpp"

#include <cassert>
#include <cstdio>

int main() {
    using namespace music_director;
    using namespace room_environment;

    assert(menuTheme(false, false) == MenuTheme::Title);
    assert(menuTheme(false, true) == MenuTheme::Pensive);
    assert(menuTheme(true, false) == MenuTheme::Pensive);

    RoomEnvironmentPlan field{};
    field.setting = RoomSetting::Field;
    RoomEnvironmentPlan city{};
    city.setting = RoomSetting::City;
    assert(stageTheme(field, 1) == StageTheme::Original);
    assert(stageTheme(city, 2) == StageTheme::Turnt);
    assert(stageTheme(field, 6) == StageTheme::Wacky);
    assert(stageTheme(city, 6) == StageTheme::Wacky);
    assert(stageTheme(field, 5, true) == StageTheme::Boss);

    std::puts("MUSIC_DIRECTOR_OK menu=TITLE_PENSIVE stage=ORIGINAL_TURNT_WACKY boss=RESERVED");
}
