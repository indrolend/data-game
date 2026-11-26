#include "MusicDirector.hpp"

#include <cassert>
#include <cstdio>

int main() {
    using namespace music_director;
    using namespace room_environment;

    assert(menuMusicAllowed(false,false,false));
    assert(!menuMusicAllowed(true,false,false));
    assert(!menuMusicAllowed(false,true,false));
    // Keep the title voice selected through the attract simulation's death
    // and reset. A human player's death must not select the title voice.
    for (const bool dead : {false,true,false}) {
        assert(menuMusicAllowed(true,dead,true));
        assert(menuMusicAllowed(false,dead,true));
    }
    assert(!menuMusicAllowed(true,true,false));
    assert(menuMusicAllowed(true,false,true));
    assert(menuTheme(false) == MenuTheme::Title);
    assert(menuTheme(true) == MenuTheme::Pensive);

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
