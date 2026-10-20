extends SceneTree

const FULL_MOON_TICKS := 425232 # 14.765 native 480-second day cycles at 60 Hz.
const OUTPUT_DIR := "res://../evidence/native-celestial-closeups-v7"
const SUN_FILE := OUTPUT_DIR + "/native-sun-disk.png"
const GAMEPLAY_NIGHT_FILE := OUTPUT_DIR + "/gameplay-night-view.png"
const MOON_GAMEPLAY_FOV_FILE := OUTPUT_DIR + "/moon-at-gameplay-fov.png"
const OUTPUT_FILE := OUTPUT_DIR + "/full-moon.png"

var failures: Array[String] = []

func _initialize() -> void:
	call_deferred("_run")

func check(condition: bool, message: String) -> void:
	if not condition:
		failures.append(message)
		push_error("FAIL: " + message)

func _run() -> void:
	var scene = load("res://scenes/main.tscn").instantiate()
	scene.agent_playtest = true
	root.add_child(scene)
	await process_frame
	scene.runtime.start_target_interaction_fixture()
	scene._sync()
	var day_snapshot: Dictionary = scene.runtime.lighting_presentation_snapshot()
	check(float(day_snapshot.daylight) > 0.8 and bool(day_snapshot.open_sky),
		"sun close-up begins from the native open-sky daytime fixture")
	scene.camera.fov = 5.0
	scene.camera.look_at(scene.camera.position + day_snapshot.sun.direction.normalized() * 360.0,
		Vector3.UP)
	scene.hud_adapter.visible = false
	scene.developer_console.visible = false
	await process_frame
	await process_frame
	var sun_image := root.get_texture().get_image()
	check(sun_image != null and not sun_image.is_empty(), "capture sun disk framebuffer")
	if sun_image != null and not sun_image.is_empty():
		check(sun_image.save_png(ProjectSettings.globalize_path(SUN_FILE)) == OK,
			"save native-driven sun disk capture")
	for _tick in range(FULL_MOON_TICKS):
		scene.runtime.tick()
	var snapshot: Dictionary = scene.runtime.lighting_presentation_snapshot()
	scene._sync()
	var view_to_camera: Vector3 = (scene.camera.position - scene.moon.position).normalized()
	var moon_lighting: float = view_to_camera.dot(snapshot.sun.direction.normalized())
	print("LUNAR_STATE frame=%d time=%.6f daylight=%.6f moon_visible=%s phase_view_lighting=%.6f" % [
		scene.runtime.frame(), snapshot.time, snapshot.daylight, str(scene.moon.visible), moon_lighting])
	check(scene.runtime.frame() == FULL_MOON_TICKS and float(snapshot.time) > 7000.0,
		"capture advances the original native simulation by the requested fixed ticks")
	check(float(snapshot.daylight) < 0.02 and scene.moon.visible and moon_lighting > 0.90,
		"captured native state is a visible, sunlit near-full Moon during night")
	# Capture the real gameplay camera before a diagnostic zoom, so this also tests
	# whether the Moon is discoverable at the native camera FOV and orientation.
	scene.hud_adapter.visible = false
	scene.developer_console.visible = false
	await process_frame
	await process_frame
	var gameplay_image := root.get_texture().get_image()
	check(gameplay_image != null and not gameplay_image.is_empty(),
		"capture gameplay-framing night sky framebuffer")
	if gameplay_image != null and not gameplay_image.is_empty():
		check(gameplay_image.save_png(ProjectSettings.globalize_path(GAMEPLAY_NIGHT_FILE)) == OK,
			"save gameplay-framing night sky capture")
	# Looking upward is an ordinary player camera action; retain the native FOV
	# here to show the Moon at its actual apparent size, not as an enlarged asset.
	var gameplay_fov: float = scene.camera.fov
	scene.camera.look_at(scene.moon.position, Vector3.UP)
	await process_frame
	await process_frame
	var moon_at_gameplay_fov := root.get_texture().get_image()
	check(moon_at_gameplay_fov != null and not moon_at_gameplay_fov.is_empty(),
		"capture Moon while looking up at the gameplay camera FOV")
	if moon_at_gameplay_fov != null and not moon_at_gameplay_fov.is_empty():
		check(moon_at_gameplay_fov.save_png(ProjectSettings.globalize_path(MOON_GAMEPLAY_FOV_FILE)) == OK,
			"save actual-apparent-size lunar capture")
	scene.camera.look_at(scene.moon.position, Vector3.UP)
	# Zoom only for surface inspection; the gameplay camera retains its native FOV.
	scene.camera.fov = 1.5
	await process_frame
	await process_frame
	var make_dir_error := DirAccess.make_dir_recursive_absolute(
		ProjectSettings.globalize_path(OUTPUT_DIR))
	check(make_dir_error == OK or make_dir_error == ERR_ALREADY_EXISTS,
		"create lunar capture evidence directory")
	var image := root.get_texture().get_image()
	check(image != null and not image.is_empty(), "capture rendered lunar framebuffer")
	if failures.is_empty():
		var capture_error := image.save_png(ProjectSettings.globalize_path(OUTPUT_FILE))
		check(capture_error == OK, "save full-Moon GPU capture")
	if failures.is_empty():
		print("LUNAR_CAPTURE frame=%d time=%.3f daylight=%.6f phase_view_lighting=%.6f gameplay_fov=%.3f sun_image=\"%s\" gameplay_night_image=\"%s\" moon_gameplay_fov_image=\"%s\" moon_image=\"%s\"" % [
			scene.runtime.frame(), snapshot.time, snapshot.daylight, moon_lighting,
			gameplay_fov, ProjectSettings.globalize_path(SUN_FILE),
			ProjectSettings.globalize_path(GAMEPLAY_NIGHT_FILE),
			ProjectSettings.globalize_path(MOON_GAMEPLAY_FOV_FILE),
			ProjectSettings.globalize_path(OUTPUT_FILE)])
		scene.free()
		quit(0)
		return
	scene.free()
	quit(2)
