extends SceneTree

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    var world: Node3D = load("res://scenes/parent_reference.tscn").instantiate()
    root.add_child(world)
    world.agent_playtest = true
    await process_frame
    world.runtime.start_target_interaction_fixture()
    world._sync()
    await process_frame
    var vertex_counts: Array[int] = []
    for index in range(world.runtime.target_count()):
        vertex_counts.append(world.runtime.target_model_vertices(index).size())
    var result := {
        "target_count": world.runtime.target_count(),
        "target_vertex_counts": vertex_counts,
        "target_vertices_per_frame": vertex_counts.reduce(func(total, count): return total + count, 0),
        "grass_blades": world.room_adapter.grass_blade_count(),
        "particles": world.particle_adapter.presentation_counts(),
        "scene_nodes": Performance.get_monitor(Performance.OBJECT_NODE_COUNT),
        "scene_objects": Performance.get_monitor(Performance.OBJECT_COUNT),
        "draw_calls": Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME),
        "primitives": Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME),
        "directional_shadow_enabled": world.sun_light.shadow_enabled,
        "spot_shadow_enabled": world.drama_spotlight.shadow_enabled,
    }
    var output := JSON.stringify(result, "  ") + "\n"
    print(output)
    var file := FileAccess.open("res://../evidence/performance/scene-inventory.json", FileAccess.WRITE)
    file.store_string(output)
    world.free()
    quit(0)
