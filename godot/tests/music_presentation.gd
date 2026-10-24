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
	var runtime: Node = world.get("runtime")
	var music: Node = world.get_node("NativeMusicPresentation")
	var snapshot: Dictionary = runtime.music_presentation_snapshot()
	check(bool(snapshot.playing) and str(snapshot.key) == "stage-original",
		"native music director selects the original stage theme for Field")
	check(music.current_key == "stage-original" and music.player.stream != null and
		(music.player.playing or not music.audio_output_enabled),
		"Godot prepares the native-selected Field music asset and plays it when audio output is available")
	check(music.track_path("stage-original") == "res://audio/game_music.mp3",
		"original stage theme maps to the mature desktop asset")
	runtime.start_city_room_fixture()
	world.call("_sync")
	snapshot = runtime.music_presentation_snapshot()
	check(str(snapshot.key) == "stage-wacky" and music.current_key == "stage-wacky",
		"room 12 keeps the native director's Wacky milestone priority over City")
	music.player.stop()
	music.player.stream = null
	world.free()
	await process_frame
	await process_frame
	if not failures.is_empty():
		print("RESULT: FAIL count=%d" % failures.size())
		quit(2)
		return
	print("RESULT: PASS native music selection and Godot playback")
	quit(0)
