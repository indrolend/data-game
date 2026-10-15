#include <cassert>
#include <cmath>
#include <cstdio>

#include "diagnostics/HistoricalPursuitComparison.hpp"
#include "gameplay/EnemyBehaviorState.hpp"

int main() {
    using namespace gameplay;

    EnemyBehaviorState evidenceState{};
    EnemyBehaviorInput noEvidence{};
    noEvidence.dt = 1.0f / 60.0f;
    EnemyBehaviorOutput evidenceOutput{};
    for (int frame = 0; frame < 60; ++frame)
        evidenceOutput = updateEnemyBehavior(evidenceState, noEvidence);

    const diagnostics::HistoricalRelentlessInput authoritativeTarget{
        {3.0f, 2.0f, 4.0f}, 5.0f, true
    };
    const auto direct = diagnostics::evaluateHistoricalRelentless(authoritativeTarget);

    assert(evidenceOutput.mode == EnemyBehaviorMode::Rest);
    assert(evidenceOutput.travelScale == 0.0f);
    assert(!evidenceOutput.mayAttack);
    assert(direct.pursuing);
    assert(direct.speedScale == 1.0f);
    assert(direct.attackCommitment == 1.0f);
    assert(std::fabs(direct.steering.x - 0.6f) < 0.0001f);
    assert(std::fabs(direct.steering.z - 0.8f) < 0.0001f);

    const auto invalid = diagnostics::evaluateHistoricalRelentless(
        {{3.0f, 0.0f, 4.0f}, NAN, true});
    assert(!invalid.pursuing && invalid.speedScale == 0.0f);

    std::puts(
        "HISTORICAL_PURSUIT_COMPARISON_OK "
        "evidence_without_contact=REST direct_with_authoritative_target=PURSUE "
        "runtime_integration=NO disposition=OPT_IN_ARCHETYPE_ONLY");
}
