#pragma once

#include "ComputedSceneObjects.hpp"
#include "Game.hpp"
#include "MaterialResponse.hpp"

#include <vector>

enum class RoomStructureKind : unsigned char {
    Floor,
    Ceiling,
    BoundaryWall,
    DoorFrame,
    TraversalSurface
};

struct RoomStructureRecord {
    std::string id;
    RoomStructureKind kind = RoomStructureKind::Floor;
    Vec3 center;
    Vec3 size;
    render_contract::MaterialDefinition material;
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
        snapshot.objects = computed_scene::environmentObjects(
            plan, state.roomSeed, state.roomIndex, tileIndex, capacity);
        return snapshot;
    }
};
