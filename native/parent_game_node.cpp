#include "parent_game_node.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <fstream>
#include <iterator>
#include <vector>
#define STB_TRUETYPE_IMPLEMENTATION
#include "third_party/stb/stb_truetype.h"
#include "phone_presentation_snapshot.hpp"
#include "PhoneDisplayLayout.hpp"
#include "BitmapFont.hpp"
#include "PhoneStencilReveal.hpp"
#include "MusicDirector.hpp"
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
    ClassDB::bind_method(D_METHOD("menu_item_at","logical_x","logical_y"),&ParentGameNode::menu_item_at);
    ClassDB::bind_method(D_METHOD("set_menu_selection","selection"),&ParentGameNode::set_menu_selection);
    ClassDB::bind_method(D_METHOD("menu_adjust","direction"),&ParentGameNode::menu_adjust);
    ClassDB::bind_method(D_METHOD("menu_confirm"),&ParentGameNode::menu_confirm);
    ClassDB::bind_method(D_METHOD("menu_back"),&ParentGameNode::menu_back);
    ClassDB::bind_method(D_METHOD("upgrade_menu_active"),&ParentGameNode::upgrade_menu_active);
    ClassDB::bind_method(D_METHOD("move_upgrade_selection","horizontal","vertical"),&ParentGameNode::move_upgrade_selection);
    ClassDB::bind_method(D_METHOD("set_upgrade_selection","selection"),&ParentGameNode::set_upgrade_selection);
    ClassDB::bind_method(D_METHOD("activate_upgrade_selection"),&ParentGameNode::activate_upgrade_selection);
    ClassDB::bind_method(D_METHOD("start_rally_fixture"),&ParentGameNode::start_rally_fixture);
    ClassDB::bind_method(D_METHOD("start_traversal_fixture"),&ParentGameNode::start_traversal_fixture);
    ClassDB::bind_method(D_METHOD("start_slope_fixture"),&ParentGameNode::start_slope_fixture);
    ClassDB::bind_method(D_METHOD("start_field_island_fixture"),&ParentGameNode::start_field_island_fixture);
    ClassDB::bind_method(D_METHOD("prepare_capture_shot_fixture"),&ParentGameNode::prepare_capture_shot_fixture);
    ClassDB::bind_method(D_METHOD("start_target_fixture"),&ParentGameNode::start_target_fixture);
    ClassDB::bind_method(D_METHOD("start_room_inspector"),&ParentGameNode::start_room_inspector);
    ClassDB::bind_method(D_METHOD("step_room_inspector","delta","reroll"),&ParentGameNode::step_room_inspector);
    ClassDB::bind_method(D_METHOD("toggle_room_inspector_enemies"),&ParentGameNode::toggle_room_inspector_enemies);
    ClassDB::bind_method(D_METHOD("room_inspector_snapshot"),&ParentGameNode::room_inspector_snapshot);
    ClassDB::bind_method(D_METHOD("room_review_line","rating"),&ParentGameNode::room_review_line);
    ClassDB::bind_method(D_METHOD("player_support_snapshot","x","z"),&ParentGameNode::player_support_snapshot);
    ClassDB::bind_method(D_METHOD("world_support_snapshot","x","z","radius"),&ParentGameNode::world_support_snapshot);
    ClassDB::bind_method(D_METHOD("start_target_interaction_fixture"),&ParentGameNode::start_target_interaction_fixture);
    ClassDB::bind_method(D_METHOD("start_damage_fixture"),&ParentGameNode::start_damage_fixture);
    ClassDB::bind_method(D_METHOD("start_death_fixture"),&ParentGameNode::start_death_fixture);
    ClassDB::bind_method(D_METHOD("start_city_room_fixture"),&ParentGameNode::start_city_room_fixture);
    ClassDB::bind_method(D_METHOD("start_coastal_room_fixture"),&ParentGameNode::start_coastal_room_fixture);
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
    ClassDB::bind_method(D_METHOD("phone_display_raster_revision"),&ParentGameNode::phone_display_raster_revision);
    ClassDB::bind_method(D_METHOD("phone_display_raster_pixels"),&ParentGameNode::phone_display_raster_pixels);
    ClassDB::bind_method(D_METHOD("phone_display_uses_source_sans"),&ParentGameNode::phone_display_uses_source_sans);
    ClassDB::bind_method(D_METHOD("music_presentation_snapshot"),&ParentGameNode::music_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("hud_presentation_snapshot"),&ParentGameNode::hud_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("restart_menu_presentation_snapshot","viewport_width","viewport_height"),&ParentGameNode::restart_menu_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("restart_menu_item_at","viewport_x","viewport_y","viewport_width","viewport_height"),&ParentGameNode::restart_menu_item_at);
    ClassDB::bind_method(D_METHOD("upgrade_presentation_snapshot"),&ParentGameNode::upgrade_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("gameplay_loop_snapshot"),&ParentGameNode::gameplay_loop_snapshot);
    ClassDB::bind_method(D_METHOD("door_transition_presentation_snapshot"),&ParentGameNode::door_transition_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("projectile_presentation_snapshot"),&ParentGameNode::projectile_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("room_presentation_snapshot","tile_index"),&ParentGameNode::room_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("field_grass_texture_pixels"),&ParentGameNode::field_grass_texture_pixels);
    ClassDB::bind_method(D_METHOD("city_asphalt_texture_pixels"),&ParentGameNode::city_asphalt_texture_pixels);
    ClassDB::bind_method(D_METHOD("grass_blade_presentation_snapshot","tile_index"),&ParentGameNode::grass_blade_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("grass_blade_tip_presentation_snapshot","tile_index"),&ParentGameNode::grass_blade_tip_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("particle_presentation_snapshot"),&ParentGameNode::particle_presentation_snapshot);
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
int ParentGameNode::menu_item_at(float logical_x,float logical_y) const{return bridge.menuItemAt(logical_x,logical_y);}
void ParentGameNode::set_menu_selection(int selection){bridge.setMenuSelection(selection);}
bool ParentGameNode::menu_adjust(int direction){return bridge.menuAdjust(direction);}
bool ParentGameNode::menu_confirm(){return bridge.menuConfirm();}
bool ParentGameNode::menu_back(){return bridge.menuBack();}
bool ParentGameNode::upgrade_menu_active() const{return bridge.upgradeMenuActive();}
void ParentGameNode::move_upgrade_selection(int horizontal,int vertical){bridge.moveUpgradeSelection(horizontal,vertical);}
void ParentGameNode::set_upgrade_selection(int selection){bridge.setUpgradeSelection(selection);}
bool ParentGameNode::activate_upgrade_selection(){return bridge.activateUpgradeSelection();}
void ParentGameNode::start_rally_fixture(){bridge.startRallyFixture();}
void ParentGameNode::start_traversal_fixture(){bridge.startTraversalFixture();}
void ParentGameNode::start_slope_fixture(){bridge.startSlopeFixture();}
void ParentGameNode::start_field_island_fixture(){bridge.startFieldIslandFixture();}
bool ParentGameNode::prepare_capture_shot_fixture(){return bridge.prepareCaptureShotFixture();}
void ParentGameNode::start_target_fixture(){bridge.startTargetFixture();}
void ParentGameNode::start_room_inspector(){bridge.startRoomInspector();}
void ParentGameNode::step_room_inspector(int delta,bool reroll){bridge.stepRoomInspector(delta,reroll);}
void ParentGameNode::toggle_room_inspector_enemies(){bridge.toggleRoomInspectorEnemies();}
Dictionary ParentGameNode::room_inspector_snapshot() const {
    const auto& state=bridge.state();
    const auto& report=state.roomInspectorReport;
    Dictionary result;
    result["active"]=state.roomInspector;
    result["enemies"]=state.roomInspectorEnemies;
    result["premise"]=static_cast<int>(report.premise);
    result["premise_name"]=room_environment::premiseName(report.premise);
    result["setting_name"]=room_environment::settingName(report.setting);
    result["form_name"]=room_environment::formName(report.form);
    result["scale_name"]=room_environment::scaleName(report.scale);
    result["condition_name"]=room_environment::conditionName(report.condition);
    result["traversal_intent"]=room_environment::traversalIntentName(report.traversalIntent);
    result["seed"]=report.seed;
    result["room_index"]=report.roomIndex;
    result["surface_count"]=report.traversalSurfaceCount;
    result["edge_count"]=report.traversalEdgeCount;
    result["required_edge_count"]=report.requiredEdgeCount;
    result["collider_count"]=report.colliderCount;
    result["presentation_prop_count"]=report.presentationPropCount;
    result["enemy_budget"]=report.enemyBudget;
    result["enemy_count"]=report.enemyCount;
    result["transparent_primitive_count"]=report.transparentPrimitiveCount;
    result["visible_primitive_estimate"]=report.visiblePrimitiveEstimate;
    result["draw_call_bucket"]=report.drawCallBucket;
    result["route_valid"]=report.requiredRouteValid;
    result["seed_valid"]=report.seedSelectionValid;
    return result;
}
String ParentGameNode::room_review_line(int rating) const {
    const int bounded=std::max(0,std::min(3,rating));
    return String::utf8(bridge.roomReviewLine(static_cast<RoomReviewRating>(bounded)).c_str());
}
static Dictionary support_dictionary(const WorldSupportSample& support) {
    Dictionary result;
    result["available"]=support.available;
    result["height"]=support.height;
    result["normal"]=Vector3(support.normal.x,support.normal.y,support.normal.z);
    result["classification"]=static_cast<int>(support.classification);
    result["source"]=static_cast<int>(support.identity.source);
    result["index"]=support.identity.index;
    return result;
}
Dictionary ParentGameNode::player_support_snapshot(float x,float z) const{return support_dictionary(bridge.playerSupportAt(x,z));}
Dictionary ParentGameNode::world_support_snapshot(float x,float z,float radius) const{return support_dictionary(bridge.worldSupportAt(x,z,radius));}
void ParentGameNode::start_target_interaction_fixture(){bridge.startTargetInteractionFixture();}
void ParentGameNode::start_damage_fixture(){bridge.startDamageFixture();}
void ParentGameNode::start_death_fixture(){bridge.startDeathFixture();}
void ParentGameNode::start_city_room_fixture(){bridge.startCityRoomFixture();}
void ParentGameNode::start_coastal_room_fixture(){bridge.startCoastalRoomFixture();}
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
Dictionary ParentGameNode::restart_menu_presentation_snapshot(float viewport_width, float viewport_height) const {
    const auto menu = bridge.deathMenuPresentation(viewport_width, viewport_height);
    Dictionary result;
    result["active"] = menu.active;
    result["ui_scale"] = menu.uiScale;
    result["canvas_width"] = menu.canvasWidth;
    result["canvas_height"] = menu.canvasHeight;
    result["fade"] = menu.fade;
    result["awaken"] = menu.awaken;
    result["pulse"] = menu.pulse;
    result["selection"] = menu.selection;
    Array rows;
    for (const auto& row : menu.rows) {
        Dictionary item;
        item["label"] = row.label;
        item["selection"] = row.selection;
        item["center_x"] = row.centerX;
        item["baseline_y"] = row.baselineY;
        item["text_scale"] = row.textScale;
        item["hit"] = Rect2(row.hit.x, row.hit.y, row.hit.w, row.hit.h);
        rows.push_back(item);
    }
    result["rows"] = rows;
    return result;
}
int ParentGameNode::restart_menu_item_at(float viewport_x, float viewport_y,
                                         float viewport_width, float viewport_height) const {
    return bridge.deathMenuItemAt(viewport_x, viewport_y, viewport_width, viewport_height);
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

Dictionary ParentGameNode::gameplay_loop_snapshot() const {
    const GameState& state=bridge.state();
    const auto plan=room_environment::roomPlan(state.roomSeed,state.roomIndex);
    int liveEnemies=0,looseSouls=0,projectiles=0;
    float looseCommitment=0.0f;
    for(const auto& target:state.targets){
        if(target.alive)++liveEnemies;
        else if(target.slurpable){++looseSouls;looseCommitment=std::max(looseCommitment,std::max(target.capture,target.ingestProgress));}
    }
    for(const auto& bullet:state.bullets)if(bullet.alive)++projectiles;
    Dictionary result;
    result["room_seed"]=state.roomSeed;
    result["room_index"]=state.roomIndex;
    result["setting"]=static_cast<int>(plan.setting);
    result["form"]=static_cast<int>(plan.form);
    result["scale"]=static_cast<int>(plan.scale);
    result["condition"]=static_cast<int>(plan.condition);
    result["room_heat"]=state.progression.run.roomHeat;
    result["room_elapsed"]=state.progression.run.roomElapsed;
    result["room_captures"]=state.progression.run.roomCaptures;
    result["battery"]=state.player.battery;
    result["battery_regen_lock"]=state.progression.run.batteryRegenLock;
    result["stored_souls"]=state.player.souls;
    result["deposited_souls"]=state.depositedSouls;
    result["required_souls"]=state.requiredSouls;
    result["room_clear"]=state.roomClear;
    result["live_enemies"]=liveEnemies;
    result["loose_souls"]=looseSouls;
    result["projectiles"]=projectiles;
    result["loose_commitment"]=looseCommitment;
    result["vacuum_target"]=state.vacuum.target;
    result["vacuum_lock"]=state.vacuum.lockStrength;
    result["dead"]=state.dead;
    result["downed"]=state.player.downed;
    result["camera_yaw"]=state.camera.yaw;
    result["camera_pitch"]=state.camera.pitch;
    result["player_velocity"]=v(state.player.vel);
    result["setting_name"]=room_environment::settingName(plan.setting);
    result["form_name"]=room_environment::formName(plan.form);
    result["required_slot_stacks"]=state.runRules.requiredSlotStacks;
    result["crowded_room_stacks"]=state.runRules.crowdedRoomStacks;
    result["faster_slurp_stacks"]=state.runRules.fasterSlurpStacks;
    result["last_added_rule"]=state.runRules.lastAdded;
    Array temporaryLevels;
    Array permanentLevels;
    for(int i=0;i<3;++i){temporaryLevels.push_back(state.progression.run.temporaryLevels[i]);permanentLevels.push_back(state.progression.permanent.levels[i]);}
    result["temporary_levels"]=temporaryLevels;
    result["permanent_levels"]=permanentLevels;
    result["tokens"]=static_cast<int64_t>(state.progression.permanent.tokens);
    result["upgrade_active"]=state.upgradeMenu.active;
    return result;
}
Dictionary ParentGameNode::door_transition_presentation_snapshot() const {
    const DoorTransitionState& transition=bridge.state().doorTransition;
    Dictionary result;
    result["stable_id"]="native-door-transition";
    result["active"]=transition.active;
    result["progress"]=transition.progress;
    result["distance_travelled"]=transition.distanceTravelled;
    result["last_player_position"]=v(transition.lastPlayerPos);
    result["frame_motion"]=v(transition.frameMotion);
    return result;
}
Array ParentGameNode::projectile_presentation_snapshot() const {
    const GameState& state=bridge.state();
    Array result;
    for(int index=0;index<static_cast<int>(state.bullets.size());++index){
        const auto& bullet=state.bullets[index];
        if(!bullet.alive)continue;
        Dictionary item;
        item["stable_id"]=String("native-projectile-")+String::num_int64(index);
        item["slot"]=index;
        item["position"]=v(bullet.pos);
        item["velocity"]=v(bullet.vel);
        item["life"]=bullet.life;
        item["spin"]=bullet.spin;
        item["brute"]=bullet.brute;
        item["dropped"]=bullet.dropped;
        result.push_back(item);
    }
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
    result["screen_light_color"] = Color(snapshot.screenLightColor.x,snapshot.screenLightColor.y,snapshot.screenLightColor.z);
    result["screen_light_intensity"] = snapshot.screenLightIntensity;
    result["screen_light_radius"] = snapshot.screenLightRadius;
    result["screen_light_forward_offset"] = snapshot.screenLightForwardOffset;
    result["screen_light_pulse"] = snapshot.screenLightPulse;
    result["interactive"] = snapshot.interactive;
    result["battery"] = snapshot.battery;
    result["souls"] = snapshot.souls;
    result["low_battery"] = snapshot.lowBattery;
    result["vacuum_active"] = snapshot.vacuumActive;
    result["vacuum_power"] = snapshot.vacuumPower;
    result["capture_pulse"] = snapshot.capturePulse;
    result["damage_pulse"] = snapshot.damagePulse;
    result["room_index"] = snapshot.roomIndex;
    result["tokens"] = static_cast<std::int64_t>(snapshot.tokens);
    result["filled_goals"] = snapshot.filledGoals;
    result["required_goals"] = snapshot.requiredGoals;
    result["room_clear"] = snapshot.roomClear;
    result["soul_capacity"] = snapshot.soulCapacity;
    result["unlocked_material_memories"] = snapshot.unlockedMaterialMemories;
    result["supplemental_active"] = snapshot.supplementalActive;
    result["supplemental_fill"] = snapshot.supplementalFill;
    result["flower_stacks"] = snapshot.flowerStacks;
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
namespace {
constexpr int PhoneRasterWidth = PhoneDisplayState::LogicalWidth;
constexpr int PhoneRasterHeight = PhoneDisplayState::LogicalHeight;

struct PhoneRasterFont {
    std::vector<unsigned char> bytes;
    stbtt_fontinfo info{};
    bool attempted = false;
    bool ready = false;

    void ensure_loaded() {
        if (attempted) return;
        attempted = true;
        const String resolved = ProjectSettings::get_singleton()->globalize_path(
            "res://fonts/SourceSans3-Semibold.ttf");
        const CharString path = resolved.utf8();
        std::ifstream input(path.get_data(), std::ios::binary);
        if (!input) return;
        bytes.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
        if (bytes.empty()) return;
        ready = stbtt_InitFont(&info, bytes.data(),
            stbtt_GetFontOffsetForIndex(bytes.data(), 0)) != 0;
    }
};

PhoneRasterFont& phone_source_sans() {
    static PhoneRasterFont font;
    font.ensure_loaded();
    return font;
}

void phone_raster_rect(uint8_t* pixels, int x, int y, int width, int height,
                       float r, float g, float b, float alpha) {
    const int x0 = std::max(0, x), y0 = std::max(0, y);
    const int x1 = std::min(PhoneRasterWidth, x + width);
    const int y1 = std::min(PhoneRasterHeight, y + height);
    const float a = clampf(alpha, 0.0f, 1.0f);
    for (int py = y0; py < y1; ++py) for (int px = x0; px < x1; ++px) {
        const int at = (py * PhoneRasterWidth + px) * 4;
        pixels[at] = static_cast<uint8_t>(pixels[at] * (1.0f - a) + 255.0f * clampf(r, 0.0f, 1.0f) * a);
        pixels[at + 1] = static_cast<uint8_t>(pixels[at + 1] * (1.0f - a) + 255.0f * clampf(g, 0.0f, 1.0f) * a);
        pixels[at + 2] = static_cast<uint8_t>(pixels[at + 2] * (1.0f - a) + 255.0f * clampf(b, 0.0f, 1.0f) * a);
        pixels[at + 3] = 255;
    }
}

int phone_bitmap_text_width(const std::string& text, float font_px) {
    const int scale = std::max(1, static_cast<int>(std::round(font_px / 8.0f)));
    return text.empty() ? 0 : static_cast<int>(text.size()) * scale * 6 - scale;
}

float phone_source_sans_width(const std::string& text, float font_px) {
    auto& font = phone_source_sans();
    if (!font.ready || text.empty()) return 0.0f;
    const float scale = stbtt_ScaleForPixelHeight(&font.info, font_px);
    float width = 0.0f;
    int previous = 0;
    for (unsigned char character : text) {
        int advance = 0, bearing = 0;
        stbtt_GetCodepointHMetrics(&font.info, character, &advance, &bearing);
        if (previous) width += static_cast<float>(
            stbtt_GetCodepointKernAdvance(&font.info, previous, character)) *
            scale * phone_stencil::HorizontalScale;
        width += static_cast<float>(advance) * scale * phone_stencil::HorizontalScale;
        width += font_px * phone_stencil::TrackingEm;
        previous = character;
    }
    return std::max(0.0f, width - font_px * phone_stencil::TrackingEm);
}

void phone_source_sans_text(uint8_t* pixels, const std::string& text, float x,
                            float baseline, float font_px, float r, float g,
                            float b, float alpha, bool centered, bool right) {
    auto& font = phone_source_sans();
    if (!font.ready || text.empty()) return;
    const float scale = stbtt_ScaleForPixelHeight(&font.info, font_px);
    const float width = phone_source_sans_width(text, font_px);
    float pen = x - (centered ? width * 0.5f : right ? width : 0.0f);
    int previous = 0;
    for (unsigned char character : text) {
        if (previous) pen += static_cast<float>(
            stbtt_GetCodepointKernAdvance(&font.info, previous, character)) *
            scale * phone_stencil::HorizontalScale;
        int advance = 0, bearing = 0;
        stbtt_GetCodepointHMetrics(&font.info, character, &advance, &bearing);
        int bitmap_width = 0, bitmap_height = 0, x_offset = 0, y_offset = 0;
        unsigned char* bitmap = stbtt_GetCodepointBitmap(&font.info, scale, scale,
            character, &bitmap_width, &bitmap_height, &x_offset, &y_offset);
        const int compressed_width = std::max(1, static_cast<int>(std::ceil(
            static_cast<float>(bitmap_width) * phone_stencil::HorizontalScale)));
        if (bitmap && bitmap_width > 0 && bitmap_height > 0) {
            for (int yy = 0; yy < bitmap_height; ++yy) {
                const int py = static_cast<int>(std::floor(baseline + y_offset + yy));
                if (py < 0 || py >= PhoneRasterHeight) continue;
                for (int xx = 0; xx < compressed_width; ++xx) {
                    const int source_x = std::min(bitmap_width - 1, static_cast<int>(
                        static_cast<float>(xx) / phone_stencil::HorizontalScale));
                    const float coverage = static_cast<float>(bitmap[yy * bitmap_width + source_x]) / 255.0f;
                    const float blended_alpha = coverage * clampf(alpha, 0.0f, 1.0f);
                    if (blended_alpha <= 0.001f) continue;
                    const int px = static_cast<int>(std::floor(
                        pen + static_cast<float>(x_offset) * phone_stencil::HorizontalScale + xx));
                    if (px < 0 || px >= PhoneRasterWidth) continue;
                    const int at = (py * PhoneRasterWidth + px) * 4;
                    pixels[at] = static_cast<uint8_t>(pixels[at] * (1.0f - blended_alpha) + r * 255.0f * blended_alpha);
                    pixels[at + 1] = static_cast<uint8_t>(pixels[at + 1] * (1.0f - blended_alpha) + g * 255.0f * blended_alpha);
                    pixels[at + 2] = static_cast<uint8_t>(pixels[at + 2] * (1.0f - blended_alpha) + b * 255.0f * blended_alpha);
                    pixels[at + 3] = 255;
                }
            }
        }
        stbtt_FreeBitmap(bitmap, nullptr);
        pen += static_cast<float>(advance) * scale * phone_stencil::HorizontalScale +
            font_px * phone_stencil::TrackingEm;
        previous = character;
    }
}

void phone_bitmap_text(uint8_t* pixels, const std::string& text, float x, float baseline,
                       float font_px, float r, float g, float b, float alpha,
                       bool centered = false, bool right = false) {
    if (phone_source_sans().ready) {
        phone_source_sans_text(pixels, text, x, baseline, font_px, r, g, b,
                               alpha, centered, right);
        return;
    }
    const int scale = std::max(1, static_cast<int>(std::round(font_px / 8.0f)));
    int pen = static_cast<int>(std::round(x));
    const int width = phone_bitmap_text_width(text, font_px);
    if (centered) pen -= width / 2;
    if (right) pen -= width;
    const int top = static_cast<int>(std::round(baseline)) - 7 * scale;
    for (char character : text) {
        const auto rows = bitmapGlyph(character);
        for (int row = 0; row < 7; ++row) for (int column = 0; column < 5; ++column) {
            if ((rows[row] & (1u << (4 - column))) == 0) continue;
            phone_raster_rect(pixels, pen + column * scale, top + row * scale,
                              scale, scale, r, g, b, alpha);
        }
        pen += 6 * scale;
    }
}

void hash_phone_text(uint64_t& hash, const std::string& text) {
    for (unsigned char value : text) { hash ^= value; hash *= 1099511628211ull; }
    hash ^= 0xffu; hash *= 1099511628211ull;
}
}

bool ParentGameNode::phone_display_uses_source_sans() const {
    return phone_source_sans().ready;
}

int64_t ParentGameNode::phone_display_raster_revision() const {
    const auto& state = bridge.state();
    const auto layout = makePhoneDisplayMenuLayout(state);
    uint64_t hash = 1469598103934665603ull;
    hash ^= static_cast<uint64_t>(state.phoneDisplay.mode); hash *= 1099511628211ull;
    hash ^= static_cast<uint64_t>(state.hud.menuSelection + 1); hash *= 1099511628211ull;
    hash_phone_text(hash, layout.title);
    hash_phone_text(hash, layout.navigationHint);
    for (int i = 0; i < layout.rowCount; ++i) {
        const auto& row = layout.rows[i];
        hash_phone_text(hash, row.label); hash_phone_text(hash, row.value);
        hash ^= static_cast<uint64_t>(row.visible) | (static_cast<uint64_t>(row.peek) << 1u);
        hash *= 1099511628211ull;
    }
    return static_cast<int64_t>(hash & 0x7fffffffffffffffull);
}

PackedByteArray ParentGameNode::phone_display_raster_pixels() const {
    PackedByteArray result;
    result.resize(PhoneRasterWidth * PhoneRasterHeight * 4);
    uint8_t* pixels = result.ptrw();
    const auto& state = bridge.state();
    const Vec3 tint = state.phoneDisplay.screenTint;
    const uint8_t red = static_cast<uint8_t>(255.0f * clampf(0.025f + tint.x * 0.18f, 0.0f, 1.0f));
    const uint8_t green = static_cast<uint8_t>(255.0f * clampf(0.045f + tint.y * 0.16f, 0.0f, 1.0f));
    const uint8_t blue = static_cast<uint8_t>(255.0f * clampf(0.060f + tint.z * 0.15f, 0.0f, 1.0f));
    for (int i = 0; i < PhoneRasterWidth * PhoneRasterHeight; ++i) {
        pixels[i * 4] = red; pixels[i * 4 + 1] = green;
        pixels[i * 4 + 2] = blue; pixels[i * 4 + 3] = 255;
    }
    phone_raster_rect(pixels, 0, 0, PhoneRasterWidth, PhoneRasterHeight,
                      0.03f, 0.55f, 0.62f, 0.10f + state.phoneDisplay.brightness * 0.08f);
    const auto layout = makePhoneDisplayMenuLayout(state);
    phone_bitmap_text(pixels, layout.title, layout.logicalW * 0.5f,
                      layout.titleCenterY + layout.titlePx * 0.34f, layout.titlePx,
                      0.90f, 0.97f, 1.0f, 0.96f, true);
    for (int i = 0; i < layout.rowCount; ++i) {
        const auto& row = layout.rows[i];
        if (!row.visible || row.peek) continue;
        const bool selected = row.selectable && row.selectableIndex == state.hud.menuSelection;
        if (selected) {
            phone_raster_rect(pixels, static_cast<int>(row.visual.x), static_cast<int>(row.visual.y),
                              static_cast<int>(row.visual.w), static_cast<int>(row.visual.h),
                              0.30f, 0.92f, 0.86f, 0.13f);
        }
        const bool section = row.kind == PhoneMenuRowKind::Section;
        const auto emphasis = phoneMenuEmphasis(row.action);
        const VisualColor color = section ? VisualIdentity::MetallicTeal :
            emphasis == PhoneMenuEmphasis::Destructive ? VisualIdentity::Copper :
            selected && emphasis == PhoneMenuEmphasis::Primary ? VisualIdentity::AcidChartreuse :
            VisualColor{selected ? 1.0f : 0.70f, selected ? 1.0f : 0.88f, 1.0f};
        phone_bitmap_text(pixels, row.label, row.labelX, row.baselineY, row.fontPx,
                          color.r, color.g, color.b, section ? 0.62f : selected ? 1.0f : 0.72f);
        if (row.kind == PhoneMenuRowKind::TwoColumn) {
            const VisualColor valueColor = selected ? VisualIdentity::ElectricCyan : VisualIdentity::MetallicTeal;
            phone_bitmap_text(pixels, row.value, row.valueRightX, row.baselineY, row.fontPx,
                              valueColor.r, valueColor.g, valueColor.b, selected ? 0.98f : 0.78f,
                              false, true);
        }
    }
    phone_bitmap_text(pixels, layout.navigationHint, layout.logicalW * 0.5f,
                      layout.logicalH - 66.0f, 26.0f, VisualIdentity::ElectricCyan.r,
                      VisualIdentity::ElectricCyan.g, VisualIdentity::ElectricCyan.b, 0.62f, true);
    return result;
}

Dictionary ParentGameNode::music_presentation_snapshot() const {
    const auto& state = bridge.state();
    Dictionary result;
    const float volume = state.localSettings.musicMuted ? 0.0f :
        clampf(state.localSettings.musicVolume, 0.0f, 1.0f);
    const bool menu = !state.started || state.attractMode;
    const bool stage = state.started && !state.attractMode && !state.dead;
    int theme = -1;
    String key;
    if (menu) {
        const auto selected = music_director::menuTheme(
            state.localSettings.menuPage != LocalMenuPage::Main);
        theme = static_cast<int>(selected);
        key = selected == music_director::MenuTheme::Pensive ? "menu-pensive" : "title";
    } else if (stage) {
        const auto plan = room_environment::roomPlan(state.roomSeed, state.roomIndex);
        const auto selected = music_director::stageTheme(plan, state.roomIndex);
        theme = static_cast<int>(selected);
        switch (selected) {
            case music_director::StageTheme::Turnt: key = "stage-turnt"; break;
            case music_director::StageTheme::Wacky: key = "stage-wacky"; break;
            case music_director::StageTheme::Boss: key = "boss"; break;
            default: key = "stage-original"; break;
        }
    }
    result["playing"] = (menu || stage) && volume > 0.0001f;
    result["menu"] = menu;
    result["theme"] = theme;
    result["key"] = key;
    result["volume"] = volume;
    result["muted"] = state.localSettings.musicMuted;
    result["room_index"] = state.roomIndex;
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
    result["presentation_time"] = snapshot.presentationTime;
    Dictionary field_island;
    field_island["enabled"] = snapshot.fieldIsland;
    field_island["main_half_width"] = snapshot.fieldIslandMainHalfWidth;
    field_island["main_half_depth"] = snapshot.fieldIslandMainHalfDepth;
    field_island["bridge_half_width"] = snapshot.fieldIslandBridgeHalfWidth;
    field_island["bridge_half_depth"] = snapshot.fieldIslandBridgeHalfDepth;
    field_island["abyss_recovery_y"] = snapshot.abyssRecoveryY;
    field_island["recovery_count"] = snapshot.abyssRecoveryCount;
    result["field_island"] = field_island;
    Dictionary coastal_water;
    coastal_water["enabled"] = snapshot.coastalWater;
    coastal_water["shore_x"] = coastal_water::ShoreX;
    coastal_water["far_x"] = coastal_water::FarX;
    coastal_water["min_z"] = coastal_water::MinZ;
    coastal_water["max_z"] = coastal_water::MaxZ;
    coastal_water["tile_offset_z"] = static_cast<float>(snapshot.tileIndex) * world::RoomDepth;
    coastal_water["native_time"] = snapshot.presentationTime;
    coastal_water["source_position"] = v(snapshot.waterSourcePosition);
    coastal_water["source_speed"] = snapshot.waterSourceSpeed;
    coastal_water["source_active"] = snapshot.waterSourceActive;
    result["coastal_water"] = coastal_water;
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
    Array capture_points;
    for(const auto& capture:snapshot.capturePoints){
        Dictionary item;
        item["id"]=capture.id.c_str();
        item["position"]=v(capture.position);
        item["filled"]=capture.filled;
        item["surround_material"]=material_dictionary(capture.surroundMaterial);
        item["recess_material"]=material_dictionary(capture.recessMaterial);
        item["soul_material"]=material_dictionary(capture.soulMaterial);
        capture_points.push_back(item);
    }
    result["capture_points"]=capture_points;
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
    Array field_backdrop_trees;
    for (const auto& tree : snapshot.fieldBackdropTrees) {
        Dictionary item;
        item["id"] = tree.id.c_str();
        item["model"] = tree.model.c_str();
        item["origin"] = v(tree.origin);
        item["scale"] = v(tree.scale);
        item["pitch"] = tree.pitch;
        item["yaw"] = tree.yaw;
        item["roll"] = tree.roll;
        item["physical"] = false;
        field_backdrop_trees.push_back(item);
    }
    result["field_backdrop_trees"] = field_backdrop_trees;
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
        if (object.primitive == room_environment::EnvironmentPrimitive::LawnFragment) {
            const auto prop = room_environment::environmentProp(
                roomPlan, snapshot.roomSeed, snapshot.roomIndex, object.propIndex);
            item["visual_color"] = Color(0.72f, 0.84f, 0.70f);
            item["visual_texture"] = "field_grass";
            item["visual_texture_world_scale"] = 1.45f;
            item["visual_texture_phase"] = std::fmod(prop.variant * 0.219f, 1.0f);
        }
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
            item["visual_texture"] = "stone";
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
        item["tone"] = 0.82f + room_environment::unit(static_cast<unsigned int>(index * 17 + tile_index * 131)) * 0.22f;
        result[index] = item;
    }
    return result;
}
Array ParentGameNode::grass_blade_tip_presentation_snapshot(int tile_index) const {
    const auto& state = bridge.state();
    if (tile_index < 0) tile_index = state.topology.currentTileIndex;
    const auto plan = room_environment::roomPlan(state.roomSeed, state.roomIndex);
    Array result;
    if (!plan.grass) return result;
    result.resize(room_environment::GrassBladeCountLow);
    room_environment::GrassReactionInputs inputs;
    inputs.player = state.player.pos;
    inputs.vacuumOrigin = state.phoneTransform.vacuumPullPoint;
    inputs.vacuumStrength = state.vacuum.fieldStrength;
    inputs.shotOrigin = state.environmentVisual.latestShotOrigin;
    inputs.shotAge = state.environmentVisual.latestShotAge;
    const float tileOffset = static_cast<float>(tile_index) * world::RoomDepth;
    for (int index = 0; index < room_environment::GrassBladeCountLow; ++index) {
        auto blade = room_environment::grassBlade(state.roomSeed, state.roomIndex, tile_index, index);
        blade.root.z += tileOffset;
        result[index] = v(room_environment::grassTip(blade, state.time, inputs));
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
    result["daylight"] = snapshot.daylight;
    result["open_sky"] = snapshot.openSky;
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
    result["shell_crack_amount"] = snapshot.shellCrackAmount;
    result["damage_surface_color"] = Color(snapshot.damageSurfaceColor.r,snapshot.damageSurfaceColor.g,snapshot.damageSurfaceColor.b,1.0f);
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
        const String path=project_root.path_join("../models/human.dbhuman").simplify_path();
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
    const auto& target = bridge.state().targets[index];
    const float armorMaximum = combat_presentation::shellArmorMaximum(target.brute);
    const float crackAmount = snapshot.shellCrackAmount;
    std::vector<Vector3> visibleVertices;
    visibleVertices.reserve(vertices.size());
    for(std::size_t triangle=0;triangle+8u<vertices.size();triangle+=9u){
        const Vector3 a(vertices[triangle],vertices[triangle+1u],vertices[triangle+2u]);
        const Vector3 b(vertices[triangle+3u],vertices[triangle+4u],vertices[triangle+5u]);
        const Vector3 c(vertices[triangle+6u],vertices[triangle+7u],vertices[triangle+8u]);
        const Vec3 center{(a.x+b.x+c.x)/3.0f,(a.y+b.y+c.y)/3.0f,(a.z+b.z+c.z)/3.0f};
        if(humanShellTriangleMissingTowardCrit(triangle/9u,crackAmount,center)) continue;
        visibleVertices.push_back(a); visibleVertices.push_back(b); visibleVertices.push_back(c);
    }
    result.resize(static_cast<int64_t>(visibleVertices.size()));
    for(int64_t i=0;i<result.size();++i) result.set(i,visibleVertices[static_cast<std::size_t>(i)]);
    return result;
}
Color ParentGameNode::target_model_color(int index) const {
    if(index<0||index>=TARGET_COUNT)return Color(1,1,1,1);
    const auto snapshot=TargetPresentationSnapshot::from(bridge.state(),index);
    const auto color=snapshot.damageSurfaceColor;
    return Color(color.r,color.g,color.b,1.0f);
}
Array ParentGameNode::particle_presentation_snapshot() const {
    const auto& state = bridge.state();
    const auto plan = room_environment::roomPlan(state.roomSeed, state.roomIndex);
    Array result;
    for (const auto& particle : state.particles) {
        if (particle.life <= 0.0f || particle.maxLife <= 0.0f) continue;
        const float remaining = clampf(particle.life / particle.maxLife, 0.0f, 1.0f);
        const auto color = particleMaterialColor(particle.material, plan.setting, remaining);
        Dictionary item;
        item["position"] = v(particle.pos);
        item["size"] = particle.size;
        item["life"] = particle.life;
        item["max_life"] = particle.maxLife;
        item["remaining"] = remaining;
        item["material"] = static_cast<int>(particle.material);
        item["settled"] = particleSurfaceSettled(particle);
        item["surface_normal"] = v(particleSurfaceNormal(particle));
        item["color"] = Color(color.r, color.g, color.b, remaining);
        result.push_back(item);
    }
    return result;
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
