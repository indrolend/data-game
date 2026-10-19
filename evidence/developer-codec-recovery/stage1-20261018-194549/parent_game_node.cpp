#include "parent_game_node.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include "phone_presentation_snapshot.hpp"
#include "room_presentation_snapshot.hpp"
#include "combat_presentation_snapshot.hpp"
#include "MaterialResponse.hpp"
using namespace godot;
void ParentGameNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("submit_input","x","z","look_x","look_y","vacuum","sprint","jump","melee","shoot","camera"),&ParentGameNode::submit_input);
    ClassDB::bind_method(D_METHOD("tick"),&ParentGameNode::tick);
    ClassDB::bind_method(D_METHOD("set_comm_signal","signal"),&ParentGameNode::set_comm_signal);
    ClassDB::bind_method(D_METHOD("set_paused","paused"),&ParentGameNode::set_paused);
    ClassDB::bind_method(D_METHOD("start_rally_fixture"),&ParentGameNode::start_rally_fixture);
    ClassDB::bind_method(D_METHOD("start_target_fixture"),&ParentGameNode::start_target_fixture);
    ClassDB::bind_method(D_METHOD("start_target_interaction_fixture"),&ParentGameNode::start_target_interaction_fixture);
    ClassDB::bind_method(D_METHOD("refill_target_interaction_battery"),&ParentGameNode::refill_target_interaction_battery);
    ClassDB::bind_method(D_METHOD("player_position"),&ParentGameNode::player_position);
    ClassDB::bind_method(D_METHOD("phone_presentation_snapshot"),&ParentGameNode::phone_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("room_presentation_snapshot","tile_index"),&ParentGameNode::room_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("player_action_snapshot"),&ParentGameNode::player_action_snapshot);
    ClassDB::bind_method(D_METHOD("target_presentation_snapshot","index"),&ParentGameNode::target_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("target_soul_presentation_snapshot","index"),&ParentGameNode::target_soul_presentation_snapshot);
    ClassDB::bind_method(D_METHOD("target_soul_lattice","index"),&ParentGameNode::target_soul_lattice);
    ClassDB::bind_method(D_METHOD("target_model_vertices","index"),&ParentGameNode::target_model_vertices);
    ClassDB::bind_method(D_METHOD("target_model_color","index"),&ParentGameNode::target_model_color);
    ClassDB::bind_method(D_METHOD("camera_position"),&ParentGameNode::camera_position);
    ClassDB::bind_method(D_METHOD("camera_target"),&ParentGameNode::camera_target);
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
void ParentGameNode::start_rally_fixture(){bridge.startRallyFixture();}
void ParentGameNode::start_target_fixture(){bridge.startTargetFixture();}
void ParentGameNode::start_target_interaction_fixture(){bridge.startTargetInteractionFixture();}
void ParentGameNode::refill_target_interaction_battery(){bridge.refillTargetInteractionBattery();}
static Vector3 v(const Vec3& p){return Vector3(p.x,p.y,p.z);}
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
    Array objects;
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
        objects.push_back(item);
    }
    result["objects"] = objects;
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
