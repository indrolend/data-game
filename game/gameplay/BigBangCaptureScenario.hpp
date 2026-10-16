#pragma once

#include "../Game.hpp"

#include <cstdint>
#include <type_traits>
#include <vector>

namespace bigbang_capture {

constexpr float FixedDt = 1.0f / 60.0f;
constexpr int ApproachTicks = 30;
constexpr int VacuumTicks = 240;
constexpr int ControlTicks = 30;
constexpr int ChangedInputTick = 20;

struct Input {
    float moveX = 0.0f;
    float moveZ = 0.0f;
    bool vacuum = false;
    bool melee = false;
};

inline void prepare(Game& game) {
    game.reset();
    GameState& state = game.networkMutableState();
    for (TargetState& target : state.targets) target = TargetState{};
    for (HumanRespawnRequest& request : state.respawnQueue) request = HumanRespawnRequest{};
    state.cinematic = CinematicState{};
    state.started = true;
    state.dead = false;
    state.uiPaused = false;
    state.player.battery = 100.0f;
    state.player.vel = {};
    state.player.grounded = true;
    state.camera.yaw = 0.0f;
    state.camera.pitch = 0.0f;
    state.camera.forward = {0.0f, 0.0f, -1.0f};
    TargetState& target = state.targets[0];
    target.alive = true;
    target.armor = 0.10f;
    target.health = 1.0f;
    target.scale = 1.0f;
    target.attackCooldown = 999.0f;
    target.pos = state.player.pos + Vec3{0.0f, 0.0f, -3.0f};
    target.walkTarget = target.pos;
}

inline std::vector<Input> inputs() {
    std::vector<Input> result;
    result.insert(result.end(), ApproachTicks, Input{0.0f, 1.0f, false, false});
    result.push_back({0.0f, 0.0f, false, true});
    result.push_back({});
    result.insert(result.end(), VacuumTicks, Input{0.0f, 0.0f, true, false});
    result.insert(result.end(), ControlTicks, Input{1.0f, 0.0f, false, false});
    return result;
}

inline void apply(Game& game, const Input& input) {
    game.setTouchControls(input.moveX, input.moveZ, 0.0f, 0.0f,
                          input.vacuum, false, false, input.melee, false, false);
    game.update(FixedDt);
}

inline void hashBytes(std::uint64_t& hash, const void* data, std::size_t size) {
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (std::size_t i = 0; i < size; ++i) { hash ^= bytes[i]; hash *= 1099511628211ull; }
}

template <typename T> inline void hashValue(std::uint64_t& hash, const T& value) {
    static_assert(std::is_trivially_copyable<T>::value, "state hash values must be stable value types");
    hashBytes(hash, &value, sizeof(value));
}

inline void hashVec(std::uint64_t& hash, const Vec3& value) {
    hashValue(hash, value.x); hashValue(hash, value.y); hashValue(hash, value.z);
}

inline std::uint64_t stateHash(const GameState& state) {
    std::uint64_t hash = 1469598103934665603ull;
    hashValue(hash, state.frame); hashValue(hash, state.time);
    hashVec(hash, state.player.pos); hashVec(hash, state.player.vel);
    hashValue(hash, state.player.battery); hashValue(hash, state.player.souls);
    hashValue(hash, state.player.grounded); hashValue(hash, state.vacuum.active);
    hashValue(hash, state.vacuum.power); hashValue(hash, state.vacuum.target);
    hashVec(hash, state.phoneTransform.vacuumPullPoint);
    for (const TargetState& target : state.targets) {
        hashValue(hash, target.alive); hashValue(hash, target.slurpable);
        hashValue(hash, target.soulState); hashValue(hash, target.health);
        hashValue(hash, target.ingestProgress); hashValue(hash, target.vacuumPullAmount);
        hashVec(hash, target.pos); hashVec(hash, target.vel); hashValue(hash, target.soul.id);
    }
    return hash;
}

} // namespace bigbang_capture
