extends SceneTree

const HumanModelAdapter = preload("res://scripts/human_model_adapter.gd")
const WARMUP_FRAMES := 180
const SAMPLE_FRAMES := 600

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func _matrix(file: FileAccess) -> Transform3D:
    var values: Array[float] = []
    for _index in range(16):
        values.append(file.get_float())
    return Transform3D(Basis(
        Vector3(values[0], values[1], values[2]),
        Vector3(values[4], values[5], values[6]),
        Vector3(values[8], values[9], values[10])),
        Vector3(values[12], values[13], values[14]))

func _parse_dbh() -> Dictionary:
    var file := FileAccess.open("res://../models/human.dbhuman", FileAccess.READ)
    check(file != null, "DBH source opens")
    if file == null:
        return {}
    check(file.get_buffer(4).get_string_from_ascii() == "DBH1", "DBH magic matches")
    var vertex_count := file.get_32()
    var bone_count := file.get_32()
    var frame_count := file.get_32()
    file.get_32()
    var min_y := file.get_float()
    var unit_scale := file.get_float()
    for _color in range(4):
        file.get_float()
    var bind_matrix := _matrix(file)
    _matrix(file) # bindMatrixInverse is not needed by the Godot mesh.
    var root_parent := _matrix(file)
    var positions := PackedVector3Array()
    var bone_indices := PackedInt32Array()
    var weights := PackedFloat32Array()
    positions.resize(vertex_count)
    bone_indices.resize(vertex_count * 4)
    weights.resize(vertex_count * 4)
    for vertex in range(vertex_count):
        positions[vertex] = bind_matrix * Vector3(file.get_float(), file.get_float(), file.get_float())
        for influence in range(4):
            bone_indices[vertex * 4 + influence] = file.get_16()
        for influence in range(4):
            weights[vertex * 4 + influence] = file.get_float()
    var parents := PackedInt32Array()
    var inverse_binds: Array[Transform3D] = []
    parents.resize(bone_count)
    for bone in range(bone_count):
        parents[bone] = file.get_32()
        file.get_buffer(4)
        inverse_binds.append(_matrix(file))
    var poses: Array[Array] = []
    for frame in range(frame_count):
        var frame_poses: Array = []
        for bone in range(bone_count):
            var position := Vector3(file.get_float(), file.get_float(), file.get_float())
            var rotation := Quaternion(file.get_float(), file.get_float(), file.get_float(), file.get_float())
            var scale := Vector3(file.get_float(), file.get_float(), file.get_float())
            frame_poses.append({"position": position, "rotation": rotation, "scale": scale})
        poses.append(frame_poses)
    file.close()
    return {
        "vertex_count": vertex_count, "bone_count": bone_count, "frame_count": frame_count,
        "min_y": min_y, "unit_scale": unit_scale, "root_parent": root_parent,
        "positions": positions, "bone_indices": bone_indices, "weights": weights,
        "parents": parents, "inverse_binds": inverse_binds, "poses": poses,
    }

func _flat_normals(vertices: PackedVector3Array) -> PackedVector3Array:
    var normals := PackedVector3Array()
    normals.resize(vertices.size())
    for triangle in range(0, vertices.size(), 3):
        var normal := (vertices[triangle + 1] - vertices[triangle]).cross(
            vertices[triangle + 2] - vertices[triangle]).normalized()
        normals[triangle] = normal
        normals[triangle + 1] = normal
        normals[triangle + 2] = normal
    return normals

func _build_conventional(data: Dictionary) -> Dictionary:
    var root_node := Node3D.new()
    root_node.name = "ConventionalEnemy"
    root_node.scale = Vector3.ONE * float(data.unit_scale)
    root_node.position = Vector3(2.2, -float(data.min_y) * float(data.unit_scale), 0.0)
    var skeleton := Skeleton3D.new()
    skeleton.name = "Skeleton3D"
    root_node.add_child(skeleton)
    for bone in range(int(data.bone_count)):
        skeleton.add_bone("bone_%02d" % bone)
        skeleton.set_bone_parent(bone, int(data.parents[bone]))
        skeleton.set_bone_rest(bone, Transform3D.IDENTITY)
    var skin := Skin.new()
    for bone in range(int(data.bone_count)):
        skin.add_bind(bone, data.inverse_binds[bone])
    var arrays := []
    arrays.resize(Mesh.ARRAY_MAX)
    arrays[Mesh.ARRAY_VERTEX] = data.positions
    arrays[Mesh.ARRAY_NORMAL] = _flat_normals(data.positions)
    arrays[Mesh.ARRAY_BONES] = data.bone_indices
    arrays[Mesh.ARRAY_WEIGHTS] = data.weights
    var mesh := ArrayMesh.new()
    mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
    var instance := MeshInstance3D.new()
    instance.name = "SkinnedMesh"
    instance.mesh = mesh
    instance.skin = skin
    instance.skeleton = NodePath("..")
    var material := StandardMaterial3D.new()
    material.albedo_color = Color(0.64, 0.62, 0.68)
    material.roughness = 0.78
    material.cull_mode = BaseMaterial3D.CULL_DISABLED
    instance.material_override = material
    skeleton.add_child(instance)
    return {"root": root_node, "skeleton": skeleton, "mesh": instance}

func _apply_clip_pose(skeleton: Skeleton3D, data: Dictionary, time: float) -> void:
    var frame_value := fposmod(time * float(data.frame_count), float(data.frame_count))
    var first := int(floor(frame_value))
    var second := (first + 1) % int(data.frame_count)
    var blend := frame_value - float(first)
    for bone in range(int(data.bone_count)):
        var a: Dictionary = data.poses[first][bone]
        var b: Dictionary = data.poses[second][bone]
        var transform := Transform3D(Basis((a.rotation as Quaternion).slerp(b.rotation, blend)).scaled(
            (a.scale as Vector3).lerp(b.scale, blend)),
            (a.position as Vector3).lerp(b.position, blend))
        if int(data.parents[bone]) < 0:
            transform = data.root_parent * transform
        skeleton.set_bone_pose(bone, transform)

func _summary(values: Array[float]) -> Dictionary:
    values.sort()
    return {"median": values[values.size() / 2],
        "p95": values[ceili(values.size() * 0.95) - 1], "worst": values[-1]}

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    var output_stem := "conventional-enemy-probe"
    for argument in OS.get_cmdline_user_args():
        if argument.begins_with("--output-stem="):
            output_stem = argument.trim_prefix("--output-stem=")
    DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
    Engine.max_fps = 0
    RenderingServer.viewport_set_measure_render_time(root.get_viewport_rid(), true)
    var data := _parse_dbh()
    if data.is_empty():
        quit(2)
        return
    check(int(data.vertex_count) == 4164 and int(data.bone_count) == 33 and int(data.frame_count) == 60,
        "representative uses the complete DBH rig")
    var scene := Node3D.new()
    root.add_child(scene)
    var camera := Camera3D.new()
    camera.position = Vector3(0.0, 1.6, 8.0)
    camera.look_at_from_position(camera.position, Vector3(0.0, 1.0, 0.0), Vector3.UP)
    camera.current = true
    scene.add_child(camera)
    var light := DirectionalLight3D.new()
    light.rotation = Vector3(-0.75, -0.45, 0.0)
    light.shadow_enabled = true
    scene.add_child(light)
    var environment := WorldEnvironment.new()
    environment.environment = Environment.new()
    environment.environment.background_mode = Environment.BG_COLOR
    environment.environment.background_color = Color(0.035, 0.04, 0.055)
    environment.environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
    environment.environment.ambient_light_color = Color(0.45, 0.48, 0.55)
    environment.environment.ambient_light_energy = 1.2
    scene.add_child(environment)

    var conventional := _build_conventional(data)
    scene.add_child(conventional.root)
    var runtime = ClassDB.instantiate("ParentGameNode")
    runtime.start_target_fixture()
    var target := 0
    while target < runtime.target_count() and not runtime.target_alive(target):
        target += 1
    var custom := HumanModelAdapter.new()
    custom.configure(target, runtime.target_model_color(target))
    scene.add_child(custom)
    var custom_snapshot: Dictionary = runtime.target_presentation_snapshot(target).duplicate(true)
    custom_snapshot["position"] = Vector3(-2.2, 0.0, 0.0)
    custom_snapshot.pose["yaw"] = 0.0
    custom_snapshot["body_pitch"] = 0.0
    custom_snapshot["body_roll"] = 0.0

    var custom_ms: Array[float] = []
    var skeleton_ms: Array[float] = []
    var frame_ms: Array[float] = []
    var gpu_ms: Array[float] = []
    var memory_mib: Array[float] = []
    for frame in range(WARMUP_FRAMES + SAMPLE_FRAMES):
        var frame_started := Time.get_ticks_usec()
        runtime.submit_input(0, 0, 0, 0, false, false, false, false, false, false)
        runtime.tick()
        custom_snapshot = runtime.target_presentation_snapshot(target).duplicate(true)
        custom_snapshot["position"] = Vector3(-2.2, 0.0, 0.0)
        custom_snapshot.pose["yaw"] = 0.0
        custom_snapshot["body_pitch"] = 0.0
        custom_snapshot["body_roll"] = 0.0
        var custom_started := Time.get_ticks_usec()
        custom.sync_from_native(custom_snapshot, runtime.target_model_vertices(target))
        var custom_elapsed := float(Time.get_ticks_usec() - custom_started) / 1000.0
        var skeleton_started := Time.get_ticks_usec()
        _apply_clip_pose(conventional.skeleton, data, float(frame) / 60.0)
        var skeleton_elapsed := float(Time.get_ticks_usec() - skeleton_started) / 1000.0
        await process_frame
        if frame < WARMUP_FRAMES:
            continue
        custom_ms.append(custom_elapsed)
        skeleton_ms.append(skeleton_elapsed)
        frame_ms.append(float(Time.get_ticks_usec() - frame_started) / 1000.0)
        gpu_ms.append(float(RenderingServer.viewport_get_measured_render_time_gpu(
            root.get_viewport_rid())))
        memory_mib.append(float(Performance.get_monitor(Performance.MEMORY_STATIC)) / 1048576.0)
    await process_frame
    var image := root.get_texture().get_image()
    image.save_png("res://../evidence/performance/%s.png" % output_stem)
    var result := {
        "schema_version": 1,
        "custom_cpu_skin_mesh_ms": _summary(custom_ms),
        "conventional_skeleton_pose_ms": _summary(skeleton_ms),
        "combined_frame_ms": _summary(frame_ms),
        "gpu_ms": _summary(gpu_ms),
        "memory_mib": _summary(memory_mib),
        "memory_first_mib": memory_mib[0], "memory_last_mib": memory_mib[-1],
        "vertex_count": data.vertex_count, "bone_count": data.bone_count,
        "frame_count": data.frame_count,
        "known_parity_gaps": ["native attack/expressiveness bone modifiers",
            "post-skin foot correction", "final-deformed flat normals",
            "shell-crack triangle removal", "soul-collapse morph"],
    }
    var json := JSON.stringify(result, "  ") + "\n"
    print("CONVENTIONAL_ENEMY_RESULT ", json)
    var file := FileAccess.open("res://../evidence/performance/%s.json" % output_stem,
        FileAccess.WRITE)
    file.store_string(json)
    scene.free()
    runtime.free()
    if not failures.is_empty():
        quit(2)
        return
    quit(0)
