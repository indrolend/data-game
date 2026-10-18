#include "parent_game_node.hpp"
#include <godot_cpp/core/class_db.hpp>
using namespace godot;
void ParentGameNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("submit_input","x","z","look_x","look_y","vacuum","sprint","jump","melee","shoot","camera"),&ParentGameNode::submit_input);
    ClassDB::bind_method(D_METHOD("tick"),&ParentGameNode::tick);
    ClassDB::bind_method(D_METHOD("start_rally_fixture"),&ParentGameNode::start_rally_fixture);
    ClassDB::bind_method(D_METHOD("start_target_fixture"),&ParentGameNode::start_target_fixture);
    ClassDB::bind_method(D_METHOD("start_target_interaction_fixture"),&ParentGameNode::start_target_interaction_fixture);
    ClassDB::bind_method(D_METHOD("refill_target_interaction_battery"),&ParentGameNode::refill_target_interaction_battery);
    ClassDB::bind_method(D_METHOD("player_position"),&ParentGameNode::player_position);
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
}
void ParentGameNode::submit_input(float x,float z,float lx,float ly,bool vacuum,bool sprint,bool jump,bool melee,bool shoot,bool camera){
    bridge.input(x,z,lx,ly,vacuum,sprint,jump&&!jump_last,melee&&!melee_last,shoot&&!shoot_last,camera&&!camera_last);
    jump_last=jump;melee_last=melee;shoot_last=shoot;camera_last=camera;
}
void ParentGameNode::tick(){bridge.tick();}
void ParentGameNode::start_rally_fixture(){bridge.startRallyFixture();jump_last=melee_last=shoot_last=camera_last=false;}
void ParentGameNode::start_target_fixture(){bridge.startTargetFixture();jump_last=melee_last=shoot_last=camera_last=false;}
void ParentGameNode::start_target_interaction_fixture(){bridge.startTargetInteractionFixture();jump_last=melee_last=shoot_last=camera_last=false;}
void ParentGameNode::refill_target_interaction_battery(){bridge.refillTargetInteractionBattery();}
static Vector3 v(const Vec3& p){return Vector3(p.x,p.y,p.z);}
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
