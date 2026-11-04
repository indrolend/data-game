extends "res://scripts/human_model_adapter.gd"

# Opt-in GGG-PERF-004 experiment. Native gameplay and deformation remain
# authoritative; this adapter only keeps the Godot render surface persistent.
var persistent_vertex_count := 0

func needs_surface_rebuild(vertex_count: int) -> bool:
    return mesh_instance.mesh == null or persistent_vertex_count != vertex_count

func sync_packed_from_native(snapshot: Dictionary, packed: Dictionary,
        rebuild_vertices: PackedVector3Array = PackedVector3Array(),
        camera_position: Variant = null, camera_target: Variant = null,
        obstruction_fade_enabled := false) -> void:
    if not sync_presentation_state(snapshot, camera_position, camera_target,
            obstruction_fade_enabled):
        return
    var vertex_count := int(packed.get("vertex_count", 0))
    var bytes: PackedByteArray = packed.get("data", PackedByteArray())
    if vertex_count <= 0 or bytes.size() != vertex_count * 20:
        return
    if needs_surface_rebuild(vertex_count):
        if rebuild_vertices.size() != vertex_count:
            return
        _build_dynamic_surface(rebuild_vertices)
    var model_mesh := mesh_instance.mesh as ArrayMesh
    model_mesh.surface_update_vertex_region(0, 0, bytes)
    model_mesh.custom_aabb = AABB(packed.aabb_position, packed.aabb_size)

func _build_dynamic_surface(vertices: PackedVector3Array) -> void:
    var arrays := []
    arrays.resize(Mesh.ARRAY_MAX)
    arrays[Mesh.ARRAY_VERTEX] = vertices
    var normals := PackedVector3Array()
    normals.resize(vertices.size())
    for triangle in range(0, vertices.size(), 3):
        if triangle + 2 >= vertices.size():
            break
        var normal := (vertices[triangle + 1] - vertices[triangle]).cross(
            vertices[triangle + 2] - vertices[triangle]).normalized()
        normals[triangle] = normal
        normals[triangle + 1] = normal
        normals[triangle + 2] = normal
    arrays[Mesh.ARRAY_NORMAL] = normals
    var model_mesh := ArrayMesh.new()
    model_mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays, [], {},
        Mesh.ARRAY_FLAG_USE_DYNAMIC_UPDATE)
    mesh_instance.mesh = model_mesh
    persistent_vertex_count = vertices.size()
