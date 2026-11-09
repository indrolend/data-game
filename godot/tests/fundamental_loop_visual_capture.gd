extends SceneTree

const OUTPUT := "res://../evidence/generated/fundamental-loop-visual"
var failures: Array[String] = []
var output_root := OUTPUT

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func capture_frame(label: String) -> void:
    await process_frame
    await process_frame
    var image := root.get_texture().get_image()
    check(image != null and not image.is_empty(), "capture %s framebuffer" % label)
    if image != null and not image.is_empty():
        check(image.save_png(ProjectSettings.globalize_path("%s/%s.png" % [output_root, label])) == OK,
            "save %s framebuffer" % label)

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    for argument in OS.get_cmdline_user_args():
        if argument.begins_with("--output-root="):
            output_root = argument.trim_prefix("--output-root=")
    DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(output_root))
    var world: Node3D = load("res://scenes/parent_reference.tscn").instantiate()
    root.add_child(world)
    world.agent_playtest = true
    await process_frame
    var runtime = world.runtime

    runtime.start_target_interaction_fixture()
    runtime.submit_input(0, 0, 0, 0, false, false, false, false, false, true)
    runtime.tick()
    runtime.submit_input(0, 0, 0, 0, false, false, false, false, false, false)
    for _frame in range(12):
        runtime.tick()
    world._sync()
    await capture_frame("00-gameplay-phone-screen")
    var gameplay_phone: Dictionary = runtime.phone_presentation_snapshot()
    check(bool(gameplay_phone.first_person) and int(gameplay_phone.required_goals) > 0,
        "first-person gameplay phone exposes native objective instrumentation")

    runtime.start_damage_fixture()
    var damage_seen := false
    for _frame in range(240):
        runtime.submit_input(0, 0, 0, 0, false, false, false, false, false, false)
        runtime.tick()
        if float(runtime.phone_presentation_snapshot().damage_pulse) > 0.9:
            damage_seen = true
            world._sync()
            await capture_frame("01-enemy-hit-phone-light")
            break
    check(damage_seen, "native enemy hit reaches the phone damage-light checkpoint")

    runtime.start_target_interaction_fixture()
    var crack_seen := false
    var attraction_seen := false
    var capture_seen := false
    var residue_seen := false
    var vacuum_start_player := Vector3.ZERO
    var vacuum_start_camera_forward := Vector3.ZERO
    var vacuum_motion_recorded := false
    for step in range(1300):
        if step == 960:
            runtime.refill_target_interaction_battery()
            vacuum_start_player = runtime.player_position()
            vacuum_start_camera_forward = (runtime.camera_target() - runtime.camera_position()).normalized()
        var melee := step > 300 and step < 1000 and step % 50 == 0
        var vacuum_motion := step > 970 and step < 1048
        runtime.submit_input(0.08 if vacuum_motion else 0.0, 0.06 if vacuum_motion else 0.0,
            120.0 if step == 960 else (0.18 if vacuum_motion else 0.0), 0,
            step > 960 and step < 1048, false, false, melee, false, false)
        runtime.tick()
        var camera_forward: Vector3 = (runtime.camera_target() - runtime.camera_position()).normalized()
        if vacuum_motion and runtime.player_position().distance_to(vacuum_start_player) > 0.05 and \
            camera_forward.dot(vacuum_start_camera_forward) < 0.9999:
            vacuum_motion_recorded = true
        if not attraction_seen and step > 960:
            for target in range(runtime.target_count()):
                var soul: Dictionary = runtime.target_soul_presentation_snapshot(target)
                if float(soul.get("pull_amount", 0.0)) > 0.1 and float(soul.get("ingest_amount", 0.0)) < 0.1:
                    attraction_seen = true
                    world._sync()
                    await capture_frame("03-soul-attraction-lit")
                    break
        if not crack_seen:
            for target in range(runtime.target_count()):
                if float(runtime.target_presentation_snapshot(target).get("shell_crack_amount", 0.0)) > 0.0:
                    crack_seen = true
                    world._sync()
                    await capture_frame("02-shell-crack")
                    break
        if not capture_seen and float(runtime.phone_presentation_snapshot().capture_pulse) > 0.8:
            capture_seen = true
            world._sync()
            await capture_frame("03-capture-phone-light")
        if capture_seen and not residue_seen:
            for particle_value in runtime.particle_presentation_snapshot():
                var particle: Dictionary = particle_value
                if int(particle.material) == 5 and bool(particle.settled):
                    residue_seen = true
                    world._sync()
                    await capture_frame("04-settled-signal-residue")
                    break
        if residue_seen:
            break
    check(crack_seen, "native melee reaches a visible shell-crack checkpoint")
    check(attraction_seen, "native vacuum reaches a visibly lit soul-attraction checkpoint")
    check(vacuum_motion_recorded,
        "soul capture remains observable while the player moves and rotates under native authority")
    check(capture_seen and runtime.souls() > 0,
        "native ingestion reaches capture phone-light and stored-soul checkpoints")
    check(residue_seen, "native capture produces a settled signal-residue visual checkpoint")

    runtime.start_target_interaction_fixture()
    runtime.set_paused(true)
    for _frame in range(45):
        runtime.tick()
    runtime.set_menu_selection(0)
    world._sync()
    await capture_frame("05-phone-menu")
    var menu: Dictionary = runtime.phone_menu_layout_snapshot()
    check(bool(menu.active) and int(menu.selection) == 0 and
        str(menu.rows[0].label) == "Resume" and bool(menu.rows[0].selected),
        "native pause menu renders the selected Resume item")
    world.free()
    if not failures.is_empty():
        print("RESULT: FAIL count=", failures.size())
        quit(2)
        return
    print("RESULT: PASS graphical enemy-hit, shell-crack, capture-light, residue, and phone-menu checkpoints")
    quit(0)
