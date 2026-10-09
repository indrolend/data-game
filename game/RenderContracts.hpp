#pragma once

#include <cmath>
#include <cstdint>
#include "Math.hpp"
#include "VisualIdentity.hpp"

namespace render_contract {

enum class ShadingModel : unsigned char { Unlit, ColorGraded, NormalLit };
enum class TextureId : unsigned char { None, FieldGrass, CityAsphalt };
enum class ShadowQuality : unsigned char { Off, Cheap, Directional };

constexpr ShadowQuality shadowQualityFor(int graphicsPreset,bool shadowsEnabled,bool directionalSupported){
    if(!shadowsEnabled)return ShadowQuality::Off;
    return graphicsPreset>=2&&directionalSupported?ShadowQuality::Directional:ShadowQuality::Cheap;
}

struct MaterialDefinition {
    VisualColor baseColor{1.0f,1.0f,1.0f};
    ShadingModel shading=ShadingModel::ColorGraded;
    float opacity=1.0f;
    bool fog=true;
    TextureId texture=TextureId::None;
    float textureWorldScale=1.0f;
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
    const float omenPulse=0.5f+0.5f*std::sin(time*0.73f+static_cast<float>(roomIndex)*0.41f);
    const float roomThreat=clampf((static_cast<float>(roomIndex)-1.0f)/18.0f,0.0f,1.0f);
    const float phonePulse=clampf(phonePower,0.0f,1.0f);
    return {{0.003f+omenPulse*0.004f,0.002f,0.009f+roomThreat*0.008f},
        {0.055f+omenPulse*0.018f,0.042f+omenPulse*0.012f,0.065f+roomThreat*0.025f+omenPulse*0.012f},
        {0.48f+roomThreat*0.12f,0.055f+omenPulse*0.035f,0.13f+roomThreat*0.16f},
        {0.04f,0.30f+omenPulse*0.10f,0.52f+roomThreat*0.18f},
        {0.18f*phonePulse,1.05f*phonePulse,1.32f*phonePulse},
        {0.025f+roomThreat*0.015f,0.018f+omenPulse*0.008f,0.040f+roomThreat*0.025f+omenPulse*0.012f},
        0.0115f+roomThreat*0.005f+omenPulse*0.0015f};
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
