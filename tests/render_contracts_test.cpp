#include "RenderContracts.hpp"
#include "RoomLighting.hpp"
#include "FieldGrassTexture.hpp"
#include <cstdio>

int main(){
    using namespace render_contract;
    using namespace room_lighting;
    if(DesktopSceneLighting.sun.direction.x!=30.0f||DesktopSceneLighting.fog.density!=0.018f){
        std::fputs("RENDER_CONTRACTS_FAIL desktop profile\n",stderr);return 1;
    }
    constexpr auto glass=sceneMatte(VisualIdentity::TvMembrane,0.25f);
    static_assert(glass.opacity==0.25f&&glass.fog&&glass.shading==ShadingModel::ColorGraded);
    constexpr auto fx=unlit(VisualIdentity::ElectricCyan,0.5f);
    static_assert(!fx.fog&&fx.shading==ShadingModel::Unlit);
    static_assert(shadowQualityFor(0,false,true)==ShadowQuality::Off);
    static_assert(shadowQualityFor(1,true,true)==ShadowQuality::Cheap);
    static_assert(shadowQualityFor(2,true,true)==ShadowQuality::Directional);
    static_assert(shadowQualityFor(2,true,false)==ShadowQuality::Cheap);
    const auto fieldRig=roomLightRig(room_environment::RoomSetting::Field,room_environment::RoomForm::Open);
    const auto cityRig=roomLightRig(room_environment::RoomSetting::City,room_environment::RoomForm::Corridor);
    const auto sterileCorridorRig=roomLightRig(room_environment::RoomSetting::Sterile,room_environment::RoomForm::Corridor);
    const auto sterileChamberRig=roomLightRig(room_environment::RoomSetting::Sterile,room_environment::RoomForm::Chamber);
    if(fieldRig.primarySource!=PrimaryLightSource::OutdoorSun||fieldRig.localLightCount!=0||
        cityRig.primarySource!=PrimaryLightSource::UrbanSky||cityRig.localLightCount!=0||
        sterileCorridorRig.primarySource!=PrimaryLightSource::CeilingFixtures||sterileCorridorRig.localLightCount!=2||
        sterileChamberRig.localLightCount!=3||sterileChamberRig.localLights[1].localPosition.z!=0.0f||
        !sterileCorridorRig.localLights[0].visibleFixture||sterileCorridorRig.localLights[0].radius<=0.0f||sterileCorridorRig.localLights[0].fixtureSize.x<=0.0f){
        std::fputs("RENDER_CONTRACTS_FAIL room light source rig\n",stderr);return 1;
    }
    static_assert(FieldOpenGround.texture==TextureId::FieldGrass&&FieldOpenGround.textureWorldScale==6.4f);
    const auto fieldPixels=field_grass_texture::pixels(),fieldPixelsAgain=field_grass_texture::pixels();
    if(fieldPixels!=fieldPixelsAgain){std::fputs("RENDER_CONTRACTS_FAIL field texture determinism\n",stderr);return 1;}
    int minimumGreen=255,maximumGreen=0;
    for(std::size_t i=1;i<fieldPixels.size();i+=3){minimumGreen=std::min(minimumGreen,static_cast<int>(fieldPixels[i]));maximumGreen=std::max(maximumGreen,static_cast<int>(fieldPixels[i]));}
    if(field_grass_texture::Size<64||maximumGreen-minimumGreen<24){std::fprintf(stderr,"RENDER_CONTRACTS_FAIL field texture range %d\n",maximumGreen-minimumGreen);return 1;}
    static_assert(CityGround.texture==TextureId::CityAsphalt&&CityGround.textureWorldScale==3.2f);
    const auto unpowered=sceneAtmosphere(0.0f);
    const auto powered=sceneAtmosphere(1.0f);
    if(unpowered.background.r!=VisualIdentity::Background.r||unpowered.ambient.g!=DesktopSceneLighting.ambient.g||
        unpowered.sun.b!=DesktopSceneLighting.sun.color.b*DesktopSceneLighting.sun.intensity||
        unpowered.fill.g!=DesktopSceneLighting.fill.color.g*DesktopSceneLighting.fill.intensity||
        unpowered.fogDensity!=DesktopSceneLighting.fog.density){
        std::fputs("RENDER_CONTRACTS_FAIL accepted environment profile\n",stderr);return 1;
    }
    if(unpowered.phone.r!=0.0f||powered.phone.r!=0.12f||powered.phone.g!=0.74f||powered.phone.b!=0.92f){
        std::fputs("RENDER_CONTRACTS_FAIL phone light response\n",stderr);return 1;
    }
    const auto progressiveOpening=progressiveSceneAtmosphere(0.0f,1,0.0f);
    const auto progressiveDeep=progressiveSceneAtmosphere(10.0f,19,1.0f);
    if(!(progressiveDeep.fogDensity>progressiveOpening.fogDensity&&progressiveDeep.fog.r>progressiveOpening.fog.r&&progressiveDeep.phone.b==1.32f)){
        std::fputs("RENDER_CONTRACTS_FAIL progressive candidate\n",stderr);return 1;
    }
    if(progressiveOpening.ambient.r<0.055f||progressiveOpening.ambient.g<0.042f||progressiveOpening.ambient.b<0.065f||
        progressiveDeep.fogDensity>DesktopSceneLighting.fog.density||
        progressiveOpening.fog.r<=progressiveOpening.background.r||progressiveOpening.fog.g<=progressiveOpening.background.g||progressiveOpening.fog.b<=progressiveOpening.background.b){
        std::fputs("RENDER_CONTRACTS_FAIL progressive readability floor\n",stderr);return 1;
    }
    RuntimeLightingControl control;control.reference=AtmosphereProfile::ProgressiveCandidate;
    setAtmosphereColorOverride(control,AtmosphereChannel::Fill,{0.16f,0.36f,0.52f});setAtmosphereFogDensityOverride(control,0.0125f);
    control.timeFixed=true;control.roomFixed=true;control.phoneFixed=true;control.fixedInputs={7.25f,6,0.45f};
    const auto manipulated=resolveSceneAtmosphere(control,{99.0f,19,1.0f});const auto fixed=effectiveAtmosphereInputs(control,{99.0f,19,1.0f});
    if(manipulated.fill.g!=0.36f||manipulated.fogDensity!=0.0125f||fixed.time!=7.25f||fixed.roomIndex!=6||fixed.phonePower!=0.45f){
        std::fputs("RENDER_CONTRACTS_FAIL runtime lighting control\n",stderr);return 1;
    }
    std::puts("RENDER_CONTRACTS_OK profiles=2 light_sources=3 sterile_fixture_rigs=2 shading_models=3 shadow_qualities=3 atmosphere=ACCEPTED_STATIC field_grass=TEXTURED city_ground=TEXTURED");
    return 0;
}
