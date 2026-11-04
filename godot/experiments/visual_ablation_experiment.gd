extends SceneTree

const OUTPUT := "res://../evidence/generated/visual-ablation"
const WARMUP_FRAMES := 90
const SAMPLE_FRAMES := 360

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func _initialize() -> void:
    call_deferred("_run")

func _percentile(values: Array[float], fraction: float) -> float:
    if values.is_empty():
        return 0.0
    var sorted := values.duplicate()
    sorted.sort()
    var index := clampi(int(ceil(fraction * float(sorted.size()))) - 1, 0, sorted.size() - 1)
    return sorted[index]

func _mean(values: Array[float]) -> float:
    if values.is_empty():
        return 0.0
    var total := 0.0
    for value in values:
        total += value
    return total / float(values.size())

func _vector(value: Vector3) -> Array[float]:
    return [value.x, value.y, value.z]

func _native_digest(world: Node3D) -> Dictionary:
    var loop: Dictionary = world.runtime.gameplay_loop_snapshot()
    return {
        "frame": int(world.runtime.frame()),
        "player": _vector(world.runtime.player_position()),
        "camera_position": _vector(world.runtime.camera_position()),
        "camera_target": _vector(world.runtime.camera_target()),
        "camera_fov": float(world.runtime.camera_fov_degrees()),
        "battery": float(world.runtime.battery()),
        "souls": int(world.runtime.souls()),
        "room_index": int(loop.get("room_index", -1)),
        "room_seed": int(loop.get("room_seed", -1)),
        "living_targets": int(loop.get("living_targets", -1)),
        "captured": int(loop.get("captured", -1)),
    }

func _set_dictionary_nodes_visible(nodes: Dictionary, visible: bool) -> void:
    for node in nodes.values():
        if is_instance_valid(node):
            node.visible = visible

func _set_point_lights_visible(world: Node3D, visible: bool) -> void:
    for light in world.native_point_lights:
        if is_instance_valid(light):
            light.visible = visible

func _set_shadow_casting(node: Node, enabled: bool) -> void:
    if node is GeometryInstance3D:
        node.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_ON if enabled \
            else GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
    for child in node.get_children():
        _set_shadow_casting(child, enabled)

func _set_dictionary_shadow_casting(nodes: Dictionary, enabled: bool) -> void:
    for node in nodes.values():
        if is_instance_valid(node):
            _set_shadow_casting(node, enabled)

func _apply_ablation(world: Node3D, variant: String) -> void:
    var room: Node3D = world.room_adapter
    match variant:
        "no_shadows":
            world.sun_light.shadow_enabled = false
            world.fill_light.shadow_enabled = false
            world.drama_spotlight.shadow_enabled = false
            for light in world.native_point_lights:
                light.shadow_enabled = false
        "no_sun_shadow":
            world.sun_light.shadow_enabled = false
        "no_fill_shadow":
            world.fill_light.shadow_enabled = false
        "no_drama_shadow":
            world.drama_spotlight.shadow_enabled = false
        "no_point_shadows":
            for light in world.native_point_lights:
                light.shadow_enabled = false
        "no_grass_shadows":
            if room._grass_node != null:
                _set_shadow_casting(room._grass_node, false)
        "no_tree_shadows":
            _set_dictionary_shadow_casting(room._tree_nodes, false)
            _set_dictionary_shadow_casting(room._field_backdrop_tree_nodes, false)
        "no_structure_shadows":
            _set_dictionary_shadow_casting(room._nodes, false)
        "no_house_shadows":
            _set_dictionary_shadow_casting(room._house_nodes, false)
        "no_city_facade_shadows":
            _set_dictionary_shadow_casting(room._city_facade_nodes, false)
        "no_enemy_shadows":
            for target in world.targets:
                _set_shadow_casting(target, false)
        "freeze_static_geometry":
            room.set_static_geometry_sync_enabled(false)
        "no_sun":
            world.sun_light.visible = false
        "no_fill":
            world.fill_light.visible = false
        "no_drama_spot":
            world.drama_spotlight.visible = false
        "no_point_lights":
            _set_point_lights_visible(world, false)
        "no_ambient":
            world.scene_environment.ambient_light_energy = 0.0
        "no_fog":
            world.scene_environment.fog_enabled = false
        "no_structures":
            _set_dictionary_nodes_visible(room._nodes, false)
        "no_grass":
            if room._grass_node != null:
                room._grass_node.visible = false
        "no_trees":
            _set_dictionary_nodes_visible(room._tree_nodes, false)
            _set_dictionary_nodes_visible(room._field_backdrop_tree_nodes, false)
        "no_rocks":
            _set_dictionary_nodes_visible(room._rock_nodes, false)
        "no_lawns":
            _set_dictionary_nodes_visible(room._lawn_nodes, false)
        "no_houses":
            _set_dictionary_nodes_visible(room._house_nodes, false)
        "no_markers":
            _set_dictionary_nodes_visible(room._marker_nodes, false)
        "no_ruins":
            _set_dictionary_nodes_visible(room._ruin_nodes, false)
        "no_city_facades":
            _set_dictionary_nodes_visible(room._city_facade_nodes, false)
        "no_capture_points":
            _set_dictionary_nodes_visible(room._capture_point_nodes, false)

func _family_counts(world: Node3D) -> Dictionary:
    var room: Node3D = world.room_adapter
    return {
        "structures": room._nodes.size(),
        "grass_blades": room.grass_blade_count(),
        "trees": room._tree_nodes.size(),
        "backdrop_trees": room._field_backdrop_tree_nodes.size(),
        "rocks": room._rock_nodes.size(),
        "lawns": room._lawn_nodes.size(),
        "houses": room._house_nodes.size(),
        "markers": room._marker_nodes.size(),
        "ruins": room._ruin_nodes.size(),
        "city_facades": room._city_facade_nodes.size(),
        "capture_points": room._capture_point_nodes.size(),
        "point_lights": world.native_point_lights.size(),
    }

func _shadow_state(world: Node3D) -> Dictionary:
    var point_states: Array[bool] = []
    for light in world.native_point_lights:
        point_states.append(bool(light.shadow_enabled))
    return {
        "sun": bool(world.sun_light.shadow_enabled),
        "fill": bool(world.fill_light.shadow_enabled),
        "drama_spot": bool(world.drama_spotlight.shadow_enabled),
        "point_lights": point_states,
    }

func _has_relevant_family(counts: Dictionary, variant: String) -> bool:
    var keys := {
        "no_structures": ["structures"],
        "no_grass": ["grass_blades"],
        "no_trees": ["trees", "backdrop_trees"],
        "no_rocks": ["rocks"],
        "no_lawns": ["lawns"],
        "no_houses": ["houses"],
        "no_markers": ["markers"],
        "no_ruins": ["ruins"],
        "no_city_facades": ["city_facades"],
        "no_capture_points": ["capture_points"],
        "no_point_lights": ["point_lights"],
    }
    if not keys.has(variant):
        return true
    for key in keys[variant]:
        if int(counts.get(key, 0)) > 0:
            return true
    return false

func _capture(path: String) -> void:
    await RenderingServer.frame_post_draw
    var image := root.get_texture().get_image()
    check(image != null and not image.is_empty(), "capture framebuffer for %s" % path)
    if image != null and not image.is_empty():
        check(image.save_png(path) == OK, "save %s" % path)

func _run_trial(scenario: Dictionary, variant: String) -> Dictionary:
    var world: Node3D = load("res://scenes/parent_reference.tscn").instantiate()
    world.agent_playtest = true
    root.add_child(world)
    await process_frame
    world.agent_playtest = true
    world.runtime.call(str(scenario.fixture))
    world.performance_profile_enabled = true
    world.performance_profile_variant = "baseline"
    var construction_started := Time.get_ticks_usec()
    world._sync()
    var fixture_sync_us := Time.get_ticks_usec() - construction_started
    var production_shadow_state := _shadow_state(world)
    _apply_ablation(world, variant)
    var counts := _family_counts(world)
    var initial_digest := _native_digest(world)

    for frame in range(WARMUP_FRAMES):
        var cycle := frame % 90
        world.agent_step(1, 0.22 if cycle < 45 else -0.22, 0.55, 0.06, 0.01,
            cycle >= 20 and cycle < 45, cycle < 30, false, cycle == 12, false, false)
        _apply_ablation(world, variant)
        await RenderingServer.frame_post_draw

    var frame_ms: Array[float] = []
    var sync_us: Array[float] = []
    var room_bridge_us: Array[float] = []
    var room_adapter_us: Array[float] = []
    var draw_calls: Array[float] = []
    var primitives: Array[float] = []
    for frame in range(SAMPLE_FRAMES):
        var cycle := (frame + WARMUP_FRAMES) % 90
        var started := Time.get_ticks_usec()
        world.agent_step(1, 0.22 if cycle < 45 else -0.22, 0.55, 0.06, 0.01,
            cycle >= 20 and cycle < 45, cycle < 30, false, cycle == 12, false, false)
        _apply_ablation(world, variant)
        await RenderingServer.frame_post_draw
        frame_ms.append(float(Time.get_ticks_usec() - started) / 1000.0)
        var profile: Dictionary = world.performance_profile_snapshot()
        sync_us.append(float(profile.get("sync_total_us", 0.0)))
        room_bridge_us.append(float(profile.get("room_bridge_us", 0.0)))
        room_adapter_us.append(float(profile.get("room_adapter_us", 0.0)))
        draw_calls.append(Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME))
        primitives.append(Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME))

    var capture_path := ProjectSettings.globalize_path("%s/%s-%s.png" % [
        OUTPUT, str(scenario.name), variant])
    await _capture(capture_path)
    var final_digest := _native_digest(world)
    var result := {
        "scenario": str(scenario.name),
        "fixture": str(scenario.fixture),
        "variant": variant,
        "family_counts_before_ablation": counts,
        "production_shadow_state": production_shadow_state,
        "fixture_first_sync_us": fixture_sync_us,
        "initial_native_digest": initial_digest,
        "final_native_digest": final_digest,
        "frame_ms": {
            "median": _percentile(frame_ms, 0.50),
            "p95": _percentile(frame_ms, 0.95),
            "worst": _percentile(frame_ms, 1.0),
        },
        "sync_us": {
            "median": _percentile(sync_us, 0.50),
            "p95": _percentile(sync_us, 0.95),
        },
        "room_bridge_us_median": _percentile(room_bridge_us, 0.50),
        "room_adapter_us_median": _percentile(room_adapter_us, 0.50),
        "draw_calls_mean": _mean(draw_calls),
        "primitives_mean": _mean(primitives),
        "capture": capture_path,
    }
    world.free()
    await process_frame
    await process_frame
    return result

func _run() -> void:
    var output_path := ProjectSettings.globalize_path(OUTPUT)
    DirAccess.make_dir_recursive_absolute(output_path)
    var scenarios := [
        {
            "name": "field",
            "fixture": "start_ruin_room_fixture",
            "variants": ["baseline", "freeze_static_geometry", "no_shadows", "no_sun_shadow", "no_fill_shadow",
                "no_drama_shadow", "no_point_shadows", "no_grass_shadows", "no_tree_shadows",
                "no_structure_shadows", "no_enemy_shadows", "no_sun", "no_fill", "no_drama_spot",
                "no_point_lights", "no_ambient", "no_fog", "no_structures", "no_grass",
                "no_trees", "no_rocks", "no_lawns", "no_houses", "no_markers", "no_ruins",
                "no_capture_points"],
        },
        {
            "name": "city",
            "fixture": "start_city_room_fixture",
            "variants": ["baseline", "freeze_static_geometry", "no_shadows", "no_sun_shadow", "no_fill_shadow",
                "no_drama_shadow", "no_point_shadows", "no_structure_shadows",
                "no_house_shadows", "no_city_facade_shadows", "no_enemy_shadows",
                "no_sun", "no_fill", "no_drama_spot",
                "no_point_lights", "no_ambient", "no_fog", "no_structures", "no_trees",
                "no_houses", "no_city_facades", "no_capture_points"],
        },
    ]
    if "--focused-static" in OS.get_cmdline_user_args():
        for scenario in scenarios:
            scenario["variants"] = ["baseline", "freeze_static_geometry"]
    var results: Array[Dictionary] = []
    for scenario in scenarios:
        var baseline_final: Dictionary = {}
        for variant in scenario.variants:
            var result := await _run_trial(scenario, variant)
            if not _has_relevant_family(result.family_counts_before_ablation, variant):
                print("SKIP %s/%s absent family" % [scenario.name, variant])
                continue
            if variant == "baseline":
                baseline_final = result.final_native_digest
            else:
                check(result.final_native_digest == baseline_final,
                    "%s/%s preserves identical native outcome" % [scenario.name, variant])
            results.append(result)
            print("ABLATION %s/%s median=%.3fms p95=%.3fms draw_calls=%.1f" % [
                scenario.name, variant, result.frame_ms.median, result.frame_ms.p95,
                result.draw_calls_mean])

    var evidence := {
        "schema": "digital-breakdown.visual-ablation/v1",
        "engine": Engine.get_version_info(),
        "warmup_frames": WARMUP_FRAMES,
        "sample_frames": SAMPLE_FRAMES,
        "fixed_input_cycle_frames": 90,
        "results": results,
        "failures": failures,
    }
    var file := FileAccess.open(output_path.path_join("summary.json"), FileAccess.WRITE)
    check(file != null, "open visual ablation summary")
    if file != null:
        file.store_string(JSON.stringify(evidence, "  ") + "\n")
        file.close()
    if failures.is_empty():
        print("RESULT: PASS visual ablations preserve native authority; output=", output_path)
        quit(0)
    else:
        print("RESULT: FAIL count=", failures.size())
        quit(2)
