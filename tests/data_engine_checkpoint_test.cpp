#include "CounterfactualEvaluator.hpp"
#include "DataEngine.hpp"
#include "MultiplayerProtocol.hpp"
#include "ReferenceScenario.hpp"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstddef>
#include <vector>

namespace {

constexpr int kWarmupTicks = 120;
constexpr int kBranchTicks = 120;
constexpr int kContinuationTicks = 240;
constexpr int kConsequentialTicks = 120;

bool sameVec(const Vec3& a, const Vec3& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

template <typename A, typename B, typename Equal>
bool sameArray(const A& a, const B& b, Equal equal) {
    if (a.size() != b.size()) return false;
    for (std::size_t index = 0; index < a.size(); ++index) {
        if (!equal(a[index], b[index])) return false;
    }
    return true;
}

bool samePerception(const gameplay::EnemyPerceptionState& a, const gameplay::EnemyPerceptionState& b) {
    return sameVec(a.lastSeenPosition, b.lastSeenPosition) &&
           sameVec(a.lastSeenVelocity, b.lastSeenVelocity) &&
           a.confidence == b.confidence && a.uncertainty == b.uncertainty &&
           a.headYaw == b.headYaw && a.headPitch == b.headPitch &&
           a.searchPhase == b.searchPhase && a.confirmed == b.confirmed;
}

bool sameBehavior(const gameplay::EnemyBehaviorState& a, const gameplay::EnemyBehaviorState& b) {
    return a.mode == b.mode && a.modeTime == b.modeTime &&
           a.quietTime == b.quietTime && a.lostContactTime == b.lostContactTime &&
           a.hasSeenThreat == b.hasSeenThreat;
}

template <typename T>
bool sameFoot(const T& a, const T& b) {
    return sameVec(a.position, b.position) && sameVec(a.plantPosition, b.plantPosition) &&
           sameVec(a.swingStart, b.swingStart) && sameVec(a.swingTarget, b.swingTarget) &&
           sameVec(a.supportNormal, b.supportNormal) && a.phase == b.phase &&
           a.contact == b.contact && a.load == b.load &&
           a.swingProgress == b.swingProgress && a.planted == b.planted;
}

bool sameLocomotion(const gameplay::EnemyLocomotionState& a, const gameplay::EnemyLocomotionState& b) {
    return sameFoot(a.left, b.left) && sameFoot(a.right, b.right) &&
           a.swingFoot == b.swingFoot && a.nextSwingFoot == b.nextSwingFoot &&
           a.stepCooldown == b.stepCooldown && a.stanceTime == b.stanceTime &&
           sameVec(a.committedTravelDirection, b.committedTravelDirection) &&
           a.directionalCommitmentTimer == b.directionalCommitmentTimer &&
           sameVec(a.feasibleTravelDirection, b.feasibleTravelDirection) &&
           a.feasibleSpeed == b.feasibleSpeed && a.crouch == b.crouch &&
           a.recoveryUrgency == b.recoveryUrgency && a.recoveryPhase == b.recoveryPhase &&
           a.recoveryPhaseTime == b.recoveryPhaseTime &&
           a.physicalGaitPhase == b.physicalGaitPhase && a.initialized == b.initialized;
}

bool sameBody(const gameplay::PhysicalEnemyBodyState& a, const gameplay::PhysicalEnemyBodyState& b) {
    return a.initialized == b.initialized && a.fallen == b.fallen &&
           a.bodyPitch == b.bodyPitch && a.bodyRoll == b.bodyRoll &&
           a.pitchVelocity == b.pitchVelocity && a.rollVelocity == b.rollVelocity &&
           a.yawVelocity == b.yawVelocity && a.gaitPhase == b.gaitPhase &&
           a.recovery == b.recovery && a.supportContact == b.supportContact &&
           a.disruption == b.disruption && a.impactInstability == b.impactInstability &&
           a.scrambleAmount == b.scrambleAmount;
}

bool sameMotorMemory(const gameplay::EnemyMotorMemory& a, const gameplay::EnemyMotorMemory& b) {
    return sameArray(a.hidden, b.hidden, [](float left, float right) { return left == right; }) &&
           sameArray(a.tendency, b.tendency, [](float left, float right) { return left == right; }) &&
           a.disruptionExposure == b.disruptionExposure && a.arousal == b.arousal &&
           a.caution == b.caution && a.fixation == b.fixation &&
           a.animalPhase == b.animalPhase &&
           a.headLungeExposure == b.headLungeExposure;
}

bool sameMotorOutput(const gameplay::EnemyMotorOutput& a, const gameplay::EnemyMotorOutput& b) {
    return sameVec(a.steering, b.steering) && a.speedScale == b.speedScale &&
           a.brace == b.brace && a.paceExpression == b.paceExpression &&
           a.lungeEvasion == b.lungeEvasion;
}

template <typename T>
bool sameEnemyRuntime(const T& a, const T& b) {
    return a.hasEnemyRuntime == b.hasEnemyRuntime &&
           sameArray(a.enemyRuntime.perceptions, b.enemyRuntime.perceptions, samePerception) &&
           sameArray(a.enemyRuntime.behaviors, b.enemyRuntime.behaviors, sameBehavior) &&
           sameArray(a.enemyRuntime.locomotions, b.enemyRuntime.locomotions, sameLocomotion) &&
           sameArray(a.enemyRuntime.bodies, b.enemyRuntime.bodies, sameBody) &&
           sameArray(a.enemyRuntime.motors, b.enemyRuntime.motors, sameMotorMemory) &&
           sameArray(a.enemyRuntime.motorOutputs, b.enemyRuntime.motorOutputs, sameMotorOutput) &&
           a.enemyRuntime.perceptionCursor == b.enemyRuntime.perceptionCursor;
}

bool sameGameCheckpoint(const Game::Checkpoint& a, const Game::Checkpoint& b) {
    return dbnet::authoritativeStateDigest(a.state) == dbnet::authoritativeStateDigest(b.state) &&
           sameEnemyRuntime(a, b) &&
           a.simulationPlayerId == b.simulationPlayerId &&
           a.enemyMotorExpressionEnabled == b.enemyMotorExpressionEnabled;
}

bool sameEngineCheckpoint(const data_engine::DataEngine::Checkpoint& a,
                          const data_engine::DataEngine::Checkpoint& b) {
    return sameGameCheckpoint(a.game, b.game) && a.events == b.events &&
           a.hasPrevious == b.hasPrevious && a.previousRoom == b.previousRoom &&
           a.previousSouls == b.previousSouls &&
           a.previousFilledGoals == b.previousFilledGoals &&
           a.previousStarted == b.previousStarted && a.previousDead == b.previousDead &&
           a.previousRoomClear == b.previousRoomClear &&
           a.previousUpgradeOffered == b.previousUpgradeOffered;
}

data_engine::Command commandForTick(int tick, int variant) {
    data_engine::Command command;
    if (variant == 0) {
        command.moveX = tick < 80 ? 0.25f : 0.0f;
        command.moveZ = tick < 120 ? 0.70f : 0.0f;
        command.lookX = tick < 30 ? 0.35f : 0.0f;
        command.vacuum = tick >= 60 && tick < 140;
        command.sprint = tick < 50;
        command.jump = tick == 12;
        command.melee = tick == 72;
        command.shoot = tick == 170;
    } else {
        command.moveX = -0.45f;
        command.moveZ = tick < 60 ? -0.50f : 0.20f;
        command.lookX = -0.25f;
        command.lookY = tick < 40 ? 0.10f : 0.0f;
        command.vacuum = tick >= 20 && tick < 100;
        command.sprint = tick >= 100;
        command.melee = tick == 40;
        command.shoot = tick == 90;
    }
    return command;
}

void driveGame(Game& game, int tick) {
    const data_engine::Command command = commandForTick(tick, 0);
    game.setTouchControls(command.moveX, command.moveZ, command.lookX, command.lookY,
                          command.vacuum, command.sprint, command.jump, command.melee,
                          command.shoot, command.cameraToggle);
    game.update(data_engine::EngineContract::fixedStepSeconds);
}

int activeEnemies(const GameState& state) {
    int count = 0;
    for (const TargetState& target : state.targets) {
        if (target.alive) ++count;
    }
    return count;
}

bool runGameCheckpoint(int seed) {
    Game original;
    Game restored;
    original.debugStartGeneratedRoomFixture(seed, 3);
    restored.debugStartGeneratedRoomFixture(seed, 3);
    for (int tick = 0; tick < kWarmupTicks; ++tick) {
        driveGame(original, tick);
        driveGame(restored, tick);
    }

    const Game::Checkpoint checkpoint = original.captureCheckpoint();
    restored.restoreCheckpoint(checkpoint);
    bool ok = activeEnemies(checkpoint.state) > 0;
    ok = ok && sameGameCheckpoint(checkpoint, restored.captureCheckpoint());
    ok = ok && dbnet::authoritativeStateDigest(original.state()) ==
                   dbnet::authoritativeStateDigest(restored.state());

    for (int tick = kWarmupTicks; tick < kWarmupTicks + kContinuationTicks; ++tick) {
        driveGame(original, tick);
        driveGame(restored, tick);
        ok = ok && dbnet::authoritativeStateDigest(original.state()) ==
                       dbnet::authoritativeStateDigest(restored.state());
    }
    return ok;
}

bool runEngineCheckpoint(int seed, int& divergentObservations) {
    data_engine::DataEngine warmed;
    warmed.startGeneratedScenario(seed, 3);
    for (int tick = 0; tick < kWarmupTicks; ++tick) {
        warmed.step(commandForTick(tick, 0));
    }

    const data_engine::DataEngine::Checkpoint checkpoint = warmed.checkpoint();
    bool ok = activeEnemies(checkpoint.game.state) > 0;

    data_engine::DataEngine restored;
    restored.restore(checkpoint);
    ok = ok && sameEngineCheckpoint(checkpoint, restored.checkpoint());
    ok = ok && restored.observe() == warmed.observe();
    ok = ok && restored.events() == warmed.events();

    data_engine::DataEngine full;
    full.startGeneratedScenario(seed, 3);
    data_engine::DataEngine same;
    same.restore(checkpoint);
    data_engine::DataEngine changed;
    changed.restore(checkpoint);
    data_engine::DataEngine changedRepeat;
    changedRepeat.restore(checkpoint);

    divergentObservations = 0;
    for (int tick = 0; tick < kWarmupTicks + kContinuationTicks; ++tick) {
        const data_engine::Command originalCommand = commandForTick(tick, 0);
        const data_engine::Command changedCommand = commandForTick(tick, 1);
        const data_engine::Observation fullObservation = full.step(originalCommand);
        if (tick >= kWarmupTicks) {
            const data_engine::Observation sameObservation = same.step(originalCommand);
            const data_engine::Observation changedObservation = changed.step(changedCommand);
            const data_engine::Observation changedRepeatObservation = changedRepeat.step(changedCommand);
            ok = ok && sameObservation == fullObservation;
            ok = ok && changedObservation == changedRepeatObservation;
            if (!(changedObservation == sameObservation)) ++divergentObservations;
        }
    }

    ok = ok && same.events() == full.events();
    ok = ok && changed.events() == changedRepeat.events();
    ok = ok && divergentObservations > 0;
    return ok;
}

struct OutcomeSummary {
    std::vector<data_engine::Observation> observations;
    std::vector<std::string> digests;
    data_engine::Observation finalObservation;
    std::string finalDigest;
    int activeEnemies = 0;
    Vec3 playerPos;
    Vec3 playerVel;
    int eventCount = 0;
    data_engine::DataEngine::Checkpoint checkpoint;
};

OutcomeSummary executeBranch(data_engine::DataEngine& engine,
                             const data_engine::DataEngine::Checkpoint& checkpoint,
                             int variant) {
    engine.restore(checkpoint);
    OutcomeSummary outcome;
    for (int tick = 0; tick < kBranchTicks; ++tick) {
        const data_engine::Observation observation =
            engine.step(commandForTick(kWarmupTicks + tick, variant));
        outcome.observations.push_back(observation);
        outcome.digests.push_back(observation.stateDigest);
    }
    outcome.finalObservation = outcome.observations.back();
    outcome.finalDigest = dbnet::authoritativeStateDigestHex(engine.authoritativeState());
    outcome.activeEnemies = activeEnemies(engine.authoritativeState());
    outcome.playerPos = engine.authoritativeState().player.pos;
    outcome.playerVel = engine.authoritativeState().player.vel;
    outcome.eventCount = static_cast<int>(engine.events().size());
    outcome.checkpoint = engine.checkpoint();
    return outcome;
}

bool sameOutcome(const OutcomeSummary& a, const OutcomeSummary& b) {
    if (a.observations.size() != b.observations.size()) return false;
    if (a.finalDigest != b.finalDigest) return false;
    for (std::size_t tick = 0; tick < a.observations.size(); ++tick) {
        if (!(a.observations[tick] == b.observations[tick])) return false;
    }
    return sameEngineCheckpoint(a.checkpoint, b.checkpoint);
}

int firstDifferentTick(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    if (a.size() != b.size()) return -1;
    for (std::size_t tick = 0; tick < a.size(); ++tick) {
        if (a[tick] != b[tick]) return static_cast<int>(tick);
    }
    return -1;
}

bool runAlternativeFuturesEvidence(int seed) {
    data_engine::DataEngine warmed;
    warmed.startGeneratedScenario(seed, 3);
    for (int tick = 0; tick < kWarmupTicks; ++tick) {
        warmed.step(commandForTick(tick, 0));
    }
    const data_engine::DataEngine::Checkpoint checkpoint = warmed.checkpoint();
    const int checkpointEnemies = activeEnemies(checkpoint.game.state);
    const std::string checkpointDigest = dbnet::authoritativeStateDigestHex(checkpoint.game.state);

    data_engine::DataEngine branch;
    const OutcomeSummary a = executeBranch(branch, checkpoint, 0);
    const OutcomeSummary b = executeBranch(branch, checkpoint, 1);
    const OutcomeSummary aRepeat = executeBranch(branch, checkpoint, 0);
    const OutcomeSummary bRepeat = executeBranch(branch, checkpoint, 1);

    int divergentObservations = 0;
    for (std::size_t tick = 0; tick < a.digests.size(); ++tick) {
        if (a.digests[tick] != b.digests[tick]) ++divergentObservations;
    }
    const int firstDivergentTick = firstDifferentTick(a.digests, b.digests);
    const bool ok = checkpointEnemies > 0 && sameOutcome(a, aRepeat) &&
                    sameOutcome(b, bRepeat) && firstDivergentTick >= 0 &&
                    divergentObservations > 0;

    std::printf("ALTERNATIVE_FUTURES_SETUP seed=%d room=3 warmup_ticks=%d branch_ticks=%d checkpoint_enemies=%d checkpoint_digest=%s\n",
                seed, kWarmupTicks, kBranchTicks, checkpointEnemies, checkpointDigest.c_str());
    std::printf("ALTERNATIVE_FUTURES_REPLAY seed=%d A_identical=%d B_identical=%d hidden_runtime=1 events=1 previous_bookkeeping=1\n",
                seed, sameOutcome(a, aRepeat) ? 1 : 0, sameOutcome(b, bRepeat) ? 1 : 0);
    std::printf("ALTERNATIVE_FUTURES_DIVERGENCE seed=%d first_branch_tick=%d first_simulation_tick=%d divergent_observations=%d/%d\n",
                seed, firstDivergentTick,
                firstDivergentTick < 0 ? -1 : kWarmupTicks + firstDivergentTick,
                divergentObservations, kBranchTicks);
    std::printf("ALTERNATIVE_FUTURES_DIGESTS seed=%d A=%s A_repeat=%s B=%s B_repeat=%s\n",
                seed, a.finalDigest.c_str(), aRepeat.finalDigest.c_str(),
                b.finalDigest.c_str(), bRepeat.finalDigest.c_str());
    std::printf("ALTERNATIVE_FUTURES_GAMEPLAY seed=%d A{room=%d souls=%d goals=%d/%d dead=%d clear=%d upgrade=%d enemies=%d events=%d pos=(%.6f,%.6f,%.6f) vel=(%.6f,%.6f,%.6f)} B{room=%d souls=%d goals=%d/%d dead=%d clear=%d upgrade=%d enemies=%d events=%d pos=(%.6f,%.6f,%.6f) vel=(%.6f,%.6f,%.6f)} delta{pos=%.6f vel=%.6f enemies=%d events=%d}\n",
                seed,
                a.finalObservation.room, a.finalObservation.souls, a.finalObservation.filledGoals,
                a.finalObservation.requiredGoals, a.finalObservation.dead ? 1 : 0,
                a.finalObservation.roomClear ? 1 : 0, a.finalObservation.upgradeOffered ? 1 : 0,
                a.activeEnemies, a.eventCount,
                a.playerPos.x, a.playerPos.y, a.playerPos.z,
                a.playerVel.x, a.playerVel.y, a.playerVel.z,
                b.finalObservation.room, b.finalObservation.souls, b.finalObservation.filledGoals,
                b.finalObservation.requiredGoals, b.finalObservation.dead ? 1 : 0,
                b.finalObservation.roomClear ? 1 : 0, b.finalObservation.upgradeOffered ? 1 : 0,
                b.activeEnemies, b.eventCount,
                b.playerPos.x, b.playerPos.y, b.playerPos.z,
                b.playerVel.x, b.playerVel.y, b.playerVel.z,
                std::fabs(a.playerPos.x - b.playerPos.x) +
                    std::fabs(a.playerPos.y - b.playerPos.y) +
                    std::fabs(a.playerPos.z - b.playerPos.z),
                std::fabs(a.playerVel.x - b.playerVel.x) +
                    std::fabs(a.playerVel.y - b.playerVel.y) +
                    std::fabs(a.playerVel.z - b.playerVel.z),
                a.activeEnemies - b.activeEnemies, a.eventCount - b.eventCount);
    return ok;
}

data_engine::Command consequentialCommand(int variant, int tick) {
    data_engine::Command command;
    if (variant == 1) {
        if (tick == 0) command.melee = true;
        else command.vacuum = true;
    }
    return command;
}

struct ConsequentialMetrics {
    int souls = 0;
    int filledGoals = 0;
    int enemies = 0;
    int slurpable = 0;
    float totalArmor = 0.0f;
    float totalHealth = 0.0f;
    float battery = 0.0f;
    int events = 0;
};

ConsequentialMetrics consequentialMetrics(const data_engine::DataEngine& engine) {
    const GameState& state = engine.authoritativeState();
    ConsequentialMetrics metrics;
    metrics.souls = state.player.souls;
    metrics.filledGoals = engine.observe().filledGoals;
    metrics.battery = state.player.battery;
    metrics.events = static_cast<int>(engine.events().size());
    for (const TargetState& target : state.targets) {
        if (target.alive) {
            ++metrics.enemies;
            metrics.totalArmor += target.armor;
            metrics.totalHealth += target.health;
        }
        if (target.slurpable) ++metrics.slurpable;
    }
    return metrics;
}

std::string observationDigest(const data_engine::DataEngine& engine) {
    return engine.observe().stateDigest;
}

bool meaningfullyDifferent(const ConsequentialMetrics& a, const ConsequentialMetrics& b) {
    return a.souls != b.souls || a.filledGoals != b.filledGoals ||
           a.events != b.events || a.slurpable != b.slurpable ||
           std::fabs(a.totalArmor - b.totalArmor) > 0.001f ||
           std::fabs(a.totalHealth - b.totalHealth) > 0.001f ||
           std::fabs(a.battery - b.battery) > 0.001f;
}

const char* meaningfulCause(const ConsequentialMetrics& a, const ConsequentialMetrics& b) {
    if (std::fabs(a.totalArmor - b.totalArmor) > 0.001f) return "armor_damage";
    if (std::fabs(a.totalHealth - b.totalHealth) > 0.001f) return "health_damage";
    if (a.slurpable != b.slurpable) return "soul_exposed";
    if (a.souls != b.souls) return "soul_captured";
    if (a.filledGoals != b.filledGoals) return "objective_progress";
    if (a.events != b.events) return "event_count";
    if (std::fabs(a.battery - b.battery) > 0.001f) return "battery_consumed";
    return "none";
}

bool runConsequentialCounterfactual(int seed, int room) {
    data_engine::DataEngine prepared;
    prepared.startGeneratedScenario(seed, room);
    if (!data_engine::ReferenceScenario::prepareEncounter(prepared)) {
        std::fprintf(stderr, "COUNTERFACTUAL_FAIL setup seed=%d room=%d\n", seed, room);
        return false;
    }
    const data_engine::DataEngine::Checkpoint checkpoint = prepared.checkpoint();
    const data_engine::Observation checkpointObservation = prepared.observe();
    const ConsequentialMetrics checkpointMetrics = consequentialMetrics(prepared);
    const std::string checkpointDigest = dbnet::authoritativeStateDigestHex(checkpoint.game.state);

    data_engine::DataEngine a;
    data_engine::DataEngine aRepeat;
    data_engine::DataEngine b;
    data_engine::DataEngine bRepeat;
    a.restore(checkpoint);
    aRepeat.restore(checkpoint);
    b.restore(checkpoint);
    bRepeat.restore(checkpoint);
    const bool restoreA = sameEngineCheckpoint(checkpoint, a.checkpoint()) &&
                          a.observe() == checkpointObservation;
    const bool restoreARepeat = sameEngineCheckpoint(checkpoint, aRepeat.checkpoint()) &&
                                aRepeat.observe() == checkpointObservation;
    const bool restoreB = sameEngineCheckpoint(checkpoint, b.checkpoint()) &&
                          b.observe() == checkpointObservation;
    const bool restoreBRepeat = sameEngineCheckpoint(checkpoint, bRepeat.checkpoint()) &&
                                bRepeat.observe() == checkpointObservation;
    const bool restoresOk = restoreA && restoreARepeat && restoreB && restoreBRepeat;

    int firstDigestTick = -1;
    int firstMeaningfulTick = -1;
    int firstArmorTick = -1;
    int firstSoulTick = -1;
    int firstBatteryTick = -1;
    std::string firstCause = "none";
    bool aRepeatsOk = true;
    bool bRepeatsOk = true;

    for (int tick = 0; tick < kConsequentialTicks; ++tick) {
        const data_engine::Command commandA = consequentialCommand(0, tick);
        const data_engine::Command commandB = consequentialCommand(1, tick);
        const data_engine::Observation observationA = a.step(commandA);
        const data_engine::Observation observationB = b.step(commandB);
        const data_engine::Observation observationARepeat = aRepeat.step(commandA);
        const data_engine::Observation observationBRepeat = bRepeat.step(commandB);
        aRepeatsOk = aRepeatsOk && observationA == observationARepeat;
        bRepeatsOk = bRepeatsOk && observationB == observationBRepeat;

        if (firstDigestTick < 0 && !(observationA == observationB)) firstDigestTick = tick;
        const ConsequentialMetrics metricsA = consequentialMetrics(a);
        const ConsequentialMetrics metricsB = consequentialMetrics(b);
        if (firstMeaningfulTick < 0 && meaningfullyDifferent(metricsA, metricsB)) {
            firstMeaningfulTick = tick;
            firstCause = meaningfulCause(metricsA, metricsB);
        }
        if (firstArmorTick < 0 &&
            std::fabs(metricsA.totalArmor - metricsB.totalArmor) > 0.001f) firstArmorTick = tick;
        if (firstSoulTick < 0 && metricsA.souls != metricsB.souls) firstSoulTick = tick;
        if (firstBatteryTick < 0 &&
            std::fabs(metricsA.battery - metricsB.battery) > 0.001f) firstBatteryTick = tick;
    }

    const ConsequentialMetrics finalA = consequentialMetrics(a);
    const ConsequentialMetrics finalB = consequentialMetrics(b);
    const bool aFinalStatesOk = sameEngineCheckpoint(a.checkpoint(), aRepeat.checkpoint());
    const bool bFinalStatesOk = sameEngineCheckpoint(b.checkpoint(), bRepeat.checkpoint());
    const bool finalStatesOk = aFinalStatesOk && bFinalStatesOk;
    const bool ok = restoresOk && aRepeatsOk && bRepeatsOk && finalStatesOk &&
                    firstMeaningfulTick >= 0 && firstSoulTick >= 0;

    std::printf("COUNTERFACTUAL_SETUP seed=%d room=%d checkpoint_digest=%s frame=%d enemies=%d souls=%d battery=%.3f events=%d\n",
                seed, room, checkpointDigest.c_str(), checkpointObservation.frame,
                checkpointMetrics.enemies, checkpointMetrics.souls,
                checkpointMetrics.battery, checkpointMetrics.events);
    std::printf("COUNTERFACTUAL_RESTORE all_state_restored=%d A=%d A_repeat=%d B=%d B_repeat=%d\n",
                restoresOk ? 1 : 0, restoreA ? 1 : 0, restoreARepeat ? 1 : 0,
                restoreB ? 1 : 0, restoreBRepeat ? 1 : 0);
    std::printf("COUNTERFACTUAL_DIVERGENCE first_digest_tick=%d first_meaningful_tick=%d cause=%s first_armor_damage_tick=%d first_soul_captured_tick=%d first_battery_consumed_tick=%d\n",
                firstDigestTick, firstMeaningfulTick, firstCause.c_str(), firstArmorTick,
                firstSoulTick, firstBatteryTick);
    std::printf("COUNTERFACTUAL_DIGESTS A=%s A_repeat=%s B=%s B_repeat=%s\n",
                observationDigest(a).c_str(), observationDigest(aRepeat).c_str(),
                observationDigest(b).c_str(), observationDigest(bRepeat).c_str());
    std::printf("COUNTERFACTUAL_REPEAT A_identical=%d B_identical=%d hidden_runtime=1 event_bookkeeping=1 previous_bookkeeping=1\n",
                aRepeatsOk && aFinalStatesOk ? 1 : 0,
                bRepeatsOk && bFinalStatesOk ? 1 : 0);
    std::printf("COUNTERFACTUAL_OUTCOMES A{souls=%d goals=%d armor=%.3f health=%.3f slurpable=%d battery=%.3f enemies=%d events=%d} B{souls=%d goals=%d armor=%.3f health=%.3f slurpable=%d battery=%.3f enemies=%d events=%d} delta{souls=%d armor=%.3f battery=%.3f}\n",
                finalA.souls, finalA.filledGoals, finalA.totalArmor, finalA.totalHealth,
                finalA.slurpable, finalA.battery, finalA.enemies, finalA.events,
                finalB.souls, finalB.filledGoals, finalB.totalArmor, finalB.totalHealth,
                finalB.slurpable, finalB.battery, finalB.enemies, finalB.events,
                finalA.souls - finalB.souls, finalA.totalArmor - finalB.totalArmor,
                finalA.battery - finalB.battery);
    return ok;
}

bool runEvaluatorScenario(const char* scenario,
                          const data_engine::DataEngine::Checkpoint& checkpoint,
                          const char* expectedAction) {
    namespace counterfactual = data_engine::counterfactual;
    const std::vector<counterfactual::Candidate> candidates = counterfactual::defaultCandidates();
    constexpr int horizon = counterfactual::kMaxHorizonTicks;

    const GameState& initial = checkpoint.game.state;
    int initialEnemies = 0;
    for (const TargetState& target : initial.targets) {
        if (target.alive) ++initialEnemies;
    }
    std::printf("EVALUATOR_SETUP scenario=%s checkpoint_digest=%s frame=%d enemies=%d souls=%d battery=%.3f\n",
                scenario, dbnet::authoritativeStateDigestHex(initial).c_str(), initial.frame,
                initialEnemies, initial.player.souls, initial.player.battery);

    data_engine::DataEngine live;
    live.restore(checkpoint);
    const data_engine::DataEngine::Checkpoint liveBefore = live.checkpoint();
    const data_engine::Observation liveObservationBefore = live.observe();

    const auto evaluationStart = std::chrono::steady_clock::now();
    const counterfactual::Evaluation evaluation = counterfactual::evaluate(checkpoint, candidates, horizon);
    const auto evaluationEnd = std::chrono::steady_clock::now();

    const bool liveUnchanged = sameEngineCheckpoint(liveBefore, live.checkpoint()) &&
                               liveObservationBefore == live.observe();
    std::printf("EVALUATOR_LIVE_UNCHANGED scenario=%s checkpoint=%d observation=%d\n",
                scenario, sameEngineCheckpoint(liveBefore, live.checkpoint()) ? 1 : 0,
                liveObservationBefore == live.observe() ? 1 : 0);

    if (!evaluation.ok || evaluation.selectedIndex < 0 || !liveUnchanged) {
        std::fprintf(stderr, "EVALUATOR_FAIL scenario=%s evaluation=%d selected=%d live_unchanged=%d\n",
                     scenario, evaluation.ok ? 1 : 0, evaluation.selectedIndex,
                     liveUnchanged ? 1 : 0);
        return false;
    }

    for (const counterfactual::Outcome& outcome : evaluation.outcomes) {
        std::printf("EVALUATOR_CANDIDATE scenario=%s index=%d action=%s score=%d survival=%d neutralization=%d souls=%d resource=%d battery_delta=%.3f digest=%s\n",
                    scenario, outcome.index, outcome.action.c_str(), outcome.score.total,
                    outcome.score.survival, outcome.score.neutralization, outcome.score.souls,
                    outcome.score.resource, outcome.batteryDelta,
                    outcome.observation.stateDigest.c_str());
    }

    const counterfactual::Outcome& predicted = evaluation.outcomes[evaluation.selectedIndex];
    const bool selectedExpected = predicted.action == expectedAction;
    std::printf("EVALUATOR_SELECTED scenario=%s index=%d action=%s expected=%s match=%d\n",
                scenario, predicted.index, predicted.action.c_str(), expectedAction,
                selectedExpected ? 1 : 0);

    live.restore(checkpoint);
    const auto executionStart = std::chrono::steady_clock::now();
    const data_engine::Observation actualObservation =
        counterfactual::execute(live, predicted.actionKind, evaluation.horizonTicks);
    const auto executionEnd = std::chrono::steady_clock::now();

    const bool digestEquivalent = predicted.observation.stateDigest == actualObservation.stateDigest;
    const bool observationEquivalent = predicted.observation == actualObservation;
    const bool eventsEquivalent = predicted.events == live.events();
    const bool hiddenRuntimeEquivalent = sameEngineCheckpoint(predicted.checkpoint, live.checkpoint());
    const bool predictedEquivalent = digestEquivalent && observationEquivalent &&
                                     eventsEquivalent && hiddenRuntimeEquivalent;
    std::printf("EVALUATOR_PREDICTED_EXECUTED scenario=%s digest=%d observation=%d events=%d hidden_runtime=%d all=%d\n",
                scenario, digestEquivalent ? 1 : 0, observationEquivalent ? 1 : 0,
                eventsEquivalent ? 1 : 0, hiddenRuntimeEquivalent ? 1 : 0,
                predictedEquivalent ? 1 : 0);

    const double evaluateMs =
        std::chrono::duration<double, std::milli>(evaluationEnd - evaluationStart).count();
    const double executeMs =
        std::chrono::duration<double, std::milli>(executionEnd - executionStart).count();
    std::printf("EVALUATOR_COST scenario=%s candidates=%zu horizon=%d evaluated_ticks=%d executed_ticks=%d total_ticks=%d evaluate_ms=%.3f execute_ms=%.3f\n",
                scenario, candidates.size(), horizon, evaluation.evaluatedTicks,
                evaluation.horizonTicks, evaluation.evaluatedTicks + evaluation.horizonTicks,
                evaluateMs, executeMs);
    return selectedExpected && predictedEquivalent;
}

bool runEvaluatorExperiments() {
    data_engine::DataEngine vulnerable;
    vulnerable.startGeneratedScenario(424242, 3);
    if (!data_engine::ReferenceScenario::prepareEncounter(vulnerable)) {
        std::fprintf(stderr, "EVALUATOR_FAIL setup=vulnerable_encounter\n");
        return false;
    }
    const bool vulnerableOk = runEvaluatorScenario(
        "vulnerable_encounter", vulnerable.checkpoint(), "melee_vacuum");

    data_engine::DataEngine distant;
    distant.startGeneratedScenario(424242, 3);
    for (int tick = 0; tick < kWarmupTicks; ++tick) {
        distant.step(commandForTick(tick, 0));
    }
    const bool distantOk = runEvaluatorScenario(
        "distant_enemies", distant.checkpoint(), "idle");
    return vulnerableOk && distantOk;
}

}  // namespace

int main() {
    const int seeds[] = {424242, 900001, 1300001, 1700001, 2265753};
    const bool consequentialOk = runConsequentialCounterfactual(424242, 3);
    const bool alternativeOk = runAlternativeFuturesEvidence(seeds[0]);
    const bool evaluatorOk = runEvaluatorExperiments();
    bool ok = consequentialOk && alternativeOk && evaluatorOk;
    int minimumDivergence = kContinuationTicks;
    for (int seed : seeds) {
        ok = ok && runGameCheckpoint(seed);
        int divergentObservations = 0;
        ok = ok && runEngineCheckpoint(seed, divergentObservations);
        if (divergentObservations < minimumDivergence) minimumDivergence = divergentObservations;
    }

    if (!ok) {
        std::fprintf(stderr, "CHECKPOINT_REGRESSION_FAIL\n");
        return 1;
    }
    std::printf("CHECKPOINT_REGRESSION_OK seeds=%zu continuation_ticks=%d minimum_divergent_observations=%d hidden_runtime=1 event_bookkeeping=1\n",
                sizeof(seeds) / sizeof(seeds[0]), kContinuationTicks, minimumDivergence);
    return 0;
}
