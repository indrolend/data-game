#include "parent_game_node.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include "phone_presentation_snapshot.hpp"
#include "PhoneDisplayLayout.hpp"
#include "room_presentation_snapshot.hpp"
#include "lighting_presentation_snapshot.hpp"
#include "combat_presentation_snapshot.hpp"
#include "MaterialResponse.hpp"
#include "FieldGrassTexture.hpp"
#include "CitySurfaceTexture.hpp"
using namespace godot;
void ParentGameNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("submit_input","x","z","look_x","look_y","vacuum","sprint","jump","melee","shoot","camera"),&ParentGameNode::submit_input);
    ClassDB::bind_method(D_METHOD("tick"),&ParentGameNode::tick);
    ClassDB::bind_method(D_METHOD("set_comm_signal","signal"),&ParentGameNode::set_comm_signal);
    ClassDB::bind_method(D_METHOD("set_paused","paused"),&ParentGameNode::set_paused);
    ClassDB::bind_method(D_METHOD("menu_move_selection","direction"),&ParentGameNode::menu_move_selection);
    ClassDB::bind_method(D_METHOD("menu_adjust","direction"),&ParentGameNode::menu_adjust);
    ClassDB::bind_method(D_METHOD("menu_confirm"),&ParentGameNode::menu_confirm);
    ClassDB::bind_method(D_METHOD("menu_back"),&ParentGameNode::menu_back);
    ClassDB::bind_method(D_METHOD("upgrade_menu_active"),&ParentGameNode::upgrade_menu_active);
    ClassDB::bind_method(D_METHOD("move_upgrade_selection","horizontal","vertical"),&ParentGameNode::move_upgrade_selection);
    ClassDB::bind_method(D_METHOD("set_upgrade_selection","selection"),&ParentGameNode::set_upgrade_selection);
    ClassDB::bind_method(D_METHOD("activate_upgrade_selection"),&ParentGameNode::activate_upgrade_selection);
    ClassDB::bind_method(D_METHOD("start_rally_fixture"),&ParentGameNode::start_rally_fixture);
    ClassDB::bind_method(D_METHOD("start_target_fixture"),&ParentGameNode::start_target_fixture);
    ClassDB::bind_method(D_METHOD("start_target_interaction_fixture"),&ParentGameNode::start_target_interaction_fixture);
    ClassDB::bind_method(D_METHOD("start_city_room_fixture"),&ParentGameNode::start_city_room_fixture);
    ClassDB::bind_method(D_METHOD("start_sterile_room_fixture"),&ParentGameNode::start_sterile_room_fixture);
    ClassDB::bind_method(D_METHOD("start_ruin_room_fixture"),&ParentGameNode::start_ruin_room_fixture);
    ClassDB::bind_method(D_METHOD("start_upgrade_fixture"),&ParentGameNode::start_upgrade_fixture);
    ClassDB::bind_method(D_METHOD("start_room_transition_fixture"),&ParentGameNode::start_room_transition_fixture);
    ClassDB::bind_method(D_METHOD("refill_target_interaction_battery"),&ParentGameNode::refill_target_interaction_battery);
    ClassDB::bind_method(D_METHOD("execute_developer_command","command"),&ParentGameNode::execute_developer_command);
    ClassDB::bind_method(D_METHOD("set_developer_console_open","open"),&ParentGameNode::set_developer_console_open);
    ClassDB::bind_method(D_METHOD("developer_console_open"),&ParentGameNode::developer_console_open);
    ClassDB::bind_method(D_METHOD("developer_show_colliders"),&ParentGameNode::developer_show_colliders);
    ClassDB::bind_method(D_METHOD("player_position"),&ParentGameNode::player_position);
    ClassDB::bind_method(D_METHOD("phone_presentation_snapshot"),&ParentGameNode::phone_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("phone_menu_layout_snapshot"),&ParentGameNode::phone_menu_layout_snapshot);
    ClassDB::bind_method(D_METHOD("hud_presentation_snapshot"),&ParentGameNode::hud_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("upgrade_presentation_snapshot"),&ParentGameNode::upgrade_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("room_presentation_snapshot","tile_index"),&ParentGameNode::room_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("field_grass_texture_pixels"),&ParentGameNode::field_grass_texture_pixels);
    ClassDB::bind_method(D_METHOD("city_asphalt_texture_pixels"),&ParentGameNode::city_asphalt_texture_pixels);
    ClassDB::bind_method(D_METHOD("grass_blade_presentation_snapshot","tile_index"),&ParentGameNode::grass_blade_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("lighting_presentation_snapshot"),&ParentGameNode::lighting_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("player_action_snapshot"),&ParentGameNode::player_action_snapshot);
    ClassDB::bind_method(D_METHOD("target_presentation_snapshot","index"),&ParentGameNode::target_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("target_soul_presentation_snapshot","index"),&ParentGameNode::target_soul_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("target_soul_lattice","index"),&ParentGameNode::target_soul_lattice);
    ClassDB::bind_method(D_METHOD("target_model_vertices","index"),&ParentGameNode::target_model_vertices);
    ClassDB::bind_method(D_METHOD("target_model_color","index"),&ParentGameNode::target_model_color);
    ClassDB::bind_method(D_METHOD("camera_position"),&ParentGameNode::camera_position);
    ClassDB::bind_method(D_METHOD("camera_target"),&ParentGameNode::camera_target);
    ClassDB::bind_method(D_METHOD("camera_fov_degrees"),&ParentGameNode::camera_fov_degrees);
    ClassDB::bind_method(D_METHOD("target_position","index"),&ParentGameNode::target_position);
    ClassDB::bind_method(D_METHOD("target_alive","index"),&ParentGameNode::target_alive);
    ClassDB::bind_method(D_METHOD("target_slurpable","index"),&ParentGameNode::target_slurpable);
    ClassDB::bind_method(D_METHOD("target_armor","index"),&ParentGameNode::target_armor);
    ClassDB::bind_method(D_METHOD("target_ingest_progress","index"),&ParentGameNode::target_ingest_progress);
    ClassDB::bind_method(D_METHOD("target_count"),&ParentGameNode::target_count);
    ClassDB::bind_method(D_METHOD("souls"),&ParentGameNode::souls);
    ClassDB::bind_method(D_METHOD("battery"),&ParentGameNode::battery);
    ClassDB::bind_method(D_METHOD("frame"),&ParentGameNode::frame);
    ClassDB::bind_method(D_METHOD("grounded"),&ParentGameNode::grounded);
    ClassDB::bind_method(D_METHOD("first_person"),&ParentGameNode::first_person);
    ClassDB::bind_method(D_METHOD("vacuum_active"),&ParentGameNode::vacuum_active);
    ClassDB::bind_method(D_METHOD("vacuum_power"),&ParentGameNode::vacuum_power);
    ClassDB::bind_method(D_METHOD("melee_pose"),&ParentGameNode::melee_pose);
    ClassDB::bind_method(D_METHOD("live_bullets"),&ParentGameNode::live_bullets);
    ClassDB::bind_method(D_METHOD("comm_signal"),&ParentGameNode::comm_signal);
    ClassDB::bind_method(D_METHOD("comm_signal_timer"),&ParentGameNode::comm_signal_timer);
    ClassDB::bind_method(D_METHOD("paused"),&ParentGameNode::paused);
    ClassDB::bind_method(D_METHOD("simulation_time"),&ParentGameNode::simulation_time);
}
void ParentGameNode::submit_input(float x,float z,float lx,float ly,bool vacuum,bool sprint,bool jump,bool melee,bool shoot,bool camera){
    bridge.input(x,z,lx,ly,vacuum,sprint,jump,melee,shoot,camera);
}
void ParentGameNode::tick(){bridge.tick();}
void ParentGameNode::set_comm_signal(int signal){bridge.setCommSignal(signal);}
void ParentGameNode::set_paused(bool paused){bridge.setPaused(paused);}
void ParentGameNode::menu_move_selection(int direction){bridge.menuMoveSelection(direction);}
bool ParentGameNode::menu_adjust(int direction){return bridge.menuAdjust(direction);}
bool ParentGameNode::menu_confirm(){return bridge.menuConfirm();}
bool ParentGameNode::menu_back(){return bridge.menuBack();}
bool ParentGameNode::upgrade_menu_active() const{return bridge.upgradeMenuActive();}
void ParentGameNode::move_upgrade_selection(int horizontal,int vertical){bridge.moveUpgradeSelection(horizontal,vertical);}
void ParentGameNode::set_upgrade_selection(int selection){bridge.setUpgradeSelection(selection);}
bool ParentGameNode::activate_upgrade_selection(){return bridge.activateUpgradeSelection();}
void ParentGameNode::start_rally_fixture(){bridge.startRallyFixture();}
void ParentGameNode::start_target_fixture(){bridge.startTargetFixture();}
void ParentGameNode::start_target_interaction_fixture(){bridge.startTargetInteractionFixture();}
void ParentGameNode::start_city_room_fixture(){bridge.startCityRoomFixture();}
void ParentGameNode::start_sterile_room_fixture(){bridge.startSterileRoomFixture();}
void ParentGameNode::start_ruin_room_fixture(){bridge.startRuinRoomFixture();}
void ParentGameNode::start_upgrade_fixture(){bridge.startUpgradeFixture();}
void ParentGameNode::start_room_transition_fixture(){bridge.startRoomTransitionFixture();}
void ParentGameNode::refill_target_interaction_battery(){bridge.refillTargetInteractionBattery();}
String ParentGameNode::execute_developer_command(const String& command) {
    const CharString utf8 = command.utf8();
    const std::string result = bridge.executeDeveloperCommand(
        std::string(utf8.get_data())
    );
    return String::utf8(result.c_str());
}

void ParentGameNode::set_developer_console_open(bool open) {
    bridge.setDeveloperConsoleOpen(open);
}

bool ParentGameNode::developer_console_open() const {
    return bridge.developerConsoleOpen();
}

bool ParentGameNode::developer_show_colliders() const {
    return bridge.developerShowColliders();
}
static Vector3 v(const Vec3& p){return Vector3(p.x,p.y,p.z);}
static Dictionary resolved_light_dictionary(const room_lighting::ResolvedLight& light){
    Dictionary result;
    result["enabled"] = light.enabled;
    result["directional"] = light.directional;
    if (light.directional) result["direction"] = v(light.position);
    else result["position"] = v(light.position);
    result["color"] = Color(light.diffuse.r, light.diffuse.g, light.diffuse.b);
    result["constant_attenuation"] = light.constantAttenuation;
    result["linear_attenuation"] = light.linearAttenuation;
    result["quadratic_attenuation"] = light.quadraticAttenuation;
    return result;
}
static Dictionary material_dictionary(const render_contract::MaterialDefinition& material){
    Dictionary result;
    result["base_color"] = Color(material.baseColor.r,material.baseColor.g,material.baseColor.b,material.opacity);
    result["shading"] = static_cast<int>(material.shading);
    result["opacity"] = material.opacity;
    result["fog"] = material.fog;
    result["texture"] = static_cast<int>(material.texture);
    result["texture_world_scale"] = material.textureWorldScale;
    result["specular"] = Color(material.specular.r,material.specular.g,material.specular.b,1.0f);
    result["shininess"] = material.shininess;
    result["emission"] = Color(material.emission.r,material.emission.g,material.emission.b,1.0f);
    return result;
}
Dictionary ParentGameNode::hud_presentation_snapshot() const {
    const auto& state = bridge.state();
    const auto& hud = state.hud;

    Dictionary result;
    result["battery_fill"] = hud.batteryFill;
    result["stored_souls"] = hud.storedSouls;
    result["filled_goals"] = hud.filledGoals;
    result["required_goals"] = hud.requiredGoals;
    result["vacuum_field"] = hud.vacuumField;
    result["lock_strength"] = hud.lockStrength;
    result["crosshair_rotation_degrees"] = hud.crosshairRotationDegrees;
    result["crosshair_spread_pixels"] = hud.crosshairSpreadPixels;
    result["crosshair_opacity"] = hud.crosshairOpacity;
    result["shoot_join_timer"] = hud.shootJoinTimer;
    result["supplemental_fill"] = hud.supplementalFill;
    result["flower_stacks"] = hud.flowerStacks;
    result["has_target"] = hud.hasTarget;
    result["low_battery"] = hud.lowBattery;
    result["game_over"] = hud.gameOver;
    result["energy_ticker"] = String::utf8(hud.energyTicker.data());
    result["energy_ticker_until"] = hud.energyTickerUntil;
    result["energy_ticker_type"] = hud.energyTickerType;
    result["headshot_pulse"] = hud.headshotPulse;
    result["perfect_pulse"] = hud.perfectPulse;
    result["headshot_kill_charge"] = hud.headshotKillCharge;
    result["critical_hit_pulse"] = hud.criticalHitPulse;
    result["crit_marker_opacity"] = hud.critMarkerOpacity;
    result["build_label"] = String::utf8(hud.buildLabel.data());
    result["menu_selection"] = hud.menuSelection;

    result["room_index"] = state.roomIndex;
    result["room_clear"] = state.roomClear;
    result["deposited_souls"] = state.depositedSouls;
    result["required_souls"] = state.requiredSouls;
    result["started"] = state.started;
    result["dead"] = state.dead;
    result["ui_paused"] = state.uiPaused;
    result["upgrade"] = upgrade_presentation_snapshot();

    return result;
}
Dictionary ParentGameNode::upgrade_presentation_snapshot() const {
    const auto& state = bridge.state();
    Dictionary result;
    result["active"] = state.upgradeMenu.active;
    result["presentation_time"] = state.upgradeMenu.presentationTime;
    result["room_index"] = state.roomIndex;
    result["selection"] = state.hud.menuSelection;
    result["tokens"] = static_cast<int64_t>(state.progression.permanent.tokens);
    result["remote_choice"] = state.multiplayer.enabled && !state.multiplayer.authoritativeHost;
    Array temporary_levels;
    Array permanent_levels;
    Array temporary_available;
    Array permanent_available;
    for (int track = 0; track < static_cast<int>(UpgradeTrack::Count); ++track) {
        const int temporary = state.progression.run.temporaryLevels[track];
        const int permanent = state.progression.permanent.levels[track];
        temporary_levels.push_back(temporary);
        permanent_levels.push_back(permanent);
        temporary_available.push_back(temporary < 12);
        permanent_available.push_back(state.progression.permanent.tokens > 0 && permanent < 5);
    }
    result["temporary_levels"] = temporary_levels;
    result["permanent_levels"] = permanent_levels;
    result["temporary_available"] = temporary_available;
    result["permanent_available"] = permanent_available;
    return result;
}
Dictionary ParentGameNode::phone_presentation_snapshot() const {
    const PhonePresentationSnapshot snapshot = PhonePresentationSnapshot::from(bridge.state());
    Dictionary result;
    result["position"] = v(snapshot.position);
    result["orientation"] = Quaternion(snapshot.orientation.x,snapshot.orientation.y,snapshot.orientation.z,snapshot.orientation.w);
    result["body_size"] = v(snapshot.bodySize);
    result["body_scale"] = v(snapshot.bodyScale);
    result["screen_center_offset"] = v(snapshot.screenCenterOffset);
    result["screen_size"] = v(snapshot.screenSize);
    result["screen_mode"] = snapshot.screenMode;
    result["body_visible"] = snapshot.bodyVisible;
    result["screen_visible"] = snapshot.screenVisible;
    result["first_person"] = snapshot.firstPerson;
    result["screen_brightness"] = snapshot.screenBrightness;
    result["content_opacity"] = snapshot.contentOpacity;
    result["screen_tint"] = Color(snapshot.screenTint.x,snapshot.screenTint.y,snapshot.screenTint.z,1.0f);
    result["screen_glow"] = snapshot.screenGlow;
    result["emission_strength"] = snapshot.emissionStrength;
    result["interactive"] = snapshot.interactive;
    result["battery"] = snapshot.battery;
    result["souls"] = snapshot.souls;
    result["low_battery"] = snapshot.lowBattery;
    result["vacuum_active"] = snapshot.vacuumActive;
    result["vacuum_power"] = snapshot.vacuumPower;
    result["capture_pulse"] = snapshot.capturePulse;
    result["menu_layout"] = phone_menu_layout_snapshot();
    return result;
}
static Dictionary phone_rect_dictionary(const PhoneDisplayRect& rect) {
    Dictionary result;
    result["x"] = rect.x; result["y"] = rect.y;
    result["w"] = rect.w; result["h"] = rect.h;
    return result;
}
Dictionary ParentGameNode::phone_menu_layout_snapshot() const {
    const auto& state = bridge.state();
    const auto layout = makePhoneDisplayMenuLayout(state);
    Dictionary result;
    const int mode = static_cast<int>(state.phoneDisplay.mode);
    result["active"] = (mode >= static_cast<int>(PhoneDisplayMode::MainMenu) &&
        mode <= static_cast<int>(PhoneDisplayMode::Graphics)) ||
        mode == static_cast<int>(PhoneDisplayMode::Pause);
    result["logical_width"] = layout.logicalW;
    result["logical_height"] = layout.logicalH;
    result["title"] = String::utf8(layout.title.c_str());
    result["navigation_hint"] = String::utf8(layout.navigationHint.c_str());
    result["title_center_y"] = layout.titleCenterY;
    result["title_px"] = layout.titlePx;
    result["title_max_width"] = layout.titleMaxWidth;
    result["selection"] = state.hud.menuSelection;
    result["table_page"] = layout.tablePage;
    result["join_code"] = layout.joinCode;
    result["row_count"] = layout.rowCount;
    result["selectable_count"] = layout.selectableCount;
    result["scroll_offset"] = layout.scrollOffset;
    result["max_scroll"] = layout.maxScroll;
    result["safe"] = phone_rect_dictionary(layout.safe);
    result["content"] = phone_rect_dictionary(layout.content);
    Array rows;
    for (int i = 0; i < layout.rowCount; ++i) {
        const auto& row = layout.rows[i];
        Dictionary item;
        item["kind"] = static_cast<int>(row.kind);
        item["action"] = static_cast<int>(row.action);
        item["selectable_index"] = row.selectableIndex;
        item["binding_action"] = row.bindingAction;
        item["horizontal"] = static_cast<int>(row.horizontal);
        item["label"] = String::utf8(row.label.c_str());
        item["value"] = String::utf8(row.value.c_str());
        item["visual"] = phone_rect_dictionary(row.visual);
        item["baseline_y"] = row.baselineY;
        item["label_x"] = row.labelX;
        item["value_right_x"] = row.valueRightX;
        item["font_px"] = row.fontPx;
        item["selectable"] = row.selectable;
        item["visible"] = row.visible;
        item["peek"] = row.peek;
        item["fixed_footer"] = row.fixedFooter;
        item["selected"] = row.selectable && row.selectableIndex == state.hud.menuSelection;
        item["emphasis"] = static_cast<int>(phoneMenuEmphasis(row.action));
        item["amount"] = phoneMenuVisualAmount(row.action, state.localSettings);
        rows.push_back(item);
    }
    result["rows"] = rows;
    return result;
}
Dictionary ParentGameNode::room_presentation_snapshot(int tile_index) const {
    if(tile_index<0)tile_index=bridge.state().topology.currentTileIndex;
    const RoomPresentationSnapshot snapshot = RoomPresentationSnapshot::from(bridge.state(), tile_index);
    Dictionary result;
    result["room_seed"] = snapshot.roomSeed;
    result["room_index"] = snapshot.roomIndex;
    result["tile_index"] = snapshot.tileIndex;
    result["current_tile_index"] = snapshot.currentTileIndex;
    result["setting"] = snapshot.setting;
    result["form"] = snapshot.form;
    result["scale"] = snapshot.scale;
    result["condition"] = snapshot.condition;
    result["traversal_intent"] = snapshot.traversalIntent;
    result["room_clear"] = snapshot.roomClear;
    result["advancing"] = snapshot.advancing;
    result["grass"] = snapshot.grass;
    result["sidewalks"] = snapshot.sidewalks;
    result["grass_amount"] = snapshot.grassAmount;
    Array structures;
    for (const auto& structure : snapshot.structures) {
        Dictionary item;
        item["id"] = structure.id.c_str();
        item["kind"] = static_cast<int>(structure.kind);
        item["center"] = v(structure.center);
        item["size"] = v(structure.size);
        item["material"] = material_dictionary(structure.material);
        structures.push_back(item);
    }
    result["structures"] = structures;
    Array city_facade_modules;
    for (const auto& module : snapshot.cityFacadeModules) {
        Dictionary item;
        item["id"] = module.id.c_str();
        item["model"] = module.model.c_str();
        item["origin"] = v(module.origin);
        item["scale"] = v(module.scale);
        item["yaw"] = module.yaw;
        city_facade_modules.push_back(item);
    }
    result["city_facade_modules"] = city_facade_modules;
    Array objects;
    const auto roomPlan = room_environment::roomPlan(snapshot.roomSeed, snapshot.roomIndex);
    for (const auto& object : snapshot.objects) {
        Dictionary item;
        item["id"] = object.id.c_str();
        item["tile_index"] = object.tileIndex;
        item["prop_index"] = object.propIndex;
        item["part_index"] = object.partIndex;
        item["primitive"] = static_cast<int>(object.primitive);
        item["role"] = static_cast<int>(object.role);
        item["geometry"] = static_cast<int>(object.geometry);
        item["part"] = static_cast<int>(object.part);
        item["center"] = v(object.center);
        item["size"] = v(object.size);
        item["yaw"] = object.yaw;
        item["surface"] = static_cast<int>(object.surface);
        item["physical"] = object.physical;
        if (object.primitive == room_environment::EnvironmentPrimitive::House) {
            const auto prop = room_environment::environmentProp(
                roomPlan, snapshot.roomSeed, snapshot.roomIndex, object.propIndex);
            const float variation = 0.92f + 0.045f * static_cast<float>(
                (object.partIndex + prop.variant) % 3);
            VisualColor color{};
            if (object.surface == 0) color = {0.40f * variation, 0.47f * variation, 0.50f * variation};
            else if (object.surface == 3) color = {0.035f, 0.045f, 0.048f};
            else if (object.surface == 1) color = {0.30f * variation, 0.37f * variation, 0.41f * variation};
            else color = {0.26f * variation, 0.32f * variation, 0.36f * variation};
            item["visual_color"] = Color(color.r, color.g, color.b);
            item["visual_texture"] = object.surface == 0
                ? (roomPlan.setting == room_environment::RoomSetting::City ? "metal" : "stone")
                : "none";
            item["visual_texture_world_scale"] = 1.35f;
        }
        if (object.primitive == room_environment::EnvironmentPrimitive::MarkerPillar) {
            const VisualColor color = object.surface == 0
                ? VisualColor{0.70f, 0.76f, 0.78f}
                : VisualColor{0.72f, 0.90f, 0.94f};
            item["visual_color"] = Color(color.r, color.g, color.b);
            item["visual_texture"] = object.surface == 0 ? "metal" : "none";
            item["visual_texture_world_scale"] = 0.92f;
        }
        if (object.primitive == room_environment::EnvironmentPrimitive::Ruin) {
            const auto prop = room_environment::environmentProp(
                roomPlan, snapshot.roomSeed, snapshot.roomIndex, object.propIndex);
            const float phase = std::fmod((prop.variant + object.partIndex) * 0.137f, 1.0f);
            const float weather = 0.88f + 0.055f * static_cast<float>(
                (object.partIndex + prop.variant) % 3);
            const VisualColor color = object.surface == 0
                ? VisualColor{0.84f * weather, 0.82f * weather, 0.72f * weather}
                : VisualColor{0.70f * weather, 0.70f * weather, 0.66f * weather};
            item["visual_color"] = Color(color.r, color.g, color.b);
            item["visual_texture"] = "stone";
            item["visual_texture_world_scale"] = 1.25f;
            item["visual_texture_phase"] = phase;
            item["visual_has_scar"] = object.size.y > 0.42f;
            item["visual_scar_center"] = v(object.center + Vec3{0, -object.size.y * 0.39f,
                -object.size.z * 0.505f});
            item["visual_scar_size"] = v({object.size.x * 0.74f, 0.045f, 0.024f});
            item["visual_scar_color"] = Color(0.20f, 0.27f, 0.20f);
        }
        if (object.geometry == computed_scene::GeometryKind::FacetedMesh && object.propIndex >= 0) {
            const auto prop = room_environment::environmentProp(
                roomPlan, snapshot.roomSeed, snapshot.roomIndex, object.propIndex);
            const auto mesh = faceted_rock::makeVisualMesh(prop, snapshot.roomSeed,
                snapshot.roomIndex, object.propIndex,
                static_cast<float>(object.tileIndex) * world::RoomDepth);
            PackedVector3Array vertices;
            PackedVector3Array normals;
            vertices.resize(mesh.vertexCount);
            normals.resize(mesh.vertexCount);
            for (int vertex = 0; vertex < mesh.vertexCount; ++vertex) {
                const int offset = vertex * 3;
                vertices.set(vertex, Vector3(mesh.positions[offset] - object.center.x,
                    mesh.positions[offset + 1] - object.center.y,
                    mesh.positions[offset + 2] - object.center.z));
                normals.set(vertex, Vector3(mesh.normals[offset], mesh.normals[offset + 1],
                    mesh.normals[offset + 2]));
            }
            const auto substrate = roomSubstrateColor(roomPlan.setting);
            item["visual_mesh_vertices"] = vertices;
            item["visual_mesh_normals"] = normals;
            item["visual_color"] = Color(substrate.r * 0.82f,
                substrate.g * 0.82f, substrate.b * 0.82f);
            item["visual_texture_world_scale"] = 1.35f;
        }
        objects.push_back(item);
    }
    result["objects"] = objects;
    return result;
}
PackedByteArray ParentGameNode::field_grass_texture_pixels() const {
    static const auto pixels = field_grass_texture::pixels();
    PackedByteArray result;
    result.resize(static_cast<int64_t>(pixels.size()));
    for (int64_t index = 0; index < static_cast<int64_t>(pixels.size()); ++index) {
        result.set(index, pixels[static_cast<size_t>(index)]);
    }
    return result;
}
PackedByteArray ParentGameNode::city_asphalt_texture_pixels() const {
    static const auto pixels = city_surface_texture::pixels();
    PackedByteArray result;
    result.resize(static_cast<int64_t>(pixels.size()));
    for (int64_t index = 0; index < static_cast<int64_t>(pixels.size()); ++index) {
        result.set(index, pixels[static_cast<size_t>(index)]);
    }
    return result;
}
Array ParentGameNode::grass_blade_presentation_snapshot(int tile_index) const {
    const auto& state = bridge.state();
    if (tile_index < 0) tile_index = state.topology.currentTileIndex;
    const auto plan = room_environment::roomPlan(state.roomSeed, state.roomIndex);
    Array result;
    if (!plan.grass) return result;
    result.resize(room_environment::GrassBladeCountLow);
    const float tileOffset = static_cast<float>(tile_index) * world::RoomDepth;
    for (int index = 0; index < room_environment::GrassBladeCountLow; ++index) {
        const auto blade = room_environment::grassBlade(
            state.roomSeed, state.roomIndex, tile_index, index);
        Dictionary item;
        item["root"] = v({blade.root.x, blade.root.y, blade.root.z + tileOffset});
        item["height"] = blade.height;
        item["width"] = blade.width;
        item["phase"] = blade.phase;
        result[index] = item;
    }
    return result;
}
Dictionary ParentGameNode::lighting_presentation_snapshot() const {
    const auto snapshot = LightingPresentationSnapshot::from(bridge.state());
    const auto& lighting = snapshot.lighting;
    Dictionary result;
    result["room_index"] = snapshot.roomIndex;
    result["setting"] = snapshot.setting;
    result["time"] = snapshot.time;
    result["background"] = Color(snapshot.background.r, snapshot.background.g, snapshot.background.b);
    result["ambient"] = Color(lighting.ambient.r, lighting.ambient.g, lighting.ambient.b);
    result["fog_color"] = Color(lighting.fog.color.r, lighting.fog.color.g, lighting.fog.color.b);
    result["fog_density"] = lighting.fog.density;
    result["sun"] = resolved_light_dictionary(lighting.sun);
    result["fill"] = resolved_light_dictionary(lighting.fill);
    result["phone"] = resolved_light_dictionary(lighting.phone);
    result["shot"] = resolved_light_dictionary(lighting.shot);
    result["exit"] = resolved_light_dictionary(lighting.exit);
    Array local_lights;
    for (const auto& light : lighting.localLights) {
        if (light.enabled) local_lights.push_back(resolved_light_dictionary(light));
    }
    result["local_lights"] = local_lights;
    return result;
}
Dictionary ParentGameNode::player_action_snapshot() const {
    const PlayerActionPresentationSnapshot snapshot = PlayerActionPresentationSnapshot::from(bridge.state());
    Dictionary result;
    result["sequence"] = static_cast<int>(snapshot.sequence);
    result["combo_index"] = snapshot.comboIndex;
    result["variant"] = snapshot.variant;
    result["visual_timer"] = snapshot.visualTimer;
    result["visual_duration"] = snapshot.visualDuration;
    result["phase"] = snapshot.normalizedPhase;
    result["range"] = snapshot.range;
    result["hit_radius"] = snapshot.hitRadius;
    result["damage"] = snapshot.damage;
    result["origin"] = v(snapshot.origin);
    result["direction"] = v(snapshot.direction);
    result["impact"] = v(snapshot.impact);
    result["visual_hit"] = snapshot.visualHit;
    result["hit_mask"] = static_cast<int64_t>(snapshot.hitMask);
    result["locomotion_lunge"] = snapshot.locomotionLunge;
    result["air_lunge_pending"] = snapshot.airLungePending;
    result["air_lunge_landing_pending"] = snapshot.airLungeLandingPending;
    result["air_lunge_timer"] = snapshot.airLungeTimer;
    result["air_lunge_rotation"] = snapshot.airLungeRotation;
    result["landing_recovery"] = snapshot.landingRecovery;
    result["landing_recovery_duration"] = snapshot.landingRecoveryDuration;
    result["landing_pose_pitch"] = snapshot.landingPosePitch;
    result["wall_grip_timer"] = snapshot.wallGripTimer;
    result["player_position"] = v(snapshot.playerPosition);
    result["player_velocity"] = v(snapshot.playerVelocity);
    return result;
}
Dictionary ParentGameNode::target_presentation_snapshot(int index) const {
    const TargetPresentationSnapshot snapshot = TargetPresentationSnapshot::from(bridge.state(), index);
    Dictionary result;
    result["index"] = snapshot.index;
    result["position"] = v(snapshot.position);
    result["velocity"] = v(snapshot.velocity);
    result["alive"] = snapshot.alive;
    result["slurpable"] = snapshot.slurpable;
    result["brute"] = snapshot.brute;
    result["scale"] = snapshot.scale;
    result["armor"] = snapshot.armor;
    result["health"] = snapshot.health;
    result["visual_yaw"] = snapshot.visualYaw;
    result["locomotion_amount"] = snapshot.locomotionAmount;
    result["locomotion_phase"] = snapshot.locomotionPhase;
    result["animation_time"] = snapshot.animationTime;
    result["hit_amount"] = snapshot.hitAmount;
    result["hit_direction_local"] = snapshot.hitDirectionLocal;
    result["attack_active"] = snapshot.attackActive;
    result["attack_timer"] = snapshot.attackTimer;
    result["attack_phase"] = snapshot.attackPhase;
    result["attack_commit_phase"] = snapshot.attackCommitPhase;
    result["attack_end_phase"] = snapshot.attackEndPhase;
    result["attack_variant"] = snapshot.attackVariant;
    result["attack_hit"] = snapshot.attackHit;
    result["attack_direction"] = v(snapshot.attackDirection);
    result["attack_target_player_id"] = snapshot.attackTargetPlayerId;
    result["grabbed_player_id"] = snapshot.grabbedPlayerId;
    result["grab_cooldown"] = snapshot.grabCooldown;
    result["body_pitch"] = snapshot.bodyPitch;
    result["body_roll"] = snapshot.bodyRoll;
    result["left_foot_contact"] = snapshot.leftFootContact;
    result["right_foot_contact"] = snapshot.rightFootContact;
    result["left_foot_target"] = v(snapshot.leftFootTarget);
    result["right_foot_target"] = v(snapshot.rightFootTarget);
    result["tether_visible"] = snapshot.tetherVisible;
    result["tether_anchor"] = v(snapshot.tetherAnchor);
    result["tether_destination"] = v(snapshot.tetherDestination);
    result["tether_width"] = snapshot.tetherWidth;
    result["body_fallen"] = snapshot.bodyFallen;
    result["body_authoritative"] = snapshot.bodyAuthoritative;
    result["head_center"] = v(snapshot.headCenter);
    result["head_radius"] = snapshot.headRadius;
    result["torso_center"] = v(snapshot.torsoCenter);
    result["torso_size"] = v(snapshot.torsoSize);
    Dictionary pose;
    pose["yaw"] = snapshot.pose.yaw;
    pose["scale"] = snapshot.pose.scale;
    pose["morph_shrink"] = snapshot.pose.morphShrink;
    pose["root_bob"] = snapshot.pose.rootBob;
    pose["torso_pitch"] = snapshot.pose.torsoPitch;
    pose["torso_roll"] = snapshot.pose.torsoRoll;
    pose["head_pitch"] = snapshot.pose.headPitch;
    pose["left_arm_swing"] = snapshot.pose.leftArmSwing;
    pose["right_arm_swing"] = snapshot.pose.rightArmSwing;
    pose["left_leg_swing"] = snapshot.pose.leftLegSwing;
    pose["right_leg_swing"] = snapshot.pose.rightLegSwing;
    pose["hit_lean"] = snapshot.pose.hitLean;
    pose["soul_morph"] = snapshot.pose.soulMorph;
    pose["vacuum_lean"] = snapshot.pose.vacuumLean;
    pose["collapse"] = snapshot.pose.collapse;
    pose["expressive_scale"] = v(snapshot.pose.expressiveScale);
    result["pose"] = pose;
    Dictionary reaction;
    reaction["locomotion_phase"] = snapshot.reaction.locomotionPhase;
    reaction["locomotion_amount"] = snapshot.reaction.locomotionAmount;
    reaction["hit_amount"] = snapshot.reaction.hitAmount;
    reaction["hit_direction_local"] = snapshot.reaction.hitDirectionLocal;
    reaction["vacuum_pull_amount"] = snapshot.reaction.vacuumPullAmount;
    reaction["capture_collapse_amount"] = snapshot.reaction.captureCollapseAmount;
    reaction["visibility"] = snapshot.reaction.visibility;
    reaction["soul_cube_amount"] = snapshot.reaction.soulCubeAmount;
    reaction["attack_timer"] = snapshot.reaction.attackTimer;
    reaction["attack_variant"] = snapshot.reaction.attackVariant;
    reaction["awareness"] = snapshot.reaction.awareness;
    reaction["uncertainty"] = snapshot.reaction.uncertainty;
    reaction["commitment"] = snapshot.reaction.commitment;
    reaction["disruption"] = snapshot.reaction.disruption;
    reaction["search_amount"] = snapshot.reaction.searchAmount;
    reaction["individuality"] = snapshot.reaction.individuality;
    result["reaction"] = reaction;
    return result;
}
Dictionary ParentGameNode::target_soul_presentation_snapshot(int index) const {
    Dictionary result;
    if (index < 0 || index >= TARGET_COUNT) return result;
    const auto& target = bridge.state().targets[index];
    const auto& soul = target.soulVisual;
    result["alive"] = target.alive;
    result["slurpable"] = target.slurpable;
    result["visible"] = soul.visible;
    result["position"] = v(target.pos);
    result["vertical_offset"] = soul.verticalOffset;
    result["rotation_y"] = soul.rotationY;
    result["morph_scale"] = soul.morphScale;
    result["shell_opacity"] = soul.shellOpacity;
    result["emission"] = soul.emission;
    result["scale"] = v(soul.scale);
    result["deformation"] = v(soul.deformation);
    result["pull_amount"] = soul.pullAmount;
    result["latch_amount"] = soul.latchAmount;
    result["ingest_amount"] = soul.ingestAmount;
    result["elasticity"] = soul.elasticity;
    result["phase"] = soul.phase;
    result["color"] = Color(soul.color.r, soul.color.g, soul.color.b, 1.0f);
    return result;
}

PackedVector3Array ParentGameNode::target_soul_lattice(int index) const {
    PackedVector3Array result;
    if (index < 0 || index >= TARGET_COUNT) return result;
    const auto& target = bridge.state().targets[index];
    result.resize(SOUL_LATTICE_NODE_COUNT);
    for (int i = 0; i < SOUL_LATTICE_NODE_COUNT; ++i)
        result.set(i, v(target.latticeSurfacePos[i]));
    return result;
}
PackedVector3Array ParentGameNode::target_model_vertices(int index) const {
    PackedVector3Array result;
    if(index<0||index>=TARGET_COUNT)return result;
    if(!human_model_load_attempted){
        human_model_load_attempted=true;
        const String project_root=ProjectSettings::get_singleton()->globalize_path("res://");
        const String path=project_root.path_join("../parent/models/human.dbhuman").simplify_path();
        human_model.load(path.utf8().get_data());
    }
    if(!human_model.valid())return result;
    const auto snapshot=TargetPresentationSnapshot::from(bridge.state(),index);
    HumanModelExpressiveness expression;
    expression.hitAmount=snapshot.reaction.hitAmount;
    expression.hitDirection=snapshot.reaction.hitDirectionLocal;
    expression.time=bridge.state().time;
    expression.locomotionPhase=snapshot.reaction.locomotionPhase;
    expression.locomotionAmount=snapshot.reaction.locomotionAmount;
    expression.commitment=snapshot.reaction.commitment;
    expression.searchAmount=snapshot.reaction.searchAmount;
    expression.disruption=snapshot.reaction.disruption;
    expression.vacuumPull=snapshot.reaction.vacuumPullAmount;
    expression.individuality=snapshot.reaction.individuality;
    HumanModelFootTargets feet;
    feet.left[0]=snapshot.leftFootTarget.x;feet.left[1]=snapshot.leftFootTarget.y;feet.left[2]=snapshot.leftFootTarget.z;
    feet.right[0]=snapshot.rightFootTarget.x;feet.right[1]=snapshot.rightFootTarget.y;feet.right[2]=snapshot.rightFootTarget.z;
    feet.leftContact=snapshot.leftFootContact;
    feet.rightContact=snapshot.rightFootContact;
    feet.active=snapshot.bodyAuthoritative;
    std::vector<float> vertices;
    human_model.skin(snapshot.animationTime,snapshot.attackTimer,snapshot.attackVariant,vertices,expression,feet);
    result.resize(static_cast<int64_t>(vertices.size()/3u));
    for(int64_t i=0;i<result.size();++i)
        result.set(i,Vector3(vertices[static_cast<std::size_t>(i)*3u],
            vertices[static_cast<std::size_t>(i)*3u+1u],vertices[static_cast<std::size_t>(i)*3u+2u]));
    return result;
}
Color ParentGameNode::target_model_color(int index) const {
    if(index<0||index>=TARGET_COUNT)return Color(1,1,1,1);
    const auto& target=bridge.state().targets[index];
    const auto color=enemyShellColor(target.brute,target.attackVariant,bridge.state().roomIndex);
    return Color(color.r,color.g,color.b,1.0f);
}
Vector3 ParentGameNode::player_position()const{return v(bridge.state().player.pos);}
Vector3 ParentGameNode::camera_position()const{return v(bridge.state().camera.pos);}
Vector3 ParentGameNode::camera_target()const{return v(bridge.state().camera.lookTarget);}
float ParentGameNode::camera_fov_degrees()const{return bridge.state().camera.verticalFovDegrees;}
Vector3 ParentGameNode::target_position(int i)const{return i>=0&&i<TARGET_COUNT?v(bridge.state().targets[i].pos):Vector3();}
bool ParentGameNode::target_alive(int i)const{return i>=0&&i<TARGET_COUNT&&bridge.state().targets[i].alive;}
bool ParentGameNode::target_slurpable(int i)const{return i>=0&&i<TARGET_COUNT&&bridge.state().targets[i].slurpable;}
float ParentGameNode::target_armor(int i)const{return i>=0&&i<TARGET_COUNT?bridge.state().targets[i].armor:0.0f;}
float ParentGameNode::target_ingest_progress(int i)const{return i>=0&&i<TARGET_COUNT?bridge.state().targets[i].ingestProgress:0.0f;}
int ParentGameNode::target_count()const{return TARGET_COUNT;}
int ParentGameNode::souls()const{return bridge.state().player.souls;}
float ParentGameNode::battery()const{return bridge.state().player.battery;}
int ParentGameNode::frame()const{return bridge.state().frame;}
bool ParentGameNode::grounded()const{return bridge.state().player.grounded;}
bool ParentGameNode::first_person()const{return bridge.state().camera.firstPerson;}
bool ParentGameNode::vacuum_active()const{return bridge.state().vacuum.active;}
float ParentGameNode::vacuum_power()const{return bridge.state().vacuum.power;}
float ParentGameNode::melee_pose()const{return bridge.state().meleePose;}
int ParentGameNode::live_bullets()const{int count=0;for(const auto& bullet:bridge.state().bullets)if(bullet.alive)++count;return count;}
int ParentGameNode::comm_signal()const{return bridge.state().player.commSignal;}
float ParentGameNode::comm_signal_timer()const{return bridge.state().player.commSignalTimer;}
bool ParentGameNode::paused()const{return bridge.state().uiPaused;}
float ParentGameNode::simulation_time()const{return bridge.state().time;}
