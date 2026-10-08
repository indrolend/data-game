#pragma once

#include "Game.hpp"
#include "ModelData.hpp"
#include "HumanModelData.hpp"
#include "DeveloperCodec.hpp"
#include "RenderContracts.hpp"
#include "SceneLightingResponse.hpp"
#include "LightingStateSnapshot.hpp"

#include <filesystem>
#include <vector>
#include "../game/TvGifWall.hpp"

class DesktopRenderer {
public:
    static void drawBox(const Vec3& position, const Vec3& scale, float pitch, float yaw, float roll, float r, float g, float b, float a = 1.0f);
    static void drawBox(const Vec3& position, const Vec3& scale, const Quat& orientation, float r, float g, float b);
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
    static void drawFacetedRock(const room_environment::EnvironmentPropSpec& prop, int roomSeed, int roomIndex, int propIndex, float zOffset, const VisualColor& color);
    static void drawSlopeWedge(const SlopeSupport& slope, float zOffset, const VisualColor& color);
    TvGifWall tvGifWall_;
    mutable int width_ = 1280;
    mutable int height_ = 720;
    unsigned int phoneModelList_ = 0;
    unsigned int phoneShadowList_ = 0;
    HumanModelData humanModel_;
    mutable std::vector<float> humanVertices_;
    mutable unsigned int datamoshTexture_ = 0;
    mutable unsigned int tvScreenTexture_ = 0;
    mutable unsigned int phoneDisplayTexture_ = 0;
    mutable unsigned int fieldGrassTexture_ = 0;
    mutable unsigned int citySurfaceTexture_ = 0;
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

    void drawRoomTile(const GameState& state, int tileIndex, const scene_lighting_response::Response& lightingResponse) const;
    void drawFieldGrass(int tileIndex) const;
    void drawCityGround(int tileIndex) const;
    static void applyCamera(const GameState& state, float aspect);
    static void drawStaticModel(unsigned int list, const Vec3& position, const Vec3& scale, const Quat& orientation);
    void drawHumanModel(const TargetState& target, float time, room_environment::RoomSetting setting, bool shadow = false) const;
    static void drawSoulFlesh(const TargetState& target,const Vec3& center);
    void drawSecretTvScreen(const GameState& state, float phoneProximity) const;
    void drawPhoneDisplayTexture(const GameState& state) const;
    void drawHud(const GameState& state) const;
    void drawDeveloperCodec(const DeveloperCodecState& codec) const;
    void drawDoorDataMosh(const GameState& state) const;
};
