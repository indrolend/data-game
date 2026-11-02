extends SceneTree

const OUTPUT := "res://../evidence/generated/scene-telemetry"
const FRAME_COUNT := 120
const TARGET_INDEX := 0

var failures: Array[String] = []

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

func valid_transform(value: Transform3D) -> bool:
    return value.origin.is_finite() and value.basis.x.is_finite() \
        and value.basis.y.is_finite() and value.basis.z.is_finite()

func pose_json(pose: Array) -> Array[Dictionary]:
    var result: Array[Dictionary] = []
    for bone_index in range(pose.size()):
        var transform: Transform3D = pose[bone_index]
        check(valid_transform(transform), "native bone %d transform is finite" % bone_index)
        result.append({"index": bone_index, "local_transform": transform_json(transform)})
    return result

func skeleton_pose_json(skeleton: Skeleton3D) -> Array[Dictionary]:
    var result: Array[Dictionary] = []
    for bone_index in range(skeleton.get_bone_count()):
        var transform := skeleton.get_bone_pose(bone_index)
        check(valid_transform(transform), "Godot bone %d transform is finite" % bone_index)
        result.append({"index": bone_index, "local_transform": transform_json(transform)})
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

func export_glb(node: Node, path: String) -> void:
    var document := GLTFDocument.new()
    var state := GLTFState.new()
    var append_error := document.append_from_scene(node, state)
    check(append_error == OK, "append live skeleton scene to GLTF state")
    if append_error != OK:
        return
    var write_error := document.write_to_filesystem(state, path)
    check(write_error == OK and FileAccess.file_exists(path), "write scene.glb")
    if write_error != OK:
        return
    var reopened_state := GLTFState.new()
    var reopen_error := document.append_from_file(path, reopened_state)
    check(reopen_error == OK, "reopen scene.glb")
    if reopen_error == OK:
        var reopened := document.generate_scene(reopened_state)
        check(reopened != null, "generate scene from reopened GLB")
        if reopened != null:
            check(count_skeleton_bones(reopened) == 33, "reopened GLB rig has 33 bones")
            reopened.free()

func capture_render(path: String) -> void:
    await process_frame
    await process_frame
    var image := root.get_texture().get_image()
    check(image != null and not image.is_empty(), "graphical framebuffer is available")
    if image != null and not image.is_empty():
        check(image.save_png(path) == OK, "save render.png")

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    OS.set_environment("DB_ENABLE_SKELETON_ENEMY_SLICE", "1")
    var output_path := ProjectSettings.globalize_path(OUTPUT)
    DirAccess.make_dir_recursive_absolute(output_path)
    var frames_path := output_path.path_join("frames.jsonl")
    var glb_path := output_path.path_join("scene.glb")
    var render_path := output_path.path_join("render.png")
    var metadata_path := output_path.path_join("metadata.json")

    var world: Node3D = load("res://scenes/parent_reference.tscn").instantiate()
    world.agent_playtest = true
    root.add_child(world)
    await process_frame
    check(world.skeleton_enemy_slice_enabled, "skeleton enemy slice is enabled")
    world.runtime.start_target_interaction_fixture()
    world._sync()

    var control = ClassDB.instantiate("ParentGameNode")
    root.add_child(control)
    control.start_target_interaction_fixture()
    var enemy: Node3D = world.targets[TARGET_INDEX]
    var skeleton: Skeleton3D = enemy.skeleton
    check(skeleton.get_bone_count() == 33, "live exported rig has 33 bones")

    var frames_file := FileAccess.open(frames_path, FileAccess.WRITE)
    check(frames_file != null, "open frames.jsonl")
    var initial_frame := int(world.runtime.frame())
    var previous_frame := initial_frame
    for capture_index in range(FRAME_COUNT):
        var cycle := capture_index % 60
        var move_x := 0.3 if cycle < 30 else -0.3
        var move_z := 0.7
        var look_x := 0.1
        var look_y := 0.02
        var vacuum := cycle >= 15 and cycle < 40
        var sprint := cycle < 25
        var melee := cycle == 10
        world.agent_step(1, move_x, move_z, look_x, look_y, vacuum, sprint,
            false, melee, false, false)
        control.submit_input(move_x, move_z, look_x, look_y, vacuum, sprint,
            false, melee, false, false)
        control.tick()
        var frame_id := int(world.runtime.frame())
        check(frame_id == previous_frame + 1, "native frame identifier advances once at capture %d" % capture_index)
        previous_frame = frame_id
        var snapshot: Dictionary = world.runtime.target_presentation_snapshot(TARGET_INDEX)
        var native_pose: Array = world.runtime.target_model_pose(TARGET_INDEX)
        check(native_pose.size() == 33, "native frame %d has 33 bone transforms" % frame_id)
        check(skeleton.get_bone_count() == 33, "Godot frame %d has 33 bone transforms" % frame_id)
        for bone_index in range(mini(native_pose.size(), skeleton.get_bone_count())):
            check((native_pose[bone_index] as Transform3D).is_equal_approx(
                skeleton.get_bone_pose(bone_index)),
                "native and Godot bone %d agree at frame %d" % [bone_index, frame_id])
        check(world.runtime.gameplay_loop_snapshot() == control.gameplay_loop_snapshot(),
            "telemetry observation preserves gameplay state at frame %d" % frame_id)
        check(snapshot == control.target_presentation_snapshot(TARGET_INDEX),
            "telemetry observation preserves target state at frame %d" % frame_id)
        if frames_file != null:
            var record := {
                "capture_index": capture_index,
                "native_frame_id": frame_id,
                "enemy_index": TARGET_INDEX,
                "enemy_position": vector3_json(snapshot.position),
                "body_orientation_radians": {
                    "pitch": float(snapshot.body_pitch),
                    "yaw": float(snapshot.pose.yaw),
                    "roll": float(snapshot.body_roll),
                },
                "foot_targets": {
                    "left": vector3_json(snapshot.left_foot_target),
                    "right": vector3_json(snapshot.right_foot_target),
                    "left_contact": float(snapshot.left_foot_contact),
                    "right_contact": float(snapshot.right_foot_contact),
                },
                "enemy_node_transform": transform_json(enemy.transform),
                "native_bone_poses": pose_json(native_pose),
                "godot_bone_poses": skeleton_pose_json(skeleton),
            }
            frames_file.store_line(JSON.stringify(record))
    if frames_file != null:
        frames_file.close()

    check(int(world.runtime.frame()) == initial_frame + FRAME_COUNT,
        "capture contains exactly 120 native simulation steps")
    # Export only the supported native-backed rig subtree. Preserve its final
    # world placement by folding the adapter transform into the copied root.
    var export_rig: Node3D = enemy.rig_root.duplicate()
    export_rig.name = "CapturedNativeBackedEnemy"
    export_rig.transform = enemy.transform * enemy.rig_root.transform
    export_glb(export_rig, glb_path)
    export_rig.free()
    await capture_render(render_path)

    var metadata := {
        "schema": "digital-breakdown.scene-telemetry/v1",
        "source_revision": source_revision(),
        "engine": Engine.get_version_info(),
        "scene": "res://scenes/parent_reference.tscn",
        "captured_node": str(enemy.get_path()) + "/GodotRig",
        "target_index": TARGET_INDEX,
        "coordinate_conventions": {
            "system": "Godot right-handed 3D",
            "axes": {"up": "+Y", "right": "+X", "forward": "-Z"},
            "transform_encoding": "basis columns x/y/z plus origin",
            "bone_poses": "local to parent bone",
        },
        "units": {"distance": "meters", "angles": "radians", "simulation_frame": "one native tick"},
        "capture": {
            "frame_count": FRAME_COUNT,
            "first_native_frame": initial_frame + 1,
            "last_native_frame": int(world.runtime.frame()),
            "deterministic_input": "same 60-frame cycle as skeleton_enemy_benchmark.gd",
            "render_size": vector3_json(Vector3(root.size.x, root.size.y, 0.0)).slice(0, 2),
            "glb_state": "after the final recorded simulation frame",
            "render_state": "same final state as GLB and last telemetry record",
        },
        "artifacts": ["scene.glb", "frames.jsonl", "metadata.json", "render.png"],
        "export_limitations": [
            "scene.glb contains a faithful export copy of the live enemy rig subtree: Node3D, Skeleton3D, Skin, material, final transform, and native-backed mesh; it does not claim to serialize the full runtime world.",
            "Native ParentGameNode state, scripts, WorldEnvironment, procedural sky shaders, HUD, particles, lights, and collision/gameplay objects are not represented in GLB.",
            "GLB stores only the final captured pose; the 120-frame sequence is preserved in frames.jsonl.",
            "The mesh is the actual 4,164-vertex native human-model boundary; no substitute geometry is fabricated.",
        ],
    }
    var metadata_file := FileAccess.open(metadata_path, FileAccess.WRITE)
    check(metadata_file != null, "open metadata.json")
    if metadata_file != null:
        metadata_file.store_string(JSON.stringify(metadata, "  ") + "\n")
        metadata_file.close()

    world.queue_free()
    control.queue_free()
    if failures.is_empty():
        print("RESULT: PASS scene telemetry capture frames=%d output=%s" % [FRAME_COUNT, output_path])
        quit(0)
    else:
        print("RESULT: FAIL count=", failures.size())
        quit(1)
