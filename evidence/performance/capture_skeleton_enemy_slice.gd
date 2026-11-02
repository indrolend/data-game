extends SceneTree

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    var world: Node3D = load("res://scenes/parent_reference.tscn").instantiate()
    root.add_child(world)
    world.agent_playtest = true
    await process_frame
    world.runtime.start_target_fixture()
    for _frame in range(90):
        world.agent_step(1, 0.0, 0.7, 0.0, 0.0, false, false,
            false, false, false, false)
        await process_frame
    await RenderingServer.frame_post_draw
    root.get_texture().get_image().save_png(
        "res://../evidence/performance/godot-native-convergence/skeleton-slice.png")
    quit(0)
