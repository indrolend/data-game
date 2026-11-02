extends SceneTree

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    var world: Node3D = load("res://scenes/parent_reference.tscn").instantiate()
    root.add_child(world)
    world.agent_playtest = true
    await process_frame
    world.runtime.start_target_interaction_fixture()
    world._sync()
    var meshes: Array[Mesh] = []
    check(world.packed_enemies_enabled, "packed enemy presentation is the default")
    check(world.targets.size() == world.runtime.target_count(),
        "presentation owns one adapter per native target")
    for index in range(world.targets.size()):
        var adapter: Node3D = world.targets[index]
        check(adapter.has_method("sync_packed_from_native"),
            "target %d uses the packed adapter" % index)
        var mesh: Mesh = adapter.mesh_instance.mesh
        if world.runtime.target_alive(index):
            check(mesh != null and mesh.get_surface_count() == 1,
                "live target %d owns one persistent surface" % index)
        else:
            check(mesh == null, "dead target %d avoids model construction" % index)
        meshes.append(mesh)
    for frame in range(90):
        world.agent_step(1, 0.25 if frame < 45 else -0.25, 0.65, 0.08, 0.01,
            frame > 20 and frame < 55, frame < 40, false, frame == 15, false, false)
        for index in range(world.targets.size()):
            if meshes[index] != null:
                check(world.targets[index].mesh_instance.mesh == meshes[index],
                    "target %d keeps its surface on animation frame %d" % [index, frame])
    world.free()
    if failures.is_empty():
        print("RESULT: PASS all targets use persistent packed enemy surfaces")
        quit(0)
    else:
        print("RESULT: FAIL %d checks" % failures.size())
        quit(2)
