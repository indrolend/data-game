extends SceneTree

const OUTPUT := "res://../evidence/generated/fundamental-loop-visual"
var failures: Array[String] = []

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
        check(image.save_png(ProjectSettings.globalize_path("%s/%s.png" % [OUTPUT, label])) == OK,
            "save %s framebuffer" % label)

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(OUTPUT))
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
    var capture_seen := false
    for step in range(1100):
        if step == 960:
            runtime.refill_target_interaction_battery()
        var melee := step > 300 and step < 1000 and step % 50 == 0
        runtime.submit_input(0, 0, 120.0 if step == 960 else 0.0, 0,
            step > 960, false, false, melee, false, false)
        runtime.tick()
        if not crack_seen:
            for target in range(runtime.target_count()):
                if float(runtime.target_presentation_snapshot(target).get("shell_crack_amount", 0.0)) > 0.0:
                    crack_seen = true
                    world._sync()
                    await capture_frame("02-shell-crack")
                    break
        if float(runtime.phone_presentation_snapshot().capture_pulse) > 0.8:
            capture_seen = true
            world._sync()
            await capture_frame("03-capture-phone-light")
            break
    check(crack_seen, "native melee reaches a visible shell-crack checkpoint")
    check(capture_seen and runtime.souls() > 0,
        "native ingestion reaches capture phone-light and stored-soul checkpoints")

    runtime.start_target_interaction_fixture()
    runtime.set_paused(true)
    for _frame in range(45):
        runtime.tick()
    world._sync()
    await capture_frame("04-phone-menu")
    check(bool(runtime.phone_menu_layout_snapshot().active),
        "native pause displays the interactive phone menu")
    world.free()
    if not failures.is_empty():
        print("RESULT: FAIL count=", failures.size())
        quit(2)
        return
    print("RESULT: PASS graphical enemy-hit, shell-crack, capture-light, and phone-menu checkpoints")
    quit(0)
