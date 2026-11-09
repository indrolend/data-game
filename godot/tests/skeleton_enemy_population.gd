extends SceneTree

var failures: Array[String] = []

func check(value: bool, message: String) -> void:
    if not value:
        failures.append(message)
        push_error("FAIL: " + message)

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    OS.set_environment("DB_DISABLE_SKELETON_ENEMIES", "")
    var world: Node3D = load("res://scenes/parent_reference.tscn").instantiate()
    root.add_child(world)
    world.agent_playtest = true
    var control: Node = ClassDB.instantiate("ParentGameNode")
    root.add_child(control)
    await process_frame
    check(world.skeleton_enemies_enabled, "all-enemy skeleton experiment is enabled")
    check(world.targets.size() == world.runtime.target_count(),
        "one presentation node exists for every native target")
    var persistent_meshes: Array[Mesh] = []
    for index in range(world.targets.size()):
        var target: Node = world.targets[index]
        check(target.get_script().resource_path.ends_with("skeleton_human_model_adapter.gd"),
            "target %d uses the persistent skeleton adapter" % index)
        check(target.skeleton.get_bone_count() == 33,
            "target %d owns the complete native rig" % index)
        persistent_meshes.append(target.skinned_mesh.mesh)
    var baseline_frame: int = world.runtime.frame()
    for frame in range(120):
        world.runtime.submit_input(0.0, 0.0, 0.0, 0.0, false, false,
            false, frame == 30, false, false)
        control.submit_input(0.0, 0.0, 0.0, 0.0, false, false,
            false, frame == 30, false, false)
        world.runtime.tick()
        control.tick()
        world._sync()
        check(world.runtime.gameplay_loop_snapshot() == control.gameplay_loop_snapshot(),
            "presentation leaves native gameplay state identical at frame %d" % frame)
        for index in range(world.targets.size()):
            check(world.targets[index].skinned_mesh.mesh == persistent_meshes[index],
                "target %d keeps persistent mesh identity at frame %d" % [index, frame])
            check(world.runtime.target_presentation_snapshot(index) ==
                    control.target_presentation_snapshot(index),
                "target %d native state remains identical at frame %d" % [index, frame])
    check(world.runtime.frame() == baseline_frame + 120,
        "multi-enemy presentation does not alter deterministic stepping")
    world.queue_free()
    control.queue_free()
    if failures.is_empty():
        print("RESULT: PASS persistent skeleton population")
        quit(0)
    else:
        quit(1)
