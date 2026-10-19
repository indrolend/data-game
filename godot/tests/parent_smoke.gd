extends SceneTree

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func new_game() -> Object:
    return ClassDB.instantiate("ParentGameNode")

func live_targets(game: Object) -> int:
    var count := 0
    for i in range(game.target_count()):
        if game.target_alive(i):
            count += 1
    return count

func tick(game: Object, count: int, x := 0.0, z := 0.0, vacuum := false,
        sprint := false, jump_first := false, melee_first := false,
        shoot_first := false, camera_first := false, look_x := 0.0, look_y := 0.0) -> void:
    for i in range(count):
        game.submit_input(x, z, look_x, look_y, vacuum, sprint,
            jump_first and i == 0, melee_first and i == 0,
            shoot_first and i == 0, camera_first and i == 0)
        game.tick()

func _initialize() -> void:
    var probe := new_game()
    if probe == null:
        push_error("FAIL: ParentGameNode could not be instantiated")
        quit(1)
        return
    print("PARENT_GAME_NODE_LOADED")
    probe.free()

    var a := new_game()
    var b := new_game()
    tick(a, 120, 0.0, 1.0)
    tick(b, 120, 0.0, 1.0)
    check(a.frame() == 120, "fixed-step frame count")
    check(a.player_position().distance_to(b.player_position()) <= 0.000001,
        "identical runs produce identical player positions")
    check(absf(a.battery() - b.battery()) <= 0.000001,
        "identical runs produce identical battery")
    check(a.player_position().distance_to(Vector3(0.0, 0.08, 15.5)) > 0.001,
        "movement input changes position")
    a.free()
    b.free()

    var walk := new_game()
    var run := new_game()
    var walk_start: Vector3 = walk.player_position()
    var run_start: Vector3 = run.player_position()
    tick(walk, 30, 0.0, 1.0)
    tick(run, 30, 0.0, 1.0, false, true)
    check(run.player_position().distance_to(run_start) > walk.player_position().distance_to(walk_start),
        "sprint travels farther than walk over the same ticks")
    walk.free()
    run.free()

    var actions := new_game()
    var initial_target: Vector3 = actions.camera_target()
    tick(actions, 1, 0.0, 0.0, false, false, false, false, false, false, 8.0, -3.0)
    check(actions.camera_target().distance_to(initial_target) > 0.001, "look deltas change camera target")
    tick(actions, 1, 0.0, 0.0, false, false, false, false, false, true)
    check(actions.first_person(), "camera toggle changes camera mode")
    tick(actions, 1, 0.0, 0.0, false, false, true)
    check(not actions.grounded() or actions.player_position().y > 0.08, "jump leaves ground")
    var vacuum_game := new_game()
    tick(vacuum_game, 1, 0.0, 0.0, true)
    check(vacuum_game.vacuum_active(), "vacuum held activates vacuum while grounded")
    vacuum_game.free()
    var battery_before_melee: float = actions.battery()
    tick(actions, 1, 0.0, 0.0, false, false, false, true)
    check(actions.melee_pose() > 0.0, "melee press starts melee pose")
    check(actions.battery() < battery_before_melee, "melee consumes native battery")
    check(actions.target_count() > 0, "native target collection is exposed")
    actions.free()

    var melee_contract := new_game()
    var melee_before: Dictionary = melee_contract.player_action_snapshot()
    tick(melee_contract, 1, 0.0, 0.0, false, false, false, true)
    var melee_started: Dictionary = melee_contract.player_action_snapshot()
    check(int(melee_started.sequence) > int(melee_before.sequence),
        "grounded melee advances the native action sequence")
    check(float(melee_started.range) > 0.0 and float(melee_started.hit_radius) > 0.0,
        "native melee exposes attack reach and hit radius")
    check((melee_started.direction as Vector3).length() > 0.99,
        "native melee exposes a normalized attack direction")
    tick(melee_contract, 5)
    var melee_advanced: Dictionary = melee_contract.player_action_snapshot()
    check(float(melee_advanced.phase) > float(melee_started.phase),
        "native melee presentation phase advances with native mechanic time")
    melee_contract.free()

    var communication := new_game()
    communication.set_comm_signal(2)
    communication.tick()
    check(communication.comm_signal() == 2, "communication signal is owned and activated by parent Game")
    check(communication.comm_signal_timer() > 0.0, "parent Game owns communication signal lifetime")
    tick(communication, 150)
    check(communication.comm_signal() == 0, "parent Game expires communication signal")
    communication.free()

    var pause := new_game()
    tick(pause, 30, 0.0, 1.0)
    var position_before_pause: Vector3 = pause.player_position()
    var time_before_pause: float = pause.simulation_time()
    pause.set_paused(true)
    tick(pause, 60, 0.0, 1.0)
    check(pause.paused(), "parent Game owns paused state")
    check(pause.player_position().distance_to(position_before_pause) <= 0.000001,
        "solo pause freezes parent player simulation")
    check(absf(pause.simulation_time() - time_before_pause) <= 0.000001,
        "solo pause freezes parent simulation clock")
    pause.set_paused(false)
    tick(pause, 30, 0.0, 1.0)
    check(not pause.paused(), "parent Game resumes from pause")
    check(pause.player_position().distance_to(position_before_pause) > 0.001,
        "movement resumes after parent pause ends")
    pause.free()

    var room := new_game()
    var room_snapshot: Dictionary = room.room_presentation_snapshot(0)
    var room_snapshot_again: Dictionary = room.room_presentation_snapshot(0)
    check(int(room_snapshot.room_seed) == 12345, "room presentation uses native room seed")
    check(int(room_snapshot.room_index) == 1, "room presentation uses native room index")
    check((room_snapshot.structures as Array).size() == 8,
        "native open-field room produces floor, two complete door frames, and selected traversal surface")
    check((room_snapshot.objects as Array).size() > 0, "native room produces computed scene objects")
    check((room_snapshot.objects as Array).size() == (room_snapshot_again.objects as Array).size(),
        "native computed room object count is deterministic")
    if (room_snapshot.objects as Array).size() > 0:
        check(str((room_snapshot.objects as Array)[0].id) == str((room_snapshot_again.objects as Array)[0].id),
            "native computed room object IDs are stable")
    check(str((room_snapshot.structures as Array)[0].id) == "room/0/structure/floor",
        "native room structure IDs are stable")
    var floor_material: Dictionary = (room_snapshot.structures as Array)[0].material
    var floor_color: Color = floor_material.base_color
    check(absf(floor_color.r - 63.0 / 255.0) <= 0.000001,
        "native field floor material carries VisualIdentity field color")
    check(int(floor_material.texture) == 1,
        "native field floor material carries field-grass texture identity")
    check(str((room_snapshot.structures as Array)[7].id) == "room/0/structure/traversal/4",
        "native room exposes selected traversal surface by stable graph index")
    room.free()

    var rally := new_game()
    rally.start_rally_fixture()
    check(rally.souls() == 1, "parent rally fixture seeds one stored soul")
    var bullets_before: int = rally.live_bullets()
    tick(rally, 1, 0.0, 0.0, false, false, false, false, true)
    check(rally.souls() == 0, "shoot consumes the stored soul")
    tick(rally, 8)
    check(rally.live_bullets() > bullets_before, "shoot creates a native projectile")
    rally.free()

    var target_fixture_a := new_game()
    var target_fixture_b := new_game()
    target_fixture_a.start_target_fixture()
    target_fixture_b.start_target_fixture()
    var live_count := 0
    var first_live := -1
    for i in range(target_fixture_a.target_count()):
        if target_fixture_a.target_alive(i):
            live_count += 1
            if first_live < 0:
                first_live = i
    check(live_count > 0, "parent room inspector fixture starts live targets")
    check(live_count == 0 or target_fixture_b.target_alive(first_live),
        "parent target fixture is deterministic")
    if first_live >= 0:
        var target_pose_start: Dictionary = target_fixture_a.target_presentation_snapshot(first_live)
        check(int(target_pose_start.index) == first_live,
            "target presentation snapshot preserves native target identity")
        check(float(target_pose_start.head_radius) > 0.0,
            "target presentation exposes native-scaled head volume")
        check(target_fixture_a.target_position(first_live).distance_to(
            target_fixture_b.target_position(first_live)) <= 0.000001,
            "deterministic fixture target positions match")
    tick(target_fixture_a, 120)
    tick(target_fixture_b, 120)
    if first_live >= 0:
        var target_pose_moved: Dictionary = target_fixture_a.target_presentation_snapshot(first_live)
        check(bool(target_pose_moved.body_authoritative),
            "moving enemy exposes authoritative native body presentation")
        check(float(target_pose_moved.attack_commit_phase) < float(target_pose_moved.attack_end_phase),
            "enemy presentation exposes ordered native damage window boundaries")
        check(target_fixture_a.target_position(first_live).distance_to(
            target_fixture_b.target_position(first_live)) <= 0.000001,
            "deterministic enemy trajectories match after neutral ticks")
    target_fixture_a.free()
    target_fixture_b.free()

    var interaction := new_game()
    interaction.start_target_interaction_fixture()
    var interaction_target := -1
    var interaction_distance_squared := INF
    var interaction_player: Vector3 = interaction.player_position()
    var interaction_live_start := 0
    for i in range(interaction.target_count()):
        if interaction.target_alive(i):
            interaction_live_start += 1
            var candidate: Vector3 = interaction.target_position(i)
            var dx := candidate.x - interaction_player.x
            var dz := candidate.z - interaction_player.z
            var distance_squared := dx * dx + dz * dz
            if distance_squared < interaction_distance_squared:
                interaction_distance_squared = distance_squared
                interaction_target = i
    check(interaction_target >= 0, "interaction fixture selects a live parent target")
    var interaction_start_position: Vector3 = interaction.target_position(interaction_target)
    var interaction_start_armor: float = interaction.target_armor(interaction_target)
    for step in range(1050):
        if step == 960:
            interaction.refill_target_interaction_battery()
        var melee := step > 300 and step < 1000 and step % 50 == 0
        var vacuum := step > 960
        var look_x := 120.0 if step == 960 else 0.0
        interaction.submit_input(0, 0, look_x, 0, vacuum, false, false, melee, false, false)
        interaction.tick()
        if interaction.frame() == 600:
            check(interaction.target_position(interaction_target).distance_to(interaction_start_position) > 0.001,
                "parent target pursuit changes target position")
        elif interaction.frame() == 900:
            check(interaction.target_armor(interaction_target) < interaction_start_armor,
                "parent melee damages target armor")
        elif interaction.frame() == 960:
            check(interaction.target_slurpable(interaction_target),
                "parent melee defeats the target shell")
        elif interaction.frame() == 1020:
            check(interaction.target_ingest_progress(interaction_target) > 0.0,
                "parent vacuum advances soul ingestion")
    check(not interaction.target_alive(interaction_target), "parent vacuum captures the defeated target")
    check(live_targets(interaction) == interaction_live_start - 1,
        "capture removes exactly one live target")
    check(interaction.souls() == 1, "capture stores one parent-owned soul")
    interaction.free()

    if not failures.is_empty():
        print("RESULT: FAIL count=", failures.size())
        quit(2)
        return
    print("RESULT: PASS deterministic movement, sprint, jump, look, camera, vacuum, melee action/pose contracts, enemy body/attack contracts, communication signals, native pause/resume, native room/object contracts, target pursuit/damage/defeat/ingestion/capture, stored-soul shooting")
    quit(0)
