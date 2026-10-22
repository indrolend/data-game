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
var follow_commands := false

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
        elif argument == "--follow-commands":
            follow_commands = true
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
    # This script owns every fixed tick. Deliberation or command-file polling must
    # never advance the authoritative simulation in real wall-clock time.
    world.agent_playtest = true
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
    var waiting_for_command := false
    while true:
        if commands.eof_reached():
            if not follow_commands:
                break
            var resume_offset := commands.get_position()
            commands.close()
            if not waiting_for_command:
                print("AGENT_WAITING frame=%d command_offset=%d" % [world.runtime.frame(), resume_offset])
                waiting_for_command = true
            await create_timer(0.05).timeout
            commands = FileAccess.open(commands_path, FileAccess.READ)
            if commands == null:
                print("AGENT_PLAYTEST_ERROR reason=commands_reopen_failed")
                quit(2)
                return
            commands.seek(resume_offset)
            continue
        var line: String = commands.get_line().strip_edges()
        if line.is_empty() or line.begins_with("#"):
            continue
        waiting_for_command = false
        var command := _parse_command(line)
        if command.has("error"):
            print("AGENT_PLAYTEST_ERROR reason=%s" % command.error)
            commands.close()
            quit(2)
            return
        if authentic_playthrough and _is_authentic_fixture(command.kind):
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
            "room_inspector":
                world.runtime.start_room_inspector()
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "room_next":
                world.runtime.step_room_inspector(1, false)
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "room_reroll":
                world.runtime.step_room_inspector(0, true)
                world._sync()
                await process_frame
                if not await _observe():
                    commands.close()
                    return
            "room_enemies":
                world.runtime.toggle_room_inspector_enemies()
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
    var nearest_snapshot := {}
    for index in range(world.runtime.target_count()):
        var target: Dictionary = world.runtime.target_presentation_snapshot(index)
        if bool(target.get("alive", false)) or bool(target.get("slurpable", false)):
            var distance: float = player.distance_to(target.position)
            if distance < nearest_distance:
                nearest_distance = distance
                nearest = index
                nearest_snapshot = target
    var objective := -1
    var objective_position := Vector3.ZERO
    var objective_distance := INF
    var room_snapshot: Dictionary = world.runtime.room_presentation_snapshot(-1)
    var capture_points: Array = room_snapshot.get("capture_points", [])
    for index in range(capture_points.size()):
        var capture: Dictionary = capture_points[index]
        if bool(capture.get("filled", false)):
            continue
        var distance: float = player.distance_to(capture.position)
        if distance < objective_distance:
            objective_distance = distance
            objective = index
            objective_position = capture.position
    var loop: Dictionary = world.runtime.gameplay_loop_snapshot()
    print("AGENT_STATE schema=%d frame=%d room=%d seed=%d environment=%s/%s heat=%.6f captures=%d deposited=%d required=%d clear=%d tokens=%d upgrade=%d pos=%.6f,%.6f,%.6f yaw=%.6f pitch=%.6f battery=%.6f souls=%d living=%d loose=%d projectiles=%d action=%s vacuum=%d vacuum_target=%d vacuum_lock=%.6f loose_commitment=%.6f dead=%d downed=%d grounded=%d first_person=%d nearest_target=%d nearest_distance=%.6f target_pos=%.6f,%.6f,%.6f target_alive=%d target_slurpable=%d target_armor=%.6f target_health=%.6f target_crack=%.6f target_attack=%d target_attack_phase=%.6f objective=%d objective_pos=%.6f,%.6f,%.6f objective_distance=%.6f frame_capture=%d frame_file=\"%s\"" % [
        SCHEMA, world.runtime.frame(), int(loop.room_index), int(loop.room_seed),
        str(loop.setting_name), str(loop.form_name), float(loop.room_heat),
        int(loop.room_captures), int(loop.deposited_souls), int(loop.required_souls),
        int(loop.room_clear), int(loop.tokens), int(loop.upgrade_active),
        player.x, player.y, player.z, float(loop.camera_yaw), float(loop.camera_pitch),
        world.runtime.battery(), world.runtime.souls(), int(loop.live_enemies), int(loop.loose_souls),
        int(loop.projectiles), _agent_action(loop), int(world.runtime.vacuum_active()),
        int(loop.vacuum_target), float(loop.vacuum_lock), float(loop.loose_commitment),
        int(loop.dead), int(loop.downed), int(world.runtime.grounded()),
        int(world.runtime.first_person()), nearest,
        nearest_distance, float(nearest_snapshot.get("position", Vector3.ZERO).x),
        float(nearest_snapshot.get("position", Vector3.ZERO).y),
        float(nearest_snapshot.get("position", Vector3.ZERO).z),
        int(nearest_snapshot.get("alive", false)), int(nearest_snapshot.get("slurpable", false)),
        float(nearest_snapshot.get("armor", 0.0)), float(nearest_snapshot.get("health", 0.0)),
        float(nearest_snapshot.get("shell_crack_amount", 0.0)),
        int(nearest_snapshot.get("attack_active", false)),
        float(nearest_snapshot.get("attack_phase", 0.0)), objective,
        objective_position.x, objective_position.y, objective_position.z, objective_distance,
        int(capture_error == OK), ProjectSettings.globalize_path(frame_path)])
    var inspector: Dictionary = world.runtime.room_inspector_snapshot()
    if bool(inspector.get("active", false)):
        print("ROOM_INSPECTOR frame=%d premise=%s seed=%d room=%d setting=%s form=%s scale=%s condition=%s traversal=%s route_valid=%d seed_valid=%d surfaces=%d edges=%d required_edges=%d colliders=%d props=%d enemies=%d visible_estimate=%d draw_bucket=%d" % [
            world.runtime.frame(), str(inspector.premise_name), int(inspector.seed),
            int(inspector.room_index), str(inspector.setting_name), str(inspector.form_name),
            str(inspector.scale_name), str(inspector.condition_name), str(inspector.traversal_intent),
            int(inspector.route_valid), int(inspector.seed_valid), int(inspector.surface_count),
            int(inspector.edge_count), int(inspector.required_edge_count), int(inspector.collider_count),
            int(inspector.presentation_prop_count), int(inspector.enemy_count),
            int(inspector.visible_primitive_estimate), int(inspector.draw_call_bucket)])
    observation += 1
    return true

func _agent_action(loop: Dictionary) -> String:
    if bool(loop.dead):
        return "dead"
    if bool(loop.downed):
        return "downed"
    if world.runtime.vacuum_active():
        return "vacuum"
    if int(loop.projectiles) > 0:
        return "projectile"
    var velocity: Vector3 = loop.player_velocity
    return "move" if Vector2(velocity.x, velocity.z).length_squared() > 0.01 else "idle"

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
    if verb in ["observe", "reset", "field_island_fixture", "soul_spawn", "battery_full", "enemies_off", "capture_shot_fixture", "city_fixture", "coastal_fixture", "sterile_fixture", "ruin_fixture", "upgrade_fixture", "room_inspector", "room_next", "room_reroll", "room_enemies", "upgrade_confirm", "quit", "pause", "menu_confirm", "menu_back"]:
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

func _is_authentic_fixture(kind: String) -> bool:
    return kind in ["reset", "field_island_fixture", "soul_spawn", "battery_full",
        "enemies_off", "capture_shot_fixture", "city_fixture", "coastal_fixture",
        "sterile_fixture", "ruin_fixture", "upgrade_fixture", "room_inspector",
        "room_next", "room_reroll", "room_enemies"]
