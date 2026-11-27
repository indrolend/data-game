#pragma once

#include "RoomEnvironment.hpp"
#include "world/RoomGeometry.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace field_island {

using world::finite_ground::MainHalfWidth;
using world::finite_ground::MainHalfDepth;
using world::finite_ground::DoorBridgeHalfWidth;
using world::finite_ground::DoorBridgeHalfDepth;
using world::finite_ground::Surfaces;
using world::finite_ground::hasGroundSupport;
constexpr float AbyssRecoveryY = -8.0f;

inline room_environment::GrassBlade placeGrass(room_environment::GrassBlade blade) {
    blade.root.x *= (MainHalfWidth - 0.15f) / 13.2f;
    blade.root.z *= (MainHalfDepth - 0.15f) / 18.0f;
    blade.root.y = world::GroundSurfaceY;
    return blade;
}

// Split an axis-aligned surface overlay at the same tile-local footprint
// edges used by support. World Z is restored only for the emitted pieces.
template<class Emit>
void forEachGroundOverlay(Vec3 center, Vec3 size, float tileZ, bool finite, Emit emit, world::finite_ground::Footprint footprint = {}) {
    if (!finite) {
        emit(0, center, size);
        return;
    }
    const float localZ = center.z-tileZ;
    const auto surfaces = footprint.surfaces();
    for (std::size_t i=0;i<surfaces.size();++i) {
        const auto& surface=surfaces[i];
        const float minX=std::max(center.x-size.x*0.5f,-surface.halfWidth);
        const float maxX=std::min(center.x+size.x*0.5f,surface.halfWidth);
        const float minZ=std::max(localZ-size.z*0.5f,surface.minZ);
        const float maxZ=std::min(localZ+size.z*0.5f,surface.maxZ);
        if (minX>=maxX || minZ>=maxZ) continue;
        emit(i,{(minX+maxX)*0.5f,center.y+world::GroundSurfaceY,tileZ+(minZ+maxZ)*0.5f},
            {maxX-minX,size.y,maxZ-minZ});
    }
}

} // namespace field_island
