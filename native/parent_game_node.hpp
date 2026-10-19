#pragma once
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
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
    void menu_move_selection(int direction);
    bool menu_adjust(int direction);
    bool menu_confirm();
    bool menu_back();
    bool upgrade_menu_active() const;
    void move_upgrade_selection(int horizontal, int vertical);
    void set_upgrade_selection(int selection);
    bool activate_upgrade_selection();
    void start_rally_fixture();
    void start_target_fixture();
    void start_target_interaction_fixture();
    void start_city_room_fixture();
    void start_coastal_room_fixture();
    void start_sterile_room_fixture();
    void start_ruin_room_fixture();
    void start_upgrade_fixture();
    void start_room_transition_fixture();
    void refill_target_interaction_battery();
    String execute_developer_command(const String& command);
    void set_developer_console_open(bool open);
    bool developer_console_open() const;
    bool developer_show_colliders() const;
    Vector3 player_position() const;
    Dictionary phone_presentation_snapshot() const;
    Dictionary phone_menu_layout_snapshot() const;
    Dictionary hud_presentation_snapshot() const;
    Dictionary upgrade_presentation_snapshot() const;
    Dictionary room_presentation_snapshot(int tile_index) const;
    PackedByteArray field_grass_texture_pixels() const;
    PackedByteArray city_asphalt_texture_pixels() const;
    Array grass_blade_presentation_snapshot(int tile_index) const;
    Array grass_blade_tip_presentation_snapshot(int tile_index) const;
    Array particle_presentation_snapshot() const;
    Dictionary lighting_presentation_snapshot() const;
    Dictionary player_action_snapshot() const;
    Dictionary target_presentation_snapshot(int index) const;
    Dictionary target_soul_presentation_snapshot(int index) const;
    PackedVector3Array target_soul_lattice(int index) const;
    PackedVector3Array target_model_vertices(int index) const;
    Color target_model_color(int index) const;
    Vector3 camera_position() const;
    Vector3 camera_target() const;
    float camera_fov_degrees() const;
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
