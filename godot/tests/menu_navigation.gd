extends SceneTree

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
	if not condition:
		failures.append(message)
		push_error("FAIL: " + message)

func send_key(world: Node, key: Key) -> void:
	var event := InputEventKey.new()
	event.keycode = key
	event.pressed = true
	world.call("_unhandled_input", event)
	# SceneTree --script does not schedule the scene's physics callback. Advance
	# exactly one authoritative native tick, as the live scene does at 60 Hz.
	var runtime: Node = world.get("runtime")
	runtime.tick()
	world.call("_sync")

func selected_label(menu: Dictionary) -> String:
	for row in menu.rows:
		if bool(row.selected):
			return str(row.label)
	return ""

func row_center(menu: Dictionary, label: String) -> Vector2:
	for row in menu.rows:
		if str(row.label) == label:
			var visual: Dictionary = row.visual
			return Vector2(float(visual.x) + float(visual.w) * 0.5,
				float(visual.y) + float(visual.h) * 0.5)
	return Vector2(-1.0, -1.0)

func logical_to_viewport(world: Node, logical: Vector2, layout: Dictionary) -> Vector2:
	var adapter: Node3D = world.get("phone_adapter")
	var camera: Camera3D = world.get("camera")
	var phone_root: Node3D = adapter.get("phone_root")
	var screen: MeshInstance3D = adapter.get("glass")
	var screen_size: Vector3 = (screen.mesh as BoxMesh).size
	var logical_width := float(layout.logical_width)
	var logical_height := float(layout.logical_height)
	var local_point := screen.position + Vector3(
		(logical.x / logical_width - 0.5) * screen_size.x,
		(0.5 - logical.y / logical_height) * screen_size.y,
		screen_size.z * 0.5)
	return camera.unproject_position(phone_root.to_global(local_point))

func dispatch_mouse_motion(position: Vector2, relative := Vector2.ZERO) -> void:
	var event := InputEventMouseMotion.new()
	event.position = position
	event.global_position = position
	event.relative = relative
	Input.parse_input_event(event)
	await process_frame

func dispatch_left_click(position: Vector2) -> void:
	for pressed in [true, false]:
		var event := InputEventMouseButton.new()
		event.position = position
		event.global_position = position
		event.button_index = MOUSE_BUTTON_LEFT
		event.pressed = pressed
		Input.parse_input_event(event)
		await process_frame

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	var world: Node = load("res://scenes/main.tscn").instantiate()
	root.add_child(world)
	await process_frame
	await process_frame
	var runtime: Node = world.get("runtime")
	# Start from the original Game's deterministic rally fixture so the test
	# covers gameplay pause/navigation rather than its intro/boot presentation.
	runtime.start_rally_fixture()
	world.call("_sync")
	var menu: Dictionary = runtime.phone_menu_layout_snapshot()
	check(bool(runtime.hud_presentation_snapshot().started),
		"native gameplay fixture is active before menu input")
	check(not bool(runtime.phone_presentation_snapshot().first_person),
		"third-person pause regression begins from the native third-person camera")
	# The original pause phone is a readable camera composition even when gameplay
	# was in third person. Verify that the presentation adapter cannot regress to
	# an unobservably small world-space handset.
	send_key(world, KEY_ESCAPE)
	menu = runtime.phone_menu_layout_snapshot()
	check(bool(menu.active) and bool(runtime.hud_presentation_snapshot().ui_paused),
		"Escape opens the native pause menu from third-person gameplay")
	var third_person_phone_root: Node3D = world.get("phone_adapter").get("phone_root")
	check(third_person_phone_root.is_set_as_top_level(),
		"third-person pause presents the native menu as a camera-anchored handset")
	send_key(world, KEY_ESCAPE)
	check(not bool(runtime.hud_presentation_snapshot().ui_paused),
		"Escape resumes native gameplay after third-person pause presentation")
	# Exercise the actual first-person PhoneRoot transform used for the pause UI.
	runtime.submit_input(0, 0, 0, 0, false, false, false, false, false, true)
	runtime.tick()
	world.call("_sync")
	check(bool(runtime.phone_presentation_snapshot().first_person),
		"pointer test uses the native first-person camera state")

	# Exercise the same key-event handler used by player input, not bridge shortcuts.
	send_key(world, KEY_ESCAPE)
	menu = runtime.phone_menu_layout_snapshot()
	check(bool(menu.active) and bool(runtime.hud_presentation_snapshot().ui_paused),
		"Escape opens the native pause menu during gameplay")
	check(selected_label(menu) == "Resume", "pause menu initially selects Resume")
	var adapter: Node3D = world.get("phone_adapter")
	var camera: Camera3D = world.get("camera")
	var phone_root: Node3D = adapter.get("phone_root")
	check(phone_root.is_set_as_top_level(),
		"pause pointer proof uses the actual camera-anchored PhoneRoot")

	var resume_logical := row_center(menu, "Resume")
	var settings_logical := row_center(menu, "Settings")
	var resume_viewport := logical_to_viewport(world, resume_logical, menu)
	var recovered = adapter.call("viewport_to_phone_logical", resume_viewport, camera, menu)
	check(recovered != null and (recovered as Vector2).distance_to(resume_logical) <= 0.25,
		"transformed PhoneRoot display projection round-trips native logical coordinates")
	var original_viewport_size: Vector2i = root.size
	for test_viewport_size in [Vector2i(640, 720), Vector2i(1280, 720)]:
		root.size = test_viewport_size
		await process_frame
		world.call("_sync")
		var resized_viewport := logical_to_viewport(world, resume_logical, menu)
		var resized_recovered = adapter.call(
			"viewport_to_phone_logical", resized_viewport, camera, menu)
		check(resized_recovered != null and
			(resized_recovered as Vector2).distance_to(resume_logical) <= 0.25,
			"phone projection round-trips at viewport %dx%d" % [
				test_viewport_size.x, test_viewport_size.y])
	root.size = original_viewport_size
	await process_frame
	world.call("_sync")
	resume_viewport = logical_to_viewport(world, resume_logical, menu)
	var outside_logical := Vector2(-20.0, float(menu.logical_height) * 0.5)
	var outside_viewport := logical_to_viewport(world, outside_logical, menu)
	check(adapter.call("viewport_to_phone_logical", outside_viewport, camera, menu) == null,
		"inverse projection rejects points outside the physical phone display")

	# Hover and click through Godot's input dispatcher and the real scene handler;
	# native hit rectangles remain the only selection/activation authority.
	await dispatch_mouse_motion(resume_viewport)
	check(selected_label(runtime.phone_menu_layout_snapshot()) == "Resume",
		"engine-dispatched Resume hover selects native Resume")
	var settings_viewport := logical_to_viewport(world, settings_logical, menu)
	await dispatch_mouse_motion(settings_viewport)
	check(selected_label(runtime.phone_menu_layout_snapshot()) == "Settings",
		"engine-dispatched Settings hover selects native Settings")
	await dispatch_left_click(outside_viewport)
	check(bool(runtime.hud_presentation_snapshot().ui_paused) and
		selected_label(runtime.phone_menu_layout_snapshot()) == "Settings",
		"outside-phone click neither activates nor dismisses the native menu")
	var nonselectable_logical := Vector2(18.0, 18.0)
	check(int(runtime.menu_item_at(nonselectable_logical.x, nonselectable_logical.y)) == -1,
		"inside-display point outside native rows is nonselectable")
	await dispatch_left_click(logical_to_viewport(world, nonselectable_logical, menu))
	check(bool(runtime.hud_presentation_snapshot().ui_paused) and
		selected_label(runtime.phone_menu_layout_snapshot()) == "Settings",
		"nonselectable phone-area click performs no action")
	await dispatch_left_click(settings_viewport)
	menu = runtime.phone_menu_layout_snapshot()
	check(bool(menu.active) and menu.rows.size() == 3 and str(menu.rows[0].label) == "Controls",
		"clicking Settings opens the authoritative native Settings page")
	# Keyboard remains routed to the same native model after pointer activation.
	send_key(world, KEY_ESCAPE)
	check(selected_label(runtime.phone_menu_layout_snapshot()) == "Settings",
		"keyboard Back still returns from pointer-opened Settings")
	menu = runtime.phone_menu_layout_snapshot()
	resume_logical = row_center(menu, "Resume")
	resume_viewport = logical_to_viewport(world, resume_logical, menu)
	await dispatch_mouse_motion(resume_viewport)
	await dispatch_left_click(resume_viewport)
	check(not bool(runtime.hud_presentation_snapshot().ui_paused),
		"clicking Resume immediately clears native pause")
	check(bool(world.call("_gameplay_pointer_active")),
		"Resume click immediately returns pointer routing to gameplay")
	if DisplayServer.get_name() != "headless":
		check(Input.mouse_mode == Input.MOUSE_MODE_CAPTURED,
			"Resume click immediately captures the platform mouse")
	var gameplay_motion := Vector2(7.0, -4.0)
	var camera_target_before: Vector3 = runtime.camera_target()
	await dispatch_mouse_motion(Vector2(320.0, 240.0), gameplay_motion)
	var queued_or_applied: bool = (world.get("pending_look") as Vector2).is_equal_approx(gameplay_motion) or \
		runtime.camera_target().distance_to(camera_target_before) > 0.00001
	check(queued_or_applied,
		"first post-Resume mouse motion reaches gameplay without a second click")
	world.set("pending_look", Vector2.ZERO)

	# Reopen and retain the existing keyboard-navigation regression sequence.
	send_key(world, KEY_ESCAPE)
	check(bool(runtime.hud_presentation_snapshot().ui_paused),
		"keyboard can reopen pause after pointer Resume")

	send_key(world, KEY_DOWN)
	check(selected_label(runtime.phone_menu_layout_snapshot()) == "Settings",
		"Down navigates from Resume to Settings")
	send_key(world, KEY_ENTER)
	menu = runtime.phone_menu_layout_snapshot()
	check(bool(menu.active) and menu.rows.size() == 3 and
		str(menu.rows[0].label) == "Controls" and str(menu.rows[1].label) == "Audio" and
		str(menu.rows[2].label) == "Graphics",
		"Enter opens the native Settings submenu")

	send_key(world, KEY_ENTER)
	menu = runtime.phone_menu_layout_snapshot()
	check(str(menu.rows[0].label) == "Movement" and selected_label(menu) == "Forward",
		"Controls submenu opens with its native movement bindings")
	var initial_controls_revision := int(runtime.phone_display_raster_revision())
	var initial_controls_pixels: PackedByteArray = runtime.phone_display_raster_pixels()
	for _index in range(8):
		send_key(world, KEY_DOWN)
	menu = runtime.phone_menu_layout_snapshot()
	check(int(runtime.phone_display_raster_revision()) != initial_controls_revision and
		runtime.phone_display_raster_pixels() != initial_controls_pixels,
		"scrolling Controls invalidates and changes the native phone raster")
	check(int(phone_root.get_meta("native_raster_revision", -1)) ==
		int(runtime.phone_display_raster_revision()),
		"Godot presents the current scrolled phone raster revision")
	check(selected_label(menu) == "Camera",
		"Controls list can navigate beyond the initial viewport")
	check(float(menu.scroll_offset) > 0.0,
		"long Controls list scrolls to keep the selected binding visible")

	send_key(world, KEY_ESCAPE)
	check(str(runtime.phone_menu_layout_snapshot().rows[0].label) == "Controls",
		"Escape returns from Controls to Settings without unpausing")
	send_key(world, KEY_ESCAPE)
	check(selected_label(runtime.phone_menu_layout_snapshot()) == "Settings",
		"Escape returns from Settings to the paused root menu")
	send_key(world, KEY_UP)
	check(selected_label(runtime.phone_menu_layout_snapshot()) == "Resume",
		"Up navigates from Settings back to Resume")
	send_key(world, KEY_ENTER)
	check(not bool(runtime.hud_presentation_snapshot().ui_paused),
		"Enter activates Resume and returns to gameplay")
	send_key(world, KEY_ESCAPE)
	check(bool(runtime.hud_presentation_snapshot().ui_paused),
		"Escape can reopen the pause menu after resuming")
	send_key(world, KEY_ESCAPE)
	menu = runtime.phone_menu_layout_snapshot()
	check(not bool(menu.active) and not bool(runtime.hud_presentation_snapshot().ui_paused),
		"Escape at the paused root also resumes gameplay")

	world.free()
	if not failures.is_empty():
		print("RESULT: FAIL count=%d" % failures.size())
		quit(2)
		return
	print("RESULT: PASS parent-driven pause, settings, controls scrolling, and resume key routes")
	quit(0)
