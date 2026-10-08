#include "LightingStateSnapshot.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
// Minimal strict JSON syntax checker (objects, arrays, strings, numbers, literals).
struct Json {
    const std::string& t; std::size_t i = 0;
    void ws() { while (i < t.size() && (t[i] == ' ' || t[i] == '\n' || t[i] == '\t' || t[i] == '\r')) ++i; }
    bool lit(const char* w) { const std::size_t n = std::strlen(w); if (t.compare(i, n, w) != 0) return false; i += n; return true; }
    bool str() { if (t[i] != '"') return false; ++i; while (i < t.size() && t[i] != '"') { if (t[i] == '\\') ++i; ++i; } if (i >= t.size()) return false; ++i; return true; }
    bool num() { const std::size_t s = i; if (i < t.size() && t[i] == '-') ++i; while (i < t.size() && (std::isdigit(static_cast<unsigned char>(t[i])) || t[i] == '.' || t[i] == 'e' || t[i] == 'E' || t[i] == '+' || t[i] == '-')) ++i; return i > s; }
    bool val() {
        ws(); if (i >= t.size()) return false;
        if (t[i] == '{') { ++i; ws(); if (t[i] == '}') { ++i; return true; } for (;;) { ws(); if (!str()) return false; ws(); if (t[i++] != ':') return false; if (!val()) return false; ws(); if (t[i] == ',') { ++i; continue; } if (t[i] == '}') { ++i; return true; } return false; } }
        if (t[i] == '[') { ++i; ws(); if (t[i] == ']') { ++i; return true; } for (;;) { if (!val()) return false; ws(); if (t[i] == ',') { ++i; continue; } if (t[i] == ']') { ++i; return true; } return false; } }
        if (t[i] == '"') return str();
        if (lit("true") || lit("false") || lit("null")) return true;
        return num();
    }
    bool valid() { if (!val()) return false; ws(); return i == t.size(); }
};

bool has(const std::string& s, const std::string& needle) { return s.find(needle) != std::string::npos; }
}  // namespace

int main() {
    using namespace lighting_evidence;
    LightingStateSnapshot snap;
    snap.sourceCommit = "0123456789abcdef"; snap.sourceCommitShort = "0123456"; snap.buildConfiguration = "Release";
    snap.simulationTick = 120; snap.simulationTime = 2.0f;
    snap.cameraPos = {1, 2, 3}; snap.cameraLookTarget = {0, 1, -2}; snap.cameraFov = 52.0f;
    snap.roomSeed = 424242; snap.roomIndex = 7; snap.roomSetting = "sterile"; snap.roomForm = "corridor";
    snap.graphicsPreset = 1; snap.shadows = true; snap.atmosphereReference = "readable-static"; snap.overrideMask = 5;
    snap.frameWidth = 64; snap.frameHeight = 32; snap.capturePath = "frames/clean/a\"b.ppm";
    snap.pixelHashAvailable = true; snap.pixelHash = fnv1a(Fnv1aOffset, "abc", 3);
    snap.renderer.valid = true;
    snap.renderer.rig = room_lighting::roomLightRig(room_environment::RoomSetting::Sterile, room_environment::RoomForm::Corridor);
    snap.renderer.atmosphere.fogDensity = 0.0125f;
    snap.renderer.gl.lights = {true, "test-point"};
    snap.renderer.gl.light[2].enabled = true;
    snap.renderer.gl.light[2].constantAttenuation = 1.0f;
    snap.renderer.gl.globalAmbientBlock = {true, "test-point"};
    snap.renderer.gl.fogBlock = {false, "test-point"};  // deliberately unobserved
    snap.renderer.gl.flagsBlock = {true, "test-point"};
    snap.renderer.gl.depthTest = true;

    const std::string a = serializeSnapshot(snap), b = serializeSnapshot(snap);
    bool ok = true;
    const auto expect = [&](bool c, const char* what) { if (!c) { std::fprintf(stderr, "LIGHTING_STATE_SNAPSHOT_FAIL %s\n", what); ok = false; } };
    expect(a == b, "deterministic output");
    expect(a.find('\n') == std::string::npos, "single line");
    expect(Json{a}.valid(), "valid JSON structure");
    expect(has(a, "\"schema_version\":1"), "schema_version");
    for (const char* field : {"\"source\"", "\"commit\":\"0123456789abcdef\"", "\"simulation\"", "\"camera\"", "\"room\"", "\"graphics_preset\"", "\"shadows\":true",
             "\"atmosphere_reference\"", "\"override_mask\":5", "\"effective_inputs\"", "\"resolved_atmosphere\"", "\"primary_light_source\":\"ceiling-fixtures\"",
             "\"local_lights\"", "\"gameplay_response\"", "\"inputs\"", "\"outputs\"", "\"gl_state\"", "\"global_ambient\"", "\"gl_normalize\"", "\"capture_point\":\"test-point\"",
             "\"frame\":{\"width\":64,\"height\":32", "\"pixel_hash\":\"" })
        expect(has(a, field), field);
    expect(has(a, "\"fog\":{\"observed\":false,\"capture_point\":\"test-point\",\"status\":\"unavailable\"}"), "unobserved fog emits unavailable");
    expect(has(a, "a\\\"b.ppm"), "string escaping");
    expect(has(a, "\"pixel_hash\":\"" + hashHex(fnv1a(Fnv1aOffset, "abc", 3)) + "\""), "pixel hash value");
    expect(hashHex(fnv1a(Fnv1aOffset, "a", 1)) == "af63dc4c8601ec8c", "FNV-1a reference vector");

    LightingStateSnapshot missing = snap;
    missing.renderer.valid = false; missing.pixelHashAvailable = false;
    const std::string c = serializeSnapshot(missing);
    expect(Json{c}.valid(), "valid JSON when renderer unavailable");
    expect(has(c, "\"status\":\"unavailable\"") && !has(c, "\"gl_state\""), "renderer unavailable block");
    expect(has(c, "\"pixel_hash\":\"unavailable\""), "pixel hash unavailable");

    LightingStateSnapshot bad = snap;
    bad.simulationTime = std::nanf("");
    expect(has(serializeSnapshot(bad), "\"time\":null") && Json{serializeSnapshot(bad)}.valid(), "non-finite emitted as null");

    if (!ok) return 1;
    std::puts("LIGHTING_STATE_SNAPSHOT_OK");
    return 0;
}
