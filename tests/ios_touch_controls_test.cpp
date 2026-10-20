#include "Game.hpp"
#include "TouchControls.hpp"

#include <cmath>
#include <cstdio>

using ios_touch::Button;
using ios_touch::Output;
using ios_touch::TouchControls;

namespace {
bool check(bool ok, const char* name) {
    if (!ok) std::fprintf(stderr, "IOS_TOUCH_CONTROLS_FAIL %s\n", name);
    return ok;
}
void apply(Game& game, const Output& o) {
    game.setTouchControls(o.moveX, o.moveZ, o.lookX, o.lookY, o.vacuumHeld, o.sprintHeld,
                          o.jumpPressed, o.meleePressed, o.shootPressed, o.cameraPressed);
}
}

int main() {
    bool ok = true;
    TouchControls controls;
    controls.setLayout(844.0f, 390.0f, {47, 0, 47, 21});
    const auto vac = controls.button(Button::Vacuum);
    const auto jump = controls.button(Button::Jump);

    // Stick in the lower-left zone: drag up == forward, held across polls.
    controls.touchBegan(1, 120, 300);
    controls.touchMoved(1, 120, 300 - 40);
    Output a = controls.poll(), b = controls.poll();
    ok &= check(a.moveZ > 0.5f && std::fabs(a.moveX) < 0.01f, "stick forward");
    ok &= check(b.moveZ == a.moveZ, "stick stays held across frames");
    ok &= check(!a.sprintHeld, "no sprint at partial deflection");

    // Second finger holds vacuum while the stick is still down (simultaneous).
    controls.touchBegan(2, vac.x, vac.y);
    Output c = controls.poll(), d = controls.poll();
    ok &= check(c.vacuumHeld && d.vacuumHeld && d.moveZ > 0.5f, "vacuum held with movement");

    // Third finger taps jump: one-shot edge, held controls unaffected.
    controls.touchBegan(3, jump.x, jump.y);
    Output e = controls.poll(), f = controls.poll();
    ok &= check(e.jumpPressed && !f.jumpPressed, "jump fires once");
    ok &= check(f.vacuumHeld && f.moveZ > 0.5f, "held state survives tap");
    controls.touchEnded(3);

    // Fourth finger on the right side drags the camera; delta is consumed once.
    controls.touchBegan(4, 600, 150);
    controls.touchMoved(4, 620, 140);
    Output g = controls.poll(), h = controls.poll();
    ok &= check(g.lookX > 0.0f && g.lookY < 0.0f && h.lookX == 0.0f, "look delta consumed once");

    // Releasing stick / vacuum stops those actions.
    controls.touchEnded(1);
    controls.touchEnded(2);
    Output i = controls.poll();
    ok &= check(i.moveX == 0.0f && i.moveZ == 0.0f && !i.vacuumHeld, "release stops held actions");

    // Full deflection sprints; edge of the dead zone does not move.
    controls.touchBegan(5, 100, 300);
    controls.touchMoved(5, 100, 300 - 80);
    ok &= check(controls.poll().sprintHeld, "full deflection sprints");
    controls.touchMoved(5, 102, 299);
    Output j = controls.poll();
    ok &= check(j.moveX == 0.0f && j.moveZ == 0.0f, "deadzone");
    controls.releaseAll();

    // The real Game consumes the held output every frame until release.
    Game game;
    game.reset();
    for (int n = 0; n < 5; ++n) game.update(1.0f / 60.0f);
    const Vec3 start = game.state().player.pos;
    apply(game, {0.0f, 1.0f, 0, 0, true, false, false, false, false, false});
    for (int n = 0; n < 60; ++n) {
        apply(game, {0.0f, 1.0f, 0, 0, true, false, false, false, false, false});
        game.update(1.0f / 60.0f);
    }
    const Vec3 moved = game.state().player.pos;
    ok &= check(length(moved - start) > 0.5f, "game player moves while forward held");
    ok &= check(game.state().input.touchPrimaryHeld, "game sees vacuum held");
    for (int n = 0; n < 60; ++n) {
        apply(game, {});
        game.update(1.0f / 60.0f);
    }
    ok &= check(!game.state().input.touchPrimaryHeld, "game sees vacuum released");
    const Vec3 stopped = game.state().player.pos;
    for (int n = 0; n < 30; ++n) { apply(game, {}); game.update(1.0f / 60.0f); }
    ok &= check(length(game.state().player.pos - stopped) < 0.05f, "game player stops after release");

    if (!ok) return 1;
    std::puts("IOS_TOUCH_CONTROLS_OK");
    return 0;
}
