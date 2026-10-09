#pragma once

#include <array>

#include "Math.hpp"
#include "RenderContracts.hpp"
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

inline render_contract::SceneAtmosphere settingAtmosphere(
    render_contract::SceneAtmosphere atmosphere,
    room_environment::RoomSetting setting,
    render_contract::AtmosphereProfile profile,
    std::uint32_t overrideMask=0){
    using render_contract::AtmosphereChannel;
    using render_contract::atmosphereChannelBit;
    if(profile!=render_contract::AtmosphereProfile::ProgressiveCandidate)return atmosphere;
    const bool ambientOverridden=(overrideMask&atmosphereChannelBit(AtmosphereChannel::Ambient))!=0;
    const bool fogOverridden=(overrideMask&atmosphereChannelBit(AtmosphereChannel::Fog))!=0;
    if(!ambientOverridden){
        switch(setting){
            case room_environment::RoomSetting::Field:
                atmosphere.ambient={atmosphere.ambient.r*0.96f,atmosphere.ambient.g*1.06f,atmosphere.ambient.b*0.94f};break;
            case room_environment::RoomSetting::City:
                atmosphere.ambient={atmosphere.ambient.r*1.02f,atmosphere.ambient.g*0.94f,atmosphere.ambient.b*1.10f};break;
            case room_environment::RoomSetting::Sterile:
                atmosphere.ambient={atmosphere.ambient.r+0.018f,atmosphere.ambient.g+0.028f,atmosphere.ambient.b+0.026f};break;
            case room_environment::RoomSetting::Coastal:
                atmosphere.ambient={atmosphere.ambient.r,atmosphere.ambient.g*1.08f,atmosphere.ambient.b*1.14f};break;
        }
    }
    if(!fogOverridden){
        switch(setting){
            case room_environment::RoomSetting::Field:
                atmosphere.fog={atmosphere.fog.r*0.90f,atmosphere.fog.g*1.08f,atmosphere.fog.b*0.94f};break;
            case room_environment::RoomSetting::City:
                atmosphere.fog={atmosphere.fog.r,atmosphere.fog.g*0.90f,atmosphere.fog.b*1.12f};break;
            case room_environment::RoomSetting::Sterile:
                atmosphere.fog={atmosphere.fog.r*0.88f,atmosphere.fog.g*1.05f,atmosphere.fog.b*1.12f};break;
            case room_environment::RoomSetting::Coastal:
                atmosphere.fog={atmosphere.fog.r*0.88f,atmosphere.fog.g*1.08f,atmosphere.fog.b*1.16f};break;
        }
    }
    return atmosphere;
}

} // namespace room_lighting
