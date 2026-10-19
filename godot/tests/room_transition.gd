extends SceneTree

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
	if not condition:
		failures.append(message)
		push_error("FAIL: " + message)

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	await process_frame
	var runtime = scene.runtime
	runtime.start_room_transition_fixture()
	var before: Dictionary = runtime.upgrade_presentation_snapshot()
	check(not bool(before.active) and int(before.room_index) == 1,
		"native rally fixture starts cleared in room one before door crossing")
	var frames_to_transition := -1
	for frame in range(120):
		runtime.submit_input(0.0, 1.0, 0.0, 0.0, false, false, false, false, false, false)
		runtime.tick()
		var snapshot: Dictionary = runtime.upgrade_presentation_snapshot()
		if bool(snapshot.active):
			frames_to_transition = frame + 1
			break
	var after: Dictionary = runtime.upgrade_presentation_snapshot()
	check(frames_to_transition > 0,
		"native movement through the cleared doorway opens the room-upgrade screen")
	check(int(after.room_index) == 2 and bool(runtime.paused()),
		"door crossing advances native room progression and pauses for native upgrade choice")
	check(frames_to_transition <= 120,
		"door transition occurs within bounded native movement input")
	scene.free()
	if not failures.is_empty():
		print("RESULT: FAIL count=", failures.size(), " crossing_frame=", frames_to_transition,
			" room=", after.get("room_index", -1), " player=", runtime.player_position() if is_instance_valid(runtime) else Vector3.ZERO)
		quit(2)
		return
	print("RESULT: PASS native cleared-room doorway progression frames=", frames_to_transition,
		" room=", after.room_index, " upgrade_active=", after.active)
	quit(0)
