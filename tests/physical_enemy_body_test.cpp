#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>

#include "gameplay/PhysicalEnemyBody.hpp"

namespace {
constexpr float dt = 1.0f / 60.0f;

gameplay::PhysicalEnemyBodyOutput step(
    gameplay::PhysicalEnemyBodyState& body,
    gameplay::PhysicalEnemyBodyInput& input,
    float& yaw)
{
    input.dt = dt;
    const auto output = gameplay::updatePhysicalEnemyBody(body, input, yaw);
    input.actualVelocity = output.velocity;
    yaw = output.yaw;
    return output;
}

void responseAndSupportContracts() {
    gameplay::PhysicalEnemyBodyState accelerating{};
    gameplay::PhysicalEnemyBodyInput input{};
    input.desiredVelocity = {0.0f, 0.0f, -3.0f};
    float yaw = 0.0f;
    const auto first = step(accelerating, input, yaw);
    assert(first.velocity.z < 0.0f && first.velocity.z > -3.0f);
    for (int frame = 0; frame < 24; ++frame) step(accelerating, input, yaw);
    assert(accelerating.bodyPitch < -0.08f);
    assert(accelerating.disruption == 0.0f);

    gameplay::PhysicalEnemyBodyState unsupported{};
    gameplay::PhysicalEnemyBodyInput noContact{};
    noContact.desiredVelocity = {0.0f, 0.0f, -3.0f};
    noContact.leftFootContact = noContact.rightFootContact = 0.0f;
    float unsupportedYaw = 0.0f;
    for (int frame = 0; frame < 90; ++frame) step(unsupported, noContact, unsupportedYaw);
    assert(horizontalLength(noContact.actualVelocity) < 0.001f);
    assert(unsupported.scrambleAmount > 0.9f);
    assert(unsupported.disruption > 0.9f);
    noContact.leftFootContact = 1.0f;
    for (int frame = 0; frame < 30; ++frame) step(unsupported, noContact, unsupportedYaw);
    assert(horizontalLength(noContact.actualVelocity) > 1.0f);
    assert(unsupported.disruption < 0.05f);

    gameplay::PhysicalEnemyBodyState alternating{};
    gameplay::PhysicalEnemyBodyInput alternatingInput{};
    alternatingInput.desiredVelocity = {0.0f, 0.0f, -2.0f};
    float alternatingYaw = 0.0f;
    for (int frame = 0; frame < 240; ++frame) {
        alternatingInput.leftFootContact = frame % 2 == 0 ? 1.0f : 0.0f;
        alternatingInput.rightFootContact = 1.0f - alternatingInput.leftFootContact;
        step(alternating, alternatingInput, alternatingYaw);
        assert(alternating.supportContact == 1.0f);
        assert(std::abs(alternating.bodyRoll) < 0.76f);
        assert(alternating.disruption == 0.0f);
    }
}

void terrainImpactAndRecoveryContracts() {
    gameplay::PhysicalEnemyBodyState slope{};
    gameplay::PhysicalEnemyBodyInput slopeInput{};
    slopeInput.supportNormal = normalized(Vec3{0.0f, 0.92f, 0.38f});
    float slopeYaw = 0.0f;
    for (int frame = 0; frame < 90; ++frame) step(slope, slopeInput, slopeYaw);
    assert(std::abs(slope.bodyPitch) > 0.02f && std::abs(slope.bodyPitch) < 0.20f);

    gameplay::PhysicalEnemyBodyState first{}, repeat{};
    gameplay::PhysicalEnemyBodyInput a{}, b{};
    a.desiredVelocity = b.desiredVelocity = {0.0f, 0.0f, -1.4f};
    a.individuality = b.individuality = -0.23f;
    float firstYaw = 0.35f, repeatYaw = 0.35f;
    gameplay::applyPhysicalEnemyImpact(first, {1.8f, 0.0f, 2.4f});
    gameplay::applyPhysicalEnemyImpact(repeat, {1.8f, 0.0f, 2.4f});
    for (int frame = 0; frame < 360; ++frame) {
        const auto firstOutput = step(first, a, firstYaw);
        const auto repeatOutput = step(repeat, b, repeatYaw);
        assert(firstOutput.velocity.x == repeatOutput.velocity.x);
        assert(firstOutput.velocity.z == repeatOutput.velocity.z);
        assert(first.bodyPitch == repeat.bodyPitch && first.bodyRoll == repeat.bodyRoll);
        assert(std::isfinite(first.bodyPitch) && std::isfinite(first.bodyRoll));
    }
    assert(!first.fallen);
    assert(std::abs(first.bodyPitch) < 0.20f && std::abs(first.bodyRoll) < 0.20f);

    gameplay::PhysicalEnemyBodyState fallen{};
    fallen.initialized = true;
    fallen.fallen = true;
    fallen.bodyPitch = 1.0f;
    gameplay::PhysicalEnemyBodyInput rest{};
    float restYaw = 0.0f;
    for (int frame = 0; frame < 480; ++frame) step(fallen, rest, restYaw);
    assert(!fallen.fallen);
}

void adversarialInputsRecover() {
    gameplay::PhysicalEnemyBodyState body{};
    body.initialized = true;
    body.bodyPitch = NAN;
    body.bodyRoll = INFINITY;
    body.pitchVelocity = -INFINITY;
    body.rollVelocity = NAN;
    body.yawVelocity = INFINITY;
    body.gaitPhase = NAN;
    body.recovery = INFINITY;
    body.supportContact = NAN;
    body.disruption = INFINITY;
    gameplay::PhysicalEnemyBodyInput input{};
    input.desiredVelocity = {NAN, INFINITY, -INFINITY};
    input.actualVelocity = {INFINITY, NAN, -INFINITY};
    input.desiredYaw = NAN;
    input.dt = NAN;
    input.supportNormal = {NAN, NAN, NAN};
    const auto output = gameplay::updatePhysicalEnemyBody(body, input, INFINITY);
    assert(std::isfinite(output.velocity.x) && std::isfinite(output.velocity.z));
    assert(std::isfinite(output.yaw) && std::isfinite(output.locomotion));
    assert(std::isfinite(body.bodyPitch) && std::isfinite(body.bodyRoll));
    assert(body.supportContact >= 0.0f && body.supportContact <= 1.0f);
    assert(body.disruption >= 0.0f && body.disruption <= 1.0f);
}
} // namespace

int main() {
    responseAndSupportContracts();
    terrainImpactAndRecoveryContracts();
    adversarialInputsRecover();

    std::array<gameplay::PhysicalEnemyBodyState, 32> crowd{};
    std::array<Vec3, 32> velocity{};
    const auto start = std::chrono::steady_clock::now();
    for (int frame = 0; frame < 3600; ++frame) {
        for (int index = 0; index < 32; ++index) {
            gameplay::PhysicalEnemyBodyInput input{};
            input.desiredVelocity = {std::sin(index * 0.71f) * 1.5f, 0.0f, -1.2f};
            input.actualVelocity = velocity[index];
            input.desiredYaw = std::sin(frame * 0.002f + index) * 0.8f;
            input.individuality = std::sin(index * 12.9898f);
            input.brace = 0.5f;
            input.dt = dt;
            velocity[index] = gameplay::updatePhysicalEnemyBody(crowd[index], input, 0.0f).velocity;
        }
    }
    const double milliseconds = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();
    assert(std::isfinite(velocity.back().x));
    std::puts("PHYSICAL_ENEMY_BODY_TEST_OK");
    std::printf("PHYSICAL_ENEMY_BODY_BENCHMARK crowd=32 simulated_seconds=60 total_ms=%.3f us_per_enemy_frame=%.5f\n",
        milliseconds, milliseconds * 1000.0 / (3600.0 * 32.0));
}
