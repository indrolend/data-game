extends SceneTree

const SCHEMA := 3
const MAX_STEP_FRAMES := 120
const MAX_COMMAND_BYTES := 1024

var world: Node3D
var observation := 0
var capture_dir := "res://../evidence/generated/agent-playtest"
var commands_path := ""
var authentic_playthrough := false
var allow_existing_captures := false

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    for argument in OS.get_cmdline_user_args():
        if argument.begins_with("--capture-dir="):
            capture_dir = argument.trim_prefix("--capture-dir=")
        elif argument.begins_with("--commands="):
            commands_path = argument.trim_prefix("--commands=")
        elif argument == "--authentic-playthrough":
            authentic_playthrough = true
        elif argument == "--allow-existing-captures":
            allow_existing_captures = true
    if commands_path.is_empty():
        push_error("AGENT_PLAYTEST_ERROR reason=missing_commands")
        quit(2)
        return
    var absolute_capture_dir := ProjectSettings.globalize_path(capture_dir)
    if not allow_existing_captures and DirAccess.dir_exists_absolute(absolute_capture_dir):
        var existing := DirAccess.get_files_at(absolute_capture_dir)
        if not existing.is_empty():
            print("AGENT_PLAYTEST_ERROR reason=capture_directory_not_empty path=\"%s\"" % absolute_capture_dir)
            quit(2)
            return
    world = load("res://scenes/parent_reference.tscn").instantiate()
    root.add_child(world)
    await process_frame
    await process_frame
    print("AGENT_PLAYTEST_READY schema=%d fixed_dt=0.016666667 max_step_frames=%d persistent_save=DISABLED authentic=%d" % [SCHEMA, MAX_STEP_FRAMES, int(authentic_playthrough)])
    if not await _observe():
        return
    var commands := FileAccess.open(commands_path, FileAccess.READ)
    if commands == null:
        push_error("AGENT_PLAYTEST_ERROR reason=commands_open_failed")
        quit(2)
        return
    while not commands.eof_reached():
        var line: String = commands.get_line().strip_edges()
        if line.is_empty() or line.begins_with("#"):
            continue
        var command := _parse_command(line)
        if command.has("error"):
            print("AGENT_PLAYTEST_ERROR reason=%s" % command.error)
            commands.close()
            quit(2)
            return
        if authentic_playthrough and command.kind in ["field_island_fixture", "soul_spawn",
                "battery_full", "enemies_off", "capture_shot_fixture", "city_fixture",
                "coastal_fixture", "sterile_fixture", "ruin_fixture", "upgrade_fixture"]:
            print("AGENT_PLAYTEST_ERROR reason=authentic_mode_rejects command=%s" % command.kind)
            commands.close()
            quit(2)
            return
        match command.kind:
            "observe":
                if not await _observe():
                    commands.close()
                    return
            "reset":
                world.agent_reset()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "city_fixture":
                world.runtime.start_city_room_fixture()
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "field_island_fixture":
                world.runtime.start_field_island_fixture()
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "soul_spawn":
                var response: String = world.runtime.execute_developer_command("soul spawn")
                if response != "SOUL SPAWNED":
                    print("AGENT_PLAYTEST_ERROR reason=soul_spawn_failed response=\"%s\"" % response)
                    commands.close()
                    quit(2)
                    return
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "battery_full", "enemies_off":
                var developer_command := "battery full" if command.kind == "battery_full" else "enemies off"
                var response: String = world.runtime.execute_developer_command(developer_command)
                if response.begins_with("INVALID") or response.begins_with("UNKNOWN"):
                    print("AGENT_PLAYTEST_ERROR reason=developer_fixture_failed response=\"%s\"" % response)
                    commands.close()
                    quit(2)
                    return
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "capture_shot_fixture":
                if not world.runtime.prepare_capture_shot_fixture():
                    print("AGENT_PLAYTEST_ERROR reason=capture_shot_fixture_failed")
                    commands.close()
                    quit(2)
                    return
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "coastal_fixture":
                world.runtime.start_coastal_room_fixture()
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "sterile_fixture":
                world.runtime.start_sterile_room_fixture()
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "ruin_fixture":
                world.runtime.start_ruin_room_fixture()
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "upgrade_fixture":
                world.runtime.start_upgrade_fixture()
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "upgrade_move":
                world.runtime.move_upgrade_selection(command.horizontal, command.vertical)
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "upgrade_select":
                world.runtime.set_upgrade_selection(command.index)
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "upgrade_confirm":
                world.runtime.activate_upgrade_selection()
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "pause":
                world.runtime.set_paused(true)
                world.runtime.tick()
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "menu_move":
                world.runtime.menu_move_selection(command.direction)
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "menu_adjust":
                world.runtime.menu_adjust(command.direction)
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "menu_confirm":
                world.runtime.menu_confirm()
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "menu_back":
                world.runtime.menu_back()
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "quit":
                print("AGENT_PLAYTEST_QUIT frame=%d" % world.runtime.frame())
                quit(0)
                return
            "assert":
                if not _assert_state(command.expectations):
                    commands.close()
                    return
                print("AGENT_ASSERT_PASS frame=%d expectations=\"%s\"" % [world.runtime.frame(), command.raw])
            "step":
                world.agent_step(command.frames, command.move_x, command.move_z,
                    command.look_x, command.look_y, command.vacuum, command.sprint,
                    command.jump, command.melee, command.shoot, command.camera)
                await process_frame
                if not await _observe():
                    commands.close()
                    return
    print("AGENT_PLAYTEST_EOF frame=%d" % world.runtime.frame())
    quit(0)

func _observe() -> bool:
    # `frame_post_draw` is not guaranteed to fire in headless script mode.
    # A SceneTree frame boundary keeps observation/capture deterministic there.
    await process_frame
    var directory_error := DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(capture_dir))
    if directory_error != OK and directory_error != ERR_ALREADY_EXISTS:
        print("AGENT_PLAYTEST_ERROR reason=capture_directory_failed code=%d" % directory_error)
        quit(3)
        return false
    var frame_path := "%s/frame-%03d-tick-%06d.png" % [capture_dir, observation, world.runtime.frame()]
    var viewport_texture := root.get_texture()
    if viewport_texture == null:
        print("AGENT_PLAYTEST_ERROR reason=framebuffer_unavailable")
        quit(3)
        return false
    var image := viewport_texture.get_image()
    if image == null or image.is_empty():
        print("AGENT_PLAYTEST_ERROR reason=framebuffer_empty")
        quit(3)
        return false
    var capture_error := image.save_png(ProjectSettings.globalize_path(frame_path))
    if capture_error != OK:
        print("AGENT_PLAYTEST_ERROR reason=frame_capture_failed code=%d frame_file=\"%s\"" % [capture_error, ProjectSettings.globalize_path(frame_path)])
        quit(3)
        return false
    var player: Vector3 = world.runtime.player_position()
    var nearest := -1
    var nearest_distance := INF
    for index in range(world.runtime.target_count()):
        if world.runtime.target_alive(index):
            var distance: float = player.distance_to(world.runtime.target_position(index))
            if distance < nearest_distance:
                nearest_distance = distance
                nearest = index
    var loop: Dictionary = world.runtime.gameplay_loop_snapshot()
    print("AGENT_STATE schema=%d frame=%d room=%d heat=%.6f captures=%d deposited=%d required=%d clear=%d tokens=%d upgrade=%d pos=%.6f,%.6f,%.6f battery=%.6f souls=%d grounded=%d first_person=%d vacuum=%d nearest_target=%d nearest_distance=%.6f frame_capture=%d frame_file=\"%s\"" % [
        SCHEMA, world.runtime.frame(), int(loop.room_index), float(loop.room_heat),
        int(loop.room_captures), int(loop.deposited_souls), int(loop.required_souls),
        int(loop.room_clear), int(loop.tokens), int(loop.upgrade_active),
        player.x, player.y, player.z,
        world.runtime.battery(), world.runtime.souls(), int(world.runtime.grounded()),
        int(world.runtime.first_person()), int(world.runtime.vacuum_active()), nearest,
        nearest_distance, int(capture_error == OK), ProjectSettings.globalize_path(frame_path)])
    observation += 1
    return true

func _assert_state(expectations: Dictionary) -> bool:
    var loop: Dictionary = world.runtime.gameplay_loop_snapshot()
    var actual := {
        "room": int(loop.room_index),
        "deposited": int(loop.deposited_souls),
        "required": int(loop.required_souls),
        "clear": int(loop.room_clear),
        "tokens": int(loop.tokens),
        "upgrade": int(loop.upgrade_active),
        "souls": int(loop.stored_souls),
        "grounded": int(world.runtime.grounded()),
        "first_person": int(world.runtime.first_person()),
        "vacuum": int(world.runtime.vacuum_active()),
    }
    for key in expectations:
        if not actual.has(key) or actual[key] != expectations[key]:
            print("AGENT_PLAYTEST_ERROR reason=assertion_failed key=%s expected=%s actual=%s frame=%d" % [
                key, expectations[key], actual.get(key, "unsupported"), world.runtime.frame()])
            quit(4)
            return false
    return true

func _parse_command(line: String) -> Dictionary:
    if line.length() > MAX_COMMAND_BYTES:
        return {"error": "command_too_long"}
    var tokens := line.split(" ", false)
    if tokens.is_empty():
        return {"error": "empty_command"}
    var verb := tokens[0]
    if verb in ["observe", "reset", "field_island_fixture", "soul_spawn", "battery_full", "enemies_off", "capture_shot_fixture", "city_fixture", "coastal_fixture", "sterile_fixture", "ruin_fixture", "upgrade_fixture", "upgrade_confirm", "quit", "pause", "menu_confirm", "menu_back"]:
        return {"kind": verb} if tokens.size() == 1 else {"error": "unexpected_argument"}
    if verb == "assert":
        if tokens.size() < 2:
            return {"error": "assert_requires_expectations"}
        var expectations := {}
        var raw_parts: PackedStringArray = []
        for token in tokens.slice(1):
            var equals := token.find("=")
            if equals <= 0 or equals == token.length() - 1:
                return {"error": "assert_expected_key_value"}
            var key := token.left(equals)
            var value := token.substr(equals + 1)
            if key not in ["room", "deposited", "required", "clear", "tokens", "upgrade",
                    "souls", "grounded", "first_person", "vacuum"]:
                return {"error": "assert_unknown_key:%s" % key}
            if expectations.has(key) or not value.is_valid_int():
                return {"error": "assert_invalid_value:%s" % key}
            expectations[key] = int(value)
            raw_parts.append(token)
        return {"kind": verb, "expectations": expectations, "raw": " ".join(raw_parts)}
    if verb == "upgrade_select":
        if tokens.size() != 2 or not tokens[1].begins_with("index="):
            return {"error": "expected_index"}
        var index_text := tokens[1].trim_prefix("index=")
        if not index_text.is_valid_int() or int(index_text) < 0 or int(index_text) > 5:
            return {"error": "invalid_value:index"}
        return {"kind": verb, "index": int(index_text)}
    if verb == "upgrade_move":
        if tokens.size() != 3 or not tokens[1].begins_with("horizontal=") or \
                not tokens[2].begins_with("vertical="):
            return {"error": "expected_horizontal_vertical"}
        var horizontal_text := tokens[1].trim_prefix("horizontal=")
        var vertical_text := tokens[2].trim_prefix("vertical=")
        if not horizontal_text.is_valid_int() or int(horizontal_text) < -1 or int(horizontal_text) > 1 or \
                not vertical_text.is_valid_int() or int(vertical_text) < -1 or int(vertical_text) > 1:
            return {"error": "invalid_value:upgrade_direction"}
        return {"kind": verb, "horizontal": int(horizontal_text), "vertical": int(vertical_text)}
    if verb == "menu_move" or verb == "menu_adjust":
        if tokens.size() != 2 or not tokens[1].begins_with("direction="):
            return {"error": "expected_direction"}
        var direction_text := tokens[1].trim_prefix("direction=")
        if not direction_text.is_valid_int() or int(direction_text) not in [-1, 1]:
            return {"error": "invalid_value:direction"}
        return {"kind": verb, "direction": int(direction_text)}
    if verb != "step":
        return {"error": "unknown_command"}
    var command := {"kind": "step", "frames": 1, "move_x": 0.0, "move_z": 0.0,
        "look_x": 0.0, "look_y": 0.0, "sprint": false, "vacuum": false,
        "jump": false, "melee": false, "shoot": false, "camera": false}
    var keys := {}
    for token in tokens.slice(1):
        var equals := token.find("=")
        if equals <= 0 or equals == token.length() - 1:
            return {"error": "expected_key_value"}
        var key := token.left(equals)
        var value := token.substr(equals + 1)
        if keys.has(key):
            return {"error": "duplicate_key:%s" % key}
        keys[key] = true
        match key:
            "frames":
                if not value.is_valid_int() or int(value) < 1 or int(value) > MAX_STEP_FRAMES:
                    return {"error": "invalid_value:frames"}
                command.frames = int(value)
            "moveX", "moveZ", "lookX", "lookY":
                if not value.is_valid_float():
                    return {"error": "invalid_value:%s" % key}
                var number := float(value)
                var limit := 1.0 if key in ["moveX", "moveZ"] else 400.0
                if not is_finite(number) or number < -limit or number > limit:
                    return {"error": "invalid_value:%s" % key}
                var target_key: String = {"moveX":"move_x", "moveZ":"move_z", "lookX":"look_x", "lookY":"look_y"}[key]
                command[target_key] = number
            "sprint", "vacuum", "jump", "melee", "shoot", "camera":
                var parsed: Variant = _parse_bool(value)
                if parsed == null:
                    return {"error": "invalid_value:%s" % key}
                command[key] = parsed
            _:
                return {"error": "unknown_key:%s" % key}
    return command

func _parse_bool(value: String) -> Variant:
    if value in ["1", "true", "on"]:
        return true
    if value in ["0", "false", "off"]:
        return false
    return null
