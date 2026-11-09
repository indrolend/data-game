extends SceneTree

const WARMUP_FRAMES := 180
const SAMPLE_FRAMES := 600

var premise_index := 0
var output_path := ""
var capture_path := ""

func _initialize() -> void:
    for argument in OS.get_cmdline_user_args():
        if argument.begins_with("--premise="):
            premise_index = clampi(argument.trim_prefix("--premise=").to_int(), 0, 7)
        elif argument.begins_with("--output="):
            output_path = argument.trim_prefix("--output=")
        elif argument.begins_with("--capture="):
            capture_path = argument.trim_prefix("--capture=")
    call_deferred("_run")

func _percentile(values: Array[float], fraction: float) -> float:
    if values.is_empty():
        return 0.0
    values.sort()
    return values[clampi(ceili(values.size() * fraction) - 1, 0, values.size() - 1)]

func _summary(values: Array[float]) -> Dictionary:
    return {
        "median": _percentile(values.duplicate(), 0.50),
        "p95": _percentile(values.duplicate(), 0.95),
        "worst": _percentile(values.duplicate(), 1.0),
    }

func _run() -> void:
    DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
    Engine.max_fps = 0
    RenderingServer.viewport_set_measure_render_time(root.get_viewport_rid(), true)
    var world: Node3D = load("res://scenes/parent_reference.tscn").instantiate()
    root.add_child(world)
    world.agent_playtest = true
    await process_frame
    world.runtime.start_room_inspector()
    for index in range(premise_index):
        world.runtime.step_room_inspector(1, false)
    world.configure_performance_profile("baseline")
    world._sync()
    await process_frame
    var inspector: Dictionary = world.runtime.room_inspector_snapshot()
    var frame_ms: Array[float] = []
    var room_adapter_ms: Array[float] = []
    var room_bridge_ms: Array[float] = []
    var render_cpu_ms: Array[float] = []
    var render_gpu_ms: Array[float] = []
    var draw_calls: Array[float] = []
    var primitives: Array[float] = []
    var objects: Array[float] = []
    var memory_mib: Array[float] = []
    for frame in range(WARMUP_FRAMES + SAMPLE_FRAMES):
        var started := Time.get_ticks_usec()
        world.agent_step(1, 0.0, 0.0, 0.12, 0.0, false, false, false, false, false, false)
        await process_frame
        if frame == WARMUP_FRAMES - 1 and not capture_path.is_empty():
            root.get_viewport().get_texture().get_image().save_png(capture_path)
        if frame < WARMUP_FRAMES:
            continue
        var profile: Dictionary = world.performance_profile_snapshot()
        frame_ms.append((Time.get_ticks_usec() - started) / 1000.0)
        room_adapter_ms.append(float(profile.get("room_adapter_us", 0)) / 1000.0)
        room_bridge_ms.append(float(profile.get("room_bridge_us", 0)) / 1000.0)
        render_cpu_ms.append(RenderingServer.viewport_get_measured_render_time_cpu(root.get_viewport_rid()))
        render_gpu_ms.append(RenderingServer.viewport_get_measured_render_time_gpu(root.get_viewport_rid()))
        draw_calls.append(Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME))
        primitives.append(Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME))
        objects.append(Performance.get_monitor(Performance.OBJECT_COUNT))
        memory_mib.append(Performance.get_monitor(Performance.MEMORY_STATIC) / 1048576.0)
    var result := {
        "schema_version": 1,
        "premise_index": premise_index,
        "premise_name": str(inspector.premise_name),
        "seed": int(inspector.seed),
        "room_index": int(inspector.room_index),
        "setting": str(inspector.setting_name),
        "form": str(inspector.form_name),
        "scale": str(inspector.scale_name),
        "condition": str(inspector.condition_name),
        "traversal": str(inspector.traversal_intent),
        "route_valid": bool(inspector.route_valid),
        "surfaces": int(inspector.surface_count),
        "edges": int(inspector.edge_count),
        "colliders": int(inspector.collider_count),
        "props": int(inspector.presentation_prop_count),
        "visible_estimate": int(inspector.visible_primitive_estimate),
        "draw_bucket": int(inspector.draw_call_bucket),
        "warmup_frames": WARMUP_FRAMES,
        "sample_frames": SAMPLE_FRAMES,
        "resolution": [DisplayServer.window_get_size().x, DisplayServer.window_get_size().y],
        "renderer": RenderingServer.get_video_adapter_name(),
        "frame_ms": _summary(frame_ms),
        "room_adapter_ms": _summary(room_adapter_ms),
        "room_bridge_ms": _summary(room_bridge_ms),
        "viewport_render_cpu_ms": _summary(render_cpu_ms),
        "viewport_render_gpu_ms": _summary(render_gpu_ms),
        "draw_calls": _summary(draw_calls),
        "primitives": _summary(primitives),
        "objects": _summary(objects),
        "static_memory_mib": _summary(memory_mib),
    }
    var json := JSON.stringify(result, "  ") + "\n"
    print("ROOM_PROFILE_RESULT ", json)
    if not output_path.is_empty():
        var file := FileAccess.open(output_path, FileAccess.WRITE)
        if file == null:
            quit(2)
            return
        file.store_string(json)
    world.free()
    quit(0)
