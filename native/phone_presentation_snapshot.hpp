#pragma once

#include "Game.hpp"
#include "GameplayPhoneModel.hpp"

// A bounded, read-only projection of existing GameState for render adapters.
// Values describe the native phone and its current semantic display state;
// this type owns no gameplay state and advances no timers.
struct PhonePresentationSnapshot {
    Vec3 position;
    Quat orientation;
    Vec3 bodySize;
    Vec3 bodyScale;
    Vec3 screenCenterOffset;
    Vec3 screenSize;
    int screenMode = 0;
    bool bodyVisible = false;
    bool screenVisible = false;
    bool firstPerson = false;
    float screenBrightness = 0.0f;
    float contentOpacity = 0.0f;
    Vec3 screenTint;
    float screenGlow = 0.0f;
    float emissionStrength = 0.0f;
    Vec3 screenLightColor;
    float screenLightIntensity = 0.0f;
    float screenLightRadius = 0.0f;
    float screenLightForwardOffset = 0.0f;
    float screenLightPulse = 0.0f;
    bool interactive = false;
    float battery = 0.0f;
    int souls = 0;
    bool lowBattery = false;
    bool vacuumActive = false;
    float vacuumPower = 0.0f;
    float capturePulse = 0.0f;
    float damagePulse = 0.0f;
    int roomIndex = 0;
    long long tokens = 0;
    int filledGoals = 0;
    int requiredGoals = 0;
    bool roomClear = false;
    int soulCapacity = 0;
    int unlockedMaterialMemories = 0;
    bool supplementalActive = false;
    float supplementalFill = 0.0f;
    int flowerStacks = 0;

    static PhonePresentationSnapshot from(const GameState& state) {
        PhonePresentationSnapshot snapshot;
        snapshot.position = state.phoneTransform.position;
        snapshot.orientation = state.phoneTransform.orientation;
        snapshot.bodySize = {PHONE_BODY_WIDTH, PHONE_BODY_HEIGHT, PHONE_BODY_DEPTH};
        snapshot.bodyScale = state.phoneVisual.bodyScale;
        snapshot.screenCenterOffset = inverseRotate(
            state.phoneTransform.orientation,
            state.phoneTransform.screenCenter - state.phoneTransform.position);
        snapshot.screenSize = {
            PHONE_SCREEN_WIDTH * state.phoneVisual.screenScale.x,
            PHONE_SCREEN_HEIGHT * state.phoneVisual.screenScale.y,
            PHONE_SCREEN_DEPTH * 0.30f};
        snapshot.screenMode = static_cast<int>(state.phoneDisplay.mode);
        snapshot.bodyVisible = state.phoneVisual.visible;
        snapshot.screenVisible = state.phoneVisual.visible &&
            state.phoneDisplay.mode != PhoneDisplayMode::Off &&
            state.phoneDisplay.mode != PhoneDisplayMode::Death;
        snapshot.firstPerson = state.camera.firstPerson;
        snapshot.screenBrightness = state.phoneDisplay.brightness;
        snapshot.contentOpacity = state.phoneDisplay.contentOpacity;
        snapshot.screenTint = state.phoneDisplay.screenTint;
        snapshot.screenGlow = state.phoneVisual.screenGlow;
        snapshot.emissionStrength = state.phoneDisplay.emissionStrength;
        snapshot.screenLightColor = state.phoneDisplay.lighting.color;
        snapshot.screenLightIntensity = state.phoneDisplay.lighting.intensity;
        snapshot.screenLightRadius = state.phoneDisplay.lighting.radius;
        snapshot.screenLightForwardOffset = state.phoneDisplay.lighting.forwardOffset;
        snapshot.screenLightPulse = state.phoneDisplay.lighting.pulse;
        snapshot.interactive = state.phoneDisplay.interactive;
        snapshot.battery = state.player.battery;
        snapshot.souls = state.player.souls;
        snapshot.lowBattery = state.hud.lowBattery;
        snapshot.vacuumActive = state.vacuum.active;
        snapshot.vacuumPower = state.vacuum.power;
        snapshot.capturePulse = state.phoneDisplay.capturePulse;
        snapshot.damagePulse = state.phoneDisplay.damagePulse;
        const GameplayPhoneModel gameplay = makeGameplayPhoneModel(state);
        snapshot.roomIndex = gameplay.roomIndex;
        snapshot.tokens = static_cast<long long>(gameplay.tokens);
        snapshot.filledGoals = gameplay.filledGoals;
        snapshot.requiredGoals = gameplay.requiredGoals;
        snapshot.roomClear = gameplay.roomClear;
        snapshot.soulCapacity = gameplay.soulCapacity;
        snapshot.unlockedMaterialMemories = gameplay.unlockedMaterialMemories;
        snapshot.supplementalActive = gameplay.supplementalActive;
        snapshot.supplementalFill = gameplay.supplementalFill;
        snapshot.flowerStacks = gameplay.flowerStacks;
        return snapshot;
    }
};
