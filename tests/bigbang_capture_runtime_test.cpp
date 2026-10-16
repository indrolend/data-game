#include "Game.hpp"
#include "gameplay/TargetRoles.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <vector>

namespace {

constexpr float Dt = 1.0f / 60.0f;
constexpr int CaptureLimit = 240;

struct RecordedInput {
    bool vacuum = false;
    bool melee = false;
};

struct Result {
    std::vector<std::uint64_t> hashes;
    int captureTick = -1;
    int souls = 0;
};

void hashBytes(std::uint64_t& hash, const void* data, std::size_t size) {
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (std::size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 1099511628211ull;
    }
}

template <typename T>
void hashValue(std::uint64_t& hash, const T& value) {
    static_assert(std::is_trivially_copyable<T>::value, "state hash values must be stable value types");
    hashBytes(hash, &value, sizeof(value));
}

void hashVec(std::uint64_t& hash, const Vec3& value) {
    hashValue(hash, value.x);
    hashValue(hash, value.y);
    hashValue(hash, value.z);
}

std::uint64_t captureStateHash(const GameState& state) {
    std::uint64_t hash = 1469598103934665603ull;
    hashValue(hash, state.frame);
    hashValue(hash, state.time);
    hashVec(hash, state.player.pos);
    hashVec(hash, state.player.vel);
    hashValue(hash, state.player.battery);
    hashValue(hash, state.player.souls);
    hashValue(hash, state.player.grounded);
    hashValue(hash, state.vacuum.active);
    hashValue(hash, state.vacuum.power);
    hashValue(hash, state.vacuum.target);
    hashVec(hash, state.phoneTransform.vacuumPullPoint);
    for (const TargetState& target : state.targets) {
        hashValue(hash, target.alive);
        hashValue(hash, target.slurpable);
        hashValue(hash, target.soulState);
        hashValue(hash, target.health);
        hashValue(hash, target.ingestProgress);
        hashValue(hash, target.vacuumPullAmount);
        hashVec(hash, target.pos);
        hashVec(hash, target.vel);
        hashValue(hash, target.soul.id);
    }
    return hash;
}

void prepareCapture(Game& game) {
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
    target.pos = state.player.pos + Vec3{0.0f, 0.0f, -0.75f};
    target.walkTarget = target.pos;
}

std::vector<RecordedInput> captureInputs() {
    std::vector<RecordedInput> result;
    result.push_back({false, true});
    result.push_back({false, false});
    result.insert(result.end(), CaptureLimit, RecordedInput{true, false});
    return result;
}

Result run(Game& game, const std::vector<RecordedInput>& inputs) {
    Result result;
    result.hashes.reserve(inputs.size());
    const int soulsBefore = game.state().player.souls;
    for (std::size_t tick = 0; tick < inputs.size(); ++tick) {
        const auto input = inputs[tick];
        game.setTouchControls(0.0f, 0.0f, 0.0f, 0.0f,
                              input.vacuum, false, false, input.melee, false, false);
        game.update(Dt);
        result.hashes.push_back(captureStateHash(game.state()));
        if (result.captureTick < 0 && game.state().player.souls == soulsBefore + 1)
            result.captureTick = static_cast<int>(tick);
    }
    result.souls = game.state().player.souls;
    return result;
}

int firstDivergence(const Result& left, const Result& right) {
    const std::size_t count = std::min(left.hashes.size(), right.hashes.size());
    for (std::size_t i = 0; i < count; ++i)
        if (left.hashes[i] != right.hashes[i]) return static_cast<int>(i);
    return left.hashes.size() == right.hashes.size() ? -1 : static_cast<int>(count);
}

} // namespace

int main() {
    Game source;
    prepareCapture(source);
    const Game::Snapshot initial = source.snapshot();
    const auto inputs = captureInputs();
    const Result baseline = run(source, inputs);

    Game replay;
    replay.restore(initial);
    const Result repeated = run(replay, inputs);
    const int replayDivergence = firstDivergence(baseline, repeated);

    auto changedInputs = inputs;
    constexpr int ChangedTick = 20;
    changedInputs[ChangedTick].vacuum = false;
    Game branch;
    branch.restore(initial);
    const Result changed = run(branch, changedInputs);
    const int changedDivergence = firstDivergence(baseline, changed);

    const bool capturePassed = baseline.captureTick >= 0 && baseline.souls == 1;
    const bool replayPassed = repeated.captureTick == baseline.captureTick &&
                              repeated.souls == baseline.souls && replayDivergence < 0;
    const bool branchPassed = changedDivergence == ChangedTick;
    if (!capturePassed || !replayPassed || !branchPassed) {
        std::fprintf(stderr,
            "BIG_BANG_CAPTURE_FAIL capture_tick=%d replay_tick=%d replay_divergence=%d changed_divergence=%d souls=%d/%d/%d\n",
            baseline.captureTick, repeated.captureTick, replayDivergence, changedDivergence,
            baseline.souls, repeated.souls, changed.souls);
        return 1;
    }
    std::printf(
        "BIG_BANG_CAPTURE_OK seed=room-default inputs=%zu snapshot_tick=%d capture_tick=%d replay=MATCH first_divergence=%d changed_input_tick=%d final_hash=%016llx\n",
        inputs.size(), initial.state.frame, baseline.captureTick, changedDivergence, ChangedTick,
        static_cast<unsigned long long>(baseline.hashes.back()));
    return 0;
}
