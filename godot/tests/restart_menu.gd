extends SceneTree

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
	if not condition:
		failures.append(message)
		push_error("FAIL: " + message)

func dispatch_motion(position: Vector2) -> void:
	var event := InputEventMouseMotion.new()
	event.position = position
	event.global_position = position
	Input.parse_input_event(event)
	await process_frame

func dispatch_click(position: Vector2) -> void:
	for pressed in [true, false]:
		var event := InputEventMouseButton.new()
		event.position = position
		event.global_position = position
		event.button_index = MOUSE_BUTTON_LEFT
		event.pressed = pressed
		Input.parse_input_event(event)
		await process_frame

func dispatch_key(key: Key) -> void:
	for pressed in [true, false]:
		var event := InputEventKey.new()
		event.keycode = key
		event.pressed = pressed
		Input.parse_input_event(event)
		await process_frame

func row_center(menu: Dictionary, selection: int) -> Vector2:
	for row in menu.get("rows", []):
		if int(row.get("selection", -1)) == selection:
			var hit: Rect2 = row.get("hit", Rect2())
			return hit.get_center() * float(menu.get("ui_scale", 1.0))
	return Vector2(-1.0, -1.0)

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	var world: Node = load("res://scenes/main.tscn").instantiate()
	root.add_child(world)
	await process_frame
	await process_frame
	var runtime: Node = world.get("runtime")
	runtime.start_death_fixture()
	for _frame in range(45):
		runtime.tick()
	world.call("_sync_mouse_mode")
	world.call("_sync")
	await process_frame

	var hud: Dictionary = runtime.hud_presentation_snapshot()
	var viewport_size := root.get_visible_rect().size
	var menu: Dictionary = runtime.restart_menu_presentation_snapshot(
		viewport_size.x, viewport_size.y)
	check(bool(hud.dead) and not bool(hud.started),
		"native damage fixture reaches the authoritative death state")
	check(bool(menu.active) and int(menu.selection) == 0,
		"restart menu activates with Again selected")
	check(str(menu.rows[0].label) == "Again?" and str(menu.rows[1].label) == "Quit",
		"restart labels and order match the original desktop renderer")
	var hud_adapter: Control = world.get("hud_adapter")
	check(bool(hud_adapter.get("restart_menu").get("active", false)),
		"HUD adapter receives the native restart presentation snapshot")
	check(hud_adapter.call("_native_bitmap_glyph", "A") == [14, 17, 17, 31, 17, 17, 17] and
		hud_adapter.call("_native_bitmap_glyph", "?") == [0, 0, 0, 0, 0, 0, 0],
		"restart overlay uses the native bitmap glyph contract, including blank punctuation")
	if DisplayServer.get_name() != "headless":
		check(Input.mouse_mode == Input.MOUSE_MODE_VISIBLE,
			"death immediately releases the platform mouse")
		await process_frame
		await process_frame
		var evidence_path := "res://../evidence/restart-menu"
		var evidence_file := "restart-menu.png"
		for argument in OS.get_cmdline_user_args():
			if argument.begins_with("--evidence-dir="):
				evidence_path = argument.trim_prefix("--evidence-dir=")
			elif argument.begins_with("--evidence-file="):
				evidence_file = argument.trim_prefix("--evidence-file=")
		var evidence_directory := ProjectSettings.globalize_path(evidence_path)
		DirAccess.make_dir_recursive_absolute(evidence_directory)
		var image := root.get_texture().get_image()
		check(image != null and not image.is_empty() and
			image.save_png(evidence_directory.path_join(evidence_file)) == OK,
			"capture rendered restart menu evidence")

	var quit_center := row_center(menu, 1)
	await dispatch_motion(quit_center)
	menu = runtime.restart_menu_presentation_snapshot(viewport_size.x, viewport_size.y)
	check(int(menu.selection) == 1,
		"engine-dispatched hover selects native Quit")
	await dispatch_click(Vector2(4.0, 4.0))
	check(bool(runtime.hud_presentation_snapshot().dead),
		"outside restart-menu click performs no action")
	await dispatch_key(KEY_UP)
	menu = runtime.restart_menu_presentation_snapshot(viewport_size.x, viewport_size.y)
	check(int(menu.selection) == 0,
		"keyboard navigation returns to native Again selection")
	await dispatch_click(row_center(menu, 0))
	hud = runtime.hud_presentation_snapshot()
	check(not bool(hud.dead) and bool(hud.started),
		"clicking Again invokes the native restart action")
	check(bool(world.call("_gameplay_pointer_active")),
		"Again immediately returns pointer routing to gameplay")
	if DisplayServer.get_name() != "headless":
		check(Input.mouse_mode == Input.MOUSE_MODE_CAPTURED,
			"Again captures the platform mouse without a second click")

	runtime.start_death_fixture()
	world.call("_sync_mouse_mode")
	world.call("_sync")
	await dispatch_key(KEY_DOWN)
	await dispatch_key(KEY_ENTER)
	hud = runtime.hud_presentation_snapshot()
	check(not bool(hud.dead) and not bool(hud.started),
		"keyboard Quit returns to the native start screen")
	check(bool(runtime.phone_menu_layout_snapshot().active),
		"native start menu becomes active after Quit")

	world.free()
	if not failures.is_empty():
		print("RESULT: FAIL count=%d" % failures.size())
		quit(2)
		return
	print("RESULT: PASS native-authoritative restart menu presentation and input routes")
	quit(0)
