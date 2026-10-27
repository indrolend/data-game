extends Node3D

## Render-only geometry for particles simulated by the parent Game.
## DesktopRenderer uses rotating digital boxes and surface-aligned residue.

const SIGNAL_RESIDUE := 5
const RESIDUE_LOBES := 4
const RESIDUE_GLINTS := 3

var particles: MultiMeshInstance3D
var multimesh: MultiMesh
var residue_lobes: MultiMeshInstance3D
var residue_lobe_multimesh: MultiMesh
var residue_glints: MultiMeshInstance3D
var residue_glint_multimesh: MultiMesh


func _ready() -> void:
	particles = _make_instances("NativeDigitalParticles")
	multimesh = particles.multimesh
	residue_lobes = _make_instances("NativeSignalResidueLobes")
	residue_lobe_multimesh = residue_lobes.multimesh
	residue_glints = _make_instances("NativeSignalResidueGlints")
	residue_glint_multimesh = residue_glints.multimesh


func _make_instances(node_name: String) -> MultiMeshInstance3D:
	var instance := MultiMeshInstance3D.new()
	instance.name = node_name
	var instances := MultiMesh.new()
	instances.transform_format = MultiMesh.TRANSFORM_3D
	instances.use_colors = true
	var box := BoxMesh.new()
	box.size = Vector3.ONE
	var material := StandardMaterial3D.new()
	material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	material.vertex_color_use_as_albedo = true
	material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	material.cull_mode = BaseMaterial3D.CULL_DISABLED
	box.material = material
	instances.mesh = box
	instance.multimesh = instances
	instance.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	add_child(instance)
	return instance


func sync_from_native(snapshot: Array, camera: Camera3D = null) -> void:
	var airborne_count := 0
	var settled_count := 0
	for value in snapshot:
		var particle: Dictionary = value
		if int(particle.material) == SIGNAL_RESIDUE and bool(particle.settled):
			settled_count += 1
		else:
			airborne_count += 1
	_resize(multimesh, airborne_count)
	_resize(residue_lobe_multimesh, settled_count * RESIDUE_LOBES)
	_resize(residue_glint_multimesh, settled_count * RESIDUE_GLINTS)
	var airborne_index := 0
	var lobe_index := 0
	var glint_index := 0
	for value in snapshot:
		var particle: Dictionary = value
		if int(particle.material) == SIGNAL_RESIDUE and bool(particle.settled):
			var next := _write_residue(particle, lobe_index, glint_index)
			lobe_index = next.x
			glint_index = next.y
		else:
			_write_airborne(particle, airborne_index, camera)
			airborne_index += 1


func _resize(instances: MultiMesh, count: int) -> void:
	instances.instance_count = maxi(instances.instance_count, count)
	instances.visible_instance_count = count


func _write_airborne(particle: Dictionary, index: int, camera: Camera3D) -> void:
	var position: Vector3 = particle.position
	var size := maxf(0.001, float(particle.size) * float(particle.remaining))
	if camera != null:
		size = minf(size, position.distance_to(camera.global_position) * 0.035)
	var life := float(particle.life)
	var rotation := Basis.from_euler(Vector3(life * 8.0, life * 4.0, life * 6.0))
	multimesh.set_instance_transform(index,
		Transform3D(rotation.scaled(Vector3.ONE * size), position))
	multimesh.set_instance_color(index, particle.color)


func _write_residue(particle: Dictionary, first_lobe: int, first_glint: int) -> Vector2i:
	var position: Vector3 = particle.position
	var normal: Vector3 = particle.surface_normal.normalized()
	var remaining := float(particle.remaining)
	var spread := maxf(0.035, float(particle.size) * remaining)
	var seed := _residue_seed(position)
	var base_rotation := _hashf(seed, 1) * PI * 2.0
	var color: Color = particle.color
	var dark := Color(color.r * 0.30, color.g * 0.22, color.b * 0.34, 0.70 * remaining)
	var tangent := (Vector3.RIGHT if absf(normal.y) > 0.92 else Vector3.UP).cross(normal).normalized()
	var bitangent := normal.cross(tangent).normalized()
	var lobe_index := first_lobe
	for lobe in range(RESIDUE_LOBES):
		var angle := base_rotation + float(lobe) * PI * 0.5 + (_hashf(seed, 10 + lobe) - 0.5) * 1.1
		var radius := spread * (0.30 + 0.55 * _hashf(seed, 20 + lobe))
		var offset := tangent * (cos(angle) * radius) + bitangent * (sin(angle) * radius) + normal * 0.005
		var size := Vector3(spread * (0.95 + 0.65 * _hashf(seed, 30 + lobe)),
			0.010, spread * (0.75 + 0.60 * _hashf(seed, 40 + lobe)))
		residue_lobe_multimesh.set_instance_transform(lobe_index,
			_surface_box(position + offset, normal, tangent, bitangent, angle, size))
		residue_lobe_multimesh.set_instance_color(lobe_index, dark)
		lobe_index += 1
	var glint_index := first_glint
	for glint in range(RESIDUE_GLINTS):
		var angle := base_rotation * 1.7 + _hashf(seed, 50 + glint) * PI * 2.0
		var radius := spread * 0.4 * _hashf(seed, 60 + glint)
		var offset := tangent * (cos(angle) * radius) + bitangent * (sin(angle) * radius) + normal * 0.011
		var glint_size := spread * (0.12 + 0.10 * _hashf(seed, 70 + glint))
		residue_glint_multimesh.set_instance_transform(glint_index,
			_surface_box(position + offset, normal, tangent, bitangent, angle,
				Vector3(glint_size, 0.006, glint_size)))
		residue_glint_multimesh.set_instance_color(glint_index,
			Color(color.r, color.g, color.b, 0.38 * remaining))
		glint_index += 1
	return Vector2i(lobe_index, glint_index)


func _surface_box(position: Vector3, normal: Vector3, tangent: Vector3,
		bitangent: Vector3, angle: float, scale: Vector3) -> Transform3D:
	var x_axis := tangent * cos(angle) + bitangent * sin(angle)
	var z_axis := -tangent * sin(angle) + bitangent * cos(angle)
	return Transform3D(Basis(x_axis, normal, z_axis).scaled(scale), position)


func _residue_seed(position: Vector3) -> int:
	var seed := int(absf(position.x) * 7919.0) ^ int(absf(position.z) * 4733.0)
	seed = seed ^ int(absf(position.y) * 104729.0)
	return (seed * 2654435761) & 0xffffffff


func _hashf(seed: int, salt: int) -> float:
	var value := ((seed ^ ((salt * 2246822519) & 0xffffffff)) * 3266489917) & 0xffffffff
	value = (value ^ (value >> 15)) & 0xffffffff
	value = (value * 2654435761) & 0xffffffff
	value = (value ^ (value >> 13)) & 0xffffffff
	return float(value & 0xffff) / 65535.0


func presentation_counts() -> Dictionary:
	return {
		"airborne": multimesh.visible_instance_count,
		"residue_lobes": residue_lobe_multimesh.visible_instance_count,
		"residue_glints": residue_glint_multimesh.visible_instance_count,
	}
