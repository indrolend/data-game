extends Node3D

## Render-only instances of particles already simulated by the parent Game.
## Position, lifetime, material identity, size, and color come from native state.

var particles: MultiMeshInstance3D
var multimesh: MultiMesh

func _ready() -> void:
	particles = MultiMeshInstance3D.new()
	particles.name = "NativeDigitalParticles"
	multimesh = MultiMesh.new()
	multimesh.transform_format = MultiMesh.TRANSFORM_3D
	multimesh.use_colors = true
	var sphere := SphereMesh.new()
	sphere.radius = 0.5
	sphere.height = 1.0
	var material := StandardMaterial3D.new()
	material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	material.vertex_color_use_as_albedo = true
	material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	material.no_depth_test = false
	material.cull_mode = BaseMaterial3D.CULL_DISABLED
	sphere.material = material
	multimesh.mesh = sphere
	particles.multimesh = multimesh
	particles.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	add_child(particles)

func sync_from_native(snapshot: Array, camera: Camera3D = null) -> void:
	multimesh.instance_count = maxi(multimesh.instance_count, snapshot.size())
	multimesh.visible_instance_count = snapshot.size()
	for index in range(snapshot.size()):
		var particle: Dictionary = snapshot[index]
		var position: Vector3 = particle.position
		var size := maxf(0.001, float(particle.size))
		if camera != null:
			# Native particles retain their physical size. Cap only the Godot
			# representation near the lens, where a world-space sphere otherwise
			# expands across the whole first-person frame.
			var camera_distance := position.distance_to(camera.global_position)
			size = minf(size, camera_distance * 0.035)
		var transform := Transform3D(Basis().scaled(Vector3.ONE * size * 2.0), position)
		multimesh.set_instance_transform(index, transform)
		multimesh.set_instance_color(index, particle.color)
