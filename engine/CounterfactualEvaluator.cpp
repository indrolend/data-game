#include "CounterfactualEvaluator.hpp"

#include <algorithm>
#include <cmath>

namespace data_engine {
namespace counterfactual {

namespace {

int aliveEnemies(const GameState& state) {
    int count = 0;
    for (const TargetState& target : state.targets) {
        if (target.alive) ++count;
    }
    return count;
}

Score scoreOutcome(const Outcome& outcome) {
    Score score;
    score.survival = outcome.survived ? 1000 : -1000;
    score.neutralization = outcome.enemiesNeutralized * 250;
    score.souls = outcome.soulsCaptured * 500;
    score.resource = static_cast<int>(std::lround(outcome.batteryDelta * 10.0f));
    score.total = score.survival + score.neutralization + score.souls + score.resource;
    return score;
}

}

Command commandFor(ActionKind action, int tick) {
    Command command;
    switch (action) {
        case ActionKind::MeleeVacuum:
            if (tick == 0) command.melee = true;
            else command.vacuum = true;
            break;
        case ActionKind::VacuumOnly:
            command.vacuum = true;
            break;
        case ActionKind::ShootVacuum:
            if (tick == 0) command.shoot = true;
            else command.vacuum = true;
            break;
        case ActionKind::Idle:
        default:
            break;
    }
    return command;
}

Observation execute(DataEngine& engine, ActionKind action, int horizonTicks) {
    const int ticks = std::clamp(horizonTicks, 0, kMaxHorizonTicks);
    Observation observation = engine.observe();
    for (int tick = 0; tick < ticks; ++tick) {
        observation = engine.step(commandFor(action, tick));
    }
    return observation;
}

Evaluation evaluate(const DataEngine::Checkpoint& checkpoint,
                    const std::vector<Candidate>& candidates,
                    int horizonTicks) {
    Evaluation result;
    if (candidates.empty() || candidates.size() > kMaxCandidates ||
        horizonTicks <= 0 || horizonTicks > kMaxHorizonTicks) {
        return result;
    }

    result.ok = true;
    result.horizonTicks = horizonTicks;
    result.evaluatedTicks = static_cast<int>(candidates.size()) * horizonTicks;

    DataEngine scratch;
    int bestTotal = 0;
    for (std::size_t index = 0; index < candidates.size(); ++index) {
        scratch.restore(checkpoint);
        Outcome outcome;
        outcome.index = static_cast<int>(index);
        outcome.action = candidates[index].name;
        outcome.actionKind = candidates[index].action;
        outcome.observation = execute(scratch, candidates[index].action, horizonTicks);
        outcome.events = scratch.events();
        outcome.checkpoint = scratch.checkpoint();

        const GameState& initial = checkpoint.game.state;
        const GameState& final = scratch.authoritativeState();
        outcome.survived = final.player.alive && !final.dead && final.started;
        outcome.enemiesNeutralized = std::max(0, aliveEnemies(initial) - aliveEnemies(final));
        outcome.soulsCaptured = final.player.souls - initial.player.souls;
        outcome.batteryDelta = final.player.battery - initial.player.battery;
        outcome.score = scoreOutcome(outcome);

        result.outcomes.push_back(outcome);
        if (result.selectedIndex < 0 || outcome.score.total > bestTotal ||
            (outcome.score.total == bestTotal && outcome.index < result.selectedIndex)) {
            result.selectedIndex = outcome.index;
            bestTotal = outcome.score.total;
        }
    }
    return result;
}

}
}
