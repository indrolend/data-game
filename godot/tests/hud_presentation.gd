extends SceneTree

var failures: Array[String] = []

func _initialize() -> void:
	call_deferred("_run")

func check(condition: bool, message: String) -> void:
	if not condition:
		failures.append(message)
		push_error("FAIL: " + message)

func _run() -> void:
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	await process_frame
	var runtime = scene.runtime
	if runtime == null:
		check(false, "default scene instantiates the parent runtime")
	else:
		var hud: Dictionary = runtime.hud_presentation_snapshot()
		var required := [
			"battery_fill", "stored_souls", "low_battery", "crosshair_rotation_degrees",
			"crosshair_spread_pixels", "crosshair_opacity", "shoot_join_timer",
			"has_target", "energy_ticker", "energy_ticker_until", "energy_ticker_type",
			"headshot_pulse", "perfect_pulse", "headshot_kill_charge", "critical_hit_pulse",
			"crit_marker_opacity", "room_index", "room_clear", "deposited_souls",
			"required_souls", "started", "dead", "ui_paused"]
		for key in required:
			check(hud.has(key), "native HUD snapshot contains %s" % key)
		var expected_fill := clampf(float(runtime.battery()) / 100.0, 0.0, 1.0)
		check(absf(float(hud.get("battery_fill", -1.0)) - expected_fill) < 0.00001,
			"native HUD battery fill matches authoritative battery")
		check(int(hud.get("stored_souls", -1)) == int(runtime.souls()),
			"native HUD soul inventory matches authoritative inventory")
		check(absf(float(scene.camera.fov) - float(runtime.camera_fov_degrees())) < 0.00001,
			"Godot camera projection follows native vertical field of view")
		var hud_layer: CanvasLayer = scene.hud_adapter.get_parent() as CanvasLayer
		check(hud_layer != null and scene.hud_adapter.name == "NativeHudPresentation",
			"default scene uses the native-state HUD adapter")
		check(hud_layer.get_child_count() == 2,
			"default HUD does not layer diagnostic labels or a static reticle over the native HUD")
		runtime.tick()
		scene._sync()
		check(int(scene.hud_adapter.snapshot.get("stored_souls", -1)) == int(runtime.souls()),
			"native snapshot reaches the HUD adapter after a simulation tick")

	scene.free()
	if not failures.is_empty():
		print("RESULT: FAIL count=", failures.size())
		quit(2)
		return
	print("RESULT: PASS native HUD data, exact reticle inputs, camera projection, and single-HUD integration")
	quit(0)
