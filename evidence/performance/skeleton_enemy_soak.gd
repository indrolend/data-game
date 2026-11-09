extends SceneTree

const SAMPLE_INTERVAL := 600
var frames := 18000
var output_path := ""
var failures: Array[String] = []
var mesh_failures: Array[String] = []

func _initialize() -> void:
    for argument in OS.get_cmdline_user_args():
        if argument.begins_with("--frames="):
            frames = maxi(1, argument.trim_prefix("--frames=").to_int())
        elif argument.begins_with("--output="):
            output_path = argument.trim_prefix("--output=")
    call_deferred("_run")

func _run() -> void:
    OS.set_environment("DB_DISABLE_SKELETON_ENEMIES", "")
    DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
    Engine.max_fps = 0
    var world: Node3D = load("res://scenes/parent_reference.tscn").instantiate()
    root.add_child(world)
    var control: Node = ClassDB.instantiate("ParentGameNode")
    root.add_child(control)
    await process_frame
    world.set_process(false)
    world.set_physics_process(false)
    var meshes: Array[Mesh] = []
    for target in world.targets:
        meshes.append(target.skinned_mesh.mesh)
    var samples: Array[Dictionary] = []
    for frame in range(frames):
        var cycle := frame % 240
        var values := [0.35 if cycle < 120 else -0.35, 0.75, 0.18, 0.03,
            cycle >= 60 and cycle < 150, cycle < 90, false, cycle == 30, false, false]
        world.runtime.submit_input(values[0], values[1], values[2], values[3], values[4],
            values[5], values[6], values[7], values[8], values[9])
        control.submit_input(values[0], values[1], values[2], values[3], values[4],
            values[5], values[6], values[7], values[8], values[9])
        world.runtime.tick()
        control.tick()
        world._sync()
        if frame % SAMPLE_INTERVAL == 0 or frame == frames - 1:
            if world.runtime.gameplay_loop_snapshot() != control.gameplay_loop_snapshot():
                failures.append("gameplay divergence at frame %d" % frame)
            for index in range(world.targets.size()):
                if world.runtime.target_presentation_snapshot(index) != control.target_presentation_snapshot(index):
                    failures.append("target %d divergence at frame %d" % [index, frame])
                if world.targets[index].skinned_mesh.mesh != meshes[index]:
                    mesh_failures.append("target %d mesh replacement at frame %d" % [index, frame])
            samples.append({
                "frame": frame,
                "static_memory_mib": Performance.get_monitor(Performance.MEMORY_STATIC) / 1048576.0,
                "object_count": int(Performance.get_monitor(Performance.OBJECT_COUNT)),
                "node_count": int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT)),
            })
        if frame % 120 == 0:
            await create_timer(0.0).timeout
    var first: Dictionary = samples[0]
    var last: Dictionary = samples[-1]
    var result := {
        "schema_version": 1,
        "frames": frames,
        "sample_interval": SAMPLE_INTERVAL,
        "native_state_parity": failures.is_empty(),
        "persistent_mesh_identity": mesh_failures.is_empty(),
        "failures": failures,
        "mesh_failures": mesh_failures,
        "samples": samples,
        "memory_delta_mib": float(last.static_memory_mib) - float(first.static_memory_mib),
        "object_delta": int(last.object_count) - int(first.object_count),
        "node_delta": int(last.node_count) - int(first.node_count),
    }
    var json := JSON.stringify(result, "  ") + "\n"
    print("SKELETON_SOAK_RESULT ", json)
    if not output_path.is_empty():
        var file := FileAccess.open(output_path, FileAccess.WRITE)
        if file == null:
            quit(2)
            return
        file.store_string(json)
    world.free()
    control.free()
    quit(0 if failures.is_empty() and mesh_failures.is_empty() else 1)
