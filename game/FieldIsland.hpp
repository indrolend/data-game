#pragma once

#include "RoomEnvironment.hpp"
#include "world/RoomGeometry.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace field_island {

constexpr int StageRoomIndex = 1;
constexpr float MainHalfWidth = 13.0f;
constexpr float MainHalfDepth = 17.5f;
constexpr float DoorBridgeHalfWidth = 4.0f;
constexpr float DoorBridgeHalfDepth = world::RoomDepth * 0.5f;
constexpr float AbyssRecoveryY = -8.0f;

struct Surface {
    float halfWidth;
    float minZ;
    float maxZ;
};

inline constexpr std::array<Surface, 3> Surfaces{{
    {MainHalfWidth, -MainHalfDepth, MainHalfDepth},
    {DoorBridgeHalfWidth, MainHalfDepth, DoorBridgeHalfDepth},
    {DoorBridgeHalfWidth, -DoorBridgeHalfDepth, -MainHalfDepth},
}};

inline bool active(int roomSeed, int roomIndex) {
    return roomIndex == StageRoomIndex &&
        room_environment::roomPlan(roomSeed, roomIndex).setting == room_environment::RoomSetting::Field;
}

inline bool active(int seed, int room, bool fixtureOverride) {
    return !fixtureOverride && active(seed, room);
}

inline room_environment::GrassBlade placeGrass(room_environment::GrassBlade blade) {
    blade.root.x *= (MainHalfWidth - 0.15f) / 13.2f;
    blade.root.z *= (MainHalfDepth - 0.15f) / 18.0f;
    blade.root.y = world::GroundY;
    return blade;
}

inline bool hasGroundSupport(float x, float localZ, float radius) {
    const float inset = std::max(0.0f, radius);
    const auto& main = Surfaces[0];
    const auto& frontBridge = Surfaces[1];
    const auto& backBridge = Surfaces[2];
    const bool onMain = std::abs(x) <= main.halfWidth - inset &&
        localZ >= main.minZ + inset && localZ <= main.maxZ - inset;
    // The narrow lane continues across the main surface so the radius inset
    // does not open a gap where each bridge joins the island.
    const bool onBridgeLane = std::abs(x) <= frontBridge.halfWidth - inset &&
        localZ >= backBridge.minZ && localZ <= frontBridge.maxZ;
    return onMain || onBridgeLane;
}

} // namespace field_island
