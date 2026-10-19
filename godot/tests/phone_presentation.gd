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
	var screen_text: Label3D = phone_root.get_node("ScreenText")
	var screen_light: OmniLight3D = phone_root.get_node("NativePhoneScreenLight")
	var screen_material: StandardMaterial3D = screen.material_override
	var body_mesh: BoxMesh = body.mesh
	var screen_mesh: BoxMesh = screen.mesh
	var expected_body_size: Vector3 = snapshot.body_size * snapshot.body_scale
	check(not snapshot.first_person, "default fixture starts in third-person mode")
	check(phone_root.global_position.distance_to(snapshot.position) <= 0.00001,
		"third-person adapter follows native phone position")
	check(phone_root.quaternion.is_equal_approx(snapshot.orientation),
		"third-person adapter follows native phone orientation")
	check(body_mesh.size.distance_to(expected_body_size) <= 0.00001,
		"phone body dimensions come from native snapshot")
	check(screen_mesh.size.distance_to(snapshot.screen_size) <= 0.00001,
		"screen dimensions come from native snapshot")
	check(screen.position.distance_to(snapshot.screen_center_offset) <= 0.00001,
		"screen position comes from native screen transform")
	check(screen_text.text.contains("BAT %03d" % roundi(snapshot.battery)),
		"screen indicator reflects native battery state")
	check(screen_text.text.contains("SOUL %02d" % int(snapshot.souls)),
		"screen indicator reflects native soul inventory")
	check(screen_light.light_color.is_equal_approx(snapshot.screen_light_color) and
		absf(screen_light.light_energy - float(snapshot.screen_light_intensity) *
			(1.0 + clampf(float(snapshot.screen_light_pulse), 0.0, 1.0) * 0.25)) <= 0.00001 and
		absf(screen_light.omni_range - float(snapshot.screen_light_radius)) <= 0.00001,
		"phone screen point light color, intensity, and radius come from native display-light state")
	check(screen_light.position.distance_to(snapshot.screen_center_offset + Vector3(0.0, 0.0,
		float(snapshot.screen_light_forward_offset))) <= 0.00001,
		"phone screen light is placed at the native display-light forward offset")
	check(absf(screen_material.emission_energy_multiplier - float(snapshot.emission_strength)) <= 0.00001,
		"screen material emission strength is copied directly from native display state")
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
	check(int(phone_root.get_meta("native_menu_row_count", 0)) == 3 and
		phone_root.get_node("MenuRow00").text == "Resume",
		"Godot screen adapter renders rows from the native layout snapshot")
	var native_row: Dictionary = menu_rows[0]
	var rendered_row: Label3D = phone_root.get_node("MenuRow00")
	var expected_row_position: Vector3 = snapshot.screen_center_offset + Vector3(
		(float(native_row.label_x) / float(menu.logical_width) - 0.5) * snapshot.screen_size.x,
		(0.5 - float(native_row.baseline_y) / float(menu.logical_height)) * snapshot.screen_size.y,
		snapshot.screen_size.z * 0.5 + 0.0003)
	check(rendered_row.position.distance_to(expected_row_position) <= 0.00001,
		"Godot menu text uses the native PhoneDisplayLayout coordinates")
	runtime.menu_move_selection(1)
	world.call("_sync")
	menu = runtime.phone_menu_layout_snapshot()
	check(int(menu.selection) == 1 and bool(menu.rows[1].selected),
		"semantic menu navigation updates native selection")
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
	var expected_view_position: Vector3 = camera.global_position + camera.global_basis * Vector3(0.22, -0.17, -0.72)
	check(phone_root.global_position.distance_to(expected_view_position) <= 0.00001,
		"first-person view model is positioned relative to the presentation camera")
	check(phone_root.scale.is_equal_approx(Vector3.ONE * 1.5),
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
