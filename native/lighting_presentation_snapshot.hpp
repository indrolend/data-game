#pragma once

#include "Game.hpp"
#include "RoomLighting.hpp"
#include <algorithm>

// Read-only renderer input resolved from native game state and the existing
// renderer contracts. Godot owns the light/environment nodes, not the values.
struct LightingPresentationSnapshot {
    int roomIndex = 0;
    int setting = 0;
    float time = 0.0f;
    float daylight = 0.0f;
    bool openSky = false;
    VisualColor background{};
    room_lighting::ResolvedSceneLighting lighting{};

    static LightingPresentationSnapshot from(const GameState& state) {
        const auto plan = room_environment::roomPlan(state.roomSeed, state.roomIndex);
        const bool openSky = plan.setting != room_environment::RoomSetting::Sterile;
        const auto profile = openSky
            ? render_contract::AtmosphereProfile::ProgressiveCandidate
            : render_contract::AtmosphereProfile::ReadableStatic;
        const auto atmosphere = room_lighting::settingAtmosphere(
            render_contract::sceneAtmosphere(profile, state.time, state.roomIndex,
                state.player.battery / 100.0f), plan.setting, profile);
        scene_lighting_response::Inputs responseInputs{};
        responseInputs.vacuumPower = state.vacuum.power;
        responseInputs.discharge = state.energy.dischargePositionAmount;
        responseInputs.latestShotAge = state.environmentVisual.latestShotAge;
        responseInputs.criticalPulse = state.hud.criticalHitPulse;
        responseInputs.objectiveProgress = state.requiredSouls > 0
            ? static_cast<float>(state.depositedSouls) / static_cast<float>(state.requiredSouls)
            : 0.0f;
        responseInputs.roomClear = state.roomClear;
        responseInputs.completionAfterglow = state.phoneDisplay.capturePulse;
        for (const auto& target : state.targets)
            responseInputs.captureActivity = std::max(responseInputs.captureActivity, target.capture);
        const auto response = scene_lighting_response::resolve(responseInputs);
        auto rig = room_lighting::roomLightRig(plan.setting, plan.form);
        const auto sunState = render_contract::resolveSunState(
            state.time, openSky, rig.sunDirection);
        if (openSky) rig.sunDirection = sunState.direction;
        Vec3 capturePosition{};
        for (int i = 0; i < state.requiredSouls && i < CAPTURE_COUNT; ++i) {
            if (!state.captures[i].filled) {
                capturePosition = state.captures[i].pos;
                break;
            }
        }

        LightingPresentationSnapshot snapshot;
        snapshot.roomIndex = state.roomIndex;
        snapshot.setting = static_cast<int>(plan.setting);
        snapshot.time = state.time;
        snapshot.daylight = openSky ? sunState.daylight : 0.0f;
        snapshot.openSky = openSky;
        snapshot.background = atmosphere.background;
        snapshot.lighting = room_lighting::resolveSceneLighting(
            atmosphere, rig, response, state.phoneTransform.position,
            state.environmentVisual.latestShotOrigin,
            static_cast<float>(state.topology.currentTileIndex) * world::RoomDepth,
            capturePosition);
        if (openSky) snapshot.lighting.sun.enabled = sunState.castsDirectionalShadows;
        return snapshot;
    }
};
