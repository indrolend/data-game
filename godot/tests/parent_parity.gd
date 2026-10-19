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
    print("PARITY,%.0f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.0f,%.0f,%.0f,%.0f,%.7f,%.7f,%.0f,%.0f,%.7f" % [game.frame(), p.x, p.y, p.z, c.x, c.y, c.z, t.x, t.y, t.z, game.battery(), game.souls(), int(game.first_person()), int(game.grounded()), int(game.vacuum_active()), game.vacuum_power(), game.melee_pose(), game.live_bullets(), live_targets(game), game.camera_fov_degrees()])
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
    if game.frame() == 0:
        phone_menu_snapshot(game)

func phone_menu_snapshot(game: Object) -> void:
    var layout: Dictionary = game.phone_menu_layout_snapshot()
    var content: Dictionary = layout.content
    print("MENULAYOUT,%d,%d,%d,%d,%d,%d,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%s,%s" % [
        layout.logical_width, layout.logical_height, layout.row_count,
        layout.selectable_count, layout.selection, int(layout.active),
        layout.title_px, layout.title_center_y, layout.scroll_offset, layout.max_scroll,
        content.x, content.y, content.w, content.h, layout.title, layout.navigation_hint])
    for index in range(layout.rows.size()):
        var row: Dictionary = layout.rows[index]
        print("MENUROW,%d,%d,%d,%d,%d,%d,%s,%s,%.7f,%.7f,%.7f,%.7f,%d,%d,%d,%d,%d,%d,%.7f" % [
            index, row.kind, row.action, row.selectable_index, row.binding_action,
            row.horizontal, row.label, row.value, row.label_x, row.value_right_x,
            row.baseline_y, row.font_px, int(row.selectable), int(row.visible),
            int(row.peek), int(row.fixed_footer), int(row.selected), row.emphasis,
            row.amount])

func upgrade_snapshot(game: Object, stage: String) -> void:
    var state: Dictionary = game.upgrade_presentation_snapshot()
    print("UPGRADE,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d" % [
        stage, int(state.active), int(game.paused()), int(state.room_index), int(state.selection),
        int(state.tokens), int(state.temporary_levels[0]), int(state.temporary_levels[1]),
        int(state.temporary_levels[2]), int(state.permanent_levels[0]),
        int(state.permanent_levels[1]), int(state.permanent_levels[2])])

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
    target_pose_snapshot(game, target)

func target_pose_snapshot(game: Object, target: int) -> void:
    var p: Dictionary = game.target_presentation_snapshot(target)
    var position: Vector3 = p.position
    var velocity: Vector3 = p.velocity
    var attack_direction: Vector3 = p.attack_direction
    var pose: Dictionary = p.pose
    var head_center: Vector3 = p.head_center
    print("TARGETPOSE,%d,%d,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%d,%d,%.7f,%.7f,%.7f,%d,%.7f,%.7f,%.7f,%.7f,%d,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f" % [
        game.frame(), target, position.x, position.y, position.z,
        velocity.x, velocity.y, velocity.z, p.visual_yaw, p.locomotion_amount,
        p.locomotion_phase, p.attack_timer, p.attack_phase, p.attack_commit_phase,
        p.attack_end_phase, p.attack_variant, int(p.attack_hit), attack_direction.x,
        attack_direction.y, attack_direction.z, p.grabbed_player_id, p.body_pitch,
        p.body_roll, p.left_foot_contact, p.right_foot_contact, int(p.body_authoritative),
        pose.root_bob, pose.torso_pitch, pose.torso_roll, pose.left_arm_swing,
        pose.right_arm_swing, pose.left_leg_swing, pose.right_leg_swing,
        pose.hit_lean, pose.collapse, head_center.x, head_center.y, head_center.z,
        p.head_radius])
    var tether_anchor: Vector3 = p.tether_anchor
    var tether_destination: Vector3 = p.tether_destination
    print("TETHER,%d,%d,%d,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f" % [
        game.frame(), target, int(p.tether_visible), tether_anchor.x, tether_anchor.y,
        tether_anchor.z, tether_destination.x, tether_destination.y,
        tether_destination.z, p.tether_width])
    var pose_hit_lean := float(pose.hit_lean)
    var pose_collapse := float(pose.collapse)
    print("HIT,%d,%d,%.7f,%.7f,%.7f,%.7f" % [game.frame(), target,
        p.hit_amount, p.hit_direction_local, pose_hit_lean, pose_collapse])

func action_snapshot(game: Object) -> void:
    var p: Dictionary = game.player_action_snapshot()
    var origin: Vector3 = p.origin
    var direction: Vector3 = p.direction
    var impact: Vector3 = p.impact
    var position: Vector3 = p.player_position
    var velocity: Vector3 = p.player_velocity
    print("MELEE,%d,%d,%d,%d,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%d,%d,%d,%d,%d,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f" % [
        game.frame(), p.sequence, p.combo_index, p.variant, p.visual_timer,
        p.visual_duration, p.phase, p.range, p.hit_radius, p.damage,
        origin.x, origin.y, origin.z, direction.x, direction.y, direction.z,
        impact.x, impact.y, impact.z, int(p.visual_hit), p.hit_mask,
        int(p.locomotion_lunge), int(p.air_lunge_pending),
        int(p.air_lunge_landing_pending), p.air_lunge_timer, p.air_lunge_rotation,
        p.landing_recovery, p.landing_pose_pitch, position.x, position.y, position.z,
        velocity.x, velocity.y, velocity.z])

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
    var melee = ClassDB.instantiate("ParentGameNode")
    action_snapshot(melee)
    melee.submit_input(0, 0, 0, 0, false, false, false, true, false, false)
    melee.tick()
    action_snapshot(melee)
    for _tick in range(5):
        melee.submit_input(0, 0, 0, 0, false, false, false, false, false, false)
        melee.tick()
    action_snapshot(melee)
    for _tick in range(9):
        melee.submit_input(0, 0, 0, 0, false, false, false, false, false, false)
        melee.tick()
    action_snapshot(melee)
    melee.free()
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
    var communication = ClassDB.instantiate("ParentGameNode")
    communication.set_comm_signal(3)
    communication.tick()
    print("SIGNAL,%d,%d,%.7f" % [communication.frame(), communication.comm_signal(), communication.comm_signal_timer()])
    for _tick in range(60):
        communication.tick()
    print("SIGNAL,%d,%d,%.7f" % [communication.frame(), communication.comm_signal(), communication.comm_signal_timer()])
    for _tick in range(90):
        communication.tick()
    print("SIGNAL,%d,%d,%.7f" % [communication.frame(), communication.comm_signal(), communication.comm_signal_timer()])
    communication.free()
    var pause = ClassDB.instantiate("ParentGameNode")
    for _tick in range(30):
        pause.submit_input(0, 1, 0, 0, false, false, false, false, false, false)
        pause.tick()
    var pause_position: Vector3 = pause.player_position()
    print("PAUSE,%d,%d,%.7f,%.7f,%.7f,%.7f" % [pause.frame(), int(pause.paused()), pause.simulation_time(), pause_position.x, pause_position.y, pause_position.z])
    pause.set_paused(true)
    for _tick in range(60):
        pause.submit_input(0, 1, 0, 0, false, false, false, false, false, false)
        pause.tick()
    pause_position = pause.player_position()
    print("PAUSE,%d,%d,%.7f,%.7f,%.7f,%.7f" % [pause.frame(), int(pause.paused()), pause.simulation_time(), pause_position.x, pause_position.y, pause_position.z])
    pause.set_paused(false)
    for _tick in range(30):
        pause.submit_input(0, 1, 0, 0, false, false, false, false, false, false)
        pause.tick()
    pause_position = pause.player_position()
    print("PAUSE,%d,%d,%.7f,%.7f,%.7f,%.7f" % [pause.frame(), int(pause.paused()), pause.simulation_time(), pause_position.x, pause_position.y, pause_position.z])
    pause.free()
    var room = ClassDB.instantiate("ParentGameNode")
    var room_snapshot: Dictionary = room.room_presentation_snapshot(0)
    var room_structures: Array = room_snapshot.structures
    var room_objects: Array = room_snapshot.objects
    print("ROOM,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.7f,%d,%d" % [
        room_snapshot.room_seed, room_snapshot.room_index, room_snapshot.tile_index,
        room_snapshot.current_tile_index, room_snapshot.setting, room_snapshot.form,
        room_snapshot.scale, room_snapshot.condition, room_snapshot.traversal_intent,
        int(room_snapshot.room_clear), int(room_snapshot.advancing), int(room_snapshot.grass),
        int(room_snapshot.sidewalks), room_snapshot.grass_amount,
        room_structures.size(), room_objects.size()])
    for structure in room_structures:
        var structure_center: Vector3 = structure.center
        var structure_size: Vector3 = structure.size
        var material: Dictionary = structure.material
        var base_color: Color = material.base_color
        var specular: Color = material.specular
        var emission: Color = material.emission
        print("ROOMSTRUCT,%s,%d,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%d,%.7f,%d,%d,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f" % [
            structure.id, structure.kind, structure_center.x, structure_center.y,
            structure_center.z, structure_size.x, structure_size.y, structure_size.z,
            base_color.r, base_color.g, base_color.b, material.shading, material.opacity,
            int(material.fog), material.texture, material.texture_world_scale,
            specular.r, specular.g, specular.b, material.shininess,
            emission.r, emission.g, emission.b])
    for object in room_objects:
        var center: Vector3 = object.center
        var size: Vector3 = object.size
        print("ROOMOBJ,%s,%d,%d,%d,%d,%d,%d,%d,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%d,%d" % [
            object.id, object.tile_index, object.prop_index, object.part_index,
            object.primitive, object.role, object.geometry, object.part,
            center.x, center.y, center.z, size.x, size.y, size.z, object.yaw,
            object.surface, int(object.physical)])
        if int(object.geometry) == 1:
            var vertices: PackedVector3Array = object.visual_mesh_vertices
            var normals: PackedVector3Array = object.visual_mesh_normals
            for vertex in range(vertices.size()):
                var point := vertices[vertex]
                var normal := normals[vertex]
                print("ROCKVERTEX,%s,%d,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f" % [
                    object.id, vertex, point.x, point.y, point.z,
                    normal.x, normal.y, normal.z])
    room.free()
    var city_room = ClassDB.instantiate("ParentGameNode")
    city_room.start_city_room_fixture()
    var city_snapshot: Dictionary = city_room.room_presentation_snapshot(-1)
    for object in city_snapshot.objects as Array:
        if int(object.primitive) != 0:
            continue
        var center: Vector3 = object.center
        var size: Vector3 = object.size
        var color: Color = object.visual_color
        print("HOUSEPART,%s,%d,%d,%d,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%d,%.7f,%.7f,%.7f,%s" % [
            object.id, object.prop_index, object.part_index, object.surface,
            center.x, center.y, center.z, size.x, size.y, size.z, object.yaw,
            int(object.physical), color.r, color.g, color.b, object.visual_texture])
    city_room.free()
    var sterile_room = ClassDB.instantiate("ParentGameNode")
    sterile_room.start_sterile_room_fixture()
    var sterile_snapshot: Dictionary = sterile_room.room_presentation_snapshot(-1)
    for object in sterile_snapshot.objects as Array:
        if int(object.primitive) != 3:
            continue
        var marker_center: Vector3 = object.center
        var marker_size: Vector3 = object.size
        var marker_color: Color = object.visual_color
        print("MARKERPART,%s,%d,%d,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%d,%.7f,%.7f,%.7f,%s" % [
            object.id, object.part_index, object.surface,
            marker_center.x, marker_center.y, marker_center.z,
            marker_size.x, marker_size.y, marker_size.z, int(object.physical),
            marker_color.r, marker_color.g, marker_color.b, object.visual_texture])
    sterile_room.free()
    var ruin_room = ClassDB.instantiate("ParentGameNode")
    ruin_room.start_ruin_room_fixture()
    var ruin_snapshot: Dictionary = ruin_room.room_presentation_snapshot(-1)
    for object in ruin_snapshot.objects as Array:
        if int(object.primitive) != 4:
            continue
        var ruin_center: Vector3 = object.center
        var ruin_size: Vector3 = object.size
        var ruin_color: Color = object.visual_color
        var scar_center: Vector3 = object.visual_scar_center
        var scar_size: Vector3 = object.visual_scar_size
        print("RUINPART,%s,%d,%d,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%d,%.7f,%.7f,%.7f,%s,%.7f,%d,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f" % [
            object.id, object.part_index, object.surface,
            ruin_center.x, ruin_center.y, ruin_center.z,
            ruin_size.x, ruin_size.y, ruin_size.z, int(object.physical),
            ruin_color.r, ruin_color.g, ruin_color.b, object.visual_texture,
            object.visual_texture_phase, int(object.visual_has_scar),
            scar_center.x, scar_center.y, scar_center.z,
            scar_size.x, scar_size.y, scar_size.z])
    ruin_room.free()
    var upgrade_game = ClassDB.instantiate("ParentGameNode")
    upgrade_game.start_upgrade_fixture()
    upgrade_snapshot(upgrade_game, "initial")
    upgrade_game.move_upgrade_selection(1, 0)
    upgrade_game.move_upgrade_selection(0, 1)
    upgrade_snapshot(upgrade_game, "grid")
    upgrade_game.activate_upgrade_selection()
    upgrade_snapshot(upgrade_game, "purchase")
    upgrade_game.set_upgrade_selection(1)
    upgrade_game.activate_upgrade_selection()
    upgrade_snapshot(upgrade_game, "run")
    upgrade_game.free()
    var crossing = ClassDB.instantiate("ParentGameNode")
    crossing.start_room_transition_fixture()
    for _frame in range(120):
        if bool(crossing.upgrade_presentation_snapshot().active):
            break
        crossing.submit_input(0, 1, 0, 0, false, false, false, false, false, false)
        crossing.tick()
    var crossed: Dictionary = crossing.upgrade_presentation_snapshot()
    var crossed_position: Vector3 = crossing.player_position()
    print("ROOMCROSS,%d,%d,%.7f,%.7f,%.7f,%d,%d,%.7f,%d" % [
        crossing.frame(), crossed.room_index, crossed_position.x, crossed_position.y,
        crossed_position.z, int(crossed.active), int(crossing.paused()), crossing.battery(), crossing.souls()])
    crossing.free()
    quit(0)
