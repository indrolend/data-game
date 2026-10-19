#pragma once
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include "parent_bridge.hpp"
#include "HumanModelData.hpp"
namespace godot {
class ParentGameNode : public Node {
    GDCLASS(ParentGameNode, Node)
    ParentBridge bridge;
    mutable HumanModelData human_model;
    mutable bool human_model_load_attempted = false;
protected:
    static void _bind_methods();
public:
    void submit_input(float x,float z,float look_x,float look_y,bool vacuum,bool sprint,
                      bool jump,bool melee,bool shoot,bool camera);
    void tick();
    void set_comm_signal(int signal);
    void set_paused(bool paused);
    void start_rally_fixture();
    void start_target_fixture();
    void start_target_interaction_fixture();
    void refill_target_interaction_battery();
    Vector3 player_position() const;
    Dictionary phone_presentation_snapshot() const;
    Dictionary room_presentation_snapshot(int tile_index) const;
    Dictionary player_action_snapshot() const;
    Dictionary target_presentation_snapshot(int index) const;
    Dictionary target_soul_presentation_snapshot(int index) const;
    PackedVector3Array target_soul_lattice(int index) const;
    PackedVector3Array target_model_vertices(int index) const;
    Color target_model_color(int index) const;
    Vector3 camera_position() const;
    Vector3 camera_target() const;
    Vector3 target_position(int index) const;
    bool target_alive(int index) const;
    bool target_slurpable(int index) const;
    float target_armor(int index) const;
    float target_ingest_progress(int index) const;
    int target_count() const;
    int souls() const;
    float battery() const;
    int frame() const;
    bool grounded() const;
    bool first_person() const;
    bool vacuum_active() const;
    float vacuum_power() const;
    float melee_pose() const;
    int live_bullets() const;
    int comm_signal() const;
    float comm_signal_timer() const;
    bool paused() const;
    float simulation_time() const;
};
}
