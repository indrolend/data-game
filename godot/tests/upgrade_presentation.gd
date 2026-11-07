extends SceneTree

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
	if not condition:
		failures.append(message)
		push_error("FAIL: " + message)

func _initialize() -> void:
	call_deferred("_run")

func send_key(scene: Node, key: Key) -> void:
	var event := InputEventKey.new()
	event.keycode = key
	event.pressed = true
	scene._unhandled_input(event)

func _run() -> void:
	root.size = Vector2i(1280, 720)
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	await process_frame
	var runtime = scene.runtime
	runtime.start_upgrade_fixture()
	scene._sync()
	var snapshot: Dictionary = runtime.upgrade_presentation_snapshot()
	check(bool(snapshot.active) and bool(runtime.paused()),
		"native room recompile opens its paused upgrade choice")
	check(int(snapshot.room_index) == 1 and int(snapshot.tokens) == 2,
		"upgrade presentation reads native room and permanent token state")
	check(snapshot.temporary_levels == [2, 1, 3] and snapshot.permanent_levels == [1, 2, 0],
		"run and permanent upgrade levels come from native progression")
	check(snapshot.temporary_available == [true, true, true] and
		snapshot.permanent_available == [true, true, true],
		"native upgrade availability follows level caps and token economy")
	var hud_snapshot: Dictionary = scene.hud_adapter.snapshot
	var hud_upgrade: Dictionary = hud_snapshot.get("upgrade", {})
	check(bool(hud_upgrade.get("active", false)) and
		int(hud_upgrade.get("selection", -1)) == int(snapshot.selection),
		"single HUD adapter receives native upgrade selection for display")
	send_key(scene, KEY_RIGHT)
	send_key(scene, KEY_DOWN)
	check(int(runtime.upgrade_presentation_snapshot().selection) == 4,
		"Godot arrow input follows the original three-column, two-row upgrade grid")
	var hud: Control = scene.get("hud_adapter")
	var hud_scale := clampf(minf(hud.size.x / 1280.0, hud.size.y / 720.0), 1.0, 2.5)
	var logical_size := hud.size / hud_scale
	var card_center := Vector2(logical_size.x * 0.5,
		logical_size.y * 0.5 - 150.0 + 66.0 + 38.0) * hud_scale
	var hover := InputEventMouseMotion.new()
	hover.position = card_center
	check(int(hud.call("upgrade_item_at", card_center)) == 1,
		"upgrade hit testing maps the rendered top-center card")
	scene.call("_unhandled_input", hover)
	check(int(runtime.upgrade_presentation_snapshot().selection) == 1,
		"mouse hover selects the upgrade card under the rendered HUD position")
	check(int(hud.call("upgrade_item_at", Vector2(8.0, 8.0))) == -1,
		"upgrade hit testing rejects points outside the rendered cards")
	send_key(scene, KEY_DOWN)
	send_key(scene, KEY_ENTER)
	snapshot = runtime.upgrade_presentation_snapshot()
	check(bool(snapshot.active) and int(snapshot.tokens) == 1 and
		int(snapshot.permanent_levels[1]) == 3,
		"Godot confirmation delegates permanent purchase to Game and retains the menu")
	send_key(scene, KEY_UP)
	send_key(scene, KEY_ENTER)
	snapshot = runtime.upgrade_presentation_snapshot()
	check(not bool(snapshot.active) and not bool(runtime.paused()) and
		int(snapshot.temporary_levels[1]) == 2,
		"Godot confirmation delegates temporary run mutation and resumes gameplay")
	runtime.start_upgrade_fixture()
	scene._sync()
	var click := InputEventMouseButton.new()
	click.position = card_center
	click.button_index = MOUSE_BUTTON_LEFT
	click.pressed = true
	scene.call("_unhandled_input", click)
	check(not bool(runtime.upgrade_presentation_snapshot().active) and not bool(runtime.paused()),
		"mouse click activates the rendered temporary upgrade card and resumes gameplay")
	scene.free()
	if not failures.is_empty():
		print("RESULT: FAIL count=", failures.size())
		quit(2)
		return
	print("RESULT: PASS native room upgrade economy, grid selection, and run resume presentation")
	quit(0)
