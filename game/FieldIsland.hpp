#pragma once

#include "RoomEnvironment.hpp"
#include "world/RoomGeometry.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace field_island {

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

inline bool active(int roomIndex, bool fixtureOverride) {
    return !fixtureOverride && room_environment::authoredFiniteGround(roomIndex);
}

inline room_environment::GrassBlade placeGrass(room_environment::GrassBlade blade) {
    blade.root.x *= (MainHalfWidth - 0.15f) / 13.2f;
    blade.root.z *= (MainHalfDepth - 0.15f) / 18.0f;
    blade.root.y = world::GroundY;
    return blade;
}

// Split an axis-aligned surface overlay at the same tile-local footprint
// edges used by support. World Z is restored only for the emitted pieces.
template<class Emit>
void forEachGroundOverlay(Vec3 center, Vec3 size, float tileZ, bool finite, Emit emit) {
    if (!finite) {
        emit(0, center, size);
        return;
    }
    const float localZ = center.z-tileZ;
    for (std::size_t i=0;i<Surfaces.size();++i) {
        const auto& surface=Surfaces[i];
        const float minX=std::max(center.x-size.x*0.5f,-surface.halfWidth);
        const float maxX=std::min(center.x+size.x*0.5f,surface.halfWidth);
        const float minZ=std::max(localZ-size.z*0.5f,surface.minZ);
        const float maxZ=std::min(localZ+size.z*0.5f,surface.maxZ);
        if (minX>=maxX || minZ>=maxZ) continue;
        emit(i,{(minX+maxX)*0.5f,center.y+world::GroundY,tileZ+(minZ+maxZ)*0.5f},
            {maxX-minX,size.y,maxZ-minZ});
    }
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
