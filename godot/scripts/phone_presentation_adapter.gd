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
var _menu_title: Label3D
var _menu_hint: Label3D
var _menu_row_labels: Array[Label3D] = []

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
	_menu_title = _new_menu_label("MenuTitle")
	_menu_hint = _new_menu_label("MenuHint")
	for i in range(48):
		_menu_row_labels.append(_new_menu_label("MenuRow%02d" % i))

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
		# The original phone is physically small; enlarge and move the render-only
		# view model inward so its native menu remains readable at normal window size.
		var view_position: Vector3 = camera.global_position + camera_basis * Vector3(0.22, -0.17, -0.72)
		phone_root.global_transform = Transform3D(camera_basis, view_position)
		phone_root.scale = Vector3.ONE * 1.5
	else:
		phone_root.set_as_top_level(false)
		phone_root.position = snapshot.position
		phone_root.quaternion = snapshot.orientation
		phone_root.scale = Vector3.ONE

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
	_text_material.albedo_color = tint.lightened(0.38)
	_text_material.emission = tint
	_text_material.emission_enabled = true
	_text_material.emission_energy_multiplier = maxf(0.65, float(snapshot.emission_strength) * 0.65)
	var menu_layout: Dictionary = snapshot.get("menu_layout", {})
	var opacity := clampf(float(snapshot.content_opacity), 0.0, 1.0)
	var show_menu := bool(menu_layout.get("active", false)) and glass.visible
	screen_text.visible = glass.visible and not show_menu
	screen_text.modulate.a = opacity
	screen_text.text = _screen_status(snapshot)
	_sync_menu_layout(menu_layout, screen_size, snapshot.screen_center_offset, show_menu, opacity)
	phone_root.set_meta("native_screen_mode", int(snapshot.screen_mode))
	phone_root.set_meta("native_menu_row_count", int(menu_layout.get("row_count", 0)))
	phone_root.set_meta("native_menu_selection", int(menu_layout.get("selection", -1)))

func _new_menu_label(label_name: String) -> Label3D:
	var label := Label3D.new()
	label.name = label_name
	label.font_size = 48
	label.pixel_size = 0.0001
	label.outline_size = 5
	label.material_override = _text_material
	label.vertical_alignment = VERTICAL_ALIGNMENT_BOTTOM
	label.visible = false
	phone_root.add_child(label)
	return label

func _sync_menu_layout(layout: Dictionary, screen_size: Vector3, screen_center: Vector3,
		show_menu: bool, opacity: float) -> void:
	_menu_title.visible = false
	_menu_hint.visible = false
	for label in _menu_row_labels:
		label.visible = false
	if not show_menu:
		return
	var logical_width := maxi(1, int(layout.get("logical_width", 720)))
	var logical_height := maxi(1, int(layout.get("logical_height", 1280)))
	var pixel_size := screen_size.x / float(logical_width)
	var text_z := screen_center.z + screen_size.z * 0.5 + 0.0003
	var title := str(layout.get("title", ""))
	if not title.is_empty():
		_menu_title.text = title
		_menu_title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		_menu_title.font_size = maxi(1, roundi(float(layout.get("title_px", 52.0))))
		_menu_title.pixel_size = pixel_size
		_menu_title.position = screen_center + Vector3(0.0,
			(0.5 - float(layout.get("title_center_y", 100.0)) / logical_height) * screen_size.y,
			text_z - screen_center.z)
		_menu_title.modulate = Color(0.90, 0.97, 1.0, 0.96 * opacity)
		_menu_title.visible = true
	var rows: Array = layout.get("rows", [])
	var label_index := 0
	for row_value in rows:
		var row: Dictionary = row_value
		if not bool(row.get("visible", false)) or bool(row.get("peek", false)):
			continue
		var kind := int(row.get("kind", 1))
		var selected := bool(row.get("selected", false))
		var emphasis := int(row.get("emphasis", 0))
		var color := Color("#e6edf0")
		if kind == 0:
			color = Color("#4d7380")
		elif emphasis == 2:
			color = Color("#aa8066")
		elif selected and emphasis == 1:
			color = Color("#d4ec8e")
		if not selected and kind != 0:
			color.a = 0.72
		var row_label := _menu_row_labels[label_index]
		label_index += 1
		_configure_menu_label(row_label, str(row.get("label", "")),
			float(row.get("label_x", 0.0)), float(row.get("baseline_y", 0.0)),
			float(row.get("font_px", 34.0)), color, logical_width, logical_height,
			pixel_size, text_z, screen_size, screen_center, opacity)
		if kind == 2 and label_index < _menu_row_labels.size():
			var value_label := _menu_row_labels[label_index]
			label_index += 1
			var value_color := Color("#78d5e1") if selected else Color("#4d7380")
			value_color.a *= 0.98 if selected else 0.78
			_configure_menu_label(value_label, str(row.get("value", "")),
				float(row.get("value_right_x", 0.0)), float(row.get("baseline_y", 0.0)),
				float(row.get("font_px", 34.0)), value_color, logical_width, logical_height,
				pixel_size, text_z, screen_size, screen_center, opacity,
				HORIZONTAL_ALIGNMENT_RIGHT)
	var hint := str(layout.get("navigation_hint", ""))
	if not hint.is_empty():
		_menu_hint.text = hint
		_menu_hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		_menu_hint.font_size = 26
		_menu_hint.pixel_size = pixel_size
		_menu_hint.position = screen_center + Vector3(0.0,
			(0.5 - (logical_height - 66.0) / logical_height) * screen_size.y,
			text_z - screen_center.z)
		_menu_hint.modulate = Color("#78d5e1")
		_menu_hint.modulate.a = 0.62 * opacity
		_menu_hint.visible = true

func _configure_menu_label(label: Label3D, text: String, x: float, baseline_y: float,
		font_px: float, color: Color, logical_width: int, logical_height: int,
		pixel_size: float, text_z: float, screen_size: Vector3, screen_center: Vector3,
		opacity: float, alignment: HorizontalAlignment = HORIZONTAL_ALIGNMENT_LEFT) -> void:
	label.text = text
	label.horizontal_alignment = alignment
	label.vertical_alignment = VERTICAL_ALIGNMENT_BOTTOM
	label.font_size = maxi(1, roundi(font_px))
	label.pixel_size = pixel_size
	label.position = screen_center + Vector3(
		(x / logical_width - 0.5) * screen_size.x,
		(0.5 - baseline_y / logical_height) * screen_size.y,
		text_z - screen_center.z)
	label.modulate = Color(color.r, color.g, color.b, color.a * opacity)
	label.visible = not text.is_empty()

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
