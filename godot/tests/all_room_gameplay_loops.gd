extends SceneTree

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func neutral_tick(runtime) -> void:
    runtime.submit_input(0, 0, 0, 0, false, false, false, false, false, false)
    runtime.tick()

func verify_room(fixture: String, expected_setting: int, label: String) -> void:
    var runtime = ClassDB.instantiate("ParentGameNode")
    runtime.call(fixture)
    runtime.execute_developer_command("enemies off")
    var initial: Dictionary = runtime.gameplay_loop_snapshot()
    var initial_room := int(initial.room_index)
    var required := int(initial.required_souls)
    var initial_tokens := int(initial.tokens)
    check(int(initial.setting) == expected_setting, "%s fixture selects its native setting" % label)
    check(required > 0, "%s fixture has native capture objectives" % label)

    for goal in range(required):
        runtime.execute_developer_command("battery full")
        check(runtime.execute_developer_command("soul spawn") == "SOUL SPAWNED",
            "%s native soul proof command succeeds for goal %d" % [label, goal])
        check(runtime.prepare_capture_shot_fixture(),
            "%s native player can aim at goal %d" % [label, goal])
        runtime.submit_input(0, 0, 0, 0, false, false, false, false, true, false)
        runtime.tick()
        var deposited := false
        for frame in range(180):
            neutral_tick(runtime)
            if int(runtime.gameplay_loop_snapshot().deposited_souls) == goal + 1:
                deposited = true
                break
        check(deposited, "%s native projectile deposits at goal %d" % [label, goal])

    var cleared: Dictionary = runtime.gameplay_loop_snapshot()
    check(bool(cleared.room_clear) and int(cleared.deposited_souls) == required,
        "%s native objectives clear the room" % label)
    check(int(cleared.tokens) == initial_tokens + required,
        "%s native deposits award one token each" % label)

    var crossed := false
    for frame in range(240):
        runtime.submit_input(0, 1, 0, 0, false, true, false, false, false, false)
        runtime.tick()
        var loop: Dictionary = runtime.gameplay_loop_snapshot()
        if bool(loop.upgrade_active):
            crossed = true
            break
    var advanced: Dictionary = runtime.gameplay_loop_snapshot()
    check(crossed and int(advanced.room_index) == initial_room + 1,
        "%s native doorway advances to the next room and opens recompile" % label)
    if crossed:
        runtime.set_upgrade_selection(0)
        check(runtime.activate_upgrade_selection(), "%s native upgrade selection resumes play" % label)
        check(not bool(runtime.gameplay_loop_snapshot().upgrade_active),
            "%s native upgrade pause closes" % label)
    runtime.free()

func _initialize() -> void:
    verify_room("start_coastal_room_fixture", 3, "Coastal")
    verify_room("start_city_room_fixture", 1, "City")
    verify_room("start_sterile_room_fixture", 2, "Sterile")
    if not failures.is_empty():
        print("RESULT: FAIL count=", failures.size())
        quit(2)
        return
    print("RESULT: PASS Coastal, City, and Sterile native capture/economy/doorway/upgrade loops")
    quit(0)
