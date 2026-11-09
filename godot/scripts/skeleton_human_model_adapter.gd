extends "res://scripts/packed_human_model_adapter.gd"

# Persistent Godot rig used by the single-enemy diagnostic slice and the
# explicit all-enemy convergence experiment. It replaces native vertex transfer
# and ArrayMesh reconstruction while the native core still owns pose selection,
# combat, locomotion, targeting, and all authoritative state.
var skeleton: Skeleton3D
var skinned_mesh: MeshInstance3D
var rig_root: Node3D
var skinned_material: ShaderMaterial
var rig_unit_scale := 1.0

func configure_from_native(index: int, color: Color, rig: Dictionary) -> void:
    super.configure(index, color)
    # Allocate the exceptional crack surface up front. Keeping one transparent
    # representative render-warm avoids a first-crack render-pipeline stall;
    # every other target remains culled until its native shell actually cracks.
    _build_dynamic_surface(rig.positions)
    mesh_instance.visible = index == 0
    if index == 0:
        var warm_color := color
        warm_color.a = 0.0
        material.albedo_color = warm_color
    rig_root = Node3D.new()
    rig_root.name = "GodotRig"
    rig_unit_scale = float(rig.unit_scale)
    rig_root.scale = Vector3.ONE * rig_unit_scale
    rig_root.position.y = -float(rig.min_y) * rig_unit_scale
    add_child(rig_root)
    skeleton = Skeleton3D.new()
    skeleton.name = "Skeleton3D"
    rig_root.add_child(skeleton)
    var parents: PackedInt32Array = rig.parents
    var inverse_binds: Array = rig.inverse_binds
    var skin := Skin.new()
    for bone in range(parents.size()):
        skeleton.add_bone("bone_%02d" % bone)
        skeleton.set_bone_parent(bone, parents[bone])
        skeleton.set_bone_rest(bone, Transform3D.IDENTITY)
        skin.add_bind(bone, inverse_binds[bone])
    var arrays := []
    arrays.resize(Mesh.ARRAY_MAX)
    var positions: PackedVector3Array = rig.positions
    arrays[Mesh.ARRAY_VERTEX] = positions
    arrays[Mesh.ARRAY_NORMAL] = _flat_normals(positions)
    arrays[Mesh.ARRAY_BONES] = rig.bone_indices
    arrays[Mesh.ARRAY_WEIGHTS] = rig.weights
    arrays[Mesh.ARRAY_COLOR] = rig.foot_weights
    var persistent_mesh := ArrayMesh.new()
    persistent_mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
    skinned_mesh = MeshInstance3D.new()
    skinned_mesh.name = "SkinnedMesh"
    skinned_mesh.mesh = persistent_mesh
    skinned_mesh.skin = skin
    skinned_mesh.skeleton = NodePath("..")
    skinned_material = ShaderMaterial.new()
    var shader := Shader.new()
    shader.code = """
shader_type spatial;
render_mode cull_disabled;
uniform vec4 base_color : source_color = vec4(1.0);
uniform vec3 left_foot_correction = vec3(0.0);
uniform vec3 right_foot_correction = vec3(0.0);
void vertex() {
    VERTEX += COLOR.r * left_foot_correction + COLOR.g * right_foot_correction;
}
void fragment() {
    ALBEDO = base_color.rgb;
    ALPHA = base_color.a;
    ROUGHNESS = 0.78;
}
"""
    skinned_material.shader = shader
    skinned_material.set_shader_parameter("base_color", color)
    skinned_mesh.material_override = skinned_material
    skeleton.add_child(skinned_mesh)

func sync_pose_from_native(snapshot: Dictionary, pose: Array,
        foot_corrections: PackedVector3Array,
        camera_position: Variant = null, camera_target: Variant = null,
        obstruction_fade_enabled := false) -> void:
    visible = bool(snapshot.alive)
    _sync_tether(snapshot)
    if not visible or pose.size() != skeleton.get_bone_count():
        return
    base_color = snapshot.get("damage_surface_color", base_color)
    var shown_color: Color = base_color.lerp(Color(1.0, 0.28, 0.2),
        clampf(float(snapshot.hit_amount), 0.0, 1.0))
    var morph := clampf(float(snapshot.pose.soul_morph), 0.0, 1.0)
    shown_color.a = 1.0 - morph * morph * (3.0 - 2.0 * morph)
    camera_visibility_alpha = 1.0
    if obstruction_fade_enabled and camera_position is Vector3 and camera_target is Vector3:
        var view: Vector3 = camera_target - camera_position
        var center: Vector3 = snapshot.position + Vector3.UP * 0.75
        var along := clampf((center - camera_position).dot(view) /
            maxf(view.length_squared(), 0.0001), 0.0, 1.0)
        var view_distance := center.distance_to(camera_position + view * along)
        if along > 0.02 and along < 0.98:
            camera_visibility_alpha = clampf(inverse_lerp(0.35, 1.25, view_distance), 0.18, 1.0)
            shown_color.a *= camera_visibility_alpha
    material.albedo_color = shown_color
    skinned_material.set_shader_parameter("base_color", shown_color)
    var left_correction := Vector3.ZERO
    var right_correction := Vector3.ZERO
    if foot_corrections.size() == 2 and rig_unit_scale > 0.000001:
        left_correction = foot_corrections[0] / rig_unit_scale
        right_correction = foot_corrections[1] / rig_unit_scale
    skinned_material.set_shader_parameter("left_foot_correction", left_correction)
    skinned_material.set_shader_parameter("right_foot_correction", right_correction)
    position = snapshot.position + Vector3.UP * (float(snapshot.pose.root_bob) + 0.57 * morph)
    rotation = Vector3(float(snapshot.body_pitch), float(snapshot.pose.yaw), float(snapshot.body_roll))
    scale = snapshot.pose.expressive_scale * float(snapshot.pose.scale)
    for bone in range(pose.size()):
        skeleton.set_bone_pose(bone, pose[bone])
    rig_root.visible = true
    mesh_instance.visible = target_index == 0
    if target_index == 0:
        var warm_color := shown_color
        warm_color.a = 0.0
        material.albedo_color = warm_color

func sync_cracked_from_native(snapshot: Dictionary, packed: Dictionary,
        rebuild_vertices: PackedVector3Array,
        camera_position: Variant = null, camera_target: Variant = null,
        obstruction_fade_enabled := false) -> void:
    rig_root.visible = false
    mesh_instance.visible = true
    sync_packed_from_native(snapshot, packed, rebuild_vertices, camera_position,
        camera_target, obstruction_fade_enabled)

func _flat_normals(vertices: PackedVector3Array) -> PackedVector3Array:
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
    return normals
