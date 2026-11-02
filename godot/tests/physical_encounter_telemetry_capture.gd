extends SceneTree

const DEFAULT_OUTPUT := "res://../evidence/generated/scene-telemetry-v2"
const FRAME_COUNT := 1200
const MELEE_START := 300
const MELEE_CADENCE := 50

var failures: Array[String] = []
var world: Node3D

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func vector3_json(value: Vector3) -> Array[float]:
    return [value.x, value.y, value.z]

func basis_json(value: Basis) -> Array[Array]:
    return [vector3_json(value.x), vector3_json(value.y), vector3_json(value.z)]

func transform_json(value: Transform3D) -> Dictionary:
    return {"basis_columns": basis_json(value.basis), "origin": vector3_json(value.origin)}

func json_value(value: Variant) -> Variant:
    if value is Vector3:
        return vector3_json(value)
    if value is Vector2:
        return [value.x, value.y]
    if value is Transform3D:
        return transform_json(value)
    if value is Basis:
        return basis_json(value)
    if value is Color:
        return [value.r, value.g, value.b, value.a]
    if value is Dictionary:
        var result := {}
        for key in value:
            result[str(key)] = json_value(value[key])
        return result
    if value is Array:
        var result := []
        for item in value:
            result.append(json_value(item))
        return result
    return value

func valid_transform(value: Transform3D) -> bool:
    return value.origin.is_finite() and value.basis.x.is_finite() \
        and value.basis.y.is_finite() and value.basis.z.is_finite()

func transform_error(a: Transform3D, b: Transform3D) -> float:
    return maxf(a.origin.distance_to(b.origin), maxf(a.basis.x.distance_to(b.basis.x),
        maxf(a.basis.y.distance_to(b.basis.y), a.basis.z.distance_to(b.basis.z))))

func pose_json(pose: Array) -> Array[Dictionary]:
    var result: Array[Dictionary] = []
    for bone_index in range(pose.size()):
        var value: Transform3D = pose[bone_index]
        check(valid_transform(value), "native bone %d transform is finite" % bone_index)
        result.append({"index": bone_index, "local_transform": transform_json(value)})
    return result

func skeleton_pose_json(skeleton: Skeleton3D) -> Array[Dictionary]:
    var result: Array[Dictionary] = []
    for bone_index in range(skeleton.get_bone_count()):
        var value := skeleton.get_bone_pose(bone_index)
        check(valid_transform(value), "Godot bone %d transform is finite" % bone_index)
        result.append({"index": bone_index, "local_transform": transform_json(value)})
    return result

func count_skeleton_bones(node: Node) -> int:
    var result: int = node.get_bone_count() if node is Skeleton3D else 0
    for child in node.get_children():
        result = maxi(result, count_skeleton_bones(child))
    return result

func source_revision() -> Dictionary:
    var output: Array = []
    var exit_code := OS.execute("git", ["-C", ProjectSettings.globalize_path("res://.."),
        "rev-parse", "HEAD"], output, true)
    var revision := str(output[0]).strip_edges() if exit_code == 0 and not output.is_empty() else "unknown"
    output.clear()
    exit_code = OS.execute("git", ["-C", ProjectSettings.globalize_path("res://.."),
        "status", "--porcelain"], output, true)
    return {"commit": revision, "dirty": exit_code != 0 or not "".join(output).strip_edges().is_empty()}

func output_root() -> String:
    for argument in OS.get_cmdline_user_args():
        if argument.begins_with("--output="):
            return "res://../evidence/generated/" + argument.trim_prefix("--output=")
    return DEFAULT_OUTPUT

func add_world_geometry(export_root: Node3D, room_snapshot: Dictionary) -> int:
    var geometry_root := Node3D.new()
    geometry_root.name = "NativeRoomStructureGeometry"
    export_root.add_child(geometry_root)
    var count := 0
    for record in room_snapshot.structures as Array:
        var mesh := MeshInstance3D.new()
        mesh.name = str(record.id).replace("/", "_")
        var box := BoxMesh.new()
        box.size = record.size
        mesh.mesh = box
        mesh.position = record.center
        var material := StandardMaterial3D.new()
        material.albedo_color = record.material.base_color
        material.roughness = 0.8
        mesh.material_override = material
        geometry_root.add_child(mesh)
        count += 1
    return count

func export_glb(enemy: Node3D, room_snapshot: Dictionary, path: String) -> int:
    var export_root := Node3D.new()
    export_root.name = "PhysicalEncounterEvidence"
    var export_rig: Node3D = enemy.rig_root.duplicate()
    export_rig.name = "CapturedNativeBackedEnemy"
    export_rig.transform = enemy.transform * enemy.rig_root.transform
    export_root.add_child(export_rig)
    var structure_count := add_world_geometry(export_root, room_snapshot)
    var camera_copy := Camera3D.new()
    camera_copy.name = "CapturedCamera"
    camera_copy.transform = world.camera.global_transform
    camera_copy.fov = world.camera.fov
    export_root.add_child(camera_copy)
    var document := GLTFDocument.new()
    var state := GLTFState.new()
    var append_error := document.append_from_scene(export_root, state)
    check(append_error == OK, "append encounter scene to GLTF state")
    if append_error == OK:
        var write_error := document.write_to_filesystem(state, path)
        check(write_error == OK and FileAccess.file_exists(path), "write scene.glb")
        if write_error == OK:
            var reopened_state := GLTFState.new()
            var reopen_error := document.append_from_file(path, reopened_state)
            check(reopen_error == OK, "reopen scene.glb")
            if reopen_error == OK:
                var reopened := document.generate_scene(reopened_state)
                check(reopened != null, "generate scene from reopened GLB")
                if reopened != null:
                    check(count_skeleton_bones(reopened) == 33, "reopened GLB rig has 33 bones")
                    reopened.free()
    export_root.free()
    return structure_count

func capture_render(path: String) -> void:
    await process_frame
    await RenderingServer.frame_post_draw
    var image := root.get_texture().get_image()
    check(image != null and not image.is_empty(), "graphical framebuffer is available for " + path)
    if image != null and not image.is_empty():
        check(image.save_png(path) == OK, "save graphical capture " + path)

func file_size(path: String) -> int:
    var file := FileAccess.open(path, FileAccess.READ)
    if file == null:
        return -1
    var size := file.get_length()
    file.close()
    return size

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    var control = ClassDB.instantiate("ParentGameNode")
    root.add_child(control)
    control.start_target_interaction_fixture()
    var target_index := -1
    var nearest_distance := INF
    var player_position: Vector3 = control.player_position()
    for index in range(control.target_count()):
        if control.target_alive(index):
            var distance: float = control.target_position(index).distance_squared_to(player_position)
            if distance < nearest_distance:
                nearest_distance = distance
                target_index = index
    check(target_index >= 0, "existing encounter fixture supplies a live enemy")
    OS.set_environment("DB_ENABLE_SKELETON_ENEMY_SLICE", "1")
    OS.set_environment("DB_SKELETON_ENEMY_INDEX", str(target_index))
    var output_path := ProjectSettings.globalize_path(output_root())
    DirAccess.make_dir_recursive_absolute(output_path)
    var captures_path := output_path.path_join("captures")
    DirAccess.make_dir_recursive_absolute(captures_path)
    var frames_path := output_path.path_join("frames.jsonl")
    var glb_path := output_path.path_join("scene.glb")
    var render_path := output_path.path_join("render.png")
    var metadata_path := output_path.path_join("metadata.json")

    world = load("res://scenes/parent_reference.tscn").instantiate()
    world.agent_playtest = true
    root.add_child(world)
    await process_frame
    # _ready() derives this flag from process arguments, so assert the capture
    # ownership after the node has entered the tree as well as before it.
    world.agent_playtest = true
    check(world.skeleton_enemy_slice_enabled, "skeleton enemy slice is enabled")
    world.runtime.start_target_interaction_fixture()
    world._sync()

    var enemy: Node3D = world.targets[target_index]
    var skeleton: Skeleton3D = enemy.skeleton
    check(skeleton.get_bone_count() == 33, "live encounter rig has 33 bones")

    var frames_file := FileAccess.open(frames_path, FileAccess.WRITE)
    check(frames_file != null, "open frames.jsonl")
    var initial_frame := int(world.runtime.frame())
    var previous_frame := initial_frame
    var initial_physical: Dictionary = world.runtime.target_physical_snapshot(target_index)
    var initial_position: Vector3 = initial_physical.position
    var initial_armor := float(world.runtime.target_presentation_snapshot(target_index).armor)
    var previous_position := initial_position
    var previous_velocity: Vector3 = initial_physical.linear_velocity
    var previous_armor := initial_armor
    var impact_frame := -1
    var recovery_frame := -1
    var movement_frame := -1
    var grounded_frame := -1
    var maximum_displacement := 0.0
    var maximum_speed := 0.0
    var maximum_impact_instability := 0.0
    var maximum_disruption_after_impact := 0.0
    var maximum_knockback_delta := 0.0
    var maximum_bone_transform_error := 0.0
    var event_captures := {}
    var world_step_us := 0
    var control_step_us := 0
    var telemetry_us := 0
    var write_us := 0
    var melee_armed := true

    for capture_index in range(FRAME_COUNT):
        var move_x := 0.0
        var move_z := 0.0
        var melee := melee_armed and capture_index >= MELEE_START \
            and (capture_index - MELEE_START) % MELEE_CADENCE == 0
        var step_started := Time.get_ticks_usec()
        world.agent_step(1, move_x, move_z, 0.0, 0.0, false, false,
            false, melee, false, false)
        world_step_us += Time.get_ticks_usec() - step_started
        step_started = Time.get_ticks_usec()
        control.submit_input(move_x, move_z, 0.0, 0.0, false, false,
            false, melee, false, false)
        control.tick()
        control_step_us += Time.get_ticks_usec() - step_started

        var telemetry_started := Time.get_ticks_usec()
        var frame_id := int(world.runtime.frame())
        check(frame_id == previous_frame + 1,
            "native frame identifier advances once at capture %d" % capture_index)
        previous_frame = frame_id
        var snapshot: Dictionary = world.runtime.target_presentation_snapshot(target_index)
        var physical: Dictionary = world.runtime.target_physical_snapshot(target_index)
        var native_pose: Array = world.runtime.target_model_pose(target_index)
        var position: Vector3 = physical.position
        var velocity: Vector3 = physical.linear_velocity
        var armor := float(snapshot.armor)
        var displacement := position.distance_to(initial_position)
        maximum_displacement = maxf(maximum_displacement, displacement)
        maximum_speed = maxf(maximum_speed, velocity.length())
        maximum_impact_instability = maxf(maximum_impact_instability, float(physical.impact_instability))
        if movement_frame < 0 and displacement > 0.05:
            movement_frame = frame_id
        var ground: Dictionary = physical.ground_support
        if grounded_frame < 0 and bool(ground.available) \
                and absf(float(ground.vertical_separation)) <= 0.06 \
                and float(physical.support_contact) > 0.0:
            grounded_frame = frame_id
        var impact_now := armor < previous_armor - 0.000001
        if impact_now and impact_frame < 0:
            impact_frame = frame_id
            melee_armed = false
        if impact_frame >= 0:
            maximum_disruption_after_impact = maxf(maximum_disruption_after_impact,
                float(physical.disruption))
            maximum_knockback_delta = maxf(maximum_knockback_delta,
                (velocity - previous_velocity).length())
            if recovery_frame < 0 and frame_id >= impact_frame + 30 \
                    and float(snapshot.hit_amount) < 0.08 \
                    and float(physical.impact_instability) < 0.08 \
                    and not bool(physical.fallen) \
                    and bool(ground.available) and absf(float(ground.vertical_separation)) <= 0.06:
                recovery_frame = frame_id
        check(native_pose.size() == 33, "native frame %d has 33 bone transforms" % frame_id)
        check(skeleton.get_bone_count() == 33, "Godot frame %d has 33 bone transforms" % frame_id)
        for bone_index in range(mini(native_pose.size(), skeleton.get_bone_count())):
            var bone_error := transform_error(native_pose[bone_index], skeleton.get_bone_pose(bone_index))
            maximum_bone_transform_error = maxf(maximum_bone_transform_error, bone_error)
            check(bone_error < 0.001,
                "native and Godot bone %d agree within 0.001 at frame %d" % [bone_index, frame_id])
        check(world.runtime.gameplay_loop_snapshot() == control.gameplay_loop_snapshot(),
            "encounter capture preserves gameplay state at frame %d" % frame_id)
        check(snapshot == control.target_presentation_snapshot(target_index),
            "encounter capture preserves target presentation at frame %d" % frame_id)
        check(physical == control.target_physical_snapshot(target_index),
            "encounter capture preserves physical target state at frame %d" % frame_id)

        var events: Array[String] = []
        if frame_id == grounded_frame:
            events.append("grounded_observed")
        if frame_id == movement_frame:
            events.append("movement_observed")
        if impact_now:
            events.append("gameplay_impact_observed")
        if frame_id == recovery_frame:
            events.append("recovery_observed")
        var record := {
            "capture_index": capture_index,
            "native_frame_id": frame_id,
            "events": events,
            "input": {"move_x": move_x, "move_z": move_z, "melee": melee},
            "authority": {
                "classification": "native_authoritative",
                "enemy": physical,
                "gameplay": {
                    "alive": snapshot.alive,
                    "slurpable": snapshot.slurpable,
                    "armor": snapshot.armor,
                    "health": snapshot.health,
                    "hit_amount": snapshot.hit_amount,
                    "hit_direction_local": snapshot.hit_direction_local,
                    "attack_active": snapshot.attack_active,
                    "attack_hit": snapshot.attack_hit,
                    "body_fallen": snapshot.body_fallen,
                    "reaction": snapshot.reaction,
                    "player_action": world.runtime.player_action_snapshot(),
                },
                "camera": {
                    "position": world.runtime.camera_position(),
                    "target": world.runtime.camera_target(),
                    "fov_degrees": world.runtime.camera_fov_degrees(),
                },
                "room": {
                    "seed": int(world.runtime.room_presentation_snapshot(-1).room_seed),
                    "index": int(world.runtime.room_presentation_snapshot(-1).room_index),
                },
            },
            "presentation_derived": {
                "enemy_node_transform": enemy.transform,
                "camera_transform": world.camera.global_transform,
                "native_bone_poses": pose_json(native_pose),
                "godot_applied_bone_poses": skeleton_pose_json(skeleton),
            },
            "derived_diagnostics": {
                "displacement_from_start": displacement,
                "frame_position_delta": position - previous_position,
                "frame_velocity_delta": velocity - previous_velocity,
            },
        }
        telemetry_us += Time.get_ticks_usec() - telemetry_started
        if frames_file != null:
            var write_started := Time.get_ticks_usec()
            frames_file.store_line(JSON.stringify(json_value(record)))
            write_us += Time.get_ticks_usec() - write_started
        previous_position = position
        previous_velocity = velocity
        previous_armor = armor

        for event_name in events:
            if not event_captures.has(event_name):
                var event_path := captures_path.path_join("%04d-%s.png" % [frame_id, event_name])
                await capture_render(event_path)
                event_captures[event_name] = event_path.get_file()

    if frames_file != null:
        frames_file.close()

    check(int(world.runtime.frame()) == initial_frame + FRAME_COUNT,
        "capture contains exactly %d native simulation steps" % FRAME_COUNT)
    check(grounded_frame >= 0, "enemy begins with observed ground support")
    check(movement_frame >= 0 and maximum_displacement > 0.05,
        "enemy has actual nonzero root displacement")
    check(impact_frame >= 0 and float(world.runtime.target_presentation_snapshot(target_index).armor) < initial_armor,
        "real melee gameplay impact reduces authoritative enemy armor")
    check(maximum_knockback_delta > 0.05 and maximum_disruption_after_impact > 0.0,
        "impact produces observable physical reaction")
    check(recovery_frame > impact_frame, "enemy reaches observed post-impact recovery")

    var room_snapshot: Dictionary = world.runtime.room_presentation_snapshot(-1)
    var structure_count := export_glb(enemy, room_snapshot, glb_path)
    await capture_render(render_path)

    var unavailable_measurements := [
        "The native enemy controller does not retain a collision-contact manifold, contact point list, penetration depth, or separation distance for obstacle collisions.",
        "The gameplay impact impulse is applied internally but is not retained as a per-frame impulse record; this package reports the observed armor transition, velocity change, angular velocity, and impact-instability state instead.",
        "There is no explicit native enemy stagger enum in this encounter authority. Hit reaction, knockback, fallen state, disruption, and recovery are recorded without relabeling them as a fabricated stagger state.",
        "The 33-bone asset exposes numeric bone indices but no semantic foot-bone names at the Godot boundary. Native world-space foot state is recorded separately; no inferred bone-to-foot mapping is claimed.",
    ]
    var metadata := {
        "schema": "digital-breakdown.physical-encounter-telemetry/v2",
        "source_revision": source_revision(),
        "engine": Engine.get_version_info(),
        "scene": "res://scenes/parent_reference.tscn",
        "capture_script": "res://tests/physical_encounter_telemetry_capture.gd",
        "target_index": target_index,
        "coordinate_conventions": {
            "system": "Godot right-handed 3D",
            "axes": {"up": "+Y", "right": "+X", "forward": "-Z"},
            "transform_encoding": "basis columns x/y/z plus origin",
            "angular_velocity_encoding": "Vector3(pitch_rate, yaw_rate, roll_rate), radians/second",
            "bone_poses": "local to parent bone",
            "foot_positions": "native authoritative world space",
        },
        "units": {"distance": "meters", "angles": "radians", "velocity": "meters/second",
            "angular_velocity": "radians/second", "simulation_frame": "one native 1/60-second tick"},
        "capture": {
            "frame_count": FRAME_COUNT,
            "first_native_frame": initial_frame + 1,
            "last_native_frame": int(world.runtime.frame()),
            "deterministic_input": "existing parent-smoke neutral encounter input; melee cadence begins at capture frame 300 and stops after the first observed armor decrease",
            "render_size": [root.size.x, root.size.y],
            "glb_state": "after the final recorded simulation frame",
            "render_state": "same final state as GLB and last telemetry record",
        },
        "observed_transitions": {
            "grounded_frame": grounded_frame,
            "movement_frame": movement_frame,
            "impact_frame": impact_frame,
            "recovery_frame": recovery_frame,
        },
        "verification": {
            "maximum_root_displacement": maximum_displacement,
            "maximum_linear_speed": maximum_speed,
            "maximum_impact_instability": maximum_impact_instability,
            "maximum_post_impact_disruption": maximum_disruption_after_impact,
            "maximum_frame_velocity_delta_after_impact": maximum_knockback_delta,
            "maximum_bone_transform_error": maximum_bone_transform_error,
            "native_godot_bone_match": true,
            "lockstep_control_match": true,
            "native_frame_continuity": true,
        },
        "performance_microseconds": {
            "world_agent_step_and_sync_total": world_step_us,
            "native_only_control_tick_total": control_step_us,
            "telemetry_collection_total": telemetry_us,
            "jsonl_write_total": write_us,
            "instrumented_average_per_frame": float(world_step_us + telemetry_us + write_us) / FRAME_COUNT,
            "native_only_control_average_per_frame": float(control_step_us) / FRAME_COUNT,
        },
        "portable_scene": {
            "native_room_structure_records_exported": structure_count,
            "collision_geometry_status": "Native room structures are exported as exact snapshot boxes for spatial interpretation. They are not claimed to be a serialized collision manifold or complete collider set.",
            "animation_status": "Final pose in GLB; full frame sequence in frames.jsonl.",
        },
        "graphical_captures": event_captures,
        "measurement_provenance": {
            "authoritative": "authority fields come from native Game state or native enemy locomotion/physical-body state",
            "presentation_derived": "Godot node/camera transforms and Godot-applied bone poses are presentation observations",
            "derived_diagnostics": "displacement and deltas are computed from consecutive authoritative samples",
        },
        "unavailable_physical_measurements": unavailable_measurements,
        "artifacts": ["scene.glb", "frames.jsonl", "metadata.json", "render.png", "captures/"],
    }
    var metadata_file := FileAccess.open(metadata_path, FileAccess.WRITE)
    check(metadata_file != null, "open metadata.json")
    if metadata_file != null:
        metadata_file.store_string(JSON.stringify(json_value(metadata), "  ") + "\n")
        metadata_file.close()

    var sizes := {
        "frames.jsonl": file_size(frames_path),
        "scene.glb": file_size(glb_path),
        "metadata.json": file_size(metadata_path),
        "render.png": file_size(render_path),
    }
    print("CAPTURE_SIZES " + JSON.stringify(sizes))
    print("CAPTURE_TRANSITIONS grounded=%d movement=%d impact=%d recovery=%d displacement=%.6f" % [
        grounded_frame, movement_frame, impact_frame, recovery_frame, maximum_displacement])
    world.queue_free()
    control.queue_free()
    if failures.is_empty():
        print("RESULT: PASS physical encounter telemetry frames=%d output=%s" % [FRAME_COUNT, output_path])
        quit(0)
    else:
        print("RESULT: FAIL count=", failures.size())
        quit(1)
