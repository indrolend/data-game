#pragma once

#include <algorithm>
#include <cmath>

#include "Math.hpp"

namespace gameplay {

// Physical response state sits below behavioral and motor intent. It owns
// inertia, traction, support reaction, impacts, falling, and recovery; it does
// not decide where the enemy wants to go.
struct PhysicalEnemyBodyState {
    bool initialized = false;
    bool fallen = false;
    float bodyPitch = 0.0f;
    float bodyRoll = 0.0f;
    float pitchVelocity = 0.0f;
    float rollVelocity = 0.0f;
    float yawVelocity = 0.0f;
    float gaitPhase = 0.0f;
    float recovery = 0.0f;
    float supportContact = 0.0f;
    float disruption = 0.0f;
    float impactInstability = 0.0f;
    float scrambleAmount = 0.0f;
};

struct PhysicalEnemyBodyInput {
    Vec3 desiredVelocity{};
    Vec3 actualVelocity{};
    float desiredYaw = 0.0f;
    float individuality = 0.0f;
    float brace = 0.0f;
    float dt = 0.0f;
    bool grounded = true;
    float leftFootContact = 1.0f;
    float rightFootContact = 1.0f;
    Vec3 supportNormal{0.0f, 1.0f, 0.0f};
};

struct PhysicalEnemyBodyOutput {
    Vec3 velocity{};
    float yaw = 0.0f;
    float locomotion = 0.0f;
};

inline float physicalFinite(float value, float fallback = 0.0f) {
    return std::isfinite(value) ? value : fallback;
}

inline Vec3 physicalFiniteVector(const Vec3& value, const Vec3& fallback = {}) {
    return {physicalFinite(value.x, fallback.x), physicalFinite(value.y, fallback.y),
            physicalFinite(value.z, fallback.z)};
}

inline Vec3 boundedPhysicalMotion(const Vec3& value) {
    const Vec3 finite = physicalFiniteVector(value);
    return {clampf(finite.x, -100.0f, 100.0f), clampf(finite.y, -100.0f, 100.0f),
            clampf(finite.z, -100.0f, 100.0f)};
}

inline float physicalWrappedAngle(float angle) {
    const float finite = physicalFinite(angle);
    return std::atan2(std::sin(finite), std::cos(finite));
}

inline float physicalAngleDelta(float from, float to) {
    return std::atan2(std::sin(to - from), std::cos(to - from));
}

inline PhysicalEnemyBodyOutput updatePhysicalEnemyBody(
    PhysicalEnemyBodyState& body,
    const PhysicalEnemyBodyInput& input,
    float currentYaw)
{
    const float dt = clampf(physicalFinite(input.dt), 0.0f, 1.0f / 20.0f);
    if (!body.initialized) {
        body = {};
        body.initialized = true;
    }

    body.bodyPitch = clampf(physicalFinite(body.bodyPitch), -1.45f, 1.45f);
    body.bodyRoll = clampf(physicalFinite(body.bodyRoll), -1.45f, 1.45f);
    body.pitchVelocity = clampf(physicalFinite(body.pitchVelocity), -4.5f, 4.5f);
    body.rollVelocity = clampf(physicalFinite(body.rollVelocity), -4.5f, 4.5f);
    body.yawVelocity = clampf(physicalFinite(body.yawVelocity), -2.6f, 2.6f);
    body.gaitPhase = std::fmod(physicalFinite(body.gaitPhase), 2.0f * 3.14159265358979323846f);
    if (body.gaitPhase < 0.0f) body.gaitPhase += 2.0f * 3.14159265358979323846f;
    body.recovery = std::max(0.0f, physicalFinite(body.recovery));
    body.supportContact = clampf(physicalFinite(body.supportContact), 0.0f, 1.0f);
    body.disruption = clampf(physicalFinite(body.disruption), 0.0f, 1.0f);
    body.impactInstability = clampf(physicalFinite(body.impactInstability), 0.0f, 1.0f);
    body.scrambleAmount = clampf(physicalFinite(body.scrambleAmount), 0.0f, 1.0f);

    const Vec3 desiredVelocity = boundedPhysicalMotion(input.desiredVelocity);
    const Vec3 actualVelocity = boundedPhysicalMotion(input.actualVelocity);
    const float requestedSpeed = horizontalLength(desiredVelocity);
    const float actualSpeed = horizontalLength(actualVelocity);
    const float personality = clampf(physicalFinite(input.individuality), -1.0f, 1.0f);
    const float brace = clampf(physicalFinite(input.brace), 0.0f, 1.0f);
    const float currentYawSafe = physicalWrappedAngle(currentYaw);
    const float desiredYaw = physicalWrappedAngle(physicalFinite(input.desiredYaw, currentYawSafe));

    const float yawError = physicalAngleDelta(currentYawSafe, desiredYaw);
    const float yawTorque = yawError * (4.1f + brace * 1.2f)
        - body.yawVelocity * (2.25f + brace * 0.55f);
    body.yawVelocity = clampf(body.yawVelocity + yawTorque * dt, -2.6f, 2.6f);
    const float nextYaw = currentYawSafe + body.yawVelocity * dt;

    const float leftContact = input.grounded
        ? clampf(physicalFinite(input.leftFootContact), 0.0f, 1.0f) : 0.0f;
    const float rightContact = input.grounded
        ? clampf(physicalFinite(input.rightFootContact), 0.0f, 1.0f) : 0.0f;
    const float contact = std::max(leftContact, rightContact);
    body.supportContact = contact;
    const float velocityError = horizontalLength(desiredVelocity - actualVelocity);
    const float slip = std::min(1.0f, velocityError / 3.0f) * (1.0f - contact);
    body.scrambleAmount = slip;
    const float cadence = std::min(14.0f, 3.5f + std::min(actualSpeed, 6.0f) * 1.42f
        + slip * 2.0f + personality * 0.20f);
    if (!body.fallen && input.grounded && (actualSpeed > 0.06f || requestedSpeed > 0.20f))
        body.gaitPhase = std::fmod(body.gaitPhase + cadence * dt, 2.0f * 3.14159265358979323846f);

    Vec3 velocity = actualVelocity;
    if (!body.fallen && contact > 0.01f) {
        const float traction = 2.2f + contact * (4.7f + brace * 1.4f);
        const float response = std::min(1.0f, traction * dt);
        velocity.x += (desiredVelocity.x - velocity.x) * response;
        velocity.z += (desiredVelocity.z - velocity.z) * response;
    } else {
        const float drag = std::exp(-(body.fallen ? 4.2f : 0.35f) * dt);
        velocity.x *= drag;
        velocity.z *= drag;
    }

    const Vec3 facing{-std::sin(nextYaw), 0.0f, -std::cos(nextYaw)};
    const Vec3 right{facing.z, 0.0f, -facing.x};
    const float forwardError = dot3(desiredVelocity - velocity, facing);
    const float lateralVelocity = dot3(velocity, right);
    Vec3 supportNormal = normalized(physicalFiniteVector(input.supportNormal, {0.0f, 1.0f, 0.0f}));
    if (lengthSq(supportNormal) < 0.5f) supportNormal = {0.0f, 1.0f, 0.0f};
    const float supportPitch = std::atan2(dot3(supportNormal, facing), std::max(0.1f, supportNormal.y)) * 0.18f;
    const float supportRoll = -std::atan2(dot3(supportNormal, right), std::max(0.1f, supportNormal.y)) * 0.18f;
    const float singleSupportRoll = (rightContact - leftContact) * 0.082f;
    const float scramble = std::sin(body.gaitPhase * 1.37f + personality * 2.7f) * slip;
    const bool recovering = body.fallen && body.recovery > 1.25f && actualSpeed < 0.45f;
    const float desiredPitch = recovering ? 0.0f
        : body.fallen ? (body.bodyPitch >= 0.0f ? 1.28f : -1.28f)
        : clampf(-forwardError * 0.115f + supportPitch, -0.44f, 0.32f);
    const float desiredRoll = recovering ? 0.0f
        : body.fallen ? (body.bodyRoll >= 0.0f ? 0.82f : -0.82f)
        : clampf(lateralVelocity * 0.13f - body.yawVelocity * actualSpeed * 0.052f
            + supportRoll + singleSupportRoll + scramble * 0.075f, -0.38f, 0.38f);
    const float balanceFrequency = body.fallen ? 2.4f : 5.3f + brace * 2.0f;
    const float wobble = std::sin(body.gaitPhase * 0.5f + personality * 2.7f)
        * (actualSpeed * 0.032f + slip * 0.065f);
    body.pitchVelocity += ((desiredPitch - body.bodyPitch) * balanceFrequency * balanceFrequency
        - body.pitchVelocity * (3.35f + brace * 1.35f)) * dt;
    body.rollVelocity += ((desiredRoll + wobble - body.bodyRoll) * balanceFrequency * balanceFrequency
        - body.rollVelocity * (3.35f + brace * 1.35f)) * dt;
    body.pitchVelocity = clampf(body.pitchVelocity, -4.5f, 4.5f);
    body.rollVelocity = clampf(body.rollVelocity, -4.5f, 4.5f);
    body.bodyPitch = clampf(body.bodyPitch + body.pitchVelocity * dt, -1.45f, 1.45f);
    body.bodyRoll = clampf(body.bodyRoll + body.rollVelocity * dt, -1.45f, 1.45f);
    body.impactInstability *= std::exp(-1.8f * dt);

    if (!body.fallen && (std::abs(body.bodyPitch) > 0.82f || std::abs(body.bodyRoll) > 0.76f)) {
        body.fallen = true;
        body.recovery = 0.0f;
    }
    if (body.fallen) {
        body.recovery += dt;
        if (recovering) {
            body.pitchVelocity += (-body.bodyPitch * 17.0f - body.pitchVelocity * 4.0f) * dt;
            body.rollVelocity += (-body.bodyRoll * 17.0f - body.rollVelocity * 4.0f) * dt;
            if (std::abs(body.bodyPitch) < 0.20f && std::abs(body.bodyRoll) < 0.20f) {
                body.fallen = false;
                body.recovery = 0.0f;
            }
        }
    }

    const float lostSupport = !input.grounded ? 1.0f
        : clampf((0.08f - contact) / 0.08f, 0.0f, 1.0f);
    const float excessPitch = clampf((std::abs(body.bodyPitch) - 0.50f) / 0.32f, 0.0f, 1.0f);
    const float excessRoll = clampf((std::abs(body.bodyRoll) - 0.44f) / 0.32f, 0.0f, 1.0f);
    const float abnormalAngularMotion = std::max(
        std::max(0.0f, (std::abs(body.pitchVelocity) - 2.2f) / 2.3f),
        std::max(0.0f, (std::abs(body.rollVelocity) - 2.2f) / 2.3f));
    body.disruption = std::max(body.fallen ? 1.0f : 0.0f,
        std::max(lostSupport, std::max(body.impactInstability,
            std::max(excessPitch, std::max(excessRoll, std::min(1.0f, abnormalAngularMotion))))));

    return {velocity, physicalWrappedAngle(nextYaw),
            body.fallen ? 0.0f : std::min(1.0f, horizontalLength(velocity) / 0.7f)};
}

inline void applyPhysicalEnemyImpact(PhysicalEnemyBodyState& body, const Vec3& localImpulse) {
    if (!body.initialized) body.initialized = true;
    const Vec3 impulse = boundedPhysicalMotion(localImpulse);
    body.pitchVelocity = clampf(physicalFinite(body.pitchVelocity) + impulse.z * 0.34f, -4.5f, 4.5f);
    body.rollVelocity = clampf(physicalFinite(body.rollVelocity) - impulse.x * 0.34f, -4.5f, 4.5f);
    body.impactInstability = std::max(clampf(physicalFinite(body.impactInstability), 0.0f, 1.0f),
        std::min(1.0f, horizontalLength(impulse) / 4.0f));
}

} // namespace gameplay
