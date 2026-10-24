extends SceneTree

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
	if not condition:
		failures.append(message)
		push_error("FAIL: " + message)

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	var world: Node3D = load("res://scenes/parent_reference.tscn").instantiate()
	root.add_child(world)
	await process_frame
	await process_frame

	var runtime: Node = world.get("runtime")
	var adapter: Node = world.get_node("PhonePresentationAdapter")
	var snapshot: Dictionary = runtime.phone_presentation_snapshot()
	var phone_root: Node3D = adapter.get_node("PhoneRoot")
	var body: MeshInstance3D = phone_root.get_node("Body")
	var screen: MeshInstance3D = phone_root.get_node("Screen")
	var screen_raster: MeshInstance3D = phone_root.get_node("NativeDisplayRaster")
	var screen_text: Label3D = phone_root.get_node("ScreenText")
	var lens: MeshInstance3D = phone_root.get_node("CameraLens")
	var screen_light: OmniLight3D = phone_root.get_node("NativePhoneScreenLight")
	var screen_material: StandardMaterial3D = screen.material_override
	var body_mesh: ArrayMesh = body.mesh
	var screen_mesh: BoxMesh = screen.mesh
	check(not snapshot.first_person, "default fixture starts in third-person mode")
	check(phone_root.global_position.distance_to(snapshot.position) <= 0.00001,
		"third-person adapter follows native phone position")
	check(phone_root.quaternion.is_equal_approx(snapshot.orientation),
		"third-person adapter follows native phone orientation")
	var native_vertex_count := 0
	for surface_index in range(body_mesh.get_surface_count()):
		native_vertex_count += body_mesh.surface_get_array_len(surface_index)
	check(body_mesh.get_surface_count() == 7 and native_vertex_count == 11820 and
		body_mesh.get_aabb().size.distance_to(Vector3(0.07915, 0.16, 0.01214)) <= 0.00002 and
		body.scale.distance_to(snapshot.body_scale) <= 0.00001,
		"phone body uses the original seven-batch native mesh, bounds, and native pose scale")
	check(body.material_override == null,
		"native phone body retains non-emissive per-batch materials")
	check(screen_mesh.size.distance_to(snapshot.screen_size) <= 0.00001,
		"screen dimensions come from native snapshot")
	check(screen.position.distance_to(snapshot.screen_center_offset) <= 0.00001,
		"screen position comes from native screen transform")
	check(screen_text.text.contains("ROOM %02d" % int(snapshot.room_index)) and
		screen_text.text.contains("T %02d" % int(snapshot.tokens)),
		"gameplay screen identifies the authoritative native room and token economy")
	check(screen_text.text.contains("GOAL %02d/%02d" % [int(snapshot.filled_goals),
		int(snapshot.required_goals)]) and
		screen_text.text.contains("CHARGE %03d" % roundi(snapshot.battery)),
		"gameplay screen reflects native goal and battery state")
	check(screen_text.text.contains("SOUL %02d/%02d" % [int(snapshot.souls),
		int(snapshot.soul_capacity)]) and screen_text.text.contains("MEMORY %d/3" %
		int(snapshot.unlocked_material_memories)),
		"gameplay screen reflects native soul capacity and material-memory progression")
	check(screen_light.light_color.is_equal_approx(snapshot.screen_light_color) and
		absf(screen_light.light_energy - float(snapshot.screen_light_intensity) *
			(1.0 + clampf(float(snapshot.screen_light_pulse), 0.0, 1.0) * 0.25)) <= 0.00001 and
		absf(screen_light.omni_range - float(snapshot.screen_light_radius)) <= 0.00001,
		"phone screen point light color, intensity, and radius come from native display-light state")
	check(screen_light.position.distance_to(snapshot.screen_center_offset + Vector3(0.0, 0.0,
		float(snapshot.screen_light_forward_offset))) <= 0.00001,
		"phone screen light is placed at the native display-light forward offset")
	check(screen_light.light_cull_mask == 1 and body.layers == 2 and screen.layers == 2 and lens.layers == 2,
		"screen light illuminates the world while the phone assembly remains a non-emissive receiver")
	var duplicate_scene_phone_light := false
	for resolved_light: OmniLight3D in world.native_point_lights:
		duplicate_scene_phone_light = duplicate_scene_phone_light or resolved_light.visible
	check(not duplicate_scene_phone_light,
		"default Field scene has one screen-attached phone light, not a duplicate resolved omni source")
	check(absf(screen_material.emission_energy_multiplier - float(snapshot.emission_strength)) <= 0.00001,
		"screen material emission strength is copied directly from native display state")
	var tint: Color = snapshot.screen_tint
	var base := Color(0.025 + tint.r * 0.18, 0.045 + tint.g * 0.16,
		0.060 + tint.b * 0.15, 1.0)
	var expected_background := base.lerp(Color(0.03, 0.55, 0.62, 1.0),
		clampf(0.10 + float(snapshot.screen_brightness) * 0.08, 0.0, 1.0))
	check(screen_material.albedo_color.is_equal_approx(expected_background) and
		screen_material.emission.is_equal_approx(expected_background),
		"screen substrate follows the native phone-display background formula")
	var gameplay_screen_energy := screen_light.light_energy
	var menu: Dictionary = snapshot.menu_layout
	check(not bool(menu.active), "gameplay display is not mistaken for an interactive phone menu")
	check(int(menu.logical_width) == 720 and int(menu.logical_height) == 1280,
		"phone menu keeps the original native logical canvas dimensions")
	runtime.set_paused(true)
	runtime.tick()
	world.call("_sync")
	menu = runtime.phone_menu_layout_snapshot()
	snapshot = runtime.phone_presentation_snapshot()
	var menu_rows: Array = menu.rows
	check(bool(menu.active), "native pause mode enables the original phone menu")
	check(screen_light.light_energy > gameplay_screen_energy,
		"native menu mode increases character phone-screen light output")
	check(menu_rows.size() == 3 and menu_rows[0].label == "Resume" and
		menu_rows[1].label == "Settings" and menu_rows[2].label == "Exit Run",
		"pause labels and ordering come from the parent PhoneMenuModel")
	check(bool(menu_rows[0].selected), "native menu selection is reflected in its layout row")
	var raster_revision := int(runtime.phone_display_raster_revision())
	var raster_pixels: PackedByteArray = runtime.phone_display_raster_pixels()
	var raster_hash := hash(raster_pixels)
	check(raster_pixels.size() == 720 * 1280 * 4 and raster_hash != 0,
		"native bridge produces a complete deterministic 720x1280 RGBA phone raster")
	check(screen_raster.visible and screen_raster.material_override.albedo_texture != null and
		int(phone_root.get_meta("native_raster_revision", -1)) == raster_revision,
		"Godot uploads the current native raster revision to the phone screen")
	check(not (phone_root.get_node("MenuRow00") as Label3D).visible and
		not (phone_root.get_node("MenuRow01") as Label3D).visible,
		"native raster replaces the prior Label3D menu typography")
	check(not lens.visible,
		"interactive phone menu hides the fallback lens sphere from the display surface")
	# Both presentation modes must consume the same native menu snapshot.
	adapter.set("menu_renderer", 1)
	world.call("_sync")
	var semantic_highlight: MeshInstance3D = phone_root.get_node("MenuSelectionHighlight")
	check(not screen_raster.visible and (phone_root.get_node("MenuRow00") as Label3D).visible and
		semantic_highlight.visible,
		"Godot semantic renderer displays native menu labels and selection rectangle")
	check(int(phone_root.get_meta("native_menu_selection", -1)) == int(menu.selection),
		"Godot semantic renderer retains native menu selection authority")
	adapter.set("menu_renderer", 0)
	world.call("_sync")
	check(screen_raster.visible and not semantic_highlight.visible,
		"switching back restores native raster and hides semantic selection overlay")
	var raster_texture: Texture2D = screen_raster.material_override.albedo_texture
	world.call("_sync")
	check(screen_raster.material_override.albedo_texture == raster_texture,
		"unchanged menu revision retains the existing GPU raster texture")
	runtime.menu_move_selection(1)
	world.call("_sync")
	check(screen_raster.material_override.albedo_texture == raster_texture and
		int(phone_root.get_meta("native_raster_revision", -1)) == int(runtime.phone_display_raster_revision()),
		"changed menu selection updates the existing GPU raster texture in place")
	menu = runtime.phone_menu_layout_snapshot()
	check(int(menu.selection) == 1 and bool(menu.rows[1].selected),
		"semantic menu navigation updates native selection")
	check(int(runtime.phone_display_raster_revision()) != raster_revision and
		hash(runtime.phone_display_raster_pixels() as PackedByteArray) != raster_hash,
		"native selection changes both raster revision and display bytes")
	check(runtime.menu_confirm(), "native settings menu action is handled")
	menu = runtime.phone_menu_layout_snapshot()
	check(str(menu.rows[0].label) == "Controls" and
		str(menu.rows[1].label) == "Audio" and str(menu.rows[2].label) == "Graphics",
		"native menu confirmation enters the existing settings page")
	runtime.menu_move_selection(1)
	runtime.menu_move_selection(1)
	runtime.menu_confirm()
	menu = runtime.phone_menu_layout_snapshot()
	check(str(menu.rows[0].label) == "Quality" and str(menu.rows[1].label) == "Shadows",
		"settings navigation reuses the native graphics page and values")
	check(str(menu.rows[0].value) == "Normal", "graphics preset value comes from native settings")
	runtime.menu_adjust(1)
	menu = runtime.phone_menu_layout_snapshot()
	check(str(menu.rows[0].value) == "Pretty",
		"Godot adjustment command reuses native graphics-preset semantics")
	runtime.menu_move_selection(1)
	runtime.menu_adjust(1)
	menu = runtime.phone_menu_layout_snapshot()
	check(str(menu.rows[1].value) == "Off",
		"Godot toggle command updates the parent-owned shadow setting")
	runtime.menu_back()
	runtime.menu_back()
	runtime.menu_move_selection(-1)
	check(runtime.menu_confirm(), "native Resume action is handled")
	check(not bool(runtime.hud_presentation_snapshot().ui_paused),
		"native Resume action unpauses the original Game simulation")
	world.call("_sync")

	runtime.submit_input(0, 0, 0, 0, false, false, false, false, false, true)
	runtime.tick()
	world.call("_sync")
	snapshot = runtime.phone_presentation_snapshot()
	check(bool(snapshot.first_person), "native camera toggle changes phone snapshot mode")
	var camera: Camera3D = world.get("camera")
	var expected_view_position: Vector3 = camera.global_position + camera.global_basis * Vector3(0.0, -0.04, -0.70)
	check(phone_root.global_position.distance_to(expected_view_position) <= 0.00001,
		"first-person view model is positioned relative to the presentation camera")
	check(phone_root.scale.is_equal_approx(Vector3.ONE * 3.4),
		"first-person phone legibility scale is render-only and camera-relative")
	check(body.visible and screen.visible,
		"phone body and screen remain visible in first-person presentation")
	check(phone_root.get_meta("native_screen_mode") == int(snapshot.screen_mode),
		"adapter retains native screen mode without owning it")

	world.free()
	if not failures.is_empty():
		print("RESULT: FAIL count=%d" % failures.size())
		quit(2)
		return
	print("RESULT: PASS native phone/menu layouts, settings actions, and first-person view model")
	quit(0)
