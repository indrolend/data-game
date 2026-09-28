#include "RenderContracts.hpp"
#include <cstdio>

int main(){
    using namespace render_contract;
    if(DesktopSceneLighting.sun.direction.x!=30.0f||DesktopSceneLighting.fog.density!=0.018f){
        std::fputs("RENDER_CONTRACTS_FAIL desktop profile\n",stderr);return 1;
    }
    constexpr auto glass=sceneMatte(Pass7Visual::TvMembrane,0.25f);
    static_assert(glass.opacity==0.25f&&glass.fog&&glass.shading==ShadingModel::ColorGraded);
    constexpr auto fx=unlit(Pass7Visual::ElectricCyan,0.5f);
    static_assert(!fx.fog&&fx.shading==ShadingModel::Unlit);
    static_assert(shadowQualityFor(0,false,true)==ShadowQuality::Off);
    static_assert(shadowQualityFor(1,true,true)==ShadowQuality::Cheap);
    static_assert(shadowQualityFor(2,true,true)==ShadowQuality::Directional);
    static_assert(shadowQualityFor(2,true,false)==ShadowQuality::Cheap);
    static_assert(FieldOpenGround.texture==TextureId::FieldGrass&&FieldOpenGround.textureWorldScale==2.4f);
    static_assert(CityGround.texture==TextureId::CityAsphalt&&CityGround.textureWorldScale==3.2f);
    const auto unpowered=sceneAtmosphere(0.0f);
    const auto powered=sceneAtmosphere(1.0f);
    if(unpowered.background.r!=Pass7Visual::Background.r||unpowered.ambient.g!=DesktopSceneLighting.ambient.g||
        unpowered.sun.b!=DesktopSceneLighting.sun.color.b*DesktopSceneLighting.sun.intensity||
        unpowered.fill.g!=DesktopSceneLighting.fill.color.g*DesktopSceneLighting.fill.intensity||
        unpowered.fogDensity!=DesktopSceneLighting.fog.density){
        std::fputs("RENDER_CONTRACTS_FAIL accepted environment profile\n",stderr);return 1;
    }
    if(unpowered.phone.r!=0.0f||powered.phone.r!=0.12f||powered.phone.g!=0.74f||powered.phone.b!=0.92f){
        std::fputs("RENDER_CONTRACTS_FAIL phone light response\n",stderr);return 1;
    }
    std::puts("RENDER_CONTRACTS_OK profiles=2 shading_models=3 shadow_qualities=3 atmosphere=ACCEPTED_STATIC field_grass=TEXTURED city_ground=TEXTURED");
    return 0;
}
