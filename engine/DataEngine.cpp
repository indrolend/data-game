#include "DataEngine.hpp"

#include "MultiplayerProtocol.hpp"
#include "gameplay/TargetRoles.hpp"

#include <cstdio>

namespace data_engine {
namespace {

std::uint64_t fnv1a(const char* text) {
    std::uint64_t hash = 1469598103934665603ull;
    while (*text) {
        hash ^= static_cast<unsigned char>(*text++);
        hash *= 1099511628211ull;
    }
    return hash;
}

int filledGoals(const GameState& state) {
    int filled = 0;
    for (int index = 0; index < state.requiredSouls; ++index) {
        if (state.captures[index].filled) ++filled;
    }
    return filled;
}

}  // namespace

void DataEngine::reset() {
    game_.reset();
    events_.clear();
    hasPrevious_ = false;
    captureEvents();
}

void DataEngine::startGeneratedScenario(int seed, int room) {
    game_.debugStartGeneratedRoomFixture(seed, room);
    events_.clear();
    hasPrevious_ = false;
    captureEvents();
}

bool DataEngine::prepareReferenceEncounter() {
    GameState& state = game_.networkMutableState();
    for (TargetState& target : state.targets) target = TargetState{};
    for (HumanRespawnRequest& request : state.respawnQueue) request = HumanRespawnRequest{};
    state.cinematic = CinematicState{};
    state.started = true;
    state.dead = false;
    state.uiPaused = false;
    state.player.battery = 100.0f;
    state.player.vel = {};
    state.player.jumpVel = 0.0f;
    state.player.grounded = true;
    state.player.grabbedByTarget = -1;
    state.camera.yaw = 0.0f;
    state.camera.pitch = 0.0f;
    state.camera.forward = {0.0f, 0.0f, -1.0f};
    TargetState& target = state.targets[0];
    target.alive = true;
    target.armor = 0.10f;
    target.health = 1.0f;
    target.scale = 1.0f;
    target.attackCooldown = 999.0f;
    target.pos = state.player.pos + Vec3{0.0f, 0.0f, -0.75f};
    target.walkTarget = target.pos;
    events_.clear();
    hasPrevious_ = false;
    captureEvents();
    return gameplay::isActiveHuman(target);
}

bool DataEngine::depositReferenceGoal(int goalIndex) {
    GameState& state = game_.networkMutableState();
    if (goalIndex < 0 || goalIndex >= state.requiredSouls || goalIndex >= CAPTURE_COUNT) return false;
    constexpr float roomDepth = 42.0f;
    const float tileOrigin = static_cast<float>(state.topology.currentTileIndex) * roomDepth;
    BulletState& bullet = state.bullets[0];
    bullet = BulletState{};
    bullet.alive = true;
    bullet.life = 1.0f;
    bullet.pos = state.captures[goalIndex].pos + Vec3{0.0f, 0.0f, tileOrigin + 1.9f};
    bullet.vel = {0.0f, 0.0f, -25.0f};
    step(Command{});
    return game_.state().captures[goalIndex].filled;
}

bool DataEngine::crossReferenceExit() {
    GameState& state = game_.networkMutableState();
    constexpr float roomDepth = 42.0f;
    const float tileOrigin = static_cast<float>(state.topology.currentTileIndex) * roomDepth;
    state.player.pos = {0.0f, PHONE_MODEL_HEIGHT * 0.5f, tileOrigin - 20.8f};
    state.player.vel = {0.0f, 0.0f, -20.0f};
    state.player.grounded = true;
    step(Command{});
    step(Command{});
    return game_.state().upgradeMenu.active;
}

bool DataEngine::chooseReferenceUpgrade(int track) {
    const bool chosen = game_.chooseTemporaryUpgrade(track);
    captureEvents();
    return chosen;
}

Observation DataEngine::step(const Command& command) {
    game_.setTouchControls(
        command.moveX, command.moveZ, command.lookX, command.lookY,
        command.vacuum, command.sprint, command.jump, command.melee,
        command.shoot, command.cameraToggle);
    game_.update(EngineContract::fixedStepSeconds);
    captureEvents();
    return observe();
}

Observation DataEngine::observe() const {
    const GameState& state = game_.state();
    return Observation{
        state.frame,
        state.roomIndex,
        state.player.souls,
        filledGoals(state),
        state.requiredSouls,
        state.started,
        state.dead,
        state.roomClear,
        state.upgradeMenu.active,
        dbnet::authoritativeStateDigestHex(state),
    };
}

std::string DataEngine::contractDigest() {
    char text[17]{};
    std::snprintf(text, sizeof(text), "%016llx",
                  static_cast<unsigned long long>(fnv1a(EngineContract::identity())));
    return text;
}

const char* DataEngine::eventName(EventType type) {
    switch (type) {
        case EventType::RunStarted: return "run_started";
        case EventType::SoulCaptured: return "soul_captured";
        case EventType::GoalFilled: return "goal_filled";
        case EventType::RoomCleared: return "room_cleared";
        case EventType::RoomChanged: return "room_changed";
        case EventType::UpgradeOffered: return "upgrade_offered";
        case EventType::PlayerDied: return "player_died";
        case EventType::PlayerRestarted: return "player_restarted";
    }
    return "unknown";
}

void DataEngine::captureEvents() {
    const GameState& state = game_.state();
    if (!hasPrevious_) {
        if (state.started) events_.push_back({EventType::RunStarted, state.frame, state.roomIndex});
        rememberState();
        return;
    }
    if (!previousStarted_ && state.started) {
        events_.push_back({EventType::RunStarted, state.frame, state.roomIndex});
    }
    if (state.player.souls > previousSouls_) {
        events_.push_back({EventType::SoulCaptured, state.frame, state.player.souls - previousSouls_});
    }
    const int currentFilledGoals = filledGoals(state);
    if (currentFilledGoals > previousFilledGoals_) {
        events_.push_back({EventType::GoalFilled, state.frame, currentFilledGoals - previousFilledGoals_});
    }
    if (!previousRoomClear_ && state.roomClear) {
        events_.push_back({EventType::RoomCleared, state.frame, state.roomIndex});
    }
    if (state.roomIndex != previousRoom_) {
        events_.push_back({EventType::RoomChanged, state.frame, state.roomIndex});
    }
    if (!previousUpgradeOffered_ && state.upgradeMenu.active) {
        events_.push_back({EventType::UpgradeOffered, state.frame, state.roomIndex});
    }
    if (!previousDead_ && state.dead) {
        events_.push_back({EventType::PlayerDied, state.frame, state.roomIndex});
    }
    if (previousDead_ && !state.dead && state.started) {
        events_.push_back({EventType::PlayerRestarted, state.frame, state.roomIndex});
    }
    rememberState();
}

void DataEngine::rememberState() {
    const GameState& state = game_.state();
    hasPrevious_ = true;
    previousRoom_ = state.roomIndex;
    previousSouls_ = state.player.souls;
    previousFilledGoals_ = filledGoals(state);
    previousStarted_ = state.started;
    previousDead_ = state.dead;
    previousRoomClear_ = state.roomClear;
    previousUpgradeOffered_ = state.upgradeMenu.active;
}

}  // namespace data_engine
