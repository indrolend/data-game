#pragma once

#include "Game.hpp"
#include "ModelData.hpp"
#include "HumanModelData.hpp"
#include "DeveloperCodec.hpp"
#include "RenderContracts.hpp"
#include "SceneLightingResponse.hpp"
#include "LightingStateSnapshot.hpp"
#include "CoastalWaterSimulation.hpp"

#include <filesystem>
#include <vector>
#include "../game/TvGifWall.hpp"

class DesktopRenderer {
public:
    static void drawBox(const Vec3& position, const Vec3& scale, float pitch, float yaw, float roll, float r, float g, float b, float a = 1.0f);
    static void drawBox(const Vec3& position, const Vec3& scale, const Quat& orientation, float r, float g, float b, float a=1.0f);
    void setAssetRoot(const std::filesystem::path& root);
    void resize(int width, int height);
    void setHudVisible(bool visible);
    void setAtmosphereProfile(render_contract::AtmosphereProfile profile);
    void setLightingControl(const render_contract::RuntimeLightingControl& control);
    render_contract::RuntimeLightingControl& lightingControl();
    const render_contract::RuntimeLightingControl& lightingControl() const;
    render_contract::SceneAtmosphere resolvedAtmosphere(const GameState& state) const;
    void draw(const GameState& state, const DeveloperCodecState* codec=nullptr) const;
    const lighting_evidence::RendererLightingObservation& lastLightingObservation() const { return lightingObservation_; }

private:
    static void drawFacetedRock(const room_environment::EnvironmentPropSpec& prop, int roomSeed, int roomIndex, int propIndex, float zOffset, const VisualColor& color, unsigned int texture=0, float textureWorldScale=1.0f);
    static void drawSlopeWedge(const SlopeSupport& slope, float zOffset, const VisualColor& color);
    static void drawTaperedSegment(const Vec3& start,const Vec3& end,float startRadius,float endRadius,const VisualColor& color,int sides=8);
    static void drawFacetedCanopy(const Vec3& center,const Vec3& size,float yaw,const VisualColor& color,unsigned int seed);
    TvGifWall tvGifWall_;
    mutable int width_ = 1280;
    mutable int height_ = 720;
    unsigned int phoneModelList_ = 0;
    unsigned int phoneShadowList_ = 0;
    unsigned int commonTreeModelList_ = 0;
    unsigned int twistedTreeModelList_ = 0;
    unsigned int deadTreeModelList_ = 0;
    unsigned int pineTreeModelList_ = 0;
    unsigned int cityWindowBayModelList_ = 0;
    unsigned int cityWindowBaySmallModelList_ = 0;
    unsigned int cityDoorBayModelList_ = 0;
    unsigned int cityDoorWindowBayModelList_ = 0;
    unsigned int cityCorniceModelList_ = 0;
    unsigned int cityWallWindowBayModelList_ = 0;
    unsigned int cityWallDoorBayModelList_ = 0;
    unsigned int cityWallCornerModelList_ = 0;
    unsigned int cityCrownModelList_ = 0;
    HumanModelData humanModel_;
    mutable std::vector<float> humanVertices_;
    mutable unsigned int datamoshTexture_ = 0;
    mutable unsigned int tvScreenTexture_ = 0;
    mutable unsigned int phoneDisplayTexture_ = 0;
    mutable unsigned int fieldGrassTexture_ = 0;
    mutable unsigned int citySurfaceTexture_ = 0;
    mutable unsigned int barkTexture_ = 0;
    mutable unsigned int stoneTexture_ = 0;
    mutable unsigned int metalTexture_ = 0;
    mutable unsigned int sandTexture_ = 0;
    mutable unsigned int sterileTileTexture_ = 0;
    mutable unsigned int birdSpriteTexture_ = 0;
    mutable unsigned int foliageTexture_ = 0;
    mutable std::vector<unsigned char> phoneDisplayPixels_;
    mutable unsigned long long phoneDisplayCacheKey_ = 0;
    mutable bool phoneDisplayCacheValid_ = false;
    mutable bool phoneDisplayTextureAllocated_ = false;
    mutable bool datamoshFrameReady_ = false;
    mutable int datamoshWidth_ = 0;
    mutable int datamoshHeight_ = 0;
    bool hudVisible_ = true;
    render_contract::RuntimeLightingControl lightingControl_{};
    mutable lighting_evidence::RendererLightingObservation lightingObservation_{};
    mutable unsigned int shadowFramebuffer_ = 0;
    mutable unsigned int shadowDepthTexture_ = 0;
    mutable int shadowMapSize_ = 0;
    mutable bool shadowSupportChecked_ = false;
    mutable bool shadowMapSupported_ = false;
    mutable bool shadowGeometryPass_ = false;
    mutable float shadowProjection_[16]{};
    mutable float shadowView_[16]{};
    mutable Vec3 frameSunDirection_{30.0f,60.0f,25.0f};
    mutable coastal_water::Simulation coastalWaterSimulation_{};
    mutable float coastalWaterTime_ = -1.0f;
    mutable int coastalWaterSeed_ = 0;
    mutable int coastalWaterRoom_ = -1;
    mutable Vec3 coastalWaterPreviousPlayer_{};

    void drawRoomTile(const GameState& state, int tileIndex, const scene_lighting_response::Response& lightingResponse) const;
    bool ensureShadowMap(int size) const;
    bool renderShadowDepth(const GameState& state,const scene_lighting_response::Response& response,const Vec3& sunDirection,render_contract::ShadowQuality quality) const;
    void drawShadowGeometry(const GameState& state,const scene_lighting_response::Response& response,render_contract::ShadowQuality quality) const;
    void applyMappedShadow(const GameState& state,const scene_lighting_response::Response& response,render_contract::ShadowQuality quality,float strength) const;
    void updateCoastalWater(const GameState& state) const;
    void drawFieldGrass(int tileIndex,const VisualColor& tint={1.0f,1.0f,1.0f}) const;
    void drawCityGround(int tileIndex) const;
    static void drawTexturedBox(unsigned int texture,const Vec3& position,const Vec3& scale,float pitch,float yaw,float roll,const VisualColor& tint,float textureWorldScale,float texturePhase=0.0f);
    void drawBirdFlocks(const GameState& state, int tileIndex, room_environment::RoomSetting setting) const;
    void drawFoliageCards(const Vec3& center,const Vec3& size,float yaw,const VisualColor& tint,unsigned int seed) const;
    void drawFieldBackdrop(const GameState& state,int tileIndex) const;
    void drawCityStreet(const GameState& state,int tileIndex,room_environment::RoomForm form) const;
    static void applyCamera(const GameState& state, float aspect);
    static void drawStaticModel(unsigned int list, const Vec3& position, const Vec3& scale, const Quat& orientation);
    void drawHumanModel(const TargetState& target, float time, room_environment::RoomSetting setting, bool shadow = false) const;
    static void drawEnemyReactionBillboard(const GameState& state,const TargetState& target);
    static void drawSoulFlesh(const TargetState& target,const Vec3& center);
    void drawSecretTvScreen(const GameState& state, float phoneProximity) const;
    void drawPhoneDisplayTexture(const GameState& state) const;
    void drawHud(const GameState& state) const;
    void drawDeveloperCodec(const DeveloperCodecState& codec) const;
    void drawDoorDataMosh(const GameState& state) const;
};
