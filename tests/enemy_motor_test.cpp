#include <cassert>
#include <cmath>
#include <cstdio>

#include "gameplay/EnemyMotor.hpp"

namespace {
constexpr float Dt = 1.0f / 60.0f;

gameplay::EnemyMotorInput representativeInput() {
    gameplay::EnemyMotorInput input{};
    input.toPlayer = {0.0f, 0.0f, -1.0f};
    input.playerDistance = 7.0f;
    input.playerVelocity = {0.45f, 0.0f, -0.20f};
    input.toNearestAlly = {1.0f, 0.0f, 0.0f};
    input.nearestAllyDistance = 1.3f;
    input.roomPressure = 0.65f;
    input.bodySpeed = 2.4f;
    return input;
}
}

int main() {
    using namespace gameplay;
    auto input = representativeInput();
    EnemyMotorMemory firstMemory{}, repeatMemory{};
    EnemyMotorOutput first{};
    for (int step = 0; step < 180; ++step) {
        first = updateEnemyMotor(input, firstMemory, Dt, 0.22f);
        const auto repeat = updateEnemyMotor(input, repeatMemory, Dt, 0.22f);
        assert(first.steering.x == repeat.steering.x && first.steering.z == repeat.steering.z);
        assert(first.speedScale == repeat.speedScale && first.brace == repeat.brace);
    }
    assert(std::abs(horizontalLength(first.steering) - 1.0f) < 0.0001f);
    assert(first.speedScale >= 0.36f && first.speedScale <= 1.16f);
    assert(first.brace >= 0.0f && first.brace <= 1.0f);
    assert(std::abs(first.steering.x) > 0.02f);

    EnemyMotorInput certain = representativeInput();
    certain.pursuitCertainty = 1.0f;
    EnemyMotorInput uncertain = certain;
    uncertain.pursuitCertainty = 0.22f;
    EnemyMotorMemory certainMemory{}, uncertainMemory{};
    EnemyMotorOutput chase{}, investigate{};
    for (int step = 0; step < 120; ++step) {
        chase = updateEnemyMotor(certain, certainMemory, Dt, 0.17f);
        investigate = updateEnemyMotor(uncertain, uncertainMemory, Dt, 0.17f);
    }
    assert(investigate.speedScale < chase.speedScale);
    assert(investigate.brace > chase.brace);

    EnemyMotorInput stable = representativeInput();
    stable.pursuitProgress = 0.45f;
    stable.traction = 1.0f;
    EnemyMotorInput disrupted = stable;
    disrupted.physicalDisruption = 1.0f;
    EnemyMotorMemory stableMemory{}, disruptedMemory{};
    for (int step = 0; step < 90; ++step) {
        updateEnemyMotor(stable, stableMemory, Dt, 0.21f);
        updateEnemyMotor(stable, disruptedMemory, Dt, 0.21f);
    }
    const auto calm = updateEnemyMotor(stable, stableMemory, Dt, 0.21f);
    const auto recovery = updateEnemyMotor(disrupted, disruptedMemory, Dt, 0.21f);
    assert(recovery.speedScale < calm.speedScale);
    assert(recovery.brace > calm.brace);

    EnemyMotorMemory poisoned{};
    poisoned.hidden.fill(NAN);
    poisoned.tendency.fill(INFINITY);
    poisoned.arousal = poisoned.caution = poisoned.fixation = NAN;
    input.toPlayer = {NAN, INFINITY, -INFINITY};
    input.playerDistance = input.traction = input.physicalDisruption = NAN;
    const auto finite = updateEnemyMotor(input, poisoned, INFINITY, NAN);
    assert(std::isfinite(finite.steering.x) && std::isfinite(finite.steering.z));
    assert(std::isfinite(finite.speedScale) && std::isfinite(finite.brace));

    std::puts("ENEMY_MOTOR_TEST_OK deterministic=YES bounded=YES cognition_authority=NONE adaptation=CONTINUOUS");
}
