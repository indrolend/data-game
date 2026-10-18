extends Node3D
## Render-only translation of ParentGameNode's bounded native phone snapshot.
## First-person placement is a Godot view-model transform; it never feeds input
## or gameplay back into the native simulation.

var phone_root: Node3D
var body: MeshInstance3D
var glass: MeshInstance3D
var screen_text: Label3D
var lens: MeshInstance3D
var _body_material: StandardMaterial3D
var _screen_material: StandardMaterial3D
var _text_material: StandardMaterial3D

func _ready() -> void:
	phone_root = Node3D.new()
	phone_root.name = "PhoneRoot"
	add_child(phone_root)

	body = MeshInstance3D.new()
	body.name = "Body"
	body.mesh = BoxMesh.new()
	_body_material = StandardMaterial3D.new()
	_body_material.albedo_color = Color("#172530")
	_body_material.metallic = 0.72
	_body_material.roughness = 0.24
	body.material_override = _body_material
	phone_root.add_child(body)

	glass = MeshInstance3D.new()
	glass.name = "Screen"
	glass.mesh = BoxMesh.new()
	_screen_material = StandardMaterial3D.new()
	_screen_material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	_screen_material.albedo_color = Color("#07151c")
	_screen_material.emission_enabled = true
	_screen_material.emission = Color("#164a59")
	_screen_material.emission_energy_multiplier = 0.45
	glass.material_override = _screen_material
	phone_root.add_child(glass)

	screen_text = Label3D.new()
	screen_text.name = "ScreenText"
	screen_text.font_size = 64
	screen_text.pixel_size = 0.0002
	screen_text.line_spacing = 0.0
	screen_text.outline_size = 8
	_text_material = StandardMaterial3D.new()
	_text_material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	screen_text.material_override = _text_material
	phone_root.add_child(screen_text)

	lens = MeshInstance3D.new()
	lens.name = "CameraLens"
	var lens_mesh := SphereMesh.new()
	lens_mesh.radius = 0.004
	lens_mesh.height = 0.008
	lens.mesh = lens_mesh
	var lens_material := StandardMaterial3D.new()
	lens_material.albedo_color = Color("#061016")
	lens_material.metallic = 0.45
	lens_material.roughness = 0.16
	lens.material_override = lens_material
	phone_root.add_child(lens)

func sync_snapshot(snapshot: Dictionary, camera: Camera3D) -> void:
	var first_person: bool = snapshot.first_person
	if first_person:
		# Native first-person hides its world-space phone body. Present the same
		# native body/screen state as a camera-anchored view model in Godot.
		phone_root.set_as_top_level(true)
		var camera_basis: Basis = camera.global_basis
		var view_position: Vector3 = camera.global_position + camera_basis * Vector3(0.29, -0.27, -0.72)
		phone_root.global_transform = Transform3D(camera_basis, view_position)
	else:
		phone_root.set_as_top_level(false)
		phone_root.position = snapshot.position
		phone_root.quaternion = snapshot.orientation

	var body_size: Vector3 = snapshot.body_size
	var body_scale: Vector3 = snapshot.body_scale
	(body.mesh as BoxMesh).size = body_size * body_scale
	body.visible = bool(snapshot.body_visible) or first_person

	var screen_size: Vector3 = snapshot.screen_size
	(glass.mesh as BoxMesh).size = screen_size
	glass.position = snapshot.screen_center_offset
	glass.visible = bool(snapshot.screen_visible) or first_person
	screen_text.position = snapshot.screen_center_offset + Vector3(0.0, 0.0, screen_size.z * 0.5 + 0.0002)
	screen_text.visible = glass.visible
	lens.position = Vector3(0.0, body_size.y * body_scale.y * 0.39, body_size.z * body_scale.z * 0.5 + 0.0004)
	lens.visible = body.visible

	var tint: Color = snapshot.screen_tint
	var brightness: float = float(snapshot.screen_brightness)
	var glow: float = float(snapshot.screen_glow)
	_screen_material.albedo_color = Color(0.008, 0.018, 0.024).lerp(tint, 0.14)
	_screen_material.emission = tint
	_screen_material.emission_energy_multiplier = maxf(0.2, brightness * 0.7 + glow * 0.16)
	_text_material.albedo_color = tint.lightened(0.24)
	_text_material.emission = tint
	_text_material.emission_enabled = true
	_text_material.emission_energy_multiplier = maxf(0.25, float(snapshot.emission_strength) * 0.35)
	screen_text.modulate.a = clampf(float(snapshot.content_opacity), 0.0, 1.0)
	screen_text.text = _screen_status(snapshot)
	phone_root.set_meta("native_screen_mode", int(snapshot.screen_mode))

func _screen_status(snapshot: Dictionary) -> String:
	var line_four := "GAME"
	if bool(snapshot.low_battery):
		line_four = "LOW BAT"
	elif bool(snapshot.vacuum_active):
		line_four = "VAC %02d" % roundi(float(snapshot.vacuum_power) * 99.0)
	else:
		line_four = _mode_label(int(snapshot.screen_mode))
	return "D.BREAK\nBAT %03d\nSOUL %02d\n%s" % [
		roundi(float(snapshot.battery)), int(snapshot.souls), line_four]

func _mode_label(mode: int) -> String:
	match mode:
		1: return "BOOT"
		2: return "MENU"
		3, 4: return "ONLINE"
		5, 6, 7, 8: return "SETTINGS"
		10: return "PAUSE"
		11: return "UPGRADE"
		12: return "WARNING"
		13: return "OFFLINE"
		14: return "RESTART"
		15: return "TRANSIT"
		_: return "GAME"
