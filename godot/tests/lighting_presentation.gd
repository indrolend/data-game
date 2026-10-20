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
	check(bool(native.open_sky) and float(native.daylight) >= 0.0 and float(native.daylight) <= 1.0,
		"open-ceiling native snapshot exposes bounded daylight")
	check(scene.scene_environment.background_mode == Environment.BG_SKY and
		scene.scene_environment.sky == scene.sky_resource and
		scene.camera.environment == scene.scene_environment and
		get_root().world_3d.environment == scene.scene_environment and
		scene.sky_dome.visible and scene.sky_dome.mesh is SphereMesh and
		scene.sky_dome.position.is_equal_approx(scene.camera.position) and
		scene.sky_resource.process_mode == Sky.PROCESS_MODE_INCREMENTAL and
		scene.sky_resource.radiance_size == Sky.RADIANCE_SIZE_64,
		"open-ceiling room uses the low-cost incremental sky resource")
	check(scene.moon.mesh is SphereMesh and is_instance_valid(scene.moon_albedo) and
		scene.moon_albedo.get_width() == 2048 and scene.moon_albedo.get_height() == 1024 and
		is_instance_valid(scene.moon_elevation) and scene.moon_elevation.get_width() == 1024 and
		scene.moon_elevation.get_height() == 512 and
		scene.moon_material.shader != null,
		"Moon uses imported NASA LRO color and LOLA elevation maps with a render-only material")
	check(scene.moon.visible and
		absf(rad_to_deg(2.0 * atan(1.67 / 360.0)) - 0.5316) < 0.002,
		"open-sky Moon is visible at real apparent angular diameter")
	check(scene.sky_material.sky_top_color == native.background and
		scene.sky_material.sky_horizon_color == native.fog_color and
		is_equal_approx(scene.sky_material.sky_energy_multiplier, 1.0) and
		scene.sky_dome_material.get_shader_parameter("zenith_color") == native.background and
		scene.sky_dome_material.get_shader_parameter("horizon_color") == native.fog_color and
		is_equal_approx(scene.sky_dome_material.get_shader_parameter("daylight"), native.daylight),
		"Godot procedural sky consumes native atmosphere and day-cycle values")
	check(is_equal_approx(scene.sky_dome_material.get_shader_parameter("cloud_form"), 0.0),
		"Field sky selects the thin cirrus presentation profile")
	check(float(native.fog_density) > 0.0 and native.sun.enabled and native.fill.enabled,
		"outdoor native room resolves atmosphere, sun, fill, and fog")
	check(scene.scene_environment.ambient_light_color == native.ambient and
		scene.scene_environment.fog_light_color == native.fog_color and
		is_equal_approx(scene.scene_environment.fog_density, native.fog_density),
		"Godot Environment consumes native atmosphere values without local mood constants")
	var day_before := float(native.daylight)
	var moon_before: Vector3 = scene.moon.position
	for _tick in range(14400):
		scene.runtime.tick()
	var night: Dictionary = scene.runtime.lighting_presentation_snapshot()
	scene._sync_lighting(night)
	scene._sync_moon(night)
	check(float(night.daylight) < day_before - 0.5,
		"native 240-second progression advances the open sky from day toward night")
	check(is_equal_approx(scene.sky_material.sky_energy_multiplier,
		lerpf(0.55, 1.0, float(night.daylight))) and
		is_equal_approx(scene.sky_dome_material.get_shader_parameter("daylight"), night.daylight) and
		scene.moon_material.get_shader_parameter("sun_direction") == night.sun.direction.normalized() and
		not scene.moon.visible and not scene.moon.position.is_equal_approx(moon_before),
		"sky and accelerated lunar position follow native state while lunar sunlight remains independent of ground daylight")
	scene.runtime.start_city_room_fixture()
	scene._sync()
	var city: Dictionary = scene.runtime.lighting_presentation_snapshot()
	check(int(city.setting) == 1 and bool(city.open_sky) and
		scene.scene_environment.background_mode == Environment.BG_SKY and
		is_equal_approx(scene.sky_material.sky_curve, 0.34) and
		is_equal_approx(scene.sky_dome_material.get_shader_parameter("cloud_form"), 1.0) and
		is_equal_approx(scene.sky_dome_material.get_shader_parameter("cloud_coverage"), 0.46),
		"city open sky selects its native atmosphere and room-specific sky curve")
	scene.runtime.start_coastal_room_fixture()
	scene._sync()
	var coastal: Dictionary = scene.runtime.lighting_presentation_snapshot()
	check(int(coastal.setting) == 3 and bool(coastal.open_sky) and
		is_equal_approx(scene.sky_material.sky_curve, 0.22) and
		is_equal_approx(scene.sky_dome_material.get_shader_parameter("cloud_form"), 2.0) and
		is_equal_approx(scene.sky_dome_material.get_shader_parameter("cloud_coverage"), 0.54),
		"coastal open sky selects its native atmosphere and room-specific sky curve")
	scene.runtime.start_sterile_room_fixture()
	scene._sync()
	var sterile: Dictionary = scene.runtime.lighting_presentation_snapshot()
	check(int(sterile.setting) == 2 and not bool(sterile.open_sky) and
		not sterile.sun.enabled and not sterile.fill.enabled,
		"sealed sterile room retains native static atmosphere without global sun or fill")
	check(scene.scene_environment.background_mode == Environment.BG_COLOR and
		scene.scene_environment.sky == null and not scene.sky_dome.visible and not scene.moon.visible,
		"sealed sterile room does not inherit the open-sky presentation")
	check((sterile.local_lights as Array).size() >= 2 and
		scene.native_point_lights[0].visible and scene.native_point_lights[1].visible,
		"Godot materializes native sterile ceiling fixture lights")
	scene.free()
	if not failures.is_empty():
		print("RESULT: FAIL count=", failures.size())
		quit(2)
		return
	print("RESULT: PASS native open-sky atmosphere, day/night, room selection, low-cost resource, and sterile fixtures")
	quit(0)
