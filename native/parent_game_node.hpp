#pragma once
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include "parent_bridge.hpp"
namespace godot {
class ParentGameNode : public Node {
    GDCLASS(ParentGameNode, Node)
    ParentBridge bridge;
    bool jump_last=false, melee_last=false, shoot_last=false, camera_last=false;
protected:
    static void _bind_methods();
public:
    void submit_input(float x,float z,float look_x,float look_y,bool vacuum,bool sprint,
                      bool jump,bool melee,bool shoot,bool camera);
    void tick();
    void start_rally_fixture();
    void start_target_fixture();
    void start_target_interaction_fixture();
    void refill_target_interaction_battery();
    Vector3 player_position() const;
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
};
}
