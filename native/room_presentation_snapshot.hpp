#pragma once

#include "ComputedSceneObjects.hpp"
#include "CityFacadeGrammar.hpp"
#include "CityStreetGrammar.hpp"
#include "Game.hpp"
#include "MaterialResponse.hpp"

#include <vector>

enum class RoomStructureKind : unsigned char {
    Floor,
    Ceiling,
    BoundaryWall,
    DoorFrame,
    TraversalSurface,
    CityStreet
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
    float grassAmount = 0.0f;
    std::vector<RoomStructureRecord> structures;
    std::vector<CityFacadeModuleRecord> cityFacadeModules;
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
                : (coastal ? VisualColor{0.24f, 0.43f, 0.50f} : VisualIdentity::RoomFloor));
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
        addStructure("floor", RoomStructureKind::Floor, {0, -0.04f, z0},
            {openNatural ? world::RoomWidth * 2.8f : world::RoomWidth, 0.08f, world::RoomDepth},
            floorMaterial);
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
        snapshot.objects = computed_scene::environmentObjects(
            plan, state.roomSeed, state.roomIndex, tileIndex, capacity);
        return snapshot;
    }
};
