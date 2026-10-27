extends SceneTree

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
	if not condition:
		failures.append(message)
		push_error("FAIL: " + message)

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	var world = load("res://scenes/playable_runtime.tscn").instantiate()
	root.add_child(world)
	await process_frame
	await physics_frame
	await _capture("01-opening")
	check(world.player != null and world.enemy != null, "playable scene spawns one player and one enemy")
	check(world.player.camera.global_position.z > world.player.global_position.z + 1.0,
		"third-person camera stays behind the phone player")
	world.player.toggle_camera()
	await _capture("01-first-person")
	check(world.player.first_person, "camera action switches to the first-person phone view model")
	world.player.toggle_camera()
	check(not world.player.first_person, "camera action returns to third-person")
	var initial_distance: float = world.player.global_position.distance_to(world.enemy.global_position)
	for _frame in range(90):
		await physics_frame
	await _capture("02-enemy-approach")
	check(world.player.global_position.y <= 0.02, "player rests on the collidable Godot arena floor")
	check(world.enemy.global_position.distance_to(world.player.global_position) < initial_distance,
		"enemy approaches the player under Godot movement and collision")

	world.player.reset_player(Vector3(0, 0, 0))
	world.enemy.global_position = Vector3(0, 0, -1.2)
	world.enemy.velocity = Vector3.ZERO
	world.player.rotation.y = 0.0
	for _frame in range(2):
		await physics_frame
	var enemy_before_hit: Vector3 = world.enemy.global_position
	Input.action_press("attack")
	await physics_frame
	Input.action_release("attack")
	for _frame in range(12):
		await physics_frame
	check(world.enemy.health == world.enemy.MAX_HEALTH - 1,
		"player strike hits the enemy through the Godot melee Area3D")
	check(world.enemy.state_name() == "STAGGERED",
		"valid hit causes an enemy stagger reaction")
	check(world.enemy.global_position.distance_to(enemy_before_hit) > 0.1,
		"valid hit displaces the enemy through its physical reaction")

	for _hit_index in range(2):
		for _frame in range(46):
			await physics_frame
		world.enemy.global_position = Vector3(0, 0, -1.2)
		world.enemy.velocity = Vector3.ZERO
		Input.action_press("attack")
		await physics_frame
		Input.action_release("attack")
		for _frame in range(12):
			await physics_frame
	check(world.enemy.is_defeated(), "three valid strikes defeat the enemy")
	check(absf(world.enemy.visuals.rotation.z) > 0.3,
		"defeated enemy visibly collapses")
	check(world._ending.contains("ENCOUNTER CLEAR"), "defeat produces a readable encounter-clear state")
	await _capture("03-encounter-clear")

	world.restart_encounter()
	world.player.global_position = Vector3(0, 0, 0)
	world.enemy.global_position = Vector3(0, 0, -1.0)
	world.enemy.velocity = Vector3.ZERO
	for _frame in range(125):
		await physics_frame
	check(world.player.health < world.player.MAX_HEALTH,
		"enemy attack contact window damages the player")
	check(world.player.global_position.distance_to(Vector3(0, 0, 0)) > 0.05,
		"enemy impact produces physical player displacement")
	await _capture("04-player-hit")

	world.player.global_position = Vector3(10.8, 0, 4)
	world.player.velocity = Vector3.ZERO
	Input.action_press("move_right")
	for _frame in range(90):
		await physics_frame
	Input.action_release("move_right")
	check(world.player.global_position.x < 11.7,
		"player movement collides with and is bounded by the Godot arena wall")

	world.free()
	for action in ["move_left", "move_right", "move_forward", "move_back", "jump", "sprint", "attack", "camera_toggle"]:
		InputMap.erase_action(action)
	if not failures.is_empty():
		print("RESULT: FAIL count=", failures.size())
		quit(2)
		return
	print("RESULT: PASS Godot movement, arena collision, enemy pursuit, attack, damage, stagger, defeat, and recovery loop")
	quit(0)

func _capture(label: String) -> void:
	if not OS.get_cmdline_user_args().has("--capture"):
		return
	await RenderingServer.frame_post_draw
	var directory := ProjectSettings.globalize_path("res://../evidence/godot-playable-encounter-final-checked")
	var directory_error := DirAccess.make_dir_recursive_absolute(directory)
	if directory_error != OK and directory_error != ERR_ALREADY_EXISTS:
		failures.append("screenshot directory creation failed: %d" % directory_error)
		return
	var image := root.get_texture().get_image()
	if image == null or image.is_empty():
		failures.append("framebuffer image was unavailable for " + label)
		return
	var image_path := directory.path_join(label + ".png")
	var save_error := image.save_png(image_path)
	if save_error != OK:
		failures.append("screenshot save failed for %s: %d" % [label, save_error])
	else:
		print("CAPTURE file=\"%s\" size=%dx%d player=%s camera=%s phone=%s phone_visible=%s" % [
			image_path, image.get_width(), image.get_height(), world_player_position(),
			world_camera_position(), world_phone_position(), str(world_phone_visible())])

func world_player_position() -> Vector3:
	var world := root.get_child(0)
	return world.player.global_position

func world_camera_position() -> Vector3:
	var world := root.get_child(0)
	return world.player.camera.global_position

func world_phone_position() -> Vector3:
	var world := root.get_child(0)
	return world.player._world_phone.global_position

func world_phone_visible() -> bool:
	var world := root.get_child(0)
	return world.player._world_phone.visible
