#include "Game.hpp"
#include "gameplay/BigBangCaptureScenario.hpp"
#include "gameplay/TargetRoles.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <vector>

namespace {

using RecordedInput = bigbang_capture::Input;

struct Result {
    std::vector<std::uint64_t> hashes;
    int captureTick = -1;
    int souls = 0;
    Vec3 capturePlayerPos{};
    Vec3 finalPlayerPos{};
};

std::vector<RecordedInput> captureInputs() {
    return bigbang_capture::inputs();
}

Result run(Game& game, const std::vector<RecordedInput>& inputs, const bigbang_capture::Rules& rules = {}) {
    Result result;
    result.hashes.reserve(inputs.size());
    const int soulsBefore = game.state().player.souls;
    for (std::size_t tick = 0; tick < inputs.size(); ++tick) {
        const auto input = inputs[tick];
        bigbang_capture::apply(game, input, rules);
        result.hashes.push_back(bigbang_capture::stateHash(game.state()));
        if (result.captureTick < 0 && game.state().player.souls == soulsBefore + 1) {
            result.captureTick = static_cast<int>(tick);
            result.capturePlayerPos = game.state().player.pos;
        }
    }
    result.souls = game.state().player.souls;
    result.finalPlayerPos = game.state().player.pos;
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
    bigbang_capture::prepare(source);
    const Game::Snapshot initial = source.snapshot();
    const auto inputs = captureInputs();
    const Result baseline = run(source, inputs);

    Game replay;
    replay.restore(initial);
    const Result repeated = run(replay, inputs);
    const int replayDivergence = firstDivergence(baseline, repeated);

    auto changedInputs = inputs;
    constexpr int ChangedTick = bigbang_capture::ChangedInputTick;
    changedInputs[ChangedTick].moveZ = 0.0f;
    Game branch;
    branch.restore(initial);
    const Result changed = run(branch, changedInputs);
    const int changedDivergence = firstDivergence(baseline, changed);

    Game ruleBranch;
    ruleBranch.restore(initial);
    bigbang_capture::Rules changedRules;
    changedRules.movementScale = 0.75f;
    const Result changedRule = run(ruleBranch, inputs, changedRules);
    const int changedRuleDivergence = firstDivergence(baseline, changedRule);

    const float controlDx = baseline.finalPlayerPos.x - baseline.capturePlayerPos.x;
    const float controlDz = baseline.finalPlayerPos.z - baseline.capturePlayerPos.z;
    const float controlDistanceSq = controlDx * controlDx + controlDz * controlDz;
    const bool capturePassed = baseline.captureTick >= 0 && baseline.souls == 1 && controlDistanceSq > 0.0625f;
    const bool replayPassed = repeated.captureTick == baseline.captureTick &&
                              repeated.souls == baseline.souls && replayDivergence < 0;
    const bool branchPassed = changedDivergence == ChangedTick && changedRuleDivergence == 0;
    if (!capturePassed || !replayPassed || !branchPassed) {
        std::fprintf(stderr,
            "BIG_BANG_CAPTURE_FAIL capture_tick=%d replay_tick=%d replay_divergence=%d changed_divergence=%d rule_divergence=%d souls=%d/%d/%d control_distance_sq=%.6f\n",
            baseline.captureTick, repeated.captureTick, replayDivergence, changedDivergence,
            changedRuleDivergence, baseline.souls, repeated.souls, changed.souls, controlDistanceSq);
        return 1;
    }
    std::printf(
        "BIG_BANG_CAPTURE_OK seed=room-default inputs=%zu snapshot_tick=%d capture_tick=%d replay=MATCH first_divergence=%d changed_input_tick=%d rule_first_divergence=%d rule=movement_scale:0.75 control_after_capture=PASS final_hash=%016llx\n",
        inputs.size(), initial.state.frame, baseline.captureTick, changedDivergence, ChangedTick, changedRuleDivergence,
        static_cast<unsigned long long>(baseline.hashes.back()));
    return 0;
}
