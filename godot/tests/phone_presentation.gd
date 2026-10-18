extends SceneTree

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
	if not condition:
		failures.append(message)
		push_error("FAIL: " + message)

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	var world: Node3D = load("res://scenes/main.tscn").instantiate()
	root.add_child(world)
	await process_frame
	await process_frame

	var runtime: Node = world.get("runtime")
	var adapter: Node = world.get_node("PhonePresentationAdapter")
	var snapshot: Dictionary = runtime.phone_presentation_snapshot()
	var phone_root: Node3D = adapter.get_node("PhoneRoot")
	var body: MeshInstance3D = phone_root.get_node("Body")
	var screen: MeshInstance3D = phone_root.get_node("Screen")
	var screen_text: Label3D = phone_root.get_node("ScreenText")
	var body_mesh: BoxMesh = body.mesh
	var screen_mesh: BoxMesh = screen.mesh
	var expected_body_size: Vector3 = snapshot.body_size * snapshot.body_scale
	check(not snapshot.first_person, "default fixture starts in third-person mode")
	check(phone_root.global_position.distance_to(snapshot.position) <= 0.00001,
		"third-person adapter follows native phone position")
	check(phone_root.quaternion.is_equal_approx(snapshot.orientation),
		"third-person adapter follows native phone orientation")
	check(body_mesh.size.distance_to(expected_body_size) <= 0.00001,
		"phone body dimensions come from native snapshot")
	check(screen_mesh.size.distance_to(snapshot.screen_size) <= 0.00001,
		"screen dimensions come from native snapshot")
	check(screen.position.distance_to(snapshot.screen_center_offset) <= 0.00001,
		"screen position comes from native screen transform")
	check(screen_text.text.contains("BAT %03d" % roundi(snapshot.battery)),
		"screen indicator reflects native battery state")
	check(screen_text.text.contains("SOUL %02d" % int(snapshot.souls)),
		"screen indicator reflects native soul inventory")

	runtime.submit_input(0, 0, 0, 0, false, false, false, false, false, true)
	runtime.tick()
	world.call("_sync")
	snapshot = runtime.phone_presentation_snapshot()
	check(bool(snapshot.first_person), "native camera toggle changes phone snapshot mode")
	var camera: Camera3D = world.get("camera")
	var expected_view_position: Vector3 = camera.global_position + camera.global_basis * Vector3(0.29, -0.27, -0.72)
	check(phone_root.global_position.distance_to(expected_view_position) <= 0.00001,
		"first-person view model is positioned relative to the presentation camera")
	check(body.visible and screen.visible,
		"phone body and screen remain visible in first-person presentation")
	check(phone_root.get_meta("native_screen_mode") == int(snapshot.screen_mode),
		"adapter retains native screen mode without owning it")

	world.free()
	if not failures.is_empty():
		print("RESULT: FAIL count=%d" % failures.size())
		quit(2)
		return
	print("RESULT: PASS native phone snapshot drives third-person body/screen and first-person view model")
	quit(0)
