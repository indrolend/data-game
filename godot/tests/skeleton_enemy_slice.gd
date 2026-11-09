extends SceneTree

const SkeletonAdapter = preload("res://scripts/skeleton_human_model_adapter.gd")
var failures: Array[String] = []

func check(value: bool, message: String) -> void:
    if not value:
        failures.append(message)
        push_error("FAIL: " + message)

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    var runtime = ClassDB.instantiate("ParentGameNode")
    var control = ClassDB.instantiate("ParentGameNode")
    root.add_child(runtime)
    root.add_child(control)
    runtime.start_target_fixture()
    control.start_target_fixture()
    var rig: Dictionary = runtime.human_model_rig()
    check((rig.positions as PackedVector3Array).size() == 4164, "complete bind mesh is transferred once")
    check((rig.parents as PackedInt32Array).size() == 33, "complete native rig has 33 bones")
    var adapter := SkeletonAdapter.new()
    adapter.configure_from_native(0, runtime.target_model_color(0), rig)
    root.add_child(adapter)
    var persistent_mesh: Mesh = adapter.skinned_mesh.mesh
    var authoritative_before := {
        "player": runtime.player_position(), "souls": runtime.souls(),
        "battery": runtime.battery(), "frame": runtime.frame()
    }
    for frame in range(120):
        runtime.submit_input(0.0, 0.0, 0.0, 0.0, frame > 60, false,
            false, frame == 30, false, false)
        control.submit_input(0.0, 0.0, 0.0, 0.0, frame > 60, false,
            false, frame == 30, false, false)
        runtime.tick()
        control.tick()
        var snapshot: Dictionary = runtime.target_presentation_snapshot(0)
        adapter.sync_pose_from_native(snapshot, runtime.target_model_pose(0),
            runtime.target_model_foot_corrections(0))
        check(adapter.skinned_mesh.mesh == persistent_mesh,
            "persistent mesh identity survives frame %d" % frame)
        check(runtime.target_presentation_snapshot(0) == control.target_presentation_snapshot(0),
            "pose observation cannot diverge native target state at frame %d" % frame)
        check(runtime.gameplay_loop_snapshot() == control.gameplay_loop_snapshot(),
            "pose observation cannot diverge gameplay state at frame %d" % frame)
    check(adapter.skeleton.get_bone_count() == 33, "Skeleton3D owns the production rig")
    check((runtime.target_model_pose(0) as Array).size() == 33, "native boundary returns compact pose")
    check(runtime.frame() == int(authoritative_before.frame) + 120,
        "presentation does not alter deterministic simulation stepping")
    adapter.queue_free()
    runtime.queue_free()
    control.queue_free()
    if failures.is_empty():
        print("RESULT: PASS persistent Godot Skeleton3D enemy slice")
        quit(0)
    else:
        quit(1)
