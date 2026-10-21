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
    script.free()
    if not failures.is_empty():
        print("RESULT: FAIL count=", failures.size())
        quit(2)
        return
    print("RESULT: PASS agent playtest schema/assertion hardening")
    quit(0)
