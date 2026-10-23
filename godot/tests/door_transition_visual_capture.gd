extends SceneTree

const OUTPUT := "res://../evidence/visual-recovery-v3/captures"
var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func capture_frame(name: String) -> void:
    await process_frame
    await process_frame
    var image := root.get_texture().get_image()
    check(image != null and not image.is_empty(), "capture %s framebuffer" % name)
    if image != null and not image.is_empty():
        check(image.save_png(ProjectSettings.globalize_path("%s/%s.png" % [OUTPUT, name])) == OK,
            "save %s framebuffer" % name)

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(OUTPUT))
    var world = load("res://scenes/main.tscn").instantiate()
    root.add_child(world)
    world.agent_playtest = true
    await process_frame
    var runtime = world.runtime
    runtime.start_room_transition_fixture()
    world._sync()
    await capture_frame("00-before-door-transition")
    var crossing_frame := -1
    for frame in range(120):
        runtime.submit_input(0.0, 1.0, 0.0, 0.0, false, false, false, false, false, false)
        runtime.tick()
        if bool(runtime.door_transition_presentation_snapshot().active):
            crossing_frame = frame + 1
            break
    check(crossing_frame > 0, "native fixture crosses the cleared doorway")
    world._sync()
    await capture_frame("01-native-transition-strength-1")
    check(world.door_transition_adapter.overlay.visible,
        "Godot datamosh is visible at the native crossing checkpoint")
    check(runtime.activate_upgrade_selection(), "native upgrade selection resumes gameplay")
    var peel_seen := false
    for frame in range(90):
        runtime.submit_input(0.0, 1.0, 0.0, 0.0, false, false, false, false, false, false)
        runtime.tick()
        var transition: Dictionary = runtime.door_transition_presentation_snapshot()
        if bool(transition.active) and float(transition.progress) < 0.72 and float(transition.progress) > 0.28:
            peel_seen = true
            world._sync()
            await capture_frame("02-native-transition-peel")
            break
    check(peel_seen, "native motion reaches a mid-strength datamosh checkpoint")
    world.queue_free()
    await process_frame
    if not failures.is_empty():
        print("RESULT: FAIL count=", failures.size())
        quit(2)
        return
    print("RESULT: PASS native-driven Godot doorway datamosh captures crossing_frame=", crossing_frame)
    quit(0)
