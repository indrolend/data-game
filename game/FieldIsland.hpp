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
    const bool onMain = std::abs(x) <= MainHalfWidth - inset &&
        std::abs(localZ) <= MainHalfDepth - inset;
    const bool onDoorBridge = std::abs(x) <= DoorBridgeHalfWidth - inset &&
        std::abs(localZ) <= DoorBridgeHalfDepth;
    return onMain || onDoorBridge;
}

} // namespace field_island
