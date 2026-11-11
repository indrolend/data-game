extends SceneTree

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func new_game() -> Object:
    return ClassDB.instantiate("ParentGameNode")

func drive(game: Object, frame: int) -> void:
    var vacuum := frame >= 90 and frame < 150
    game.submit_input(0.25 if frame < 80 else 0.0, 0.70 if frame < 120 else 0.0,
        0.35 if frame < 30 else 0.0, 0.0, vacuum, frame < 60,
        frame == 12, frame == 72, frame == 170, false)
    game.tick()

func _initialize() -> void:
    var first := new_game()
    var second := new_game()
    if first == null or second == null:
        push_error("FAIL: ParentGameNode could not be instantiated")
        quit(1)
        return
    var initial: String = first.authoritative_state_digest()
    check(initial.length() == 16, "fixed-width digest")
    check(initial == second.authoritative_state_digest(), "identical reset state")
    first.set_developer_console_open(true)
    first.execute_developer_command("colliders show")
    check(first.authoritative_state_digest() == initial,
        "bridge-only presentation controls do not change authority")
    for frame in range(240):
        drive(first, frame)
        drive(second, frame)
        check(first.authoritative_state_digest() == second.authoritative_state_digest(),
            "identical input stream at frame %d" % frame)
    var before_mutation: String = first.authoritative_state_digest()
    first.execute_developer_command("soul spawn")
    check(first.authoritative_state_digest() != before_mutation,
        "gameplay mutation changes digest")
    print("GODOT_AUTHORITATIVE_STATE_DIGEST_OK initial=%s final=%s frames=240" % [
        initial, second.authoritative_state_digest()])
    first.free()
    second.free()
    if not failures.is_empty():
        print("RESULT: FAIL authoritative state digest count=", failures.size())
        quit(2)
        return
    quit(0)
