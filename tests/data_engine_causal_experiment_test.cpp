#include "DataEngine.hpp"

#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

constexpr int kSeed = 424242;
constexpr int kRoom = 3;
constexpr int kTicks = 240;
constexpr int kInterventionTick = 20;

data_engine::Command commandForTick(int tick) {
    data_engine::Command command;
    command.moveX = tick < 80 ? 0.25f : 0.0f;
    command.moveZ = tick < 120 ? 0.70f : 0.0f;
    command.lookX = tick < 30 ? 0.35f : 0.0f;
    command.vacuum = tick >= 90 && tick < 150;
    command.sprint = tick < 60;
    command.jump = tick == 12;
    command.melee = tick == 72;
    command.shoot = tick == 170;
    return command;
}

struct StateDifference {
    float posDx = 0.0f;
    float posDy = 0.0f;
    float posDz = 0.0f;
    float velocityDx = 0.0f;
    float velocityDy = 0.0f;
    float velocityDz = 0.0f;
    float yawDelta = 0.0f;
};

struct Trial {
    int firstDivergentTick = -1;
    int divergentObservations = 0;
    std::string firstControlDigest;
    std::string firstTreatmentDigest;
    StateDifference firstDifference;
    std::vector<std::string> controlDigests;
    std::vector<std::string> treatmentDigests;
    std::vector<double> separation;
    float finalControlVelocityX = 0.0f;
    float finalTreatmentVelocityX = 0.0f;
    float finalControlVelocityZ = 0.0f;
    float finalTreatmentVelocityZ = 0.0f;
};

Trial runTrial(bool intervene) {
    auto controlEngine = std::make_unique<data_engine::DataEngine>();
    auto treatmentEngine = std::make_unique<data_engine::DataEngine>();
    auto& control = *controlEngine;
    auto& treatment = *treatmentEngine;
    control.startGeneratedScenario(kSeed, kRoom);
    treatment.startGeneratedScenario(kSeed, kRoom);

    Trial result;
    result.controlDigests.reserve(kTicks);
    result.treatmentDigests.reserve(kTicks);
    result.separation.reserve(kTicks);
    for (int tick = 0; tick < kTicks; ++tick) {
        data_engine::Command controlInput = commandForTick(tick);
        data_engine::Command treatmentInput = controlInput;
        if (intervene && tick == kInterventionTick) treatmentInput.moveX = -0.25f;

        const data_engine::Observation controlObservation = control.step(controlInput);
        const data_engine::Observation treatmentObservation = treatment.step(treatmentInput);
        result.controlDigests.push_back(controlObservation.stateDigest);
        result.treatmentDigests.push_back(treatmentObservation.stateDigest);

        const PlayerState& a = control.authoritativeState().player;
        const PlayerState& b = treatment.authoritativeState().player;
        const double dx = static_cast<double>(b.pos.x) - a.pos.x;
        const double dy = static_cast<double>(b.pos.y) - a.pos.y;
        const double dz = static_cast<double>(b.pos.z) - a.pos.z;
        result.separation.push_back(std::sqrt(dx * dx + dy * dy + dz * dz));

        if (!(controlObservation == treatmentObservation)) {
            ++result.divergentObservations;
            if (result.firstDivergentTick < 0) {
                result.firstDivergentTick = tick;
                result.firstControlDigest = controlObservation.stateDigest;
                result.firstTreatmentDigest = treatmentObservation.stateDigest;
                result.firstDifference = {
                    b.pos.x - a.pos.x, b.pos.y - a.pos.y, b.pos.z - a.pos.z,
                    b.vel.x - a.vel.x, b.vel.y - a.vel.y, b.vel.z - a.vel.z,
                    b.yaw - a.yaw,
                };
            }
        }
        if (tick == kTicks - 1) {
            result.finalControlVelocityX = a.vel.x;
            result.finalTreatmentVelocityX = b.vel.x;
            result.finalControlVelocityZ = a.vel.z;
            result.finalTreatmentVelocityZ = b.vel.z;
        }
    }
    return result;
}

bool sameReplay(const Trial& a, const Trial& b) {
    return a.firstDivergentTick == b.firstDivergentTick &&
           a.divergentObservations == b.divergentObservations &&
           a.controlDigests == b.controlDigests &&
           a.treatmentDigests == b.treatmentDigests &&
           a.separation == b.separation;
}

bool near(double actual, double expected, double tolerance) {
    return std::fabs(actual - expected) <= tolerance;
}

}  // namespace

int main() {
    const Trial identicalControl = runTrial(false);
    const Trial treatment = runTrial(true);
    const Trial replay = runTrial(true);

    const bool controlsIdentical = identicalControl.firstDivergentTick == -1 &&
                                   identicalControl.divergentObservations == 0;
    const bool replayIdentical = sameReplay(treatment, replay);
    const bool authoritativeDifference =
        treatment.firstDifference.posDx != 0.0f &&
        treatment.firstDifference.velocityDx != 0.0f &&
        treatment.firstDifference.posDy == 0.0f &&
        treatment.firstDifference.yawDelta == 0.0f;
    const bool persistentPosition =
        treatment.separation[21] > treatment.separation[20] &&
        treatment.separation[60] > treatment.separation[30] &&
        near(treatment.separation[120], 0.153261662, 0.000001) &&
        near(treatment.separation[239], 0.153262138, 0.000001);
    const bool establishedResult = treatment.firstDivergentTick == kInterventionTick &&
                                   treatment.divergentObservations == 220;
    const bool ok = controlsIdentical && replayIdentical && authoritativeDifference &&
                    persistentPosition && establishedResult;

    std::printf(
        "INDRO_EVIDENCE {\"schema\":\"digital-breakdown.experiment-003/v1\","
        "\"status\":\"%s\",\"configuration\":{\"seed\":%d,\"room\":%d,"
        "\"ticks\":%d,\"fixed_step_seconds\":%.9g},"
        "\"intervention\":{\"tick\":%d,\"field\":\"moveX\","
        "\"control\":0.25,\"treatment\":-0.25},"
        "\"control\":\"%s\",\"first_divergent_tick\":%d,"
        "\"divergent_observations\":%d,\"first_control_digest\":\"%s\","
        "\"first_treatment_digest\":\"%s\","
        "\"first_state_difference\":{\"player.pos\":[%.9g,%.9g,%.9g],"
        "\"player.vel\":[%.9g,%.9g,%.9g],\"player.yaw\":%.9g},"
        "\"separation\":{\"tick20\":%.9g,\"tick21\":%.9g,\"tick30\":%.9g,"
        "\"tick60\":%.9g,\"tick120\":%.9g,\"tick239\":%.9g},"
        "\"final_velocity\":{\"control_x\":%.9g,\"treatment_x\":%.9g,"
        "\"control_z\":%.9g,\"treatment_z\":%.9g},\"replay\":\"%s\"}\n",
        ok ? "PASS" : "FAIL", kSeed, kRoom, kTicks,
        data_engine::EngineContract::fixedStepSeconds, kInterventionTick,
        controlsIdentical ? "IDENTICAL" : "DIVERGED",
        treatment.firstDivergentTick, treatment.divergentObservations,
        treatment.firstControlDigest.c_str(), treatment.firstTreatmentDigest.c_str(),
        treatment.firstDifference.posDx, treatment.firstDifference.posDy,
        treatment.firstDifference.posDz, treatment.firstDifference.velocityDx,
        treatment.firstDifference.velocityDy, treatment.firstDifference.velocityDz,
        treatment.firstDifference.yawDelta,
        treatment.separation[20], treatment.separation[21], treatment.separation[30],
        treatment.separation[60], treatment.separation[120], treatment.separation[239],
        treatment.finalControlVelocityX, treatment.finalTreatmentVelocityX,
        treatment.finalControlVelocityZ, treatment.finalTreatmentVelocityZ,
        replayIdentical ? "IDENTICAL" : "DIFFERENT");

    if (!ok) std::fprintf(stderr, "EXPERIMENT_003_FAIL causal_contract\n");
    return ok ? 0 : 1;
}

