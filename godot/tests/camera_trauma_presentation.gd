extends SceneTree

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func _initialize() -> void:
    call_deferred("_run")

func _authoritative_snapshot(world: Node3D) -> Dictionary:
    var runtime: Node = world.runtime
    return {
        "player": runtime.player_position(),
        "camera_position": runtime.camera_position(),
        "camera_target": runtime.camera_target(),
        "camera_fov": runtime.camera_fov_degrees(),
        "battery": runtime.battery(),
        "souls": runtime.souls(),
        "loop": runtime.gameplay_loop_snapshot(),
        "phone": runtime.phone_presentation_snapshot(),
        "hud": runtime.hud_presentation_snapshot(),
    }

func _run() -> void:
    var disabled: Node3D = load("res://scenes/parent_reference.tscn").instantiate()
    var enabled: Node3D = load("res://scenes/parent_reference.tscn").instantiate()
    root.add_child(disabled)
    root.add_child(enabled)
    disabled.agent_playtest = true
    enabled.agent_playtest = true
    await process_frame

    disabled.runtime.start_target_interaction_fixture()
    enabled.runtime.start_target_interaction_fixture()
    disabled.set_camera_trauma_enabled_for_test(false)
    enabled.set_camera_trauma_enabled_for_test(true)
    disabled.inject_camera_trauma_for_test(4.0)
    enabled.inject_camera_trauma_for_test(4.0)
    disabled._sync()
    enabled._sync()

    var off: Dictionary = disabled.presentation_observation_snapshot()
    var on: Dictionary = enabled.presentation_observation_snapshot()
    check((off.rendered_camera_position as Vector3).distance_to(off.base_camera_position) <= 0.000001,
        "disabled-by-default path leaves the rendered camera at its native base")
    var displacement := (on.rendered_camera_position as Vector3).distance_to(on.base_camera_position)
    check(displacement > 0.000001 and displacement <= disabled.CAMERA_TRAUMA_MAX_TRANSLATION + 0.000001,
        "enabled trauma applies a bounded render-only displacement")
    check((on.base_camera_position as Vector3).distance_to(enabled.runtime.camera_position()) <= 0.000001,
        "observation exposes the native camera independently from the rendered camera")
    check((on.camera_position as Vector3).distance_to(on.base_camera_position) <= 0.000001 and
        (on.camera_target as Vector3).distance_to(on.base_camera_target) <= 0.000001,
        "legacy camera observation fields retain native/base semantics")
    check(absf(float(on.base_camera_fov) - float(on.rendered_camera_fov)) <= 0.000001,
        "camera trauma preserves native FOV")

    var native_phone: Dictionary = enabled.runtime.phone_presentation_snapshot()
    var phone_root: Node3D = enabled.phone_adapter.get_node("PhoneRoot")
    check(phone_root.global_position.distance_to(native_phone.position) <= 0.00001,
        "third-person phone projection consumes the base camera sync path")
    for index in range(enabled.targets.size()):
        check(absf(float(enabled.targets[index].camera_visibility_alpha) -
            float(disabled.targets[index].camera_visibility_alpha)) <= 0.000001,
            "obstruction fade is identical with render trauma enabled")
    check(enabled.hud_adapter.snapshot == disabled.hud_adapter.snapshot,
        "HUD receives identical native snapshot data with render trauma enabled")

    var steps := [
        [12, 0.4, 0.8, 3.0, -1.0, false, true, false, false, false, false],
        [1, 0.0, 0.0, 0.0, 0.0, false, false, false, false, false, true],
        [18, -0.3, 0.6, -2.0, 0.5, false, false, false, false, false, false],
    ]
    for step in steps:
        disabled.agent_step(step[0], step[1], step[2], step[3], step[4], step[5],
            step[6], step[7], step[8], step[9], step[10])
        enabled.inject_camera_trauma_for_test(1.0)
        enabled.agent_step(step[0], step[1], step[2], step[3], step[4], step[5],
            step[6], step[7], step[8], step[9], step[10])
        check(_authoritative_snapshot(enabled) == _authoritative_snapshot(disabled),
            "enabled trauma causes zero authoritative-state divergence")

    var first_person: Dictionary = enabled.runtime.phone_presentation_snapshot()
    var first_observation: Dictionary = enabled.presentation_observation_snapshot()
    check(bool(first_person.first_person), "fixture sequence verifies first-person presentation")
    var base_basis: Basis = enabled._camera_base_transform.basis
    var expected_view_position: Vector3 = first_observation.base_camera_position + \
        base_basis * Vector3(0.0, 0.003, -0.69)
    check(phone_root.global_position.distance_to(expected_view_position) <= 0.00001,
        "first-person phone placement consumes the native base camera before trauma")

    var prior_base: Vector3 = first_observation.base_camera_position
    enabled._sync()
    var resynced: Dictionary = enabled.presentation_observation_snapshot()
    check((resynced.base_camera_position as Vector3).distance_to(enabled.runtime.camera_position()) <= 0.000001,
        "subsequent sync restores base before recomputing presentation")
    check((resynced.base_camera_position as Vector3).distance_to(prior_base) <= 0.000001,
        "repeated sync does not accumulate camera drift")

    disabled.free()
    enabled.free()
    if not failures.is_empty():
        print("RESULT: FAIL count=", failures.size())
        quit(2)
        return
    print("RESULT: PASS bounded disabled-by-default render-only camera trauma with zero native divergence")
    quit(0)
