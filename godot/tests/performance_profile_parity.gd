extends SceneTree

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func native_snapshot(world: Node3D) -> Dictionary:
    return {
        "player": world.runtime.player_position(),
        "camera_position": world.runtime.camera_position(),
        "camera_target": world.runtime.camera_target(),
        "camera_fov": world.runtime.camera_fov_degrees(),
        "battery": world.runtime.battery(),
        "souls": world.runtime.souls(),
        "loop": world.runtime.gameplay_loop_snapshot(),
        "phone": world.runtime.phone_presentation_snapshot(),
        "hud": world.runtime.hud_presentation_snapshot(),
    }

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    for variant in ["no_enemies", "no_grass", "no_particles", "no_shadows", "no_hud", "no_phone"]:
        var baseline: Node3D = load("res://scenes/parent_reference.tscn").instantiate()
        var candidate: Node3D = load("res://scenes/parent_reference.tscn").instantiate()
        root.add_child(baseline)
        root.add_child(candidate)
        baseline.agent_playtest = true
        candidate.agent_playtest = true
        await process_frame
        baseline.runtime.start_target_interaction_fixture()
        candidate.runtime.start_target_interaction_fixture()
        baseline.configure_performance_profile("baseline")
        candidate.configure_performance_profile(variant)
        for frame in range(90):
            var cycle := frame % 60
            for world in [baseline, candidate]:
                world.agent_step(1, 0.3 if cycle < 30 else -0.3, 0.7, 0.1, 0.02,
                    cycle >= 15 and cycle < 40, cycle < 25, false, cycle == 10, false, false)
        check(native_snapshot(candidate) == native_snapshot(baseline),
            "%s profiling variant preserves authoritative state" % variant)
        baseline.free()
        candidate.free()
        await process_frame
    if not failures.is_empty():
        print("RESULT: FAIL count=", failures.size())
        quit(2)
        return
    print("RESULT: PASS presentation profiling variants preserve native parity")
    quit(0)
