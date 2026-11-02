extends SceneTree

const PackedAdapter = preload("res://scripts/packed_human_model_adapter.gd")
var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func decoded_position(data: PackedByteArray, index: int) -> Vector3:
    var offset := index * 12
    return Vector3(data.decode_float(offset), data.decode_float(offset + 4),
        data.decode_float(offset + 8))

func decoded_normal(data: PackedByteArray, vertex_count: int, index: int) -> Vector3:
    var packed := data.decode_u32(vertex_count * 12 + index * 8)
    var oct := Vector2(float(packed & 0xffff) / 65535.0,
        float((packed >> 16) & 0xffff) / 65535.0)
    return Vector3.octahedron_decode(oct)

func verify_payload(runtime: Node, target: int, label: String) -> Dictionary:
    var vertices: PackedVector3Array = runtime.target_model_vertices(target)
    var payload: Dictionary = runtime.target_model_packed_buffer(target)
    var count := int(payload.get("vertex_count", 0))
    var data: PackedByteArray = payload.get("data", PackedByteArray())
    check(count == vertices.size(), "%s vertex count matches control" % label)
    check(data.size() == count * 20, "%s packed byte count matches Godot layout" % label)
    var bounds := AABB(payload.aabb_position, payload.aabb_size)
    for index in range(count):
        check(decoded_position(data, index).distance_to(vertices[index]) <= 0.000001,
            "%s position %d round-trips" % [label, index])
        var point := vertices[index]
        check(point.x >= bounds.position.x - 0.000001 and point.y >= bounds.position.y - 0.000001
            and point.z >= bounds.position.z - 0.000001 and point.x <= bounds.end.x + 0.000001
            and point.y <= bounds.end.y + 0.000001 and point.z <= bounds.end.z + 0.000001,
            "%s bounds contain vertex %d" % [label, index])
    for triangle in range(0, count, 3):
        var expected := (vertices[triangle + 1] - vertices[triangle]).cross(
            vertices[triangle + 2] - vertices[triangle]).normalized()
        for corner in range(3):
            check(decoded_normal(data, count, triangle + corner).dot(expected) > 0.9999,
                "%s normal %d round-trips" % [label, triangle + corner])
    return payload

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    var runtime: Node = ClassDB.instantiate("ParentGameNode")
    runtime.start_target_interaction_fixture()
    var target := -1
    var nearest_distance := INF
    var player_position: Vector3 = runtime.player_position()
    for index in range(runtime.target_count()):
        if not runtime.target_alive(index):
            continue
        var candidate: Vector3 = runtime.target_position(index)
        var distance := Vector2(candidate.x - player_position.x,
            candidate.z - player_position.z).length_squared()
        if distance < nearest_distance:
            nearest_distance = distance
            target = index
    check(target >= 0, "fixture exposes a live target")
    if target < 0:
        quit(2)
        return
    var adapter = PackedAdapter.new()
    adapter.configure(target, runtime.target_model_color(target))
    root.add_child(adapter)
    var payload := verify_payload(runtime, target, "intact")
    var vertices: PackedVector3Array = runtime.target_model_vertices(target)
    adapter.sync_packed_from_native(runtime.target_presentation_snapshot(target), payload, vertices)
    var persistent_mesh: Mesh = adapter.mesh_instance.mesh
    var surface_format: int = persistent_mesh.surface_get_format(0)
    var payload_count := int(payload.vertex_count)
    check(RenderingServer.mesh_surface_get_format_vertex_stride(surface_format, payload_count) == 12,
        "surface position stride matches native packer")
    check(RenderingServer.mesh_surface_get_format_normal_tangent_stride(surface_format,
        payload_count) == 8, "surface normal/tangent stride matches native packer")
    check(RenderingServer.mesh_surface_get_format_offset(surface_format, payload_count,
        Mesh.ARRAY_NORMAL) == payload_count * 12, "surface normal offset matches native packer")
    check(RenderingServer.mesh_surface_get_format_offset(surface_format, payload_count,
        Mesh.ARRAY_TANGENT) == payload_count * 12 + 4,
        "surface tangent offset matches native packer")
    for frame in range(120):
        runtime.submit_input(0.0, 0.0, 0.04, 0.0, frame > 40 and frame < 80,
            false, false, frame == 30, false, false)
        runtime.tick()
        payload = runtime.target_model_packed_buffer(target)
        var rebuild := PackedVector3Array()
        if adapter.needs_surface_rebuild(int(payload.vertex_count)):
            rebuild = runtime.target_model_vertices(target)
        adapter.sync_packed_from_native(runtime.target_presentation_snapshot(target), payload, rebuild)
        check(adapter.mesh_instance.mesh == persistent_mesh,
            "persistent mesh survives ordinary animation frame %d" % frame)
    verify_payload(runtime, target, "animated")

    var initial_count := int(payload.vertex_count)
    var found_crack := false
    for step in range(1100):
        var melee := step > 300 and step < 1000 and step % 50 == 0
        runtime.submit_input(0, 0, 0, 0, false, false, false, melee, false, false)
        runtime.tick()
        var snapshot: Dictionary = runtime.target_presentation_snapshot(target)
        if float(snapshot.shell_crack_amount) <= 0.0:
            continue
        payload = verify_payload(runtime, target, "cracked")
        check(int(payload.vertex_count) < initial_count, "crack removes native triangles")
        check(adapter.needs_surface_rebuild(int(payload.vertex_count)),
            "topology change requests one surface rebuild")
        vertices = runtime.target_model_vertices(target)
        adapter.sync_packed_from_native(snapshot, payload, vertices)
        check(not adapter.needs_surface_rebuild(int(payload.vertex_count)),
            "cracked topology becomes persistent after rebuild")
        found_crack = true
        break
    check(found_crack, "fixture reaches cracked topology")
    adapter.queue_free()
    runtime.free()
    if failures.is_empty():
        print("RESULT: PASS native-packed enemy buffer matches control geometry")
        quit(0)
    else:
        print("RESULT: FAIL %d checks" % failures.size())
        quit(2)
