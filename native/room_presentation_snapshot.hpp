#pragma once

#include "ComputedSceneObjects.hpp"
#include "CityFacadeGrammar.hpp"
#include "CityStreetGrammar.hpp"
#include "CoastalWater.hpp"
#include "Game.hpp"
#include "FieldIsland.hpp"
#include "MaterialResponse.hpp"

#include <vector>
#include <utility>

enum class RoomStructureKind : unsigned char {
    Floor,
    Ceiling,
    BoundaryWall,
    DoorFrame,
    TraversalSurface,
    CityStreet,
    CityBuildingMass,
    CitySkylineMass,
    CitySkylineWindow,
    CityStreetFixture,
    CityServiceFixture,
    FieldIslandCore,
    Abyss
};

struct RoomStructureRecord {
    std::string id;
    RoomStructureKind kind = RoomStructureKind::Floor;
    Vec3 center;
    Vec3 size;
    render_contract::MaterialDefinition material;
};

struct CityFacadeModuleRecord {
    std::string id;
    std::string model;
    Vec3 origin;
    Vec3 scale{1, 1, 1};
    float yaw = 0.0f;
};

// Render-only Field framing recovered from DesktopRenderer::drawFieldBackdrop.
// These meshes intentionally have no collider or gameplay identity.
struct FieldBackdropTreeRecord {
    std::string id;
    std::string model;
    Vec3 origin;
    Vec3 scale{1, 1, 1};
    float pitch = 0.0f;
    float yaw = 0.0f;
    float roll = 0.0f;
};

struct CapturePointPresentationRecord {
    std::string id;
    Vec3 position;
    bool filled = false;
    render_contract::MaterialDefinition surroundMaterial;
    render_contract::MaterialDefinition recessMaterial;
    render_contract::MaterialDefinition soulMaterial;
};

// Read-only room presentation data derived from the same native generation
// functions consumed by the desktop renderer. Godot may materialize these
// records as scene nodes, but does not choose room contents or physical rules.
struct RoomPresentationSnapshot {
    int roomSeed = 0;
    int roomIndex = 0;
    int tileIndex = 0;
    int currentTileIndex = 0;
    int setting = 0;
    int form = 0;
    int scale = 0;
    int condition = 0;
    int traversalIntent = 0;
    bool roomClear = false;
    bool advancing = false;
    bool grass = false;
    bool sidewalks = false;
    bool coastalWater = false;
    bool fieldIsland = false;
    float fieldIslandMainHalfWidth = 0.0f;
    float fieldIslandMainHalfDepth = 0.0f;
    float fieldIslandBridgeHalfWidth = 0.0f;
    float fieldIslandBridgeHalfDepth = 0.0f;
    float abyssRecoveryY = 0.0f;
    int abyssRecoveryCount = 0;
    float grassAmount = 0.0f;
    float presentationTime = 0.0f;
    Vec3 waterSourcePosition;
    float waterSourceSpeed = 0.0f;
    bool waterSourceActive = false;
    std::vector<RoomStructureRecord> structures;
    std::vector<CityFacadeModuleRecord> cityFacadeModules;
    std::vector<FieldBackdropTreeRecord> fieldBackdropTrees;
    std::vector<CapturePointPresentationRecord> capturePoints;
    std::vector<computed_scene::Object> objects;

    static RoomPresentationSnapshot from(const GameState& state, int tileIndex) {
        RoomPresentationSnapshot snapshot;
        const auto plan = room_environment::roomPlan(state.roomSeed, state.roomIndex);
        const auto capacity = state.slopeLab
            ? room_environment::RoomGeometryCapacityPlan{}
            : room_environment::roomGeometryCapacityPlan(
                plan, state.roomSeed, state.roomIndex, ROOM_COLLIDER_COUNT);
        snapshot.roomSeed = state.roomSeed;
        snapshot.roomIndex = state.roomIndex;
        snapshot.tileIndex = tileIndex;
        snapshot.currentTileIndex = state.topology.currentTileIndex;
        snapshot.setting = static_cast<int>(plan.setting);
        snapshot.form = static_cast<int>(plan.form);
        snapshot.scale = static_cast<int>(plan.scale);
        snapshot.condition = static_cast<int>(plan.condition);
        snapshot.traversalIntent = static_cast<int>(plan.traversalIntent);
        snapshot.roomClear = state.roomClear;
        snapshot.advancing = state.topology.advancing;
        snapshot.grass = plan.grass;
        snapshot.sidewalks = plan.sidewalks;
        snapshot.grassAmount = plan.grassAmount;
        snapshot.coastalWater = plan.setting == room_environment::RoomSetting::Coastal;
        snapshot.fieldIsland = !state.rallyLab && !state.traversalLab && !state.slopeLab &&
            !state.roomInspector && field_island::active(state.roomSeed, state.roomIndex);
        snapshot.fieldIslandMainHalfWidth = field_island::MainHalfWidth;
        snapshot.fieldIslandMainHalfDepth = field_island::MainHalfDepth;
        snapshot.fieldIslandBridgeHalfWidth = field_island::DoorBridgeHalfWidth;
        snapshot.fieldIslandBridgeHalfDepth = field_island::DoorBridgeHalfDepth;
        snapshot.abyssRecoveryY = field_island::AbyssRecoveryY;
        snapshot.abyssRecoveryCount = state.player.abyssRecoveryCount;
        snapshot.presentationTime = state.time;
        snapshot.waterSourcePosition = state.player.pos;
        snapshot.waterSourceSpeed = std::sqrt(state.player.vel.x * state.player.vel.x +
            state.player.vel.z * state.player.vel.z);
        const float currentTileOffset = static_cast<float>(state.topology.currentTileIndex) *
            world::RoomDepth;
        snapshot.waterSourceActive = snapshot.coastalWater && coastal_water::contains(
            state.player.pos.x, state.player.pos.z - currentTileOffset) &&
            snapshot.waterSourceSpeed >= 0.35f;
        const float z0 = static_cast<float>(tileIndex) * world::RoomDepth;
        const float doorWidth = 5.35f;
        const float doorHeight = 3.95f;
        const bool field = plan.setting == room_environment::RoomSetting::Field;
        const bool city = plan.setting == room_environment::RoomSetting::City;
        const bool sterile = plan.setting == room_environment::RoomSetting::Sterile;
        const bool coastal = plan.setting == room_environment::RoomSetting::Coastal;
        const bool openNatural = (field && plan.form == room_environment::RoomForm::Open) || coastal;
        const auto addStructure = [&](const char* suffix, RoomStructureKind kind, Vec3 center, Vec3 size,
                                      render_contract::MaterialDefinition material) {
            snapshot.structures.push_back({
                "room/" + std::to_string(tileIndex) + "/structure/" + suffix,
                kind, center, size, material});
        };
        const VisualColor floorColor = field ? VisualIdentity::FieldGround
            : (sterile ? VisualColor{0.58f, 0.61f, 0.63f}
                : (coastal ? roomSubstrateColor(plan.setting)
                    : VisualIdentity::RoomFloor));
        const SemanticSurface floorSurface = field ? SemanticSurface::FieldGround
            : (city ? SemanticSurface::CityAsphalt
                : (sterile ? SemanticSurface::SterilePanel : SemanticSurface::Default));
        const auto floorMaterial = semanticMaterial(floorSurface, floorColor);
        const auto wallMaterial = semanticMaterial(
            sterile ? SemanticSurface::SterilePanel : SemanticSurface::Default,
            VisualIdentity::RoomWall);
        const VisualColor frameColor = field ? VisualColor{0.20f, 0.27f, 0.22f}
            : VisualColor{0.33f, 0.36f, 0.34f};
        const auto frameMaterial = semanticMaterial(SemanticSurface::Default, frameColor);
        const auto traversalColor = room_environment::traversalPresentationFor(
            plan.setting, state.roomInspector).color;
        const auto traversalMaterial = semanticMaterial(SemanticSurface::Default,
            {traversalColor.x, traversalColor.y, traversalColor.z});
        const auto captureSurroundMaterial=semanticMaterial(SemanticSurface::Default,
            {VisualIdentity::MetallicTeal.r*0.74f,VisualIdentity::MetallicTeal.g*0.74f,VisualIdentity::MetallicTeal.b*0.74f});
        const auto captureRecessMaterial=semanticMaterial(SemanticSurface::Default,{0.02f,0.03f,0.04f});
        const auto capturedSoulMaterial=semanticMaterial(SemanticSurface::Default,VisualIdentity::SoulBase);
        for(int captureIndex=0;captureIndex<state.requiredSouls&&captureIndex<CAPTURE_COUNT;++captureIndex){
            const auto& capture=state.captures[captureIndex];
            snapshot.capturePoints.push_back({
                "room/"+std::to_string(tileIndex)+"/capture/"+std::to_string(captureIndex),
                capture.pos+Vec3{0,0,z0},capture.filled,captureSurroundMaterial,captureRecessMaterial,capturedSoulMaterial});
        }
        if(snapshot.fieldIsland){
            const auto islandCoreMaterial=semanticMaterial(SemanticSurface::Default,
                VisualColor{0.12f,0.10f,0.075f});
            const auto abyssMaterial=semanticMaterial(SemanticSurface::Default,
                VisualColor{0.008f,0.018f,0.024f});
            addStructure("island/core",RoomStructureKind::FieldIslandCore,{0,-0.60f,z0},
                {field_island::MainHalfWidth*2.0f,1.2f,field_island::MainHalfDepth*2.0f},islandCoreMaterial);
            addStructure("floor",RoomStructureKind::Floor,{0,0.04f,z0},
                {field_island::MainHalfWidth*2.0f,0.08f,field_island::MainHalfDepth*2.0f},floorMaterial);
            const float bridgeSegmentDepth=field_island::DoorBridgeHalfDepth-field_island::MainHalfDepth;
            const float bridgeSegmentCenter=field_island::MainHalfDepth+bridgeSegmentDepth*0.5f;
            for(const auto& bridge : {std::pair<const char*,float>{"front",bridgeSegmentCenter},
                                      std::pair<const char*,float>{"back",-bridgeSegmentCenter}}){
                addStructure((std::string("island/bridge-core/")+bridge.first).c_str(),RoomStructureKind::FieldIslandCore,
                    {0,-0.42f,z0+bridge.second},{field_island::DoorBridgeHalfWidth*2.0f,0.84f,bridgeSegmentDepth},islandCoreMaterial);
                addStructure((std::string("island/bridge-top/")+bridge.first).c_str(),RoomStructureKind::Floor,
                    {0,0.04f,z0+bridge.second},{field_island::DoorBridgeHalfWidth*2.0f,0.08f,bridgeSegmentDepth},floorMaterial);
            }
            addStructure("abyss",RoomStructureKind::Abyss,{0,field_island::AbyssRecoveryY-1.5f,z0},
                {world::RoomWidth*5.0f,0.05f,world::RoomDepth*3.0f},abyssMaterial);
        }else{
            addStructure("floor", RoomStructureKind::Floor, {0, -0.04f, z0},
                {openNatural ? world::RoomWidth * 2.8f : world::RoomWidth, 0.08f, world::RoomDepth},
                floorMaterial);
        }
        if (sterile) addStructure("ceiling", RoomStructureKind::Ceiling,
            {0, world::RoomWallHeight + 0.08f, z0},
            {world::RoomWidth, 0.16f, world::RoomDepth}, wallMaterial);
        if (openNatural) {
            int seamIndex = 0;
            for (float seam : {-world::RoomDepth * 0.5f, world::RoomDepth * 0.5f}) {
                const std::string prefix = "door-frame/" + std::to_string(seamIndex++);
                addStructure((prefix + "/left").c_str(), RoomStructureKind::DoorFrame,
                    {-doorWidth * 0.56f, doorHeight * 0.48f, z0 + seam},
                    {0.24f, doorHeight * 0.96f, 0.28f}, frameMaterial);
                addStructure((prefix + "/right").c_str(), RoomStructureKind::DoorFrame,
                    {doorWidth * 0.56f, doorHeight * 0.48f, z0 + seam},
                    {0.24f, doorHeight * 0.96f, 0.28f}, frameMaterial);
                addStructure((prefix + "/top").c_str(), RoomStructureKind::DoorFrame,
                    {0, doorHeight * 0.96f, z0 + seam},
                    {doorWidth * 1.16f, 0.24f, 0.28f}, frameMaterial);
            }
        } else if (!city) {
            const float sideWidth = (world::RoomWidth - doorWidth) * 0.5f;
            const float sideX = doorWidth * 0.5f + sideWidth * 0.5f;
            const float topHeight = world::RoomWallHeight - doorHeight;
            const float topY = doorHeight + topHeight * 0.5f;
            int seamIndex = 0;
            for (float seam : {-world::RoomDepth * 0.5f, world::RoomDepth * 0.5f}) {
                const std::string prefix = "door-wall/" + std::to_string(seamIndex++);
                addStructure((prefix + "/left").c_str(), RoomStructureKind::BoundaryWall,
                    {-sideX, world::RoomWallHeight * 0.5f, z0 + seam},
                    {sideWidth, world::RoomWallHeight, 0.5f}, wallMaterial);
                addStructure((prefix + "/right").c_str(), RoomStructureKind::BoundaryWall,
                    {sideX, world::RoomWallHeight * 0.5f, z0 + seam},
                    {sideWidth, world::RoomWallHeight, 0.5f}, wallMaterial);
                addStructure((prefix + "/top").c_str(), RoomStructureKind::BoundaryWall,
                    {0, topY, z0 + seam}, {doorWidth, topHeight, 0.5f}, wallMaterial);
            }
            addStructure("wall/left", RoomStructureKind::BoundaryWall,
                {-world::RoomWidth * 0.5f, world::RoomWallHeight * 0.5f, z0},
                {0.5f, world::RoomWallHeight, world::RoomDepth}, wallMaterial);
            addStructure("wall/right", RoomStructureKind::BoundaryWall,
                {world::RoomWidth * 0.5f, world::RoomWallHeight * 0.5f, z0},
                {0.5f, world::RoomWallHeight, world::RoomDepth}, wallMaterial);
        }
        for (int surfaceIndex = 0; surfaceIndex < plan.traversal.surfaceCount; ++surfaceIndex) {
            if (!capacity.traversalIncluded[surfaceIndex]) continue;
            const auto obstacle = room_environment::physicalTraversalObstacle(
                plan.traversal.surfaces[surfaceIndex]);
            const std::string suffix = "traversal/" + std::to_string(surfaceIndex);
            addStructure(suffix.c_str(), RoomStructureKind::TraversalSurface,
                obstacle.center + Vec3{0, 0, z0}, obstacle.size, traversalMaterial);
        }
        if (city) {
            const std::uint32_t key = room_environment::roomKey(
                state.roomSeed, state.roomIndex) ^
                static_cast<std::uint32_t>((tileIndex + 11) * 0x9e3779b9u);
            const auto street = city_street::plan(
                plan.form, key, state.localSettings.graphicsPreset);
            const auto addStreetBox = [&](const std::string& suffix, Vec3 center, Vec3 size,
                                          VisualColor color) {
                addStructure(suffix.c_str(), RoomStructureKind::CityStreet,
                    center, size, semanticMaterial(SemanticSurface::Default, color));
            };
            const float sidewalkCenter = street.roadHalfWidth + street.sidewalkWidth * 0.5f;
            addStreetBox("city/road", {0, 0.010f, z0},
                {street.roadHalfWidth * 2.0f, 0.022f, world::RoomDepth},
                {0.68f, 0.70f, 0.71f});
            for (const float side : {-1.0f, 1.0f}) {
                addStreetBox(side < 0 ? "city/sidewalk/left" : "city/sidewalk/right",
                    {side * sidewalkCenter, 0.050f, z0},
                    {street.sidewalkWidth, 0.10f, world::RoomDepth - 0.28f},
                    {0.40f, 0.405f, 0.39f});
                addStreetBox(side < 0 ? "city/curb/left" : "city/curb/right",
                    {side * (street.roadHalfWidth + 0.055f), 0.082f, z0},
                    {0.11f, 0.164f, world::RoomDepth - 0.20f},
                    {0.54f, 0.53f, 0.49f});
                addStreetBox(side < 0 ? "city/gutter/left" : "city/gutter/right",
                    {side * (street.roadHalfWidth - 0.12f), 0.022f, z0},
                    {0.16f, 0.024f, world::RoomDepth - 0.36f},
                    {0.075f, 0.085f, 0.09f});
            }
            addStreetBox("city/intersection", {0, 0.025f, z0 + street.intersectionCenter},
                {world::RoomWidth - 0.7f, 0.026f, street.intersectionWidth},
                {0.61f, 0.63f, 0.64f});
            for (const int approach : {-1, 1}) {
                for (int stripe = 0; stripe < 7; ++stripe) {
                    const float x = -street.roadHalfWidth + 0.58f +
                        static_cast<float>(stripe) * (street.roadHalfWidth * 2.0f - 1.16f) / 6.0f;
                    const float z = z0 + street.intersectionCenter +
                        static_cast<float>(approach) * (street.intersectionWidth * 0.5f + 0.42f);
                    addStreetBox("city/crosswalk/" + std::to_string(approach) + "/" +
                        std::to_string(stripe), {x, 0.041f, z}, {0.34f, 0.014f, 0.68f},
                        {0.72f, 0.71f, 0.64f});
                }
            }
            for (int dash = 0; dash < 8; ++dash) {
                const float localZ = -17.5f + static_cast<float>(dash) * 5.0f;
                if (std::abs(localZ - street.intersectionCenter) <
                    street.intersectionWidth * 0.62f) continue;
                const float wear = 0.76f + 0.12f * room_environment::unit(
                    key + static_cast<std::uint32_t>(dash) * 13u);
                addStreetBox("city/center-dash/" + std::to_string(dash),
                    {0, 0.032f, z0 + localZ}, {0.075f, 0.012f, 1.75f},
                    {0.62f * wear, 0.50f * wear, 0.22f * wear});
            }

            // Add render-only City props from the native street-plan formulas.
            // The traced C++ tree defines placement/counts but has no matching
            // lamp/service draw pass; treat these as Godot presentation work,
            // not recovered renderer parity.
            const float lampX = street.roadHalfWidth + street.sidewalkWidth - street.lampInset;
            for (const float side : {-1.0f, 1.0f}) {
                for (int lamp = 0; lamp < street.lampPairs; ++lamp) {
                    const float localZ = city_street::lampZ(street, lamp) + (side > 0.0f ? 1.1f : -1.1f);
                    const float poleHeight = 2.75f;
                    const std::string prefix = "city/lamp/" + std::to_string(static_cast<int>(side)) + "/" +
                        std::to_string(lamp);
                    const auto addLampBox = [&](const std::string& suffix, Vec3 center, Vec3 size,
                                                VisualColor color, bool unlit = false) {
                        addStructure((prefix + "/" + suffix).c_str(), RoomStructureKind::CityStreetFixture,
                            center, size, unlit ? render_contract::unlit(color)
                                : semanticMaterial(SemanticSurface::Default, color));
                    };
                    addLampBox("pole", {side * lampX, poleHeight * 0.5f, z0 + localZ},
                        {0.075f, poleHeight, 0.075f}, {0.13f, 0.15f, 0.16f});
                    addLampBox("arm", {side * (lampX - 0.18f), poleHeight - 0.04f, z0 + localZ},
                        {0.42f, 0.065f, 0.075f}, {0.13f, 0.15f, 0.16f});
                    addLampBox("bulb", {side * (lampX - 0.37f), poleHeight - 0.10f, z0 + localZ},
                        {0.16f, 0.055f, 0.11f}, {0.78f, 0.71f, 0.52f}, true);
                }
            }
            for (int cluster = 0; cluster < street.serviceClusters; ++cluster) {
                const float side = (cluster & 1) ? 1.0f : -1.0f;
                const float localZ = -10.5f + cluster * 19.0f +
                    (room_environment::unit(key + 81u + static_cast<unsigned int>(cluster)) - 0.5f) * 2.0f;
                const float outer = street.roadHalfWidth + street.sidewalkWidth + 0.46f;
                const std::string prefix = "city/service/" + std::to_string(cluster);
                const auto addServiceBox = [&](const std::string& suffix, Vec3 center, Vec3 size,
                                               VisualColor color, bool unlit = false) {
                    addStructure((prefix + "/" + suffix).c_str(), RoomStructureKind::CityServiceFixture,
                        center, size, unlit ? render_contract::unlit(color)
                            : semanticMaterial(SemanticSurface::Default, color));
                };
                addServiceBox("body", {side * outer, 0.44f, z0 + localZ},
                    {0.62f, 0.88f, 0.44f}, {0.18f, 0.21f, 0.21f});
                addServiceBox("panel", {side * (outer - side * 0.32f), 0.50f, z0 + localZ},
                    {0.018f, 0.52f, 0.25f}, {0.075f, 0.09f, 0.095f});
                for (int vent = 0; vent < 3; ++vent) {
                    addServiceBox("vent/" + std::to_string(vent),
                        {side * (outer - side * 0.335f), 0.61f - vent * 0.13f, z0 + localZ},
                        {0.012f, 0.025f, 0.19f}, {0.36f, 0.38f, 0.36f});
                }
                addServiceBox("status-light", {side * (outer - side * 0.342f), 0.76f, z0 + localZ},
                    {0.014f, 0.075f, 0.12f}, {0.42f, 0.68f, 0.64f}, true);
            }

            // The desktop renderer draws the native City obstacle boxes as
            // building masses. Keep the same authoritative boxes visible in
            // Godot; these records are presentation only and add no collision.
            const int authoredObstacleCount = std::min(state.debug.colliderCount, plan.obstacleCount);
            for (int obstacleIndex = 0; obstacleIndex < authoredObstacleCount; ++obstacleIndex) {
                const auto& obstacle = state.roomColliders[obstacleIndex];
                const VisualColor plaster = (obstacleIndex % 3) == 0
                    ? VisualColor{0.40f, 0.38f, 0.34f}
                    : ((obstacleIndex % 3) == 1
                        ? VisualColor{0.34f, 0.37f, 0.38f}
                        : VisualColor{0.42f, 0.36f, 0.33f});
                addStructure(("city/building-mass/" + std::to_string(obstacleIndex)).c_str(),
                    RoomStructureKind::CityBuildingMass,
                    {obstacle.center.x, obstacle.center.y, z0 + obstacle.center.z},
                    {obstacle.width, obstacle.height, obstacle.depth},
                    semanticMaterial(SemanticSurface::Default, plaster));
            }

            // Recover the desktop renderer's deterministic skyline silhouettes
            // from the same street grammar, including their worn roof caps.
            for (const int side : {-1, 1}) {
                for (int band = 0; band < street.skylineBands; ++band) {
                    const auto block = city_street::skylineBlock(street, key, tileIndex, side, band);
                    const VisualColor body = block.family == 0
                        ? VisualColor{0.245f, 0.26f, 0.27f}
                        : (block.family == 1
                            ? VisualColor{0.31f, 0.27f, 0.235f}
                            : VisualColor{0.20f, 0.225f, 0.245f});
                    const std::string prefix = "city/skyline/" + std::to_string(side) + "/" +
                        std::to_string(band);
                    addStructure((prefix + "/body").c_str(), RoomStructureKind::CitySkylineMass,
                        {block.x, block.height * 0.5f, z0 + block.z},
                        {block.width, block.height, block.depth},
                        semanticMaterial(SemanticSurface::Default, body));
                    addStructure((prefix + "/cap").c_str(), RoomStructureKind::CitySkylineMass,
                        {block.x, block.height + 0.10f, z0 + block.z},
                        {block.width * 1.04f, 0.20f, block.depth * 1.04f},
                        semanticMaterial(SemanticSurface::Default,
                            {body.r * 0.78f, body.g * 0.80f, body.b * 0.82f}));
                    const float faceX = block.x - static_cast<float>(side) * block.width * 0.505f;
                    const int rows = state.localSettings.graphicsPreset <= 0 ? 2 : 3;
                    const unsigned int facadeKey = key ^
                        static_cast<unsigned int>((side + 2) * 193 + (band + 1) * 977);
                    const float nightPresence = 0.34f +
                        (1.0f - render_contract::celestialDaylight(state.time)) * 0.66f;
                    for (int row = 0; row < rows; ++row) {
                        for (int bay = 0; bay < 2; ++bay) {
                            VisualColor windowColor{0.025f, 0.038f, 0.045f};
                            if (city_facade::windowOccupied(facadeKey, row, bay)) {
                                const float warmth = city_facade::occupiedWindowWarmth(facadeKey, row, bay);
                                windowColor = {
                                    (0.58f + warmth * 0.18f) * nightPresence,
                                    (0.34f + warmth * 0.12f) * nightPresence,
                                    (0.13f + warmth * 0.05f) * nightPresence};
                            }
                            const float windowY = 1.4f + static_cast<float>(row) *
                                (block.height - 2.0f) / static_cast<float>(std::max(1, rows - 1));
                            const float windowZ = z0 + block.z +
                                (bay ? 0.22f : -0.22f) * block.depth;
                            addStructure((prefix + "/window/" + std::to_string(row) + "/" +
                                std::to_string(bay)).c_str(), RoomStructureKind::CitySkylineWindow,
                                {faceX, windowY, windowZ}, {0.025f, 0.34f, block.depth * 0.26f},
                                render_contract::unlit(windowColor));
                        }
                    }
                }
            }

            // Reuse the mature renderer's authored modular bays on selected
            // eye-level facades. The underlying obstacle remains the sole
            // native collision/support mass; these module records are visual.
            const int authoredCadence = state.localSettings.graphicsPreset >= 2 ? 2 : 3;
            const float storyLimit = state.localSettings.graphicsPreset >= 2 ? 3.0f : 2.0f;
            for (int obstacleIndex = 0; obstacleIndex < state.debug.colliderCount; ++obstacleIndex) {
                const auto& obstacle = state.roomColliders[obstacleIndex];
                const unsigned int facadeKey = room_environment::roomKey(
                    state.roomSeed, state.roomIndex) ^
                    static_cast<unsigned int>((obstacleIndex + 1) * 0x9e3779b9u);
                if (state.localSettings.graphicsPreset <= 0 ||
                    obstacleIndex % authoredCadence != 0) continue;
                const auto facade = city_facade::grammar(
                    obstacle.depth, obstacle.height, facadeKey);
                const float inward = obstacle.center.x > 0.0f ? -1.0f : 1.0f;
                const float faceX = obstacle.center.x + inward * (obstacle.width * 0.5f + 0.014f);
                const float facadeZ = z0 + obstacle.center.z;
                const float orientation = inward > 0.0f ? -1.5707963267948966f : 1.5707963267948966f;
                const auto appendModule = [&](const std::string& suffix, const char* model,
                                              Vec3 origin, Vec3 scale) {
                    snapshot.cityFacadeModules.push_back({
                        "room/" + std::to_string(tileIndex) + "/city/facade/" +
                            std::to_string(obstacleIndex) + "/" + suffix,
                        model, origin, scale, orientation});
                };
                const int authoredStories = std::min(facade.stories, static_cast<int>(storyLimit));
                for (int story = 0; story < authoredStories; ++story) {
                    const float courseY = obstacle.bottomY +
                        static_cast<float>(story + 1) * facade.storyHeight - facade.corniceHeight * 0.5f;
                    for (int bay = 0; bay < facade.bays; ++bay) {
                        const float bayZ = city_facade::bayCenter(facade, bay, facadeZ);
                        const bool entrance = story == 0 && bay == facade.entranceBay;
                        const std::string bayPath = "story/" + std::to_string(story) +
                            "/bay/" + std::to_string(bay) + "/";
                        appendModule(bayPath + "wall", entrance ? "wall-door-bay" : "wall-window-bay",
                            {faceX, obstacle.bottomY + static_cast<float>(story) * facade.storyHeight, bayZ},
                            {facade.bayWidth, facade.storyHeight, 0.12f});
                        const char* opening = entrance
                            ? (facade.family == 2 ? "door-window-bay" : "door-bay")
                            : (facade.family == 2 ? "window-bay-small" : "window-bay");
                        appendModule(bayPath + "opening", opening,
                            {faceX + inward * 0.052f,
                             obstacle.bottomY + static_cast<float>(story) * facade.storyHeight, bayZ},
                            {facade.bayWidth, facade.storyHeight, 0.12f});
                        appendModule(bayPath + "cornice", "cornice",
                            {faceX + inward * 0.065f, courseY - facade.corniceHeight * 0.5f, bayZ},
                            {facade.bayWidth / 15.0f, facade.corniceHeight, 0.28f});
                    }
                }
                if (state.localSettings.graphicsPreset >= 2) {
                    for (const int edge : {-1, 1}) {
                        appendModule("corner/" + std::to_string(edge), "wall-corner",
                            {faceX, obstacle.bottomY,
                             facadeZ + static_cast<float>(edge) * (obstacle.depth * 0.5f - 0.14f)},
                            {0.28f, facade.storyHeight, 0.28f});
                    }
                }
                for (int bay = 0; bay < facade.bays; ++bay) {
                    const float bayZ = city_facade::bayCenter(facade, bay, facadeZ);
                    appendModule("crown/" + std::to_string(bay), "crown",
                        {faceX + inward * 0.035f, obstacle.bottomY + obstacle.height, bayZ},
                        {facade.bayWidth * 0.25f, facade.crownHeight, 0.18f});
                }
            }
        }
        // Match the renderer's non-physical Field perimeter dressing. The
        // generated records are presentation-only, not collision objects.
        if (field) {
            const unsigned int key = room_environment::roomKey(state.roomSeed, state.roomIndex) ^
                static_cast<unsigned int>((tileIndex + 5) * 0x9e3779b9u);
            const float driftA = (room_environment::unit(key + 1u) - 0.5f) * 2.4f;
            const float driftB = (room_environment::unit(key + 2u) - 0.5f) * 2.8f;
            struct Dressing { float x, z, height, yaw, widthCorrection; unsigned char family; const char* model; };
            const Dressing group[] = {
                {-14.15f, -15.0f + driftA, 7.6f,  0.10f, 0.78f, 0, "pine-tree"},
                {-13.65f,  -7.4f + driftB, 5.4f, -0.18f, 0.94f, 1, "common-tree"},
                { 14.20f,   4.8f - driftA, 6.7f,  0.36f, 0.82f, 2, "twisted-tree"},
                { 13.80f,  13.6f - driftB, 8.4f, -0.12f, 0.76f, 0, "pine-tree"},
                {-13.95f,  15.7f + driftA, 4.1f,  0.58f, 0.88f, 2, "dead-tree"},
                { -8.60f, -20.2f + driftB, 3.8f,  0.28f, 0.72f, 0, "pine-tree"},
                { -2.40f, -21.6f - driftA, 3.1f, -0.36f, 0.82f, 1, "common-tree"},
                {  4.80f, -20.7f + driftA, 4.4f,  0.12f, 0.70f, 0, "pine-tree"},
                { 10.40f, -21.9f - driftB, 3.5f, -0.46f, 0.74f, 2, "twisted-tree"},
            };
            for (int treeIndex = 0; treeIndex < 9; ++treeIndex) {
                const auto& tree = group[treeIndex];
                const unsigned int treeKey = key + static_cast<unsigned int>((tree.z + 20.0f) * 17.0f) +
                    static_cast<unsigned int>(treeIndex) * 97u;
                const float height = tree.height * (0.92f + room_environment::unit(treeKey) * 0.16f);
                const room_environment::EnvironmentPropSpec proxy{
                    room_environment::EnvironmentPrimitive::Tree, room_environment::EnvironmentRole::Detail,
                    {tree.x, 0, z0 + tree.z}, {height * tree.widthCorrection, height, height * tree.widthCorrection},
                    tree.yaw, tree.family};
                const auto variation = tree_geometry::authoredVariant(proxy, treeKey);
                snapshot.fieldBackdropTrees.push_back({
                    "room/" + std::to_string(tileIndex) + "/field-backdrop/tree/" + std::to_string(treeIndex),
                    tree.model, {tree.x, 0.006f, z0 + tree.z},
                    {height * tree.widthCorrection * variation.widthScale, height,
                     height * tree.widthCorrection * variation.depthScale},
                    variation.pitch, tree.yaw + variation.yawJitter, variation.roll});
            }
        }
        snapshot.objects = computed_scene::environmentObjects(
            plan, state.roomSeed, state.roomIndex, tileIndex, capacity);
        return snapshot;
    }
};
