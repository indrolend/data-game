extends SceneTree

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    var world = load("res://scenes/parent_reference.tscn").instantiate()
    root.add_child(world)
    world.agent_playtest = true
    await process_frame
    world.runtime.start_rally_fixture()
    world._sync()
    world.agent_step(1, 0.0, 0.0, 0.0, 0.0, false, false, false, false, true, false)
    world.agent_step(8, 0.0, 0.0, 0.0, 0.0, false, false, false, false, false, false)
    await process_frame
    var native: Array = world.runtime.projectile_presentation_snapshot()
    check(not native.is_empty(), "native bridge exposes a fired projectile")
    if not native.is_empty():
        var projectile: Dictionary = native[0]
        var stable_id := str(projectile.stable_id)
        check(stable_id.begins_with("native-projectile-"), "projectile has a stable native identity")
        check((projectile.velocity as Vector3).length() > 0.0, "projectile exposes authoritative velocity")
        var presentation: Dictionary = world.presentation_observation_snapshot()
        var matches := (presentation.projectiles as Array).filter(
            func(row: Dictionary) -> bool: return str(row.stable_id) == stable_id)
        check(matches.size() == 1, "Godot presentation exposes the same projectile identity")
        if matches.size() == 1:
            check((projectile.position as Vector3).distance_to(matches[0].position) < 0.0001,
                "native and Godot projectile transforms correlate")
            check(bool(matches[0].visible), "correlated projectile presentation is visible")
    world.queue_free()
    await process_frame
    if not failures.is_empty():
        print("RESULT: FAIL count=", failures.size())
        quit(2)
        return
    print("RESULT: PASS synchronized projectile observation")
    quit(0)
