#pragma once

#include "RoomEnvironment.hpp"

#include <algorithm>
#include <cmath>

namespace field_island {

constexpr int StageRoomIndex = 1;
constexpr float MainHalfWidth = 13.0f;
constexpr float MainHalfDepth = 17.5f;
constexpr float DoorBridgeHalfWidth = 2.4f;
constexpr float DoorBridgeHalfDepth = 21.0f;
constexpr float AbyssRecoveryY = -8.0f;

inline bool active(int roomSeed, int roomIndex) {
    return roomIndex == StageRoomIndex &&
        room_environment::roomPlan(roomSeed, roomIndex).setting == room_environment::RoomSetting::Field;
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
