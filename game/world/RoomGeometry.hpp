#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace world {

inline constexpr float RoomWidth = 30.0f;
inline constexpr float RoomDepth = 42.0f;
inline constexpr float RoomWallHeight = 7.2f;
inline constexpr float DoorwayHalfWidth = 2.1f;
inline constexpr float DoorwayLintelY = 3.72f;
inline constexpr float GroundY = 0.08f;

namespace finite_ground {

constexpr float MainHalfWidth = 13.0f;
constexpr float MainHalfDepth = 17.5f;
constexpr float DoorBridgeHalfWidth = 4.0f;
constexpr float DoorBridgeHalfDepth = RoomDepth * 0.5f;

struct Surface {
    float halfWidth;
    float minZ;
    float maxZ;
};

struct Footprint {
    float halfWidth = MainHalfWidth;
    float halfDepth = MainHalfDepth;

    constexpr std::array<Surface,3> surfaces() const {
        const float bridgeStart = std::min(halfDepth,DoorBridgeHalfDepth);
        return {{
            {halfWidth,-halfDepth,halfDepth},
            {DoorBridgeHalfWidth,bridgeStart,DoorBridgeHalfDepth},
            {DoorBridgeHalfWidth,-DoorBridgeHalfDepth,-bridgeStart},
        }};
    }
};

inline constexpr auto Surfaces = Footprint{}.surfaces();

inline bool hasGroundSupport(float x, float localZ, float radius, Footprint footprint = {}) {
    const float inset = std::max(0.0f, radius);
    const auto [main,frontBridge,backBridge] = footprint.surfaces();
    const bool onMain = std::abs(x) <= main.halfWidth - inset &&
        localZ >= main.minZ + inset && localZ <= main.maxZ - inset;
    // The narrow lane continues across the main surface so the radius inset
    // does not open a gap where each bridge joins the island.
    const bool onBridgeLane = std::abs(x) <= frontBridge.halfWidth - inset &&
        localZ >= backBridge.minZ && localZ <= frontBridge.maxZ;
    return onMain || onBridgeLane;
}

} // namespace finite_ground

} // namespace world
