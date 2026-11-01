extends SceneTree

const SfxPresentationAdapter = preload("res://scripts/sfx_presentation_adapter.gd")
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
	var sfx: Node = world.get_node("NativeSfxPresentation")
	var native_snapshot: Dictionary = runtime.audio_presentation_snapshot()
	check(int(native_snapshot.newest_serial) == 1 and native_snapshot.events.size() == 1,
		"ParentGameNode exposes the native serial-numbered audio event stream")
	check(int(native_snapshot.events[0].cue) == 1 and is_equal_approx(float(native_snapshot.events[0].volume), 0.58),
		"native invitation cue and event gain remain authoritative")
	check(sfx.played_serials == [1], "initial native event is consumed exactly once")
	check(sfx.voices[0].stream != null and
		(sfx.voices[0].playing or not sfx.audio_output_enabled),
		"the original invitation sound plays through the graphical Godot audio output")
	world.call("_sync")
	check(sfx.played_serials == [1], "repeated presentation sync deduplicates a native serial")
	check(sfx.cue_path(13) == "res://audio/capture_1.mp3" and
		sfx.cue_path(19) == "res://audio/headshot_critical.mp3",
		"Godot cue mapping matches the original desktop assets")

	var adapter := SfxPresentationAdapter.new()
	root.add_child(adapter)
	await process_frame
	adapter.sync_snapshot({"newest_serial": 2, "volume": 0.5, "events": [
		{"serial": 1, "cue": 11, "volume": 0.4},
		{"serial": 2, "cue": 7, "volume": 0.8},
	]})
	check(adapter.ringtone.stream != null and adapter.voices[0].stream != null,
		"ringtone and overlapping one-shot pool use independent players")
	check((adapter.ringtone.playing and adapter.voices[0].playing) or not adapter.audio_output_enabled,
		"ringtone and gameplay one-shot play concurrently when audio output is available")
	check(is_equal_approx(db_to_linear(adapter.ringtone.volume_db), 0.2) and
		is_equal_approx(db_to_linear(adapter.voices[0].volume_db), 0.4),
		"native event gain is multiplied by the native SFX volume")
	adapter.sync_snapshot({"newest_serial": 3, "volume": 0.5, "events": [
		{"serial": 2, "cue": 7, "volume": 0.8},
		{"serial": 3, "cue": 12, "volume": 0.13},
	]})
	check(adapter.ringtone.stream == null and adapter.played_serials == [1, 2, 3],
		"deduplication skips old events and the dedicated stop event releases the ringtone")
	adapter.free()
	world.free()
	await process_frame
	await process_frame
	if not failures.is_empty():
		print("RESULT: FAIL count=%d" % failures.size())
		quit(2)
		return
	print("RESULT: PASS native SFX event bridge and Godot playback adapter")
	quit(0)
