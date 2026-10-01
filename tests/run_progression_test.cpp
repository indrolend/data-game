#include "gameplay/RunProgression.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

namespace {
bool near(float a,float b){return std::abs(a-b)<0.000001f;}
}

int main(){
    RunProgressionState progression;
    progression.accuracyStacks=3;
    progression.accuracyMultiplier=1.24f;
    progression.accuracyDecayTimer=0.01f;
    gameplay::advanceAccuracyDecay(progression,0.02f);
    assert(progression.accuracyStacks==0);
    assert(near(progression.accuracyMultiplier,1.0f));
    assert(near(progression.accuracyDecayTimer,0.0f));

    progression.batteryRegenLock=0.50f;
    progression.headshotRegenTax=0.40f;
    progression.relayPrimerStacks=2;
    progression.relayPrimerTimer=0.01f;
    progression.impactGuardTimer=0.30f;
    progression.lastStandCooldown=0.20f;
    progression.lungeReboundTimer=0.10f;
    progression.headshotRechargeBoost=0.60f;
    gameplay::advanceRunProgressionTimers(progression,0.02f);
    assert(near(progression.batteryRegenLock,0.48f));
    assert(near(progression.headshotRegenTax,0.3978f));
    assert(progression.relayPrimerStacks==0);
    assert(near(progression.relayPrimerTimer,0.0f));
    assert(near(progression.impactGuardTimer,0.28f));
    assert(near(progression.lastStandCooldown,0.18f));
    assert(near(progression.lungeReboundTimer,0.08f));
    assert(near(progression.headshotRechargeBoost,0.58f));
    std::puts("RUN_PROGRESSION_OK accuracy_decay timers relay_expiry");
    return 0;
}
