#pragma once
#include "Game.hpp"
// No replacement state or simulation. This is a thin adapter over original Game.
class ParentBridge {
public:
    ParentBridge() { game.reset(); }
    void input(float x, float z, float look_x, float look_y, bool vacuum, bool sprint,
               bool jump, bool melee, bool shoot, bool camera) {
        game.setTouchControls(x,z,look_x,look_y,vacuum,sprint,jump,melee,shoot,camera);
    }
    void tick() { game.update(1.0f/60.0f); }
    void setCommSignal(int signal) { game.setCommSignal(signal); }
    void setPaused(bool paused) { game.setUiPaused(paused); }
    // Verification fixture delegates setup to the original Game debug scenario.
    void startRallyFixture() { game.debugStartRallyLab(); }
    void startTargetFixture() { game.debugStartRoomInspector(); game.debugToggleRoomInspectorEnemies(); }
    void startTargetInteractionFixture() { game.reset(); }
    void refillTargetInteractionBattery() { game.debugFillBattery(); }
    const GameState& state() const { return game.state(); }
private:
    Game game;
};
