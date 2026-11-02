extends SceneTree

const WARMUP_FRAMES := 180
const SAMPLE_FRAMES := 600
const INSTANCE_COUNT := 32

var mode := "rebuild"
var output_path := ""

func _initialize() -> void:
    for argument in OS.get_cmdline_user_args():
        if argument.begins_with("--mode="):
            mode = argument.trim_prefix("--mode=")
        elif argument.begins_with("--output="):
            output_path = argument.trim_prefix("--output=")
    call_deferred("_run")

func _summary(values: Array[float]) -> Dictionary:
    values.sort()
    return {
        "median": values[values.size() / 2],
        "p95": values[ceili(values.size() * 0.95) - 1],
        "worst": values[-1],
    }

func _normals(vertices: PackedVector3Array) -> PackedVector3Array:
    var result := PackedVector3Array()
    result.resize(vertices.size())
    for triangle in range(0, vertices.size(), 3):
        var normal := (vertices[triangle + 1] - vertices[triangle]).cross(
            vertices[triangle + 2] - vertices[triangle]).normalized()
        result[triangle] = normal
        result[triangle + 1] = normal
        result[triangle + 2] = normal
    return result

func _arrays(vertices: PackedVector3Array, normals: PackedVector3Array) -> Array:
    var arrays := []
    arrays.resize(Mesh.ARRAY_MAX)
    arrays[Mesh.ARRAY_VERTEX] = vertices
    arrays[Mesh.ARRAY_NORMAL] = normals
    return arrays

func _run() -> void:
    DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
    Engine.max_fps = 0
    RenderingServer.viewport_set_measure_render_time(root.get_viewport_rid(), true)

    var runtime = ClassDB.instantiate("ParentGameNode")
    runtime.start_target_fixture()
    var target := 0
    while target < runtime.target_count() and not runtime.target_alive(target):
        target += 1
    var pose_a: PackedVector3Array = runtime.target_model_vertices(target)
    for _tick in range(30):
        runtime.submit_input(0, 0, 0, 0, false, false, false, false, false, false)
        runtime.tick()
    var pose_b: PackedVector3Array = runtime.target_model_vertices(target)
    if pose_a.size() != pose_b.size() or pose_a.is_empty():
        push_error("Probe requires two non-empty poses with identical topology")
        quit(2)
        return
    var normals := _normals(pose_a)
    var bytes_a := pose_a.to_byte_array()
    var bytes_b := pose_b.to_byte_array()

    var scene := Node3D.new()
    root.add_child(scene)
    var camera := Camera3D.new()
    camera.position = Vector3(0.0, 11.0, 24.0)
    camera.look_at_from_position(camera.position, Vector3(0.0, 2.0, 0.0), Vector3.UP)
    camera.current = true
    scene.add_child(camera)
    var light := DirectionalLight3D.new()
    light.rotation = Vector3(-0.8, -0.5, 0.0)
    scene.add_child(light)
    var material := StandardMaterial3D.new()
    material.albedo_color = Color(0.62, 0.58, 0.66)
    material.cull_mode = BaseMaterial3D.CULL_DISABLED
    var instances: Array[MeshInstance3D] = []
    for index in range(INSTANCE_COUNT):
        var instance := MeshInstance3D.new()
        instance.position = Vector3(float(index % 8) * 2.2 - 7.7, 0.0,
            float(index / 8) * -2.4)
        instance.material_override = material
        var mesh := ArrayMesh.new()
        mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, _arrays(pose_a, normals), [], {},
            Mesh.ARRAY_FLAG_USE_DYNAMIC_UPDATE if mode == "persistent_update" else 0)
        instance.mesh = mesh
        scene.add_child(instance)
        instances.append(instance)
    await process_frame

    var operation_ms: Array[float] = []
    var frame_ms: Array[float] = []
    var gpu_ms: Array[float] = []
    var memory_mib: Array[float] = []
    for frame in range(WARMUP_FRAMES + SAMPLE_FRAMES):
        var frame_started := Time.get_ticks_usec()
        var operation_started := Time.get_ticks_usec()
        var vertices := pose_a if frame % 2 == 0 else pose_b
        if mode == "persistent_update":
            var bytes := bytes_a if frame % 2 == 0 else bytes_b
            for instance in instances:
                (instance.mesh as ArrayMesh).surface_update_vertex_region(0, 0, bytes)
        else:
            var arrays := _arrays(vertices, normals)
            for instance in instances:
                var replacement := ArrayMesh.new()
                replacement.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
                instance.mesh = replacement
        var elapsed_operation := float(Time.get_ticks_usec() - operation_started) / 1000.0
        await process_frame
        if frame < WARMUP_FRAMES:
            continue
        operation_ms.append(elapsed_operation)
        frame_ms.append(float(Time.get_ticks_usec() - frame_started) / 1000.0)
        gpu_ms.append(float(RenderingServer.viewport_get_measured_render_time_gpu(
            root.get_viewport_rid())))
        memory_mib.append(float(Performance.get_monitor(Performance.MEMORY_STATIC)) / 1048576.0)

    var result := {
        "schema_version": 1,
        "mechanism": "position-buffer-only; normals intentionally held constant",
        "mode": mode,
        "instances": INSTANCE_COUNT,
        "vertices_per_instance": pose_a.size(),
        "bytes_per_instance_update": bytes_a.size(),
        "warmup_frames": WARMUP_FRAMES,
        "sample_frames": SAMPLE_FRAMES,
        "operation_ms": _summary(operation_ms),
        "frame_ms": _summary(frame_ms),
        "gpu_ms": _summary(gpu_ms),
        "memory_mib": _summary(memory_mib),
        "memory_first_mib": memory_mib[0],
        "memory_last_mib": memory_mib[-1],
    }
    var json := JSON.stringify(result, "  ") + "\n"
    print("GGG_PERF_003_PROBE ", json)
    if not output_path.is_empty():
        var file := FileAccess.open(output_path, FileAccess.WRITE)
        file.store_string(json)
    scene.free()
    runtime.free()
    quit(0)
