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
	var native: Dictionary = scene.runtime.lighting_presentation_snapshot()
	check(int(native.room_index) == 1 and native.background is Color and native.ambient is Color,
		"native lighting snapshot identifies the room and atmosphere colors")
	check(float(native.fog_density) > 0.0 and native.sun.enabled and native.fill.enabled,
		"outdoor native room resolves atmosphere, sun, fill, and fog")
	check(scene.scene_environment.background_color == native.background and
		scene.scene_environment.ambient_light_color == native.ambient and
		scene.scene_environment.fog_light_color == native.fog_color and
		is_equal_approx(scene.scene_environment.fog_density, native.fog_density),
		"Godot Environment consumes native atmosphere values without local mood constants")
	check(scene.sun_light.visible == native.sun.enabled and scene.fill_light.visible == native.fill.enabled,
		"Godot directional lights follow native enablement")
	scene.runtime.start_city_room_fixture()
	scene._sync()
	var city: Dictionary = scene.runtime.lighting_presentation_snapshot()
	check(int(city.setting) == 1 and city.sun.direction != native.sun.direction,
		"city room uses the native urban-sky direction rather than the outdoor default")
	scene.runtime.start_sterile_room_fixture()
	scene._sync()
	var sterile: Dictionary = scene.runtime.lighting_presentation_snapshot()
	check(int(sterile.setting) == 2 and not sterile.sun.enabled and not sterile.fill.enabled,
		"sealed sterile room disables native global sun and fill sources")
	check((sterile.local_lights as Array).size() >= 2 and
		scene.native_point_lights[0].visible and scene.native_point_lights[1].visible,
		"Godot materializes native sterile ceiling fixture lights")
	scene.free()
	if not failures.is_empty():
		print("RESULT: FAIL count=", failures.size())
		quit(2)
		return
	print("RESULT: PASS native atmosphere, room-specific directional lighting, and sterile fixtures")
	quit(0)
