extends SceneTree

func live_targets(game: Object) -> int:
    var count := 0
    for i in range(game.target_count()):
        if game.target_alive(i):
            count += 1
    return count

func snapshot(game: Object) -> void:
    var p: Vector3 = game.player_position()
    var c: Vector3 = game.camera_position()
    var t: Vector3 = game.camera_target()
    print("PARITY,%.0f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.0f,%.0f,%.0f,%.0f,%.7f,%.7f,%.0f,%.0f" % [game.frame(), p.x, p.y, p.z, c.x, c.y, c.z, t.x, t.y, t.z, game.battery(), game.souls(), int(game.first_person()), int(game.grounded()), int(game.vacuum_active()), game.vacuum_power(), game.melee_pose(), game.live_bullets(), live_targets(game)])
    phone_snapshot(game)

func phone_snapshot(game: Object) -> void:
    var p: Dictionary = game.phone_presentation_snapshot()
    var position: Vector3 = p.position
    var orientation: Quaternion = p.orientation
    var body_size: Vector3 = p.body_size
    var body_scale: Vector3 = p.body_scale
    var screen_offset: Vector3 = p.screen_center_offset
    var screen_size: Vector3 = p.screen_size
    var tint: Color = p.screen_tint
    print("PHONE,%d,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%d,%d,%d,%d,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%d,%.7f,%d,%d,%d,%.7f,%.7f" % [
        game.frame(), position.x, position.y, position.z,
        orientation.x, orientation.y, orientation.z, orientation.w,
        body_size.x, body_size.y, body_size.z,
        body_scale.x, body_scale.y, body_scale.z,
        screen_offset.x, screen_offset.y, screen_offset.z,
        screen_size.x, screen_size.y, screen_size.z,
        int(p.screen_mode), int(p.body_visible), int(p.screen_visible), int(p.first_person),
        p.screen_brightness, p.content_opacity, tint.r, tint.g, tint.b,
        p.screen_glow, p.emission_strength, int(p.interactive), p.battery,
        p.souls, int(p.low_battery), int(p.vacuum_active), p.vacuum_power, p.capture_pulse])

func target_fixture(game: Object) -> void:
    game.start_target_fixture()
    var count := 0
    var first := -1
    for i in range(game.target_count()):
        if game.target_alive(i):
            count += 1
            if first < 0:
                first = i
    var p: Vector3 = game.target_position(first) if first >= 0 else Vector3.ZERO
    print("TARGET,%d,%d,%d,%.7f,%.7f,%.7f" % [game.frame(), count, first, p.x, p.y, p.z])
    for _tick in range(120):
        game.submit_input(0, 0, 0, 0, false, false, false, false, false, false)
        game.tick()
    count = 0
    for i in range(game.target_count()):
        if game.target_alive(i):
            count += 1
    p = game.target_position(first) if first >= 0 else Vector3.ZERO
    print("TARGET,%d,%d,%d,%.7f,%.7f,%.7f" % [game.frame(), count, first, p.x, p.y, p.z])

func nearest_live_target(game: Object) -> int:
    var nearest := -1
    var best_distance_squared := INF
    var player: Vector3 = game.player_position()
    for i in range(game.target_count()):
        if game.target_alive(i):
            var target: Vector3 = game.target_position(i)
            var dx := target.x - player.x
            var dz := target.z - player.z
            var distance_squared := dx * dx + dz * dz
            if distance_squared < best_distance_squared:
                best_distance_squared = distance_squared
                nearest = i
    return nearest

func interaction_snapshot(game: Object, target: int) -> void:
    var position: Vector3 = game.target_position(target)
    print("INTERACT,%d,%d,%.7f,%.7f,%.7f,%.7f,%d,%.7f,%d,%d,%d,%.7f" % [
        game.frame(), target, position.x, position.y, position.z, game.target_armor(target),
        int(game.target_slurpable(target)), game.target_ingest_progress(target),
        int(game.target_alive(target)), live_targets(game), game.souls(), game.battery()])

func target_interaction_fixture(game: Object) -> void:
    game.start_target_interaction_fixture()
    var target := nearest_live_target(game)
    interaction_snapshot(game, target)
    for step in range(1050):
        if step == 960:
            game.refill_target_interaction_battery()
        var melee := step > 300 and step < 1000 and step % 50 == 0
        var vacuum := step > 960
        var look_x := 120.0 if step == 960 else 0.0
        game.submit_input(0, 0, look_x, 0, vacuum, false, false, melee, false, false)
        game.tick()
        if game.frame() in [600, 900, 960, 1020, 1050]:
            interaction_snapshot(game, target)

func _initialize() -> void:
    var game = ClassDB.instantiate("ParentGameNode")
    if game == null:
        push_error("FAIL: ParentGameNode could not be instantiated")
        quit(1)
        return
    snapshot(game)
    for step in range(240):
        var forward := step < 120
        var sprint := step >= 60 and step < 90
        var jump := step == 90
        var camera := step == 60
        var vacuum := step >= 120 and step < 150
        var melee := step == 150
        var shoot := step == 180
        var look_x := 1.5 if step >= 30 and step < 45 else 0.0
        var look_y := -0.5 if step >= 30 and step < 45 else 0.0
        game.submit_input(0.0, 1.0 if forward else 0.0, look_x, look_y, vacuum, sprint, jump, melee, shoot, camera)
        game.tick()
        if (step + 1) % 30 == 0:
            snapshot(game)
    game.free()
    var rally = ClassDB.instantiate("ParentGameNode")
    rally.start_rally_fixture()
    print("ACTION,fixture,%d,%d,%d" % [rally.frame(), rally.souls(), rally.live_bullets()])
    rally.submit_input(0, 0, 0, 0, false, false, false, false, true, false)
    rally.tick()
    print("ACTION,shot,%d,%d,%d" % [rally.frame(), rally.souls(), rally.live_bullets()])
    for _tick in range(8):
        rally.submit_input(0, 0, 0, 0, false, false, false, false, false, false)
        rally.tick()
    print("ACTION,projectile,%d,%d,%d" % [rally.frame(), rally.souls(), rally.live_bullets()])
    rally.free()
    var targets = ClassDB.instantiate("ParentGameNode")
    target_fixture(targets)
    targets.free()
    var interaction = ClassDB.instantiate("ParentGameNode")
    target_interaction_fixture(interaction)
    interaction.free()
    quit(0)
