#pragma once

#include <cstdint>
#include <vector>
#include "Math.hpp"
#include "VisualIdentity.hpp"
#include "RenderContracts.hpp"
#include "RoomLighting.hpp"
#include "SceneLightingResponse.hpp"

namespace lighting_evidence {

// Captures renderer-owned lighting state for a single frame.
// Included in evidence timeline for traceability from source to pixel.
struct LocalLightSnapshot {
    Vec3 position{};
    VisualColor color{};
    float intensity = 0.0f;
    float radius = 0.0f;
    bool visible_fixture = false;
};

struct LightingStateSnapshot {
    // Source identity
    int source_commit_short = 0;  // First 7 chars of commit SHA as decimal int (for compact storage)
    bool source_dirty = false;
    
    // Simulation and capture context
    std::uint64_t simulation_tick = 0;
    float simulation_time = 0.0f;
    int room_seed = 0;
    int room_index = 0;
    
    // Graphics and lighting config
    int graphics_preset = 0;
    bool shadows_enabled = false;
    
    // Atmosphere profile and effective inputs
    int atmosphere_reference = 0;  // 0=ReadableStatic, 1=ProgressiveCandidate
    std::uint32_t override_mask = 0;
    float effective_time = 0.0f;
    int effective_room_index = 0;
    float effective_phone_power = 0.0f;
    
    // Resolved atmosphere (as GPU will render)
    VisualColor resolved_background{};
    VisualColor resolved_ambient{};
    VisualColor resolved_sun{};
    VisualColor resolved_fill{};
    VisualColor resolved_phone{};
    VisualColor resolved_fog{};
    float resolved_fog_density = 0.0f;
    
    // Room and local lights
    int primary_light_source = 0;  // 0=OutdoorSun, 1=UrbanSky, 2=CeilingFixtures
    int local_light_count = 0;
    std::vector<LocalLightSnapshot> local_lights;
    
    // Gameplay-responsive lighting
    float shot_light = 0.0f;
    float action_light = 0.0f;
    float critical_light = 0.0f;
    float phone_light_scale = 1.0f;
    float exit_glow = 0.0f;
};

}  // namespace lighting_evidence
