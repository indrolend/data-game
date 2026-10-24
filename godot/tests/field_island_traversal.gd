extends SceneTree

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    var scene = load("res://scenes/main.tscn").instantiate()
    root.add_child(scene)
    await process_frame
    var scene_runtime = scene.runtime
    var initial: Dictionary = scene_runtime.room_presentation_snapshot(-1)
    check(bool(initial.field_island.enabled),
        "default parent-driven Field stage exposes the native finite-island contract")
    scene._sync()
    var tile := int(initial.tile_index)
    check(scene.room_adapter.structure("room/%d/structure/floor" % tile) != null and
        scene.room_adapter.structure("room/%d/structure/island/core" % tile) != null and
        scene.room_adapter.structure("room/%d/structure/island/bridge-top/front" % tile) != null and
        scene.room_adapter.structure("room/%d/structure/island/bridge-top/back" % tile) != null and
        scene.room_adapter.structure("room/%d/structure/abyss" % tile) != null,
        "Godot materializes the native island top, visible core, bridge, and abyss")

    var runtime = ClassDB.instantiate("ParentGameNode")
    runtime.start_field_island_fixture()
    var behavior_initial: Dictionary = runtime.room_presentation_snapshot(-1)
    var left_ground := false
    var preserved_horizontal_momentum := false
    var recovered := false
    var greatest_x: float = runtime.player_position().x
    var lowest_y: float = runtime.player_position().y
    for frame in range(360):
        runtime.submit_input(1.0, 0.0, 0.0, 0.0, false, false, false, false, false, false)
        runtime.tick()
        var position: Vector3 = runtime.player_position()
        greatest_x = maxf(greatest_x, position.x)
        lowest_y = minf(lowest_y, position.y)
        left_ground = left_ground or (position.x > float(behavior_initial.field_island.main_half_width) and position.y < 0.0)
        preserved_horizontal_momentum = preserved_horizontal_momentum or (position.x > float(behavior_initial.field_island.main_half_width) + 0.5)
        var observed: Dictionary = runtime.room_presentation_snapshot(-1)
        if int(observed.field_island.recovery_count) > 0:
            recovered = true
            break
    var final_position: Vector3 = runtime.player_position()
    var final_snapshot: Dictionary = runtime.room_presentation_snapshot(-1)
    check(left_ground, "native player can walk beyond the finite Field support and fall")
    check(preserved_horizontal_momentum, "unsupported native movement carries momentum beyond the edge")
    check(lowest_y < -4.0, "native gravity visibly carries the player into the abyss")
    check(recovered and int(final_snapshot.field_island.recovery_count) == 1,
        "native abyss threshold performs exactly one deterministic recovery")
    check(final_position.distance_to(Vector3(0.0, 0.08, 15.5)) <= 0.001,
        "recovery returns the player to the native safe Field spawn")
    runtime.free()
    scene.free()
    if not failures.is_empty():
        print("RESULT: FAIL count=", failures.size(), " greatest_x=", greatest_x,
            " lowest_y=", lowest_y, " final=", final_position)
        quit(2)
        return
    print("RESULT: PASS native Field walk-off, unsupported momentum, gravity, recovery, and Godot island presentation")
    quit(0)
