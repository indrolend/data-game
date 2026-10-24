extends SceneTree

var failures: Array[String] = []
const OUTPUT := "res://../evidence/visual-recovery-v4/captures"

func check(condition: bool, message: String) -> void:
	if not condition:
		failures.append(message)
		push_error("FAIL: " + message)

func capture_frame(name: String) -> void:
	await process_frame
	await process_frame
	var image := root.get_viewport().get_texture().get_image()
	var path := ProjectSettings.globalize_path(OUTPUT.path_join(name + ".png"))
	check(image.save_png(path) == OK, "saved " + name)
	print("PHONE_RASTER_CAPTURE name=%s width=%d height=%d path=%s" % [
		name, image.get_width(), image.get_height(), path])

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(OUTPUT))
	var world: Node3D = load("res://scenes/parent_reference.tscn").instantiate()
	root.add_child(world)
	await process_frame
	await process_frame
	var runtime: Node = world.get("runtime")
	runtime.set_paused(true)
	runtime.tick()
	world.call("_sync")
	var raster: MeshInstance3D = world.get_node("PhonePresentationAdapter/PhoneRoot/NativeDisplayRaster")
	check(raster.visible and raster.material_override.albedo_texture != null,
		"pause menu presents the native raster texture")
	await capture_frame("00-pause-resume-selected")
	var first_revision := int(runtime.phone_display_raster_revision())
	runtime.menu_move_selection(1)
	world.call("_sync")
	check(int(runtime.phone_display_raster_revision()) != first_revision,
		"selection updates the native raster revision")
	await capture_frame("01-pause-settings-selected")
	check(runtime.menu_confirm(), "settings action remains native and accepted")
	world.call("_sync")
	await capture_frame("02-settings-page")
	world.free()
	if not failures.is_empty():
		print("RESULT: FAIL count=%d" % failures.size())
		quit(2)
		return
	print("RESULT: PASS native phone raster GPU sequence")
	quit(0)
