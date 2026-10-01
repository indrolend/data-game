#pragma once

#include <array>

#include "Math.hpp"
#include "RoomEnvironment.hpp"
#include "VisualIdentity.hpp"
#include "world/RoomGeometry.hpp"

namespace room_lighting {

enum class PrimaryLightSource : unsigned char { OutdoorSun, UrbanSky, CeilingFixtures };

struct LocalLightDefinition {
    Vec3 localPosition{};
    Vec3 fixtureSize{};
    VisualColor color{};
    float intensity=0.0f;
    float radius=1.0f;
    bool visibleFixture=false;
};

struct RoomLightRig {
    PrimaryLightSource primarySource=PrimaryLightSource::OutdoorSun;
    std::array<LocalLightDefinition,3> localLights{};
    int localLightCount=0;
};

inline RoomLightRig roomLightRig(room_environment::RoomSetting setting,room_environment::RoomForm form){
    using room_environment::RoomForm;
    using room_environment::RoomSetting;
    RoomLightRig rig{};
    if(setting==RoomSetting::City){rig.primarySource=PrimaryLightSource::UrbanSky;return rig;}
    if(setting!=RoomSetting::Sterile)return rig;
    rig.primarySource=PrimaryLightSource::CeilingFixtures;
    rig.localLightCount=form==RoomForm::Chamber?3:2;
    for(int i=0;i<rig.localLightCount;++i){
        const float z=rig.localLightCount==3?(-10.0f+10.0f*static_cast<float>(i)):(i==0?-8.0f:8.0f);
        rig.localLights[i]={{0.0f,world::RoomWallHeight-0.18f,z},{5.8f,0.055f,0.72f},{0.68f,0.88f,0.94f},i==rig.localLightCount-1?1.05f:1.24f,12.5f,true};
    }
    return rig;
}

} // namespace room_lighting
