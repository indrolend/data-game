extends Node3D

const ELECTRIC_MAGENTA := Color(0x98 / 255.0, 0x1e / 255.0, 0x97 / 255.0)
const ELECTRIC_CYAN := Color(0x78 / 255.0, 0xd5 / 255.0, 0xe1 / 255.0)
const ACID_CHARTREUSE := Color(0xd4 / 255.0, 0xec / 255.0, 0x8e / 255.0)
const RIBBON_SEGMENTS := 24
const STREAK_SEGMENTS := 12

var swing_arc: MeshInstance3D
var directional_streak: MeshInstance3D
var impact_ring: MeshInstance3D

func _ready() -> void:
    swing_arc = _make_effect("MagentaSwingArc", ELECTRIC_MAGENTA)
    directional_streak = _make_effect("CyanDirectionalStreak", ELECTRIC_CYAN)
    impact_ring = _make_effect("ChartreuseImpactRing", ACID_CHARTREUSE)
    visible = false

func sync_snapshot(snapshot: Dictionary) -> void:
    var visual_timer := float(snapshot.get("visual_timer", 0.0))
    var visual_duration := float(snapshot.get("visual_duration", 0.0))
    if visual_timer <= 0.0 or bool(snapshot.get("locomotion_lunge", false)):
        visible = false
        return

    visible = true
    var t := 1.0 - clampf(visual_timer / maxf(0.001, visual_duration), 0.0, 1.0)
    var fade := 1.0 - t
    var visual_hit := bool(snapshot.get("visual_hit", false))
    var hit_boost := 1.25 if visual_hit else 0.72
    var origin: Vector3 = snapshot.get("origin", Vector3.ZERO)
    var direction: Vector3 = snapshot.get("direction", Vector3.FORWARD)
    var impact: Vector3 = snapshot.get("impact", origin + direction * 0.3)
    var yaw := atan2(direction.x, direction.z)

    var slash_basis := Basis(Vector3.UP, yaw) * Basis(Vector3.RIGHT, PI * 0.5) \
        * Basis(Vector3.FORWARD, -PI * 0.28 + t * PI * 1.15)
    var slash_scale := (0.75 + t * 0.72) * hit_boost
    swing_arc.position = origin + direction * (0.66 + 0.22 * t)
    swing_arc.basis = slash_basis.scaled(Vector3(slash_scale, slash_scale, 1.0))
    swing_arc.mesh = _ribbon_mesh(0.0, PI * 1.35, 0.494, 0.546)
    _set_alpha(swing_arc, fade * 0.66)

    var delta := impact - origin
    var length := maxf(0.3, delta.length())
    directional_streak.position = origin + delta * 0.46
    directional_streak.basis = _basis_with_z_axis(delta.normalized() if delta.length_squared() > 0.000001 else direction.normalized())
    directional_streak.mesh = _streak_mesh(
        length * (0.70 + sin(t * PI) * 0.18),
        0.11 * (1.0 + sin(t * PI) * 1.2))
    _set_alpha(directional_streak, fade * 0.44)

    var ring_scale := (0.45 + t * 1.45) * hit_boost
    impact_ring.position = impact
    impact_ring.basis = Basis.IDENTITY.scaled(Vector3(ring_scale, ring_scale, 1.0))
    impact_ring.mesh = _ribbon_mesh(0.0, TAU, 0.318, 0.362)
    _set_alpha(impact_ring, fade * (0.82 if visual_hit else 0.24))

func _make_effect(effect_name: String, color: Color) -> MeshInstance3D:
    var effect := MeshInstance3D.new()
    effect.name = effect_name
    var material := StandardMaterial3D.new()
    material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
    material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
    material.cull_mode = BaseMaterial3D.CULL_DISABLED
    material.albedo_color = color
    effect.material_override = material
    add_child(effect)
    return effect

func _set_alpha(effect: MeshInstance3D, alpha: float) -> void:
    var material := effect.material_override as StandardMaterial3D
    var color := material.albedo_color
    color.a = clampf(alpha, 0.0, 1.0)
    material.albedo_color = color

func _ribbon_mesh(start: float, sweep: float, inner: float, outer: float) -> ArrayMesh:
    var vertices := PackedVector3Array()
    for i in range(RIBBON_SEGMENTS):
        var angle_a := start + sweep * float(i) / RIBBON_SEGMENTS
        var angle_b := start + sweep * float(i + 1) / RIBBON_SEGMENTS
        var inner_a := Vector3(cos(angle_a) * inner, sin(angle_a) * inner, 0.0)
        var outer_a := Vector3(cos(angle_a) * outer, sin(angle_a) * outer, 0.0)
        var inner_b := Vector3(cos(angle_b) * inner, sin(angle_b) * inner, 0.0)
        var outer_b := Vector3(cos(angle_b) * outer, sin(angle_b) * outer, 0.0)
        vertices.append_array(PackedVector3Array([outer_a, inner_a, outer_b, outer_b, inner_a, inner_b]))
    return _mesh_from_vertices(vertices)

func _streak_mesh(length: float, width: float) -> ArrayMesh:
    var vertices := PackedVector3Array()
    for i in range(STREAK_SEGMENTS):
        var angle_a := TAU * float(i) / STREAK_SEGMENTS
        var angle_b := TAU * float(i + 1) / STREAK_SEGMENTS
        var base_a := Vector3(cos(angle_a) * width, sin(angle_a) * width, -length * 0.5)
        var tip_a := Vector3(cos(angle_a) * width * 0.32, sin(angle_a) * width * 0.32, length * 0.5)
        var base_b := Vector3(cos(angle_b) * width, sin(angle_b) * width, -length * 0.5)
        var tip_b := Vector3(cos(angle_b) * width * 0.32, sin(angle_b) * width * 0.32, length * 0.5)
        vertices.append_array(PackedVector3Array([base_a, tip_a, base_b, base_b, tip_a, tip_b]))
    return _mesh_from_vertices(vertices)

func _mesh_from_vertices(vertices: PackedVector3Array) -> ArrayMesh:
    var arrays := []
    arrays.resize(Mesh.ARRAY_MAX)
    arrays[Mesh.ARRAY_VERTEX] = vertices
    var mesh := ArrayMesh.new()
    mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
    return mesh

func _basis_with_z_axis(axis: Vector3) -> Basis:
    var z_axis := axis.normalized()
    if z_axis.length_squared() < 0.000001:
        z_axis = Vector3.FORWARD
    var reference := Vector3.RIGHT if absf(z_axis.dot(Vector3.UP)) > 0.999 else Vector3.UP
    var x_axis := reference.cross(z_axis).normalized()
    var y_axis := z_axis.cross(x_axis).normalized()
    return Basis(x_axis, y_axis, z_axis)
