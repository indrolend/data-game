extends Node3D
## Render-only translation of ParentGameNode's bounded native phone snapshot.
## First-person placement is a Godot view-model transform; it never feeds input
## or gameplay back into the native simulation.

const NATIVE_STENCIL_HORIZONTAL_SCALE := 0.84
const WORLD_LIGHT_LAYER := 1
const PHONE_RECEIVER_LAYER := 2

var phone_root: Node3D
var body: MeshInstance3D
var glass: MeshInstance3D
var screen_text: Label3D
var lens: MeshInstance3D
var screen_light: OmniLight3D
var _body_material: StandardMaterial3D
var _screen_material: StandardMaterial3D
var _menu_title: Label3D
var _menu_hint: Label3D
var _menu_row_labels: Array[Label3D] = []
var _native_body_loaded := false

func _ready() -> void:
	phone_root = Node3D.new()
	phone_root.name = "PhoneRoot"
	add_child(phone_root)

	body = MeshInstance3D.new()
	body.name = "Body"
	body.mesh = _load_native_phone_mesh("res://../parent/models/phone.dbmesh")
	_native_body_loaded = body.mesh != null
	if not _native_body_loaded:
		body.mesh = BoxMesh.new()
		_body_material = StandardMaterial3D.new()
		_body_material.albedo_color = Color("#172530")
		_body_material.metallic = 0.72
		_body_material.roughness = 0.24
		body.material_override = _body_material
	body.layers = PHONE_RECEIVER_LAYER
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
	glass.layers = PHONE_RECEIVER_LAYER
	phone_root.add_child(glass)
	screen_light = OmniLight3D.new()
	screen_light.name = "NativePhoneScreenLight"
	screen_light.shadow_enabled = false
	screen_light.light_specular = 0.35
	# The display illuminates the world, not its own metal/glass assembly.
	screen_light.light_cull_mask = WORLD_LIGHT_LAYER
	phone_root.add_child(screen_light)

	screen_text = Label3D.new()
	screen_text.name = "ScreenText"
	screen_text.font_size = 52
	screen_text.pixel_size = 0.00011
	screen_text.line_spacing = 0.0
	screen_text.outline_size = 5
	screen_text.scale = Vector3(NATIVE_STENCIL_HORIZONTAL_SCALE, 1.0, 1.0)
	# Label3D owns a font-atlas shader. A StandardMaterial override turns each
	# glyph quad into a solid rectangle and makes native menu text unreadable.
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
	lens.layers = PHONE_RECEIVER_LAYER
	phone_root.add_child(lens)

func sync_snapshot(snapshot: Dictionary, camera: Camera3D) -> void:
	var first_person: bool = snapshot.first_person
	var menu_layout: Dictionary = snapshot.get("menu_layout", {})
	var menu_active := bool(menu_layout.get("active", false))
	# A gameplay phone is a native world-space object. The desktop pause/menu is
	# instead composed as a readable handset in the camera frame. Preserve the
	# native transform and alter only this render-only presentation placement.
	var camera_presented := first_person or menu_active
	if camera_presented:
		# Native first-person hides the world body; active menus use that same
		# camera-anchored presentation so third-person pause is not microscopic.
		phone_root.set_as_top_level(true)
		var camera_basis: Basis = camera.global_basis
		# Match the native pause composition at 1280x720: the complete handset is
		# centered, readable, and occupies roughly three quarters of the frame.
		var view_position := camera.global_position + camera_basis * Vector3(0.0, 0.05, -0.70)
		phone_root.global_transform = Transform3D(camera_basis, view_position)
		phone_root.scale = Vector3.ONE * 3.4
	else:
		phone_root.set_as_top_level(false)
		phone_root.position = snapshot.position
		phone_root.quaternion = snapshot.orientation
		phone_root.scale = Vector3.ONE

	var body_size: Vector3 = snapshot.body_size
	var body_scale: Vector3 = snapshot.body_scale
	if _native_body_loaded:
		body.scale = body_scale
	else:
		(body.mesh as BoxMesh).size = body_size * body_scale
		body.scale = Vector3.ONE
	body.visible = bool(snapshot.body_visible) or first_person

	var screen_size: Vector3 = snapshot.screen_size
	(glass.mesh as BoxMesh).size = screen_size
	glass.position = snapshot.screen_center_offset
	glass.visible = bool(snapshot.screen_visible) or first_person
	screen_light.position = snapshot.screen_center_offset + Vector3(0.0, 0.0,
		float(snapshot.screen_light_forward_offset))
	screen_light.visible = glass.visible and float(snapshot.screen_light_intensity) > 0.0001
	screen_light.light_color = snapshot.screen_light_color
	var light_pulse := clampf(float(snapshot.screen_light_pulse), 0.0, 1.0)
	screen_light.light_energy = float(snapshot.screen_light_intensity) * (1.0 + light_pulse * 0.25)
	screen_light.omni_range = float(snapshot.screen_light_radius)
	screen_text.position = snapshot.screen_center_offset + Vector3(0.0, 0.0, screen_size.z * 0.5 + 0.0002)
	screen_text.visible = glass.visible
	lens.position = Vector3(0.0, body_size.y * body_scale.y * 0.39, body_size.z * body_scale.z * 0.5 + 0.0004)
	# The fallback lens is a third-person body detail. Unlike the authored native
	# phone mesh, its sphere protrudes over a camera-facing menu and reads as an orb.
	lens.visible = body.visible and not _native_body_loaded and not first_person and not bool(snapshot.interactive)

	var tint: Color = snapshot.screen_tint
	var brightness: float = float(snapshot.screen_brightness)
	var native_background := _native_display_background(tint, brightness)
	_screen_material.albedo_color = native_background
	_screen_material.emission = native_background
	_screen_material.emission_energy_multiplier = maxf(0.0, float(snapshot.emission_strength))
	screen_text.modulate = tint.lightened(0.38)
	var opacity := clampf(float(snapshot.content_opacity), 0.0, 1.0)
	var show_menu := bool(menu_layout.get("active", false)) and glass.visible
	screen_text.visible = glass.visible and not show_menu
	screen_text.modulate.a = opacity
	screen_text.text = _screen_status(snapshot)
	_sync_menu_layout(menu_layout, screen_size, snapshot.screen_center_offset, show_menu, opacity)
	phone_root.set_meta("native_screen_mode", int(snapshot.screen_mode))
	phone_root.set_meta("native_menu_row_count", int(menu_layout.get("row_count", 0)))
	phone_root.set_meta("native_menu_selection", int(menu_layout.get("selection", -1)))


func viewport_to_phone_logical(viewport_position: Vector2, camera: Camera3D, layout: Dictionary):
	if phone_root == null or glass == null or camera == null or not bool(layout.get("active", false)):
		return null
	var screen_size: Vector3 = (glass.mesh as BoxMesh).size
	if screen_size.x <= 0.0 or screen_size.y <= 0.0:
		return null
	var screen_center: Vector3 = glass.position
	var plane_local_z := screen_center.z + screen_size.z * 0.5
	var plane_point := phone_root.to_global(Vector3(screen_center.x, screen_center.y, plane_local_z))
	var plane_normal := (phone_root.global_transform.basis * Vector3(0.0, 0.0, 1.0)).normalized()
	var ray_origin := camera.project_ray_origin(viewport_position)
	var ray_direction := camera.project_ray_normal(viewport_position).normalized()
	var denominator := plane_normal.dot(ray_direction)
	if absf(denominator) <= 0.000001:
		return null
	var distance := plane_normal.dot(plane_point - ray_origin) / denominator
	if distance <= 0.0:
		return null
	var local_hit := phone_root.to_local(ray_origin + ray_direction * distance)
	var local_x := local_hit.x - screen_center.x
	var local_y := local_hit.y - screen_center.y
	if absf(local_x) > screen_size.x * 0.5 or absf(local_y) > screen_size.y * 0.5:
		return null
	var logical_width := float(maxi(1, int(layout.get("logical_width", 720))))
	var logical_height := float(maxi(1, int(layout.get("logical_height", 1280))))
	return Vector2((local_x / screen_size.x + 0.5) * logical_width, (0.5 - local_y / screen_size.y) * logical_height)

func _native_display_background(tint: Color, brightness: float) -> Color:
	# Mirrors DesktopRenderer::renderPhoneDisplayPixels' clear and full-canvas
	# signal wash. Godot still draws labels separately, but the display substrate
	# now has the same state-derived color instead of an arbitrary dark glass.
	var base := Color(0.025 + tint.r * 0.18, 0.045 + tint.g * 0.16,
		0.060 + tint.b * 0.15, 1.0)
	var wash_alpha := clampf(0.10 + brightness * 0.08, 0.0, 1.0)
	return base.lerp(Color(0.03, 0.55, 0.62, 1.0), wash_alpha)

func _load_native_phone_mesh(path: String) -> ArrayMesh:
	var file := FileAccess.open(ProjectSettings.globalize_path(path), FileAccess.READ)
	if file == null:
		push_error("Could not open native phone mesh: " + path)
		return null
	var bytes := file.get_buffer(file.get_length())
	file.close()
	if bytes.size() < 12 or bytes.slice(0, 4).get_string_from_ascii() != "DBM1":
		push_error("Invalid native phone mesh header")
		return null
	var vertex_count := int(bytes.decode_u32(4))
	var batch_count := int(bytes.decode_u32(8))
	var expected_size := 12 + vertex_count * 12 + batch_count * 24
	if vertex_count <= 0 or vertex_count > 1000000 or batch_count <= 0 or \
		batch_count > 1024 or bytes.size() != expected_size:
		push_error("Invalid native phone mesh dimensions")
		return null
	var vertex_offset := 12
	var batch_offset := vertex_offset + vertex_count * 12
	var mesh := ArrayMesh.new()
	for batch_index in range(batch_count):
		var batch_at := batch_offset + batch_index * 24
		var first_vertex := int(bytes.decode_u32(batch_at))
		var batch_vertices := int(bytes.decode_u32(batch_at + 4))
		if batch_vertices <= 0 or batch_vertices % 3 != 0 or \
			first_vertex + batch_vertices > vertex_count:
			push_error("Invalid native phone mesh batch")
			return null
		var vertices := PackedVector3Array()
		var normals := PackedVector3Array()
		vertices.resize(batch_vertices)
		normals.resize(batch_vertices)
		for vertex_index in range(batch_vertices):
			var source_at := vertex_offset + (first_vertex + vertex_index) * 12
			vertices[vertex_index] = Vector3(bytes.decode_float(source_at),
				bytes.decode_float(source_at + 4), bytes.decode_float(source_at + 8))
		for triangle in range(0, batch_vertices, 3):
			var normal := (vertices[triangle + 1] - vertices[triangle]).cross(
				vertices[triangle + 2] - vertices[triangle]).normalized()
			normals[triangle] = normal
			normals[triangle + 1] = normal
			normals[triangle + 2] = normal
		var arrays: Array = []
		arrays.resize(Mesh.ARRAY_MAX)
		arrays[Mesh.ARRAY_VERTEX] = vertices
		arrays[Mesh.ARRAY_NORMAL] = normals
		mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
		var material := StandardMaterial3D.new()
		material.albedo_color = Color(bytes.decode_float(batch_at + 8),
			bytes.decode_float(batch_at + 12), bytes.decode_float(batch_at + 16),
			bytes.decode_float(batch_at + 20))
		material.metallic = 0.72
		material.roughness = 0.24
		mesh.surface_set_material(mesh.get_surface_count() - 1, material)
	return mesh

func _new_menu_label(label_name: String) -> Label3D:
	var label := Label3D.new()
	label.name = label_name
	label.font_size = 48
	label.pixel_size = 0.0001
	label.outline_size = 5
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
	# DesktopRenderer's phone stencil compresses glyphs horizontally before
	# drawing from this same native label_x. Match that footprint so rows do not
	# drift right merely because Godot uses a different font renderer.
	label.scale = Vector3(NATIVE_STENCIL_HORIZONTAL_SCALE, 1.0, 1.0)
	label.position = screen_center + Vector3(
		(x / logical_width - 0.5) * screen_size.x,
		(0.5 - baseline_y / logical_height) * screen_size.y,
		text_z - screen_center.z)
	label.modulate = Color(color.r, color.g, color.b, color.a * opacity)
	label.visible = not text.is_empty()

func _screen_status(snapshot: Dictionary) -> String:
	var battery_percent := clampi(roundi(float(snapshot.battery)), 0, 100)
	var required := maxi(0, int(snapshot.get("required_goals", 0)))
	var filled := clampi(int(snapshot.get("filled_goals", 0)), 0, required)
	var state_label := _mode_label(int(snapshot.screen_mode))
	if bool(snapshot.low_battery):
		state_label = "LOW BAT"
	elif bool(snapshot.vacuum_active):
		state_label = "VAC %02d" % roundi(float(snapshot.vacuum_power) * 99.0)
	elif bool(snapshot.get("room_clear", false)):
		state_label = "ROOM CLEAR"
	var lines: Array[String] = [
		"ROOM %02d  T %02d" % [int(snapshot.get("room_index", 0)), int(snapshot.get("tokens", 0))],
		"GOAL %02d/%02d" % [filled, required],
		"CHARGE %03d" % battery_percent,
		"SOUL %02d/%02d" % [int(snapshot.souls), int(snapshot.get("soul_capacity", 30))],
		"MEMORY %d/3" % int(snapshot.get("unlocked_material_memories", 0)),
		state_label
	]
	if bool(snapshot.get("supplemental_active", false)):
		lines.append("POWER %d  %03d%%" % [int(snapshot.get("flower_stacks", 0)),
			roundi(float(snapshot.get("supplemental_fill", 0.0)) * 100.0)])
	return "\n".join(lines)

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
