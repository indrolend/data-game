#pragma once

#include "world/RoomGeometry.hpp"

#include <algorithm>
#include <cmath>

// One inexpensive, deterministic description of Coastal water shared by
// gameplay and presentation.  This is a shallow game-water model rather than
// a fluid solver: travelling waves establish the body, while a damped radial
// impulse makes a moving phone visibly disturb it.
namespace coastal_water {

inline constexpr float ShoreX = world::RoomWidth * 0.125f;
inline constexpr float FarX = world::RoomWidth * 0.495f;
inline constexpr float MinZ = -world::RoomDepth * 0.49f;
inline constexpr float MaxZ = world::RoomDepth * 0.49f;
inline constexpr float BatteryDrainPerSecond = 0.32f;

inline float saturate(float value) {
    return std::max(0.0f, std::min(1.0f, value));
}

inline bool contains(float x, float localZ) {
    return x >= ShoreX && x <= FarX && localZ >= MinZ && localZ <= MaxZ;
}

inline float shoreAmount(float x) {
    return saturate((x - ShoreX) / std::max(0.001f, FarX - ShoreX));
}

inline float surfaceHeight(float x, float localZ, float time) {
    const float primary = x * 1.27f + localZ * 0.34f - time * 1.65f;
    const float crossing = localZ * 0.58f + time * 0.72f;
    const float longSwell = x * 0.31f - localZ * 0.19f + time * 0.38f;
    return 0.040f + std::sin(primary) * 0.044f
        + std::sin(crossing) * 0.017f
        + std::sin(longSwell) * 0.010f;
}

inline float wakeDisplacement(float x, float localZ, float sourceX,
                              float sourceLocalZ, float sourceSpeed,
                              float time) {
    if (!contains(sourceX, sourceLocalZ) || sourceSpeed < 0.35f) return 0.0f;
    const float dx = x - sourceX;
    const float dz = localZ - sourceLocalZ;
    const float distance = std::sqrt(dx * dx + dz * dz);
    if (distance > 2.8f) return 0.0f;
    const float speedAmount = saturate((sourceSpeed - 0.35f) / 8.0f);
    const float envelope = std::exp(-distance * 1.18f) * saturate(distance / 0.16f);
    return std::sin(distance * 10.5f - time * (5.8f + speedAmount * 3.0f))
        * envelope * (0.012f + speedAmount * 0.032f);
}

inline float immersion(float x, float localZ, float phoneBottom, float time) {
    if (!contains(x, localZ)) return 0.0f;
    // Water grows from ankle-deep at the wet-sand edge to meaningfully deep
    // offshore. This lets the visual shoreline and its gameplay cost agree.
    const float depth = 0.08f + shoreAmount(x) * 0.30f;
    const float vertical = saturate((surfaceHeight(x, localZ, time) + depth - phoneBottom) / 0.34f);
    const float edgeFeather = saturate((x - ShoreX) / 0.55f);
    return vertical * (0.18f + edgeFeather * 0.82f);
}

} // namespace coastal_water
