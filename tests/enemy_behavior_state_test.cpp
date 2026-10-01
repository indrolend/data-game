#include <cassert>
#include <cmath>
#include <cstdio>

#include "gameplay/EnemyBehaviorState.hpp"

namespace {
constexpr float Dt = 1.0f / 60.0f;

gameplay::EnemyBehaviorOutput advance(
    gameplay::EnemyBehaviorState& state,
    gameplay::EnemyBehaviorInput& input,
    int frames)
{
    gameplay::EnemyBehaviorOutput output{};
    input.dt = Dt;
    for (int frame = 0; frame < frames; ++frame)
        output = gameplay::updateEnemyBehavior(state, input);
    return output;
}
}

int main() {
    using namespace gameplay;
    EnemyBehaviorState state{};
    EnemyBehaviorInput input{};

    auto output = advance(state, input, 90);
    assert(output.mode == EnemyBehaviorMode::Rest);
    assert(output.travelScale == 0.0f && output.settled && !output.mayAttack);

    input.vagueAwareness = 0.32f;
    output = advance(state, input, 30);
    assert(output.mode == EnemyBehaviorMode::Orient);
    assert(output.travelScale == 0.0f && !output.mayAttack);

    input.vagueAwareness = 0.0f;
    input.hasSpatialBelief = true;
    input.confidence = 0.24f;
    output = advance(state, input, 30);
    assert(output.mode == EnemyBehaviorMode::Investigate);
    assert(output.travelScale > 0.0f && !output.mayAttack);

    input.confirmed = true;
    input.confidence = 0.80f;
    output = advance(state, input, 1);
    assert(output.mode == EnemyBehaviorMode::Engage && output.mayAttack);

    input.confirmed = false;
    input.confidence = 0.20f;
    input.uncertainty = 0.72f;
    output = advance(state, input, 30);
    assert(output.mode == EnemyBehaviorMode::Search);
    assert(output.travelScale > 0.0f && output.travelScale < 1.0f && !output.mayAttack);

    input.hasSpatialBelief = false;
    input.confidence = 0.0f;
    input.uncertainty = 1.0f;
    output = advance(state, input, 500);
    assert(output.mode == EnemyBehaviorMode::Rest);
    assert(output.settled && output.travelScale == 0.0f && !state.hasSeenThreat);

    EnemyBehaviorState first{}, repeat{};
    EnemyBehaviorInput identical{};
    identical.confidence = 0.67f;
    identical.uncertainty = 0.18f;
    identical.confirmed = true;
    identical.hasSpatialBelief = true;
    identical.dt = Dt;
    for (int frame = 0; frame < 600; ++frame) {
        const auto a = updateEnemyBehavior(first, identical);
        const auto b = updateEnemyBehavior(repeat, identical);
        assert(a.mode == b.mode && a.travelScale == b.travelScale && a.commitment == b.commitment);
        assert(first.modeTime == repeat.modeTime && first.lostContactTime == repeat.lostContactTime);
    }

    first.modeTime = NAN;
    first.quietTime = INFINITY;
    first.lostContactTime = NAN;
    identical.confidence = NAN;
    identical.uncertainty = INFINITY;
    identical.vagueAwareness = NAN;
    identical.physicalDisruption = INFINITY;
    identical.dt = INFINITY;
    const auto finite = updateEnemyBehavior(first, identical);
    assert(std::isfinite(first.modeTime) && std::isfinite(first.quietTime));
    assert(std::isfinite(first.lostContactTime));
    assert(std::isfinite(finite.travelScale) && std::isfinite(finite.commitment));

    std::puts("ENEMY_BEHAVIOR_STATE_OK explicit=6 rest=SETTLED attack=CONFIRMED deterministic=YES");
}
