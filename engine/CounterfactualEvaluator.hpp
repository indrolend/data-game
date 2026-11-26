#pragma once

#include "DataEngine.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace data_engine {
namespace counterfactual {

constexpr int kMaxHorizonTicks = 120;
constexpr std::size_t kMaxCandidates = 4;

enum class ActionKind { Idle, MeleeVacuum, VacuumOnly, ShootVacuum };

struct Candidate {
    std::string name;
    ActionKind action = ActionKind::Idle;
};

struct Score {
    int survival = 0;
    int neutralization = 0;
    int souls = 0;
    int resource = 0;
    int total = 0;
};

struct Outcome {
    int index = 0;
    std::string action;
    ActionKind actionKind = ActionKind::Idle;
    Observation observation;
    std::vector<Event> events;
    DataEngine::Checkpoint checkpoint;
    bool survived = false;
    int enemiesNeutralized = 0;
    int soulsCaptured = 0;
    float batteryDelta = 0.0f;
    Score score;
};

struct Evaluation {
    bool ok = false;
    int horizonTicks = 0;
    int selectedIndex = -1;
    std::vector<Outcome> outcomes;
    int evaluatedTicks = 0;
};

Command commandFor(ActionKind action, int tick);

inline std::vector<Candidate> defaultCandidates() {
    return {
        {"idle", ActionKind::Idle},
        {"melee_vacuum", ActionKind::MeleeVacuum},
        {"vacuum_only", ActionKind::VacuumOnly},
        {"shoot_vacuum", ActionKind::ShootVacuum},
    };
}

Evaluation evaluate(const DataEngine::Checkpoint& checkpoint,
                    const std::vector<Candidate>& candidates,
                    int horizonTicks);

Observation execute(DataEngine& engine, ActionKind action, int horizonTicks);

}
}
