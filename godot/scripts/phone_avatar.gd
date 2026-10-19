extends Node3D
## A render-only Digital Breakdown handset used by the Godot-first player.

var _screen: StandardMaterial3D
var _screen_label: Label3D
var _rear_label: Label3D

func _ready() -> void:
	var body := MeshInstance3D.new()
	body.name = "HandsetBody"
	var body_mesh := BoxMesh.new()
	body_mesh.size = Vector3(0.34, 0.58, 0.105)
	body.mesh = body_mesh
	var shell := StandardMaterial3D.new()
	shell.albedo_color = Color("#18222d")
	shell.metallic = 0.55
	shell.roughness = 0.28
	body.material_override = shell
	add_child(body)

	var screen := MeshInstance3D.new()
	screen.name = "Display"
	var screen_mesh := BoxMesh.new()
	screen_mesh.size = Vector3(0.285, 0.47, 0.008)
	screen.mesh = screen_mesh
	screen.position = Vector3(0, 0.005, -0.057)
	_screen = StandardMaterial3D.new()
	_screen.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	_screen.albedo_color = Color("#08252e")
	_screen.emission_enabled = true
	_screen.emission = Color("#10a6b3")
	_screen.emission_energy_multiplier = 0.45
	screen.material_override = _screen
	add_child(screen)

	_screen_label = Label3D.new()
	_screen_label.name = "DisplayText"
	_screen_label.text = "D.BREAK\nSIGNAL LOST\nFIGHT BACK"
	_screen_label.font_size = 72
	_screen_label.pixel_size = 0.00042
	_screen_label.position = Vector3(0, 0.005, -0.062)
	_screen_label.modulate = Color("#56e4ee")
	_screen_label.outline_size = 5
	_screen_label.no_depth_test = true
	add_child(_screen_label)
	_rear_label = Label3D.new()
	_rear_label.name = "RearMark"
	_rear_label.text = "D.BREAK\nSIGNAL"
	_rear_label.font_size = 52
	_rear_label.pixel_size = 0.00034
	_rear_label.position = Vector3(0, 0, 0.059)
	_rear_label.rotation.y = PI
	_rear_label.modulate = Color("#43d8e2")
	_rear_label.outline_size = 4
	add_child(_rear_label)

	var lens := MeshInstance3D.new()
	lens.name = "CameraLens"
	var lens_mesh := SphereMesh.new()
	lens_mesh.radius = 0.018
	lens_mesh.height = 0.036
	lens.mesh = lens_mesh
	lens.position = Vector3(0, 0.255, -0.06)
	var lens_material := StandardMaterial3D.new()
	lens_material.albedo_color = Color("#050a11")
	lens_material.metallic = 0.7
	lens.material_override = lens_material
	add_child(lens)

func set_damage_flash(active: bool) -> void:
	if _screen != null:
		_screen.emission = Color("#ff395d") if active else Color("#10a6b3")
