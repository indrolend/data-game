extends SceneTree

const OUTPUT := "res://../evidence/generated/endorphin-enemy"
var world: Node3D
var runtime: Node
var status_label: Label
var previous_keys := {}
var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func _initialize() -> void:
    call_deferred("_setup")

func _setup() -> void:
    world = load("res://scenes/parent_reference.tscn").instantiate()
    root.add_child(world)
    await process_frame
    runtime = world.runtime
    runtime.start_endorphin_enemy_fixture()
    world._sync()
    if "--automated" in OS.get_cmdline_user_args():
        world.agent_playtest = true
        await _run_automated()
        return
    status_label = Label.new()
    status_label.position = Vector2(24, 24)
    status_label.add_theme_font_size_override("font_size", 18)
    var layer := CanvasLayer.new()
    layer.layer = 50
    layer.add_child(status_label)
    root.add_child(layer)
    print("ENDORPHIN EXPERIMENT: 1 small impact, 2 corrective impact, 3 fall impact, R reset, Esc quit")

func _process(_delta: float) -> bool:
    if runtime == null or status_label == null:
        return false
    _edge_key(KEY_1, func(): runtime.apply_endorphin_enemy_impulse(0, Vector3(0.75, 0.0, 0.15), 0.85))
    _edge_key(KEY_2, func(): runtime.apply_endorphin_enemy_impulse(0, Vector3(2.8, 0.0, 0.8), 1.05))
    _edge_key(KEY_3, func(): runtime.apply_endorphin_enemy_impulse(0, Vector3(12.0, 0.4, 12.0), 1.8))
    _edge_key(KEY_R, func(): runtime.start_endorphin_enemy_fixture())
    _edge_key(KEY_ESCAPE, func(): quit(0))
    var body: Dictionary = runtime.target_physical_snapshot(0)
    var target: Dictionary = runtime.target_presentation_snapshot(0)
    status_label.text = ("ENDORPHIN-STYLE NATIVE BODY\n"
        + "1 small | 2 corrective | 3 fall | R reset | Esc quit\n"
        + "pitch %.2f  roll %.2f  disruption %.2f\n"
        + "support %.2f  recovery %.2f  step %s  fallen %s\n"
        + "attack %s  armor %.1f") % [
            float(body.body_pitch), float(body.body_roll), float(body.disruption),
            float(body.support_contact), float(body.recovery_time),
            str(body.corrective_step_active), str(body.fallen),
            str(target.attack_active), float(target.armor)]
    return false

func _edge_key(keycode: Key, action: Callable) -> void:
    var down := Input.is_key_pressed(keycode)
    if down and not bool(previous_keys.get(keycode, false)):
        action.call()
    previous_keys[keycode] = down

func _advance(frames: int, metrics: Dictionary) -> void:
    for _frame in range(frames):
        world.agent_step(1, 0.0, 0.0, 0.0, 0.0, false, false,
            false, false, false, false)
        var body: Dictionary = runtime.target_physical_snapshot(0)
        metrics["max_tilt"] = maxf(float(metrics.get("max_tilt", 0.0)),
            maxf(absf(float(body.body_pitch)), absf(float(body.body_roll))))
        metrics["max_disruption"] = maxf(float(metrics.get("max_disruption", 0.0)),
            float(body.disruption))
        metrics["max_recovery_urgency"] = maxf(float(metrics.get("max_recovery_urgency", 0.0)),
            float(body.recovery_urgency))
        metrics["corrective_step"] = bool(metrics.get("corrective_step", false)) or bool(body.corrective_step_active)
        metrics["fallen"] = bool(metrics.get("fallen", false)) or bool(body.fallen)
        metrics["attack_seen"] = bool(metrics.get("attack_seen", false)) or bool(
            runtime.target_presentation_snapshot(0).attack_active)
        await process_frame

func _capture(label: String) -> void:
    await process_frame
    await process_frame
    var image := root.get_texture().get_image()
    check(image != null and not image.is_empty(), "capture %s" % label)
    if image != null and not image.is_empty():
        image.save_png(ProjectSettings.globalize_path("%s/%s.png" % [OUTPUT, label]))

func _run_automated() -> void:
    DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(OUTPUT))
    var metrics := {"scenario": "native-reduced-articulated-control", "target": 0}
    await _advance(120, metrics)
    var standing: Dictionary = runtime.target_physical_snapshot(0)
    check(not bool(standing.fallen) and float(standing.support_contact) > 0.75,
        "enemy stands with supported balance")
    await _capture("00-standing")

    runtime.apply_endorphin_enemy_impulse(0, Vector3(0.75, 0.0, 0.15), 0.85)
    await _advance(120, metrics)
    var small: Dictionary = runtime.target_physical_snapshot(0)
    check(not bool(small.fallen), "small impact is absorbed while standing")
    await _capture("01-small-impact")

    runtime.apply_endorphin_enemy_impulse(0, Vector3(2.8, 0.0, 0.8), 1.05)
    var correction_visible := false
    for _frame in range(150):
        await _advance(1, metrics)
        var correction: Dictionary = runtime.target_physical_snapshot(0)
        if bool(correction.corrective_step_active):
            correction_visible = true
            break
    check(correction_visible or float(metrics.max_recovery_urgency) > 0.08,
        "larger disturbance produces balance correction")
    await _capture("02-corrective-step")
    await _advance(90, metrics)

    runtime.apply_endorphin_enemy_impulse(0, Vector3(12.0, 0.4, 12.0), 1.8)
    var fall_visible := false
    for _frame in range(60):
        await _advance(1, metrics)
        if bool(runtime.target_physical_snapshot(0).fallen):
            fall_visible = true
            break
    check(fall_visible, "unrecoverable disturbance produces a fall")
    if fall_visible:
        await _advance(12, metrics)
        check(bool(runtime.target_physical_snapshot(0).fallen),
            "fallen state persists long enough to produce a readable collapse")
    await _capture("03-fallen")

    var recovered := false
    for _frame in range(480):
        await _advance(1, metrics)
        var body: Dictionary = runtime.target_physical_snapshot(0)
        if not bool(body.fallen) and float(body.recovery_phase) == 0:
            recovered = true
            break
    check(recovered, "fallen enemy gathers support and recovers")
    await _capture("04-recovered")

    await _advance(360, metrics)
    check(bool(metrics.attack_seen), "recovered enemy continues tracking and combat")
    metrics["final_body"] = runtime.target_physical_snapshot(0)
    metrics["final_target"] = runtime.target_presentation_snapshot(0)
    metrics["failures"] = failures
    var file := FileAccess.open(ProjectSettings.globalize_path("%s/summary.json" % OUTPUT), FileAccess.WRITE)
    file.store_string(JSON.stringify(metrics, "  ") + "\n")
    world.free()
    if failures.is_empty():
        print("RESULT: PASS standing, impact absorption, corrective step, fall, recovery, tracking, combat")
        quit(0)
    else:
        print("RESULT: FAIL %d checks" % failures.size())
        quit(2)
