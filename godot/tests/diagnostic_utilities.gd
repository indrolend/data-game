extends SceneTree

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func _initialize() -> void:
    var runtime = ClassDB.instantiate("ParentGameNode")
    runtime.start_room_inspector()
    var initial: Dictionary = runtime.room_inspector_snapshot()
    check(bool(initial.active) and int(initial.room_index) == 12,
        "native room inspector exposes its deterministic inspection room")
    check(bool(initial.seed_valid) and bool(initial.route_valid),
        "native room inspector reports valid seed and required route")
    check(str(initial.premise_name) == "FIELD_OPEN" and not str(initial.setting_name).is_empty(),
        "native room inspector exposes premise and environment identity")
    check(str(runtime.room_review_line(0)).begins_with("ROOM_REVIEW premise=FIELD_OPEN") and
        " rating=KEEP " in str(runtime.room_review_line(0)),
        "native room review retains its stable evidence line")
    var original_premise: int = int(initial.premise)
    runtime.step_room_inspector(1, false)
    var advanced: Dictionary = runtime.room_inspector_snapshot()
    check(int(advanced.premise) != original_premise and bool(advanced.seed_valid),
        "room inspector advances to the next native premise")
    var previous_seed: int = int(advanced.seed)
    runtime.step_room_inspector(0, true)
    var rerolled: Dictionary = runtime.room_inspector_snapshot()
    check(int(rerolled.premise) == int(advanced.premise) and int(rerolled.seed) != previous_seed,
        "room inspector rerolls within the selected native premise")
    runtime.toggle_room_inspector_enemies()
    check(bool(runtime.room_inspector_snapshot().enemies),
        "room inspector enemy toggle remains native-owned")

    runtime.start_field_island_fixture()
    var supported: Dictionary = runtime.player_support_snapshot(0.0, 0.0)
    var abyss: Dictionary = runtime.player_support_snapshot(80.0, 0.0)
    check(bool(supported.available) and not bool(abyss.available),
        "support probe distinguishes Field island ground from abyss")
    check((supported.normal as Vector3).y > 0.9,
        "support probe preserves the native support normal")

    runtime.start_slope_fixture()
    var slope: Dictionary = runtime.world_support_snapshot(0.0, 7.0, 0.25)
    check(bool(slope.available) and (slope.normal as Vector3).y > 0.0,
        "native slope fixture and bounded support query are exposed")
    runtime.free()
    if not failures.is_empty():
        print("RESULT: FAIL count=", failures.size())
        quit(2)
        return
    print("RESULT: PASS native room-inspector, review, traversal, slope, and support diagnostics")
    quit(0)
