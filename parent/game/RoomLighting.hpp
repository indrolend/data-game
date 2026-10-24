#pragma once

#include <array>

#include "Math.hpp"
#include "RenderContracts.hpp"
#include "RoomEnvironment.hpp"
#include "SceneLightingResponse.hpp"
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
    Vec3 sunDirection=render_contract::DesktopSceneLighting.sun.direction;
    Vec3 fillDirection=render_contract::DesktopSceneLighting.fill.direction;
    std::array<LocalLightDefinition,3> localLights{};
    int localLightCount=0;
};

struct ResolvedLight {
    bool enabled=false;
    bool directional=false;
    Vec3 position{};
    VisualColor diffuse{};
    float constantAttenuation=1.0f;
    float linearAttenuation=0.0f;
    float quadraticAttenuation=0.0f;
};

struct ResolvedSceneLighting {
    VisualColor ambient{};
    ResolvedLight sun{},fill{},phone{};
    std::array<ResolvedLight,3> localLights{};
    ResolvedLight shot{},exit{};
    render_contract::FogDefinition fog{};
};

inline RoomLightRig roomLightRig(room_environment::RoomSetting setting,room_environment::RoomForm form){
    using room_environment::RoomForm;
    using room_environment::RoomSetting;
    RoomLightRig rig{};
    if(setting==RoomSetting::City){
        rig.primarySource=PrimaryLightSource::UrbanSky;
        rig.sunDirection={-28.0f,36.0f,42.0f};
        rig.fillDirection={36.0f,22.0f,-18.0f};
        return rig;
    }
    if(setting!=RoomSetting::Sterile)return rig;
    rig.primarySource=PrimaryLightSource::CeilingFixtures;
    rig.localLightCount=form==RoomForm::Chamber?3:2;
    for(int i=0;i<rig.localLightCount;++i){
        const float z=rig.localLightCount==3?(-10.0f+10.0f*static_cast<float>(i)):(i==0?-8.0f:8.0f);
        rig.localLights[i]={{0.0f,world::RoomWallHeight-0.18f,z},{5.8f,0.055f,0.72f},{0.68f,0.88f,0.94f},i==rig.localLightCount-1?1.05f:1.24f,7.9f,true};
    }
    return rig;
}

inline ResolvedSceneLighting resolveSceneLighting(
    const render_contract::SceneAtmosphere& atmosphere,
    const RoomLightRig& rig,
    const scene_lighting_response::Response& response,
    const Vec3& phonePosition,
    const Vec3& latestShotOrigin,
    float tileOrigin,
    const Vec3& capturePosition={}){
    ResolvedSceneLighting result{};
    result.ambient=atmosphere.ambient;
    const bool globalLights=rig.primarySource!=PrimaryLightSource::CeilingFixtures;
    result.sun={globalLights,true,rig.sunDirection,atmosphere.sun};
    result.fill={globalLights,true,rig.fillDirection,atmosphere.fill};
    result.phone={true,false,phonePosition,
        {atmosphere.phone.r*response.phoneLightScale+response.actionLight*0.08f+response.criticalLight*0.28f,
         atmosphere.phone.g*response.phoneLightScale+response.actionLight*0.18f+response.criticalLight*0.08f,
         atmosphere.phone.b*response.phoneLightScale+response.actionLight*0.24f+response.criticalLight*0.24f},
        1.0f,1.6f,0.0f};
    for(int i=0;i<rig.localLightCount;++i){
        const auto& source=rig.localLights[static_cast<std::size_t>(i)];
        result.localLights[static_cast<std::size_t>(i)]={true,false,
            {source.localPosition.x,source.localPosition.y,tileOrigin+source.localPosition.z},
            {source.color.r*source.intensity,source.color.g*source.intensity,source.color.b*source.intensity},
            0.48f,0.035f,1.25f/(source.radius*source.radius)};
    }
    if(response.captureLight>0.001f)result.shot={true,false,
        capturePosition+Vec3{0.0f,0.32f,0.0f},
        {0.34f*response.captureLight,1.18f*response.captureLight,1.42f*response.captureLight},
        0.58f,0.20f,0.055f};
    else if(response.completionLight>0.001f)result.shot={true,false,
        capturePosition+Vec3{0.0f,0.18f,0.0f},
        {0.48f*response.completionLight,0.96f*response.completionLight,1.28f*response.completionLight},
        0.46f,0.24f,0.085f};
    else if(response.shotLight>0.001f)result.shot={true,false,
        {latestShotOrigin.x,latestShotOrigin.y+0.2f,latestShotOrigin.z},
        {1.15f*response.shotLight,0.82f*response.shotLight,0.55f*response.shotLight},0.72f,0.22f,0.08f};
    if(response.exitGlow>0.01f)result.exit={true,false,
        {0.0f,2.2f,tileOrigin-world::RoomDepth*0.5f+0.8f},
        {0.34f*response.exitGlow,0.72f*response.exitGlow,0.68f*response.exitGlow},0.8f,0.11f,0.035f};
    result.fog={atmosphere.fog,atmosphere.fogDensity};
    return result;
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
    // Sterile rooms are sealed, sickly interiors.  Keeping their base light
    // independent of the outdoor clock makes the ceiling fixtures feel like
    // actual sources instead of sunlight wearing a fluorescent costume.
    if(setting==room_environment::RoomSetting::Sterile){
        atmosphere.background={0.012f,0.017f,0.019f};
        atmosphere.sun={0.035f,0.048f,0.044f};
        atmosphere.fill={0.055f,0.075f,0.069f};
        if(!ambientOverridden)atmosphere.ambient={0.086f,0.105f,0.101f};
        if(!fogOverridden){atmosphere.fog={0.055f,0.071f,0.068f};atmosphere.fogDensity=0.022f;}
        return atmosphere;
    }
    if(!ambientOverridden){
        switch(setting){
            case room_environment::RoomSetting::Field:
                atmosphere.ambient={atmosphere.ambient.r*0.92f,atmosphere.ambient.g*1.34f,atmosphere.ambient.b*1.08f};break;
            case room_environment::RoomSetting::City:
                atmosphere.ambient={atmosphere.ambient.r*1.12f,atmosphere.ambient.g*1.02f,atmosphere.ambient.b*1.38f};break;
            case room_environment::RoomSetting::Sterile:
                atmosphere.ambient={atmosphere.ambient.r+0.070f,atmosphere.ambient.g+0.092f,atmosphere.ambient.b+0.098f};break;
            case room_environment::RoomSetting::Coastal:
                atmosphere.ambient={atmosphere.ambient.r*1.16f,atmosphere.ambient.g*1.30f,atmosphere.ambient.b*1.46f};break;
        }
    }
    if(!fogOverridden){
        switch(setting){
            case room_environment::RoomSetting::Field:
                atmosphere.fog={atmosphere.fog.r*0.82f,atmosphere.fog.g*1.34f,atmosphere.fog.b*0.92f};
                atmosphere.fogDensity*=0.78f;break;
            case room_environment::RoomSetting::City:
                atmosphere.fog={atmosphere.fog.r*1.18f,atmosphere.fog.g*0.86f,atmosphere.fog.b*1.44f};
                atmosphere.fogDensity*=1.10f;break;
            case room_environment::RoomSetting::Sterile:
                atmosphere.fog={atmosphere.fog.r*1.26f,atmosphere.fog.g*1.72f,atmosphere.fog.b*1.88f};
                atmosphere.fogDensity*=0.62f;break;
            case room_environment::RoomSetting::Coastal:
                atmosphere.fog={atmosphere.fog.r*0.92f,atmosphere.fog.g*1.42f,atmosphere.fog.b*1.78f};
                atmosphere.fogDensity*=0.88f;break;
        }
    }
    return atmosphere;
}

} // namespace room_lighting
