#pragma once

#include <algorithm>
#include <cmath>

namespace soul_economy {

// Stored souls are useful capital and ammunition, but carrying them makes
// passive recovery less efficient. This deliberately affects recovery only:
// action costs and locomotion remain independent of inventory weight.
constexpr float PASSIVE_REGEN_WEIGHT_PER_SOUL = 0.025f;
constexpr float BATTERY_IDLE_REGEN = 8.0f;
constexpr float BATTERY_WALK_DRAIN = 0.45f;
constexpr float BATTERY_SPRINT_DRAIN = 3.0f;
constexpr float BATTERY_AIR_DRAIN = 0.9f;
constexpr float BATTERY_VACUUM_DRAIN = 1.35f;

struct BatteryDemandInput {
    float forward = 0.0f;
    float strafe = 0.0f;
    float vacuumPower = 0.0f;
    bool sprint = false;
    bool grounded = true;
    bool vacuumActive = false;
    bool meleeActive = false;
    bool dischargeActive = false;
};

struct BatteryDemand {
    float drainPerSecond = 0.0f;
    bool active = false;
};

inline BatteryDemand batteryDemand(const BatteryDemandInput& input) {
    const bool moving = std::abs(input.forward) + std::abs(input.strafe) > 0.0f;
    const bool running = moving && input.sprint;
    BatteryDemand demand;
    if (moving) {
        demand.drainPerSecond += running ? BATTERY_SPRINT_DRAIN : BATTERY_WALK_DRAIN;
        demand.active = true;
    }
    if (!input.grounded) {
        demand.drainPerSecond += BATTERY_AIR_DRAIN;
        demand.active = true;
    }
    if (input.vacuumActive) {
        demand.drainPerSecond += BATTERY_VACUUM_DRAIN * std::max(0.35f, input.vacuumPower);
        demand.active = true;
    }
    if (input.meleeActive || input.dischargeActive) demand.active = true;
    return demand;
}

inline float passiveRegenMultiplier(int storedSouls) {
    const float souls = static_cast<float>(std::max(0, storedSouls));
    return 1.0f / (1.0f + souls * PASSIVE_REGEN_WEIGHT_PER_SOUL);
}

}  // namespace soul_economy
