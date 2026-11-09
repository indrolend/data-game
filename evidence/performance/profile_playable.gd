extends SceneTree

const WARMUP_FRAMES := 180
const SAMPLE_FRAMES := 600

var variant := "baseline"
var output_path := ""

func _initialize() -> void:
    for argument in OS.get_cmdline_user_args():
        if argument.begins_with("--variant="):
            variant = argument.trim_prefix("--variant=")
        elif argument.begins_with("--output="):
            output_path = argument.trim_prefix("--output=")
    call_deferred("_run")

func _percentile(sorted_values: Array[float], fraction: float) -> float:
    if sorted_values.is_empty():
        return 0.0
    var index := clampi(ceili(float(sorted_values.size()) * fraction) - 1, 0, sorted_values.size() - 1)
    return sorted_values[index]

func _summary(values: Array[float]) -> Dictionary:
    values.sort()
    return {
        "median": _percentile(values, 0.50),
        "p95": _percentile(values, 0.95),
        "worst": values[-1] if not values.is_empty() else 0.0,
    }

func _run() -> void:
    DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
    Engine.max_fps = 0
    RenderingServer.viewport_set_measure_render_time(root.get_viewport_rid(), true)
    var world: Node3D = load("res://scenes/parent_reference.tscn").instantiate()
    root.add_child(world)
    world.agent_playtest = true
    await process_frame
    world.runtime.start_target_interaction_fixture()
    world.configure_performance_profile(variant)
    world._sync()

    var frame_ms: Array[float] = []
    var native_tick_ms: Array[float] = []
    var sync_ms: Array[float] = []
    var room_adapter_ms: Array[float] = []
    var enemy_adapter_ms: Array[float] = []
    var enemy_bridge_only_ms: Array[float] = []
    var enemy_adapter_only_ms: Array[float] = []
    var room_bridge_ms: Array[float] = []
    var particle_adapter_ms: Array[float] = []
    var phone_adapter_ms: Array[float] = []
    var hud_adapter_ms: Array[float] = []
    var process_ms: Array[float] = []
    var physics_ms: Array[float] = []
    var setup_cpu_ms: Array[float] = []
    var viewport_render_cpu_ms: Array[float] = []
    var viewport_render_gpu_ms: Array[float] = []
    var draw_calls: Array[float] = []
    var primitives: Array[float] = []
    var object_count: Array[float] = []
    var static_memory: Array[float] = []
    var worst_frame_index := -1
    var worst_frame_value := -1.0
    var worst_frame_target_state: Array[Dictionary] = []

    for frame in range(WARMUP_FRAMES + SAMPLE_FRAMES):
        var started := Time.get_ticks_usec()
        var cycle := frame % 240
        var move_x := 0.35 if cycle < 120 else -0.35
        var move_z := 0.75
        var vacuum := cycle >= 60 and cycle < 150
        var melee := cycle == 30
        world.agent_step(1, move_x, move_z, 0.18, 0.03, vacuum, cycle < 90,
            false, melee, false, false)
        await process_frame
        if frame < WARMUP_FRAMES:
            continue
        var profile: Dictionary = world.performance_profile_snapshot()
        var measured_frame_ms := float(Time.get_ticks_usec() - started) / 1000.0
        frame_ms.append(measured_frame_ms)
        if measured_frame_ms > worst_frame_value:
            worst_frame_value = measured_frame_ms
            worst_frame_index = frame - WARMUP_FRAMES
            worst_frame_target_state.clear()
            for target_index in range(world.runtime.target_count()):
                var target_state: Dictionary = world.runtime.target_presentation_snapshot(target_index)
                if bool(target_state.alive):
                    worst_frame_target_state.append({
                        "index": target_index,
                        "shell_crack_amount": float(target_state.shell_crack_amount),
                        "soul_morph": float(target_state.pose.soul_morph),
                        "hit_amount": float(target_state.hit_amount),
                    })
        native_tick_ms.append(float(profile.get("native_tick_us", 0)) / 1000.0)
        sync_ms.append(float(profile.get("sync_total_us", 0)) / 1000.0)
        room_adapter_ms.append(float(profile.get("room_adapter_us", 0)) / 1000.0)
        enemy_adapter_ms.append(float(profile.get("enemies_bridge_adapter_us", 0)) / 1000.0)
        enemy_bridge_only_ms.append(float(profile.get("enemy_bridge_us", 0)) / 1000.0)
        enemy_adapter_only_ms.append(float(profile.get("enemy_adapter_us", 0)) / 1000.0)
        room_bridge_ms.append(float(profile.get("room_bridge_us", 0)) / 1000.0)
        particle_adapter_ms.append(float(profile.get("particles_bridge_adapter_us", 0)) / 1000.0)
        phone_adapter_ms.append(float(profile.get("phone_bridge_adapter_us", 0)) / 1000.0)
        hud_adapter_ms.append(float(profile.get("hud_bridge_adapter_us", 0)) / 1000.0)
        process_ms.append(float(Performance.get_monitor(Performance.TIME_PROCESS)) * 1000.0)
        physics_ms.append(float(Performance.get_monitor(Performance.TIME_PHYSICS_PROCESS)) * 1000.0)
        setup_cpu_ms.append(float(RenderingServer.get_frame_setup_time_cpu()))
        viewport_render_cpu_ms.append(float(RenderingServer.viewport_get_measured_render_time_cpu(
            root.get_viewport_rid())))
        viewport_render_gpu_ms.append(float(RenderingServer.viewport_get_measured_render_time_gpu(
            root.get_viewport_rid())))
        draw_calls.append(float(Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME)))
        primitives.append(float(Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME)))
        object_count.append(float(Performance.get_monitor(Performance.OBJECT_COUNT)))
        static_memory.append(float(Performance.get_monitor(Performance.MEMORY_STATIC)) / 1048576.0)

    var result := {
        "schema_version": 1,
        "variant": variant,
        "warmup_frames": WARMUP_FRAMES,
        "sample_frames": SAMPLE_FRAMES,
        "resolution": [DisplayServer.window_get_size().x, DisplayServer.window_get_size().y],
        "renderer": RenderingServer.get_video_adapter_name(),
        "renderer_vendor": RenderingServer.get_video_adapter_vendor(),
        "renderer_type": RenderingServer.get_current_rendering_driver_name(),
        "vsync": "disabled",
        "max_fps": Engine.max_fps,
        "frame_ms": _summary(frame_ms),
        "fps_from_median_frame": 1000.0 / maxf(_percentile(frame_ms, 0.50), 0.0001),
        "native_tick_ms": _summary(native_tick_ms),
        "scene_sync_ms": _summary(sync_ms),
        "room_adapter_ms": _summary(room_adapter_ms),
        "room_bridge_ms": _summary(room_bridge_ms),
        "enemy_bridge_adapter_ms": _summary(enemy_adapter_ms),
        "enemy_bridge_ms": _summary(enemy_bridge_only_ms),
        "enemy_adapter_ms": _summary(enemy_adapter_only_ms),
        "particle_bridge_adapter_ms": _summary(particle_adapter_ms),
        "phone_bridge_adapter_ms": _summary(phone_adapter_ms),
        "hud_bridge_adapter_ms": _summary(hud_adapter_ms),
        "godot_process_ms": _summary(process_ms),
        "godot_physics_ms": _summary(physics_ms),
        "render_setup_cpu_ms": _summary(setup_cpu_ms),
        "viewport_render_cpu_ms": _summary(viewport_render_cpu_ms),
        "viewport_render_gpu_ms": _summary(viewport_render_gpu_ms),
        "draw_calls": _summary(draw_calls),
        "primitives": _summary(primitives),
        "objects": _summary(object_count),
        "static_memory_mib": _summary(static_memory),
        "gpu_timing_supported": _percentile(viewport_render_gpu_ms, 0.50) > 0.0,
        "worst_frame_index": worst_frame_index,
        "worst_frame_target_state": worst_frame_target_state,
    }
    var json := JSON.stringify(result, "  ") + "\n"
    print("PERFORMANCE_RESULT ", json)
    if not output_path.is_empty():
        var file := FileAccess.open(output_path, FileAccess.WRITE)
        if file == null:
            push_error("Unable to write performance result: " + output_path)
            quit(2)
            return
        file.store_string(json)
    world.free()
    quit(0)
