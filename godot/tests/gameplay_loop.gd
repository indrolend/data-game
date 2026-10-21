extends SceneTree

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func _initialize() -> void:
    var runtime = ClassDB.instantiate("ParentGameNode")
    var initial: Dictionary = runtime.gameplay_loop_snapshot()
    check(int(initial.room_index) == 1 and int(initial.setting) == 0 and
        int(initial.required_souls) == 5 and not bool(initial.room_clear),
        "native run starts in an uncleared five-goal Field room")
    for frame in range(60):
        runtime.submit_input(0, 0, 0, 0, false, false, false, false, false, false)
        runtime.tick()
    var heated: Dictionary = runtime.gameplay_loop_snapshot()
    check(float(heated.room_elapsed) >= 0.99 and float(heated.room_heat) > 0.02,
        "native elapsed-time pressure increases room heat without Godot-owned timers")
    var initial_tokens := int(initial.tokens)
    var last_battery := float(heated.battery)
    for goal in range(int(initial.required_souls)):
        check(runtime.execute_developer_command("soul spawn") == "SOUL SPAWNED",
            "existing native proof hook supplies the stored soul for goal %d" % goal)
        check(runtime.prepare_capture_shot_fixture(),
            "native proof fixture aims the authoritative player at the next unfilled goal")
        runtime.submit_input(0, 0, 0, 0, false, false, false, false, true, false)
        runtime.tick()
        var deposited := false
        for frame in range(180):
            runtime.submit_input(0, 0, 0, 0, false, false, false, false, false, false)
            runtime.tick()
            var loop: Dictionary = runtime.gameplay_loop_snapshot()
            if int(loop.deposited_souls) == goal + 1:
                deposited = true
                check(int(loop.stored_souls) == 0,
                    "native shot consumes its stored soul after goal %d" % goal)
                check(float(loop.battery) < last_battery,
                    "native shot battery cost is retained for goal %d" % goal)
                last_battery = float(loop.battery)
                break
        check(deposited, "native projectile fills capture goal %d" % goal)
    var cleared: Dictionary = runtime.gameplay_loop_snapshot()
    check(bool(cleared.room_clear) and int(cleared.deposited_souls) == int(cleared.required_souls),
        "five native projectile deposits clear the Field room")
    check(int(cleared.tokens) == initial_tokens + int(cleared.required_souls),
        "each native capture awards exactly one permanent-memory token")

    var crossed := false
    for frame in range(180):
        runtime.submit_input(0, 1, 0, 0, false, true, false, false, false, false)
        runtime.tick()
        if bool(runtime.gameplay_loop_snapshot().upgrade_active):
            crossed = true
            break
    var advanced: Dictionary = runtime.gameplay_loop_snapshot()
    check(crossed and int(advanced.room_index) == 2 and bool(advanced.upgrade_active) and runtime.paused(),
        "walking through the cleared doorway advances the native room and opens its upgrade pause")
    check(float(advanced.room_heat) < 0.01 and int(advanced.room_captures) == 0 and
        int(advanced.deposited_souls) == 0 and int(advanced.stored_souls) == 0,
        "new-room initialization resets room-local heat/captures while preserving run authority")
    var before_level := int((advanced.temporary_levels as Array)[1])
    runtime.set_upgrade_selection(1)
    check(runtime.activate_upgrade_selection(), "native temporary upgrade selection succeeds")
    var resumed: Dictionary = runtime.gameplay_loop_snapshot()
    check(not bool(resumed.upgrade_active) and not runtime.paused() and
        int((resumed.temporary_levels as Array)[1]) == before_level + 1,
        "native upgrade mutation resumes the run with its selected level")
    runtime.free()
    if not failures.is_empty():
        print("RESULT: FAIL count=", failures.size())
        quit(2)
        return
    print("RESULT: PASS native Field deposits, battery/token economy, room clear, doorway progression, and upgrade resume")
    quit(0)
