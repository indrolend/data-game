#pragma once

#include "RenderContracts.hpp"
#include "RoomEnvironment.hpp"
#include "VisualIdentity.hpp"

enum class ParticleMaterial : unsigned char { Impact, Flesh, Environment, Soul, Data, SignalResidue };
enum class SemanticSurface : unsigned char { Default, FieldGround, CityAsphalt, SterilePanel, PhoneMetal, PhoneGlass, Organic };

inline constexpr render_contract::MaterialDefinition semanticMaterial(
    SemanticSurface surface,VisualColor baseColor={1.0f,1.0f,1.0f},float opacity=1.0f){
    using render_contract::MaterialDefinition;
    using render_contract::ShadingModel;
    using render_contract::TextureId;
    switch(surface){
        case SemanticSurface::FieldGround:return {baseColor,ShadingModel::NormalLit,opacity,true,TextureId::FieldGrass,render_contract::FieldOpenGround.textureWorldScale,{0.015f,0.020f,0.012f},2.0f,{}};
        case SemanticSurface::CityAsphalt:return {baseColor,ShadingModel::NormalLit,opacity,true,TextureId::CityAsphalt,render_contract::CityGround.textureWorldScale,{0.025f,0.030f,0.035f},4.0f,{}};
        case SemanticSurface::SterilePanel:return {baseColor,ShadingModel::NormalLit,opacity,true,TextureId::None,1.0f,{0.16f,0.19f,0.21f},16.0f,{}};
        case SemanticSurface::PhoneMetal:return {baseColor,ShadingModel::NormalLit,opacity,true,TextureId::None,1.0f,{0.38f,0.44f,0.48f},36.0f,{}};
        case SemanticSurface::PhoneGlass:return {baseColor,ShadingModel::NormalLit,opacity,true,TextureId::None,1.0f,{0.12f,0.25f,0.30f},42.0f,{0.006f,0.018f,0.024f}};
        case SemanticSurface::Organic:return {baseColor,ShadingModel::NormalLit,opacity,true,TextureId::None,1.0f,{0.045f,0.030f,0.020f},6.0f,{}};
        case SemanticSurface::Default:return {baseColor,ShadingModel::ColorGraded,opacity,true,TextureId::None,1.0f,{0.0f,0.0f,0.0f},0.0f,{}};
    }
    return {};
}

inline constexpr VisualColor roomSubstrateColor(room_environment::RoomSetting setting) {
    using room_environment::RoomSetting;
    switch(setting) {
        case RoomSetting::Field: return {0.25f,0.45f,0.29f};
        case RoomSetting::City: return {0.43f,0.49f,0.53f};
        case RoomSetting::Sterile: return {0.62f,0.66f,0.69f};
        case RoomSetting::Coastal: return {0.54f,0.48f,0.36f};
    }
    return VisualIdentity::RoomObstacle;
}

inline VisualColor mixVisualColor(const VisualColor& from,const VisualColor& to,float amount) {
    const float t=visualSmooth01(clampf(amount,0.0f,1.0f));
    return {from.r+(to.r-from.r)*t,from.g+(to.g-from.g)*t,from.b+(to.b-from.b)*t};
}

inline float humanSubstrateAmount(float armor,float armorMax,bool slurpable) {
    if(slurpable||armorMax<=0.001f)return 1.0f;
    const float damage=1.0f-clampf(armor/armorMax,0.0f,1.0f);
    return visualSmooth01(clampf((damage-0.28f)/0.72f,0.0f,1.0f))*0.68f;
}

inline VisualColor humanDamageSurfaceColor(const VisualColor& base,
    room_environment::RoomSetting setting,float armor,float armorMax,bool slurpable,float hitFlash) {
    VisualColor color=mixVisualColor(base,roomSubstrateColor(setting),humanSubstrateAmount(armor,armorMax,slurpable));
    const float flash=clampf(hitFlash,0.0f,1.0f)*0.34f;
    return mixVisualColor(color,VisualIdentity::HitFlash,flash);
}

inline VisualColor particleMaterialColor(ParticleMaterial material,
    room_environment::RoomSetting setting,float remainingLife) {
    const VisualColor substrate=roomSubstrateColor(setting);
    switch(material) {
        case ParticleMaterial::Flesh: return VisualIdentity::SoulFlesh;
        case ParticleMaterial::Environment:
            return mixVisualColor(VisualIdentity::SoulFlesh,substrate,1.0f-clampf(remainingLife,0.0f,1.0f));
        case ParticleMaterial::Soul: return VisualIdentity::SoulBase;
        case ParticleMaterial::Data: return VisualIdentity::ElectricCyan;
        case ParticleMaterial::SignalResidue:
            return mixVisualColor(VisualIdentity::ElectricMagenta,VisualIdentity::ElectricCyan,
                0.20f+0.42f*(1.0f-clampf(remainingLife,0.0f,1.0f)));
        case ParticleMaterial::Impact: return {1.0f,0.267f,0.267f};
    }
    return VisualIdentity::HitFlash;
}
