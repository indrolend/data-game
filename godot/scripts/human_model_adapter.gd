extends Node3D

# Render-only adapter for the original native-skinned human.dbhuman triangle list.
# The parent Game remains authoritative for animation time, attacks, body response,
# foot placement, hit results, displacement, grabs, and lifecycle.
var target_index := -1
var mesh_instance: MeshInstance3D
var material: StandardMaterial3D
var base_color := Color.WHITE
var tether_instance: MeshInstance3D
var tether_material: StandardMaterial3D

func configure(index: int, color: Color) -> void:
    target_index = index
    name = "NativeHuman_%d" % index
    mesh_instance = MeshInstance3D.new()
    mesh_instance.name = "SkinnedModel"
    material = StandardMaterial3D.new()
    base_color = color
    material.albedo_color = color
    material.roughness = 0.78
    material.cull_mode = BaseMaterial3D.CULL_DISABLED
    mesh_instance.material_override = material
    add_child(mesh_instance)

    tether_instance = MeshInstance3D.new()
    tether_instance.name = "NativeVacuumTether"
    var tether_mesh := CylinderMesh.new()
    tether_mesh.top_radius = 0.5
    tether_mesh.bottom_radius = 0.5
    tether_mesh.height = 1.0
    tether_instance.mesh = tether_mesh
    tether_material = StandardMaterial3D.new()
    tether_material.albedo_color = Color(1.0, 183.0 / 255.0, 166.0 / 255.0, 0.72)
    tether_material.emission_enabled = true
    tether_material.emission = Color(1.0, 183.0 / 255.0, 166.0 / 255.0)
    tether_material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
    tether_material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
    tether_instance.material_override = tether_material
    tether_instance.set_as_top_level(true)
    add_child(tether_instance)

func sync_from_native(snapshot: Dictionary, vertices: PackedVector3Array) -> void:
    visible = bool(snapshot.alive)
    _sync_tether(snapshot)
    if not visible or vertices.is_empty():
        return
    material.albedo_color = base_color.lerp(Color(1.0, 0.28, 0.2), clampf(float(snapshot.hit_amount), 0.0, 1.0))
    position = snapshot.position + Vector3.UP * float(snapshot.pose.root_bob)
    rotation = Vector3(float(snapshot.body_pitch), float(snapshot.pose.yaw), float(snapshot.body_roll))
    var expressive: Vector3 = snapshot.pose.expressive_scale
    var model_scale := float(snapshot.pose.scale)
    scale = expressive * model_scale
    var arrays := []
    arrays.resize(Mesh.ARRAY_MAX)
    arrays[Mesh.ARRAY_VERTEX] = vertices
    var model_mesh := ArrayMesh.new()
    model_mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
    mesh_instance.mesh = model_mesh

func _sync_tether(snapshot: Dictionary) -> void:
    var anchor: Vector3 = snapshot.tether_anchor
    var destination: Vector3 = snapshot.tether_destination
    var delta := destination - anchor
    var tether_width := float(snapshot.tether_width)
    tether_instance.visible = bool(snapshot.tether_visible) and tether_width > 0.0 and delta.length_squared() > 0.000001
    if not tether_instance.visible:
        return
    tether_instance.global_position = (anchor + destination) * 0.5
    tether_instance.global_rotation = Quaternion(Vector3.UP, delta.normalized()).get_euler()
    tether_instance.scale = Vector3(tether_width, delta.length(), tether_width)
