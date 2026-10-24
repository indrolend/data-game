extends SceneTree

var failures: Array[String] = []

func check(condition: bool, description: String) -> void:
    if not condition:
        failures.append(description)
        push_error("FAIL: " + description)

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    var game = ClassDB.instantiate("ParentGameNode")

    check(not game.developer_console_open(), "console starts closed")

    game.set_developer_console_open(true)
    check(game.developer_console_open(), "console opens")

    var help: String = game.execute_developer_command("help")
    check(help.contains("battery full"), "original help command")

    var state: String = game.execute_developer_command("state")
    check(state.contains("souls"), "native state command")

    var battery: String = game.execute_developer_command("battery full")
    check(battery == "BATTERY FULL", "battery command response")
    check(absf(game.battery() - 100.0) < 0.001, "native battery mutation")

    var unknown: String = game.execute_developer_command("not-a-command")
    check(unknown == "UNKNOWN COMMAND", "unknown command handling")

    var invalid: String = game.execute_developer_command("battery wrong")
    check(invalid == "INVALID ARGUMENTS", "invalid argument handling")

    game.execute_developer_command("colliders show")
    check(game.developer_show_colliders(), "collider visibility preference")

    game.execute_developer_command("colliders hide")
    check(not game.developer_show_colliders(), "collider preference reset")

    var lighting: String = game.execute_developer_command("lighting")
    check(lighting.contains("UNSUPPORTED"), "unsupported lighting command")

    game.set_developer_console_open(false)
    check(not game.developer_console_open(), "console closes")

    game.free()

    if not failures.is_empty():
        print("RESULT: FAIL developer codec count=", failures.size())
        quit(2)
        return

    print("RESULT: PASS native developer codec command execution")
    quit(0)
