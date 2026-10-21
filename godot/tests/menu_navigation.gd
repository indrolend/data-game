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

	# Exercise the same key-event handler used by player input, not bridge shortcuts.
	send_key(world, KEY_ESCAPE)
	menu = runtime.phone_menu_layout_snapshot()
	check(bool(menu.active) and bool(runtime.hud_presentation_snapshot().ui_paused),
		"Escape opens the native pause menu during gameplay")
	check(selected_label(menu) == "Resume", "pause menu initially selects Resume")

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
	for _index in range(8):
		send_key(world, KEY_DOWN)
	menu = runtime.phone_menu_layout_snapshot()
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
