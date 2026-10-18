#pragma once

#include <cmath>

#include "Math.hpp"

namespace diagnostics {

// Evaluation-only reconstruction of the direct-pursuit policy from
// convergence/zombie-v1. It is intentionally not a selectable runtime motor:
// the historical policy received an authoritative target vector and had no
// perception, cognition, navigation, or physical-disruption inputs.
struct HistoricalRelentlessInput {
    Vec3 toAuthoritativeTarget{};
    float targetDistance = 0.0f;
    bool targetValid = false;
};

struct HistoricalRelentlessOutput {
    Vec3 steering{};
    float speedScale = 0.0f;
    float attackCommitment = 0.0f;
    float brace = 0.62f;
    bool pursuing = false;
};

inline HistoricalRelentlessOutput evaluateHistoricalRelentless(
    const HistoricalRelentlessInput& input)
{
    HistoricalRelentlessOutput output{};
    if (!input.targetValid || !std::isfinite(input.targetDistance)) return output;

    const Vec3 horizontal{
        input.toAuthoritativeTarget.x,
        0.0f,
        input.toAuthoritativeTarget.z
    };
    const float magnitude = std::sqrt(
        horizontal.x * horizontal.x + horizontal.z * horizontal.z);
    if (magnitude > 0.0001f)
        output.steering = horizontal * (1.0f / magnitude);
    output.speedScale = 1.0f;
    output.attackCommitment = 1.0f;
    output.pursuing = true;
    return output;
}

} // namespace diagnostics
