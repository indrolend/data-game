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
	scene.free()
	if not failures.is_empty():
		print("RESULT: FAIL count=", failures.size())
		quit(2)
		return
	print("RESULT: PASS native room upgrade economy, grid selection, and run resume presentation")
	quit(0)
