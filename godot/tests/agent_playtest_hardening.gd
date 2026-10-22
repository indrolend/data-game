extends SceneTree

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func _initialize() -> void:
    var script = load("res://scripts/agent_playtest.gd").new()
    var parsed: Dictionary = script._parse_command("assert room=2 deposited=5 clear=1 tokens=5 upgrade=1")
    check(not parsed.has("error") and parsed.kind == "assert", "semantic assertion command parses")
    check(int(parsed.expectations.room) == 2 and int(parsed.expectations.deposited) == 5 and
        int(parsed.expectations.clear) == 1 and int(parsed.expectations.tokens) == 5 and
        int(parsed.expectations.upgrade) == 1, "assertion values remain exact integers")
    check(script._parse_command("assert clear=true").has("error"),
        "assertions reject ambiguous non-integer booleans")
    check(script._parse_command("assert unknown=1").has("error"),
        "assertions reject unsupported snapshot keys")
    check(script._parse_command("assert room=1 room=2").has("error"),
        "assertions reject duplicate keys")
    check(script._parse_command("room_inspector").kind == "room_inspector" and
        script._parse_command("room_next").kind == "room_next" and
        script._parse_command("room_reroll").kind == "room_reroll",
        "agent protocol exposes bounded native room-inspector navigation")
    check(script._is_authentic_fixture("reset") and script._is_authentic_fixture("battery_full") and
        not script._is_authentic_fixture("observe"),
        "authentic play rejects debug reset and resource fixtures but permits observation")
    script.free()
    var runtime = ClassDB.instantiate("ParentGameNode")
    var observation: Dictionary = runtime.gameplay_loop_snapshot()
    for key in ["room_seed", "setting_name", "form_name", "player_velocity", "live_enemies",
            "loose_souls", "projectiles", "vacuum_target", "vacuum_lock",
            "loose_commitment", "dead", "downed", "camera_yaw", "camera_pitch"]:
        check(observation.has(key), "agent observation exposes native field: %s" % key)
    runtime.free()
    if not failures.is_empty():
        print("RESULT: FAIL count=", failures.size())
        quit(2)
        return
    print("RESULT: PASS agent playtest schema/assertion hardening")
    quit(0)
