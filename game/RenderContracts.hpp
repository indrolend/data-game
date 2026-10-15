#pragma once

#include <cmath>
#include <cstdint>
#include "Math.hpp"
#include "VisualIdentity.hpp"

namespace render_contract {

enum class ShadingModel : unsigned char { Unlit, ColorGraded, NormalLit };
enum class TextureId : unsigned char { None, FieldGrass, CityAsphalt };
enum class ShadowQuality : unsigned char { Off, Cheap, MappedNormal, MappedPretty };

constexpr ShadowQuality shadowQualityFor(int graphicsPreset,bool shadowsEnabled,bool directionalSupported){
    if(!shadowsEnabled)return ShadowQuality::Off;
    if(!directionalSupported||graphicsPreset<=0)return ShadowQuality::Cheap;
    return graphicsPreset>=2?ShadowQuality::MappedPretty:ShadowQuality::MappedNormal;
}

constexpr int shadowMapResolution(ShadowQuality quality){
    return quality==ShadowQuality::MappedPretty?2048:(quality==ShadowQuality::MappedNormal?1024:0);
}
constexpr float shadowCoverage(ShadowQuality quality){
    return quality==ShadowQuality::MappedPretty?34.0f:(quality==ShadowQuality::MappedNormal?24.0f:0.0f);
}

struct ResolvedSunState { Vec3 direction{};float daylight=0.0f;bool castsDirectionalShadows=false; };

struct MaterialDefinition {
    VisualColor baseColor{1.0f,1.0f,1.0f};
    ShadingModel shading=ShadingModel::ColorGraded;
    float opacity=1.0f;
    bool fog=true;
    TextureId texture=TextureId::None;
    float textureWorldScale=1.0f;
    VisualColor specular{};
    float shininess=0.0f;
    VisualColor emission{};
};

constexpr MaterialDefinition sceneMatte(VisualColor color,float opacity=1.0f){return {color,ShadingModel::ColorGraded,opacity,true,TextureId::None,1.0f};}
constexpr MaterialDefinition normalLit(VisualColor color,float opacity=1.0f){return {color,ShadingModel::NormalLit,opacity,true,TextureId::None,1.0f};}
constexpr MaterialDefinition unlit(VisualColor color,float opacity=1.0f){return {color,ShadingModel::Unlit,opacity,false,TextureId::None,1.0f};}
inline constexpr MaterialDefinition FieldOpenGround{VisualIdentity::FieldGround,ShadingModel::ColorGraded,1.0f,true,TextureId::FieldGrass,6.4f};
inline constexpr MaterialDefinition CityGround{{0.24f,0.26f,0.28f},ShadingModel::ColorGraded,1.0f,true,TextureId::CityAsphalt,3.2f};
struct DirectionalLightDefinition { Vec3 direction{};VisualColor color{1,1,1};float intensity=1.0f; };
struct FogDefinition { VisualColor color{};float density=0.0f; };
struct SceneLightingDefinition {
    VisualColor ambient{};
    DirectionalLightDefinition sun{};
    DirectionalLightDefinition fill{};
    FogDefinition fog{};
};

inline const SceneLightingDefinition DesktopSceneLighting{
    {0.32f,0.43f,0.34f},{{30.0f,60.0f,25.0f},{1,1,1},1.0f},
    {{-20.0f,25.0f,-30.0f},{0.20f,0.28f,0.35f},1.0f},{VisualIdentity::Background,0.018f}};

struct SceneAtmosphere {
    VisualColor background{};
    VisualColor ambient{};
    VisualColor sun{};
    VisualColor fill{};
    VisualColor phone{};
    VisualColor fog{};
    float fogDensity=0.0f;
};

enum class AtmosphereProfile : unsigned char { ReadableStatic, ProgressiveCandidate };
enum class AtmosphereChannel : std::uint32_t { Background=1u<<0,Ambient=1u<<1,Sun=1u<<2,Fill=1u<<3,Phone=1u<<4,Fog=1u<<5,FogDensity=1u<<6 };
struct AtmosphereInputs { float time=0.0f;int roomIndex=1;float phonePower=0.0f; };
struct RuntimeLightingControl {
    AtmosphereProfile reference=AtmosphereProfile::ProgressiveCandidate;
    std::uint32_t overrideMask=0;
    SceneAtmosphere overrides{};
    bool timeFixed=false,roomFixed=false,phoneFixed=false;
    AtmosphereInputs fixedInputs{};
};

constexpr float DayCycleSeconds=480.0f;
inline float celestialPhase(float time){
    float phase=std::fmod(std::max(0.0f,time)/DayCycleSeconds+0.20f,1.0f);
    return phase<0.0f?phase+1.0f:phase;
}
inline float celestialElevation(float time){return std::sin(celestialPhase(time)*6.28318530717958647692f);}
inline float celestialDaylight(float time){
    const float x=clampf((celestialElevation(time)+0.16f)/0.38f,0.0f,1.0f);
    return x*x*(3.0f-2.0f*x);
}
inline Vec3 celestialSunDirection(float time){
    const float angle=celestialPhase(time)*6.28318530717958647692f;
    return {std::cos(angle)*54.0f,std::sin(angle)*62.0f,28.0f};
}
inline ResolvedSunState resolveSunState(float time,bool openSky,const Vec3& sealedDirection=DesktopSceneLighting.sun.direction){
    const float daylight=openSky?celestialDaylight(time):0.0f;
    const Vec3 direction=openSky?celestialSunDirection(time):sealedDirection;
    return {direction,daylight,openSky&&daylight>0.055f&&direction.y>0.035f};
}
inline VisualColor atmosphereMix(VisualColor a,VisualColor b,float amount){
    const float t=clampf(amount,0.0f,1.0f);
    return {a.r+(b.r-a.r)*t,a.g+(b.g-a.g)*t,a.b+(b.b-a.b)*t};
}

inline SceneAtmosphere sceneAtmosphere(float phonePower){
    const float phonePulse=clampf(phonePower,0.0f,1.0f);
    return {
        VisualIdentity::Background,
        DesktopSceneLighting.ambient,
        {DesktopSceneLighting.sun.color.r*DesktopSceneLighting.sun.intensity,
            DesktopSceneLighting.sun.color.g*DesktopSceneLighting.sun.intensity,
            DesktopSceneLighting.sun.color.b*DesktopSceneLighting.sun.intensity},
        {DesktopSceneLighting.fill.color.r*DesktopSceneLighting.fill.intensity,
            DesktopSceneLighting.fill.color.g*DesktopSceneLighting.fill.intensity,
            DesktopSceneLighting.fill.color.b*DesktopSceneLighting.fill.intensity},
        {0.12f*phonePulse,0.74f*phonePulse,0.92f*phonePulse},
        DesktopSceneLighting.fog.color,
        DesktopSceneLighting.fog.density
    };
}

inline SceneAtmosphere progressiveSceneAtmosphere(float time,int roomIndex,float phonePower){
    const float elevation=celestialElevation(time),daylight=celestialDaylight(time);
    const float horizon=clampf(1.0f-std::abs(elevation)/0.42f,0.0f,1.0f);
    const float omenPulse=0.5f+0.5f*std::sin(time*0.19f+static_cast<float>(roomIndex)*0.41f);
    const float roomThreat=clampf((static_cast<float>(roomIndex)-1.0f)/18.0f,0.0f,1.0f);
    const float phonePulse=clampf(phonePower,0.0f,1.0f);
    VisualColor sky=atmosphereMix({0.008f,0.012f,0.036f},{0.25f,0.48f,0.66f},daylight);
    sky=atmosphereMix(sky,{0.42f,0.10f,0.18f},horizon*(0.30f+0.45f*daylight));
    const VisualColor ambient=atmosphereMix({0.078f,0.092f,0.145f},{0.25f,0.31f,0.34f},daylight);
    const VisualColor sun=atmosphereMix({0.085f,0.115f,0.205f},{0.98f,0.78f,0.60f},daylight);
    const VisualColor fill=atmosphereMix({0.12f,0.20f,0.36f},{0.20f,0.34f,0.48f},daylight);
    VisualColor fog=atmosphereMix({0.018f,0.026f,0.060f},{0.22f,0.34f,0.42f},daylight);
    fog=atmosphereMix(fog,{0.34f,0.09f,0.16f},horizon*0.32f);
    return {sky,
        {ambient.r+omenPulse*0.006f,ambient.g+omenPulse*0.005f,ambient.b+roomThreat*0.014f},
        {sun.r*(0.92f+roomThreat*0.08f),sun.g,sun.b+roomThreat*0.035f},
        {fill.r,fill.g+omenPulse*0.025f,fill.b+roomThreat*0.10f},
        {0.18f*phonePulse,1.05f*phonePulse,1.32f*phonePulse},
        {fog.r+roomThreat*0.012f,fog.g+omenPulse*0.006f,fog.b+roomThreat*0.020f},
        0.0088f+(1.0f-daylight)*0.0032f+roomThreat*0.0022f+horizon*0.0012f};
}

inline SceneAtmosphere sceneAtmosphere(AtmosphereProfile profile,float time,int roomIndex,float phonePower){return profile==AtmosphereProfile::ProgressiveCandidate?progressiveSceneAtmosphere(time,roomIndex,phonePower):sceneAtmosphere(phonePower);}
constexpr std::uint32_t atmosphereChannelBit(AtmosphereChannel channel){return static_cast<std::uint32_t>(channel);}
constexpr bool atmosphereChannelOverridden(const RuntimeLightingControl& control,AtmosphereChannel channel){return (control.overrideMask&atmosphereChannelBit(channel))!=0;}
inline AtmosphereInputs effectiveAtmosphereInputs(const RuntimeLightingControl& control,const AtmosphereInputs& live){return {control.timeFixed?control.fixedInputs.time:live.time,control.roomFixed?control.fixedInputs.roomIndex:live.roomIndex,control.phoneFixed?control.fixedInputs.phonePower:live.phonePower};}
inline SceneAtmosphere resolveSceneAtmosphere(const RuntimeLightingControl& control,const AtmosphereInputs& live){
    const auto input=effectiveAtmosphereInputs(control,live);SceneAtmosphere result=sceneAtmosphere(control.reference,input.time,input.roomIndex,input.phonePower);
    if(atmosphereChannelOverridden(control,AtmosphereChannel::Background))result.background=control.overrides.background;
    if(atmosphereChannelOverridden(control,AtmosphereChannel::Ambient))result.ambient=control.overrides.ambient;
    if(atmosphereChannelOverridden(control,AtmosphereChannel::Sun))result.sun=control.overrides.sun;
    if(atmosphereChannelOverridden(control,AtmosphereChannel::Fill))result.fill=control.overrides.fill;
    if(atmosphereChannelOverridden(control,AtmosphereChannel::Phone))result.phone=control.overrides.phone;
    if(atmosphereChannelOverridden(control,AtmosphereChannel::Fog))result.fog=control.overrides.fog;
    if(atmosphereChannelOverridden(control,AtmosphereChannel::FogDensity))result.fogDensity=control.overrides.fogDensity;
    return result;
}
inline void setAtmosphereColorOverride(RuntimeLightingControl& control,AtmosphereChannel channel,VisualColor value){
    control.overrideMask|=atmosphereChannelBit(channel);
    switch(channel){case AtmosphereChannel::Background:control.overrides.background=value;break;case AtmosphereChannel::Ambient:control.overrides.ambient=value;break;case AtmosphereChannel::Sun:control.overrides.sun=value;break;case AtmosphereChannel::Fill:control.overrides.fill=value;break;case AtmosphereChannel::Phone:control.overrides.phone=value;break;case AtmosphereChannel::Fog:control.overrides.fog=value;break;case AtmosphereChannel::FogDensity:break;}
}
inline void setAtmosphereFogDensityOverride(RuntimeLightingControl& control,float value){control.overrideMask|=atmosphereChannelBit(AtmosphereChannel::FogDensity);control.overrides.fogDensity=clampf(value,0.0f,1.0f);}

} // namespace render_contract
