#pragma once

#include <algorithm>

#include "../Game.hpp"

namespace gameplay {

inline void advanceAccuracyDecay(RunProgressionState& progression,float dt) noexcept {
    if(progression.accuracyStacks<=0)return;
    progression.accuracyDecayTimer=std::max(0.0f,progression.accuracyDecayTimer-dt);
    if(progression.accuracyDecayTimer<=0.0f){
        progression.accuracyStacks=0;
        progression.accuracyMultiplier=1.0f;
    }
}

inline void advanceRunProgressionTimers(RunProgressionState& progression,float dt) noexcept {
    progression.batteryRegenLock=std::max(0.0f,progression.batteryRegenLock-dt);
    progression.headshotRegenTax=std::max(0.0f,progression.headshotRegenTax-dt*0.11f);
    progression.relayPrimerTimer=std::max(0.0f,progression.relayPrimerTimer-dt);
    if(progression.relayPrimerTimer<=0.0f)progression.relayPrimerStacks=0;
    progression.impactGuardTimer=std::max(0.0f,progression.impactGuardTimer-dt);
    progression.lastStandCooldown=std::max(0.0f,progression.lastStandCooldown-dt);
    progression.lungeReboundTimer=std::max(0.0f,progression.lungeReboundTimer-dt);
    progression.headshotRechargeBoost=std::max(0.0f,progression.headshotRechargeBoost-dt);
}

} // namespace gameplay
