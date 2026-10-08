#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include "Math.hpp"
#include "VisualIdentity.hpp"
#include "RenderContracts.hpp"
#include "RoomLighting.hpp"
#include "SceneLightingResponse.hpp"

// GL-free lighting telemetry. The renderer fills RendererLightingObservation
// during draw(); the evidence scenarios wrap it in a LightingStateSnapshot and
// serialize one JSON object per frame. Nothing here may call OpenGL.
namespace lighting_evidence {

constexpr int SchemaVersion = 1;
constexpr int GlLightCount = 8;

// A block of observed data. observed==false means the values below it were
// not read and must be reported as "unavailable", never as zeros.
struct Observation {
    bool observed = false;
    std::string capturePoint;
};

struct GlLightState {
    bool enabled = false;
    std::array<float, 4> position{};
    std::array<float, 4> diffuse{};
    std::array<float, 4> ambient{};
    float constantAttenuation = 0.0f;
    float linearAttenuation = 0.0f;
    float quadraticAttenuation = 0.0f;
};

// Fixed-function state read back with glGetLightfv/glIsEnabled/glGetFloatv.
// Rooms whose rig uses CeilingFixtures disable the global sun/fill
// (GL_LIGHT0/GL_LIGHT1); lights[i].enabled records which lights were live.
struct GlFixedFunctionState {
    Observation lights;
    std::array<GlLightState, GlLightCount> light{};
    Observation globalAmbientBlock;
    std::array<float, 4> globalAmbient{};
    Observation fogBlock;
    bool fogEnabled = false;
    std::array<float, 4> fogColor{};
    float fogDensity = 0.0f;
    Observation flagsBlock;
    bool normalize = false;
    bool lighting = false;
    bool colorMaterial = false;
    bool depthTest = false;
};

struct RendererLightingObservation {
    bool valid = false;  // false until draw() has run
    render_contract::SceneAtmosphere atmosphere{};
    render_contract::AtmosphereInputs effectiveInputs{};
    room_lighting::RoomLightRig rig{};
    scene_lighting_response::Inputs responseInputs{};
    scene_lighting_response::Response response{};
    GlFixedFunctionState gl{};
};

struct Vec3Snapshot { float x = 0.0f, y = 0.0f, z = 0.0f; };

struct LightingStateSnapshot {
    int schemaVersion = SchemaVersion;
    std::string sourceCommit, sourceCommitShort, buildConfiguration;
    std::uint64_t simulationTick = 0;
    float simulationTime = 0.0f;
    Vec3Snapshot cameraPos{}, cameraLookTarget{};
    float cameraFov = 0.0f;
    int roomSeed = 0, roomIndex = 0;
    std::string roomSetting, roomForm;
    int graphicsPreset = 0;
    bool shadows = false;
    std::string atmosphereReference;
    std::uint32_t overrideMask = 0;
    RendererLightingObservation renderer{};
    int frameWidth = 0, frameHeight = 0;
    std::string capturePath;
    bool pixelHashAvailable = false;
    std::uint64_t pixelHash = 0;
};

// FNV-1a 64-bit, incremental so a writer can hash exactly the bytes it writes.
constexpr std::uint64_t Fnv1aOffset = 14695981039346656037ull;
inline std::uint64_t fnv1a(std::uint64_t hash, const void* data, std::size_t size) {
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (std::size_t i = 0; i < size; ++i) { hash ^= bytes[i]; hash *= 1099511628211ull; }
    return hash;
}

inline std::string hashHex(std::uint64_t hash) {
    char text[17]{};
    std::snprintf(text, sizeof(text), "%016llx", static_cast<unsigned long long>(hash));
    return text;
}

namespace detail {
inline void number(std::string& out, float value) {
    if (!std::isfinite(value)) { out += "null"; return; }
    char text[48]{};
    std::snprintf(text, sizeof(text), "%.6f", static_cast<double>(value));
    out += text;
}
inline void quote(std::string& out, const std::string& value) {
    out += '"';
    for (const char c : value) {
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if (static_cast<unsigned char>(c) < 0x20) { char e[8]{}; std::snprintf(e, sizeof(e), "\\u%04x", static_cast<unsigned>(c)); out += e; }
        else out += c;
    }
    out += '"';
}
inline void boolean(std::string& out, bool value) { out += value ? "true" : "false"; }
inline void array(std::string& out, const float* values, int count) {
    out += '[';
    for (int i = 0; i < count; ++i) { if (i) out += ','; number(out, values[i]); }
    out += ']';
}
inline void color(std::string& out, const VisualColor& c) { const float v[3] = {c.r, c.g, c.b}; array(out, v, 3); }
inline void vec3(std::string& out, float x, float y, float z) { const float v[3] = {x, y, z}; array(out, v, 3); }
inline void observationHeader(std::string& out, const Observation& o) {
    out += "\"observed\":"; boolean(out, o.observed);
    out += ",\"capture_point\":"; quote(out, o.capturePoint);
}
inline const char* sourceName(room_lighting::PrimaryLightSource s) {
    return s == room_lighting::PrimaryLightSource::CeilingFixtures ? "ceiling-fixtures" : (s == room_lighting::PrimaryLightSource::UrbanSky ? "urban-sky" : "outdoor-sun");
}
inline void unavailable(std::string& out, const Observation& o) {
    out += '{'; observationHeader(out, o); out += ",\"status\":\"unavailable\"}";
}
inline void glState(std::string& out, const GlFixedFunctionState& g) {
    out += "{\"lights\":";
    if (!g.lights.observed) unavailable(out, g.lights);
    else {
        out += '{'; observationHeader(out, g.lights); out += ",\"items\":[";
        for (int i = 0; i < GlLightCount; ++i) {
            const GlLightState& l = g.light[static_cast<std::size_t>(i)];
            if (i) out += ',';
            out += "{\"index\":" + std::to_string(i) + ",\"enabled\":"; boolean(out, l.enabled);
            out += ",\"position\":"; array(out, l.position.data(), 4);
            out += ",\"diffuse\":"; array(out, l.diffuse.data(), 4);
            out += ",\"ambient\":"; array(out, l.ambient.data(), 4);
            out += ",\"attenuation_constant\":"; number(out, l.constantAttenuation);
            out += ",\"attenuation_linear\":"; number(out, l.linearAttenuation);
            out += ",\"attenuation_quadratic\":"; number(out, l.quadraticAttenuation);
            out += '}';
        }
        out += "]}";
    }
    out += ",\"global_ambient\":";
    if (!g.globalAmbientBlock.observed) unavailable(out, g.globalAmbientBlock);
    else { out += '{'; observationHeader(out, g.globalAmbientBlock); out += ",\"value\":"; array(out, g.globalAmbient.data(), 4); out += '}'; }
    out += ",\"fog\":";
    if (!g.fogBlock.observed) unavailable(out, g.fogBlock);
    else {
        out += '{'; observationHeader(out, g.fogBlock);
        out += ",\"enabled\":"; boolean(out, g.fogEnabled);
        out += ",\"color\":"; array(out, g.fogColor.data(), 4);
        out += ",\"density\":"; number(out, g.fogDensity); out += '}';
    }
    out += ",\"flags\":";
    if (!g.flagsBlock.observed) unavailable(out, g.flagsBlock);
    else {
        out += '{'; observationHeader(out, g.flagsBlock);
        out += ",\"gl_normalize\":"; boolean(out, g.normalize);
        out += ",\"gl_lighting\":"; boolean(out, g.lighting);
        out += ",\"gl_color_material\":"; boolean(out, g.colorMaterial);
        out += ",\"gl_depth_test\":"; boolean(out, g.depthTest); out += '}';
    }
    out += '}';
}
}  // namespace detail

// One JSON object, no trailing newline. Output is deterministic for equal input.
inline std::string serializeSnapshot(const LightingStateSnapshot& s) {
    using namespace detail;
    const RendererLightingObservation& r = s.renderer;
    std::string out;
    out += "{\"schema_version\":" + std::to_string(s.schemaVersion);
    out += ",\"source\":{\"commit\":"; quote(out, s.sourceCommit);
    out += ",\"commit_short\":"; quote(out, s.sourceCommitShort);
    out += ",\"configuration\":"; quote(out, s.buildConfiguration); out += '}';
    out += ",\"simulation\":{\"tick\":" + std::to_string(s.simulationTick) + ",\"time\":"; number(out, s.simulationTime); out += '}';
    out += ",\"camera\":{\"pos\":"; vec3(out, s.cameraPos.x, s.cameraPos.y, s.cameraPos.z);
    out += ",\"look_target\":"; vec3(out, s.cameraLookTarget.x, s.cameraLookTarget.y, s.cameraLookTarget.z);
    out += ",\"fov\":"; number(out, s.cameraFov); out += '}';
    out += ",\"room\":{\"seed\":" + std::to_string(s.roomSeed) + ",\"index\":" + std::to_string(s.roomIndex);
    out += ",\"setting\":"; quote(out, s.roomSetting);
    out += ",\"form\":"; quote(out, s.roomForm); out += '}';
    out += ",\"graphics_preset\":" + std::to_string(s.graphicsPreset) + ",\"shadows\":"; boolean(out, s.shadows);
    out += ",\"atmosphere_reference\":"; quote(out, s.atmosphereReference);
    out += ",\"override_mask\":" + std::to_string(s.overrideMask);
    out += ",\"renderer\":{\"observed\":"; boolean(out, r.valid);
    out += ",\"capture_point\":\"DesktopRenderer::draw:world-pass-lights-set\"";
    if (!r.valid) out += ",\"status\":\"unavailable\"}";
    else {
        out += ",\"effective_inputs\":{\"time\":"; number(out, r.effectiveInputs.time);
        out += ",\"room_index\":" + std::to_string(r.effectiveInputs.roomIndex) + ",\"phone_power\":"; number(out, r.effectiveInputs.phonePower); out += '}';
        out += ",\"resolved_atmosphere\":{\"background\":"; color(out, r.atmosphere.background);
        out += ",\"ambient\":"; color(out, r.atmosphere.ambient);
        out += ",\"sun\":"; color(out, r.atmosphere.sun);
        out += ",\"fill\":"; color(out, r.atmosphere.fill);
        out += ",\"phone\":"; color(out, r.atmosphere.phone);
        out += ",\"fog\":"; color(out, r.atmosphere.fog);
        out += ",\"fog_density\":"; number(out, r.atmosphere.fogDensity); out += '}';
        out += ",\"primary_light_source\":"; quote(out, sourceName(r.rig.primarySource));
        out += ",\"local_lights\":[";
        for (int i = 0; i < r.rig.localLightCount; ++i) {
            const auto& l = r.rig.localLights[static_cast<std::size_t>(i)];
            if (i) out += ',';
            out += "{\"position\":"; vec3(out, l.localPosition.x, l.localPosition.y, l.localPosition.z);
            out += ",\"color\":"; color(out, l.color);
            out += ",\"intensity\":"; number(out, l.intensity);
            out += ",\"radius\":"; number(out, l.radius);
            out += ",\"visible_fixture\":"; boolean(out, l.visibleFixture); out += '}';
        }
        out += "],\"gameplay_response\":{\"inputs\":{\"vacuum_power\":"; number(out, r.responseInputs.vacuumPower);
        out += ",\"discharge\":"; number(out, r.responseInputs.discharge);
        out += ",\"latest_shot_age\":"; number(out, r.responseInputs.latestShotAge);
        out += ",\"critical_pulse\":"; number(out, r.responseInputs.criticalPulse);
        out += ",\"objective_progress\":"; number(out, r.responseInputs.objectiveProgress);
        out += ",\"room_clear\":"; boolean(out, r.responseInputs.roomClear);
        out += "},\"outputs\":{\"shot_light\":"; number(out, r.response.shotLight);
        out += ",\"action_light\":"; number(out, r.response.actionLight);
        out += ",\"critical_light\":"; number(out, r.response.criticalLight);
        out += ",\"phone_light_scale\":"; number(out, r.response.phoneLightScale);
        out += ",\"exit_glow\":"; number(out, r.response.exitGlow); out += "}}";
        out += ",\"gl_state\":"; glState(out, r.gl);
        out += '}';
    }
    out += ",\"frame\":{\"width\":" + std::to_string(s.frameWidth) + ",\"height\":" + std::to_string(s.frameHeight);
    out += ",\"capture_path\":"; quote(out, s.capturePath);
    out += ",\"pixel_hash_algorithm\":\"fnv1a64\",\"pixel_hash\":";
    if (s.pixelHashAvailable) quote(out, hashHex(s.pixelHash)); else out += "\"unavailable\"";
    out += "}}";
    return out;
}

}  // namespace lighting_evidence
