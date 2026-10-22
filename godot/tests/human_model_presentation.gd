extends SceneTree

const HumanModelAdapter = preload("res://scripts/human_model_adapter.gd")
const SoulLatticeAdapter = preload("res://scripts/soul_lattice_adapter.gd")
var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    var runtime = ClassDB.instantiate("ParentGameNode")
    runtime.start_target_fixture()
    var target := -1
    for i in range(runtime.target_count()):
        if runtime.target_alive(i):
            target = i
            break
    check(target >= 0, "fixture provides a native human target")
    if target >= 0:
        var vertices: PackedVector3Array = runtime.target_model_vertices(target)
        check(not vertices.is_empty(), "original human.dbhuman loads through native model contract")
        check(vertices.size() % 3 == 0, "native human model remains a triangle list")
        var adapter := HumanModelAdapter.new()
        adapter.configure(target, runtime.target_model_color(target))
        get_root().add_child(adapter)
        var snapshot: Dictionary = runtime.target_presentation_snapshot(target)
        check(float(snapshot.shell_crack_amount) == 0.0,
            "undamaged native target begins with an intact shell contract")
        adapter.sync_from_native(snapshot, vertices)
        var model := adapter.get_node("SkinnedModel") as MeshInstance3D
        check(model.mesh is ArrayMesh, "Godot builds an ArrayMesh from native-skinned vertices")
        if model.mesh != null:
            check(model.mesh.get_surface_count() == 1, "human model has one native triangle surface")
            var model_arrays: Array = model.mesh.surface_get_arrays(0)
            var normals: PackedVector3Array = model_arrays[Mesh.ARRAY_NORMAL]
            check(normals.size() == vertices.size() and not normals.is_empty(),
                "Godot human mesh preserves one source-equivalent flat normal per native vertex")
            if normals.size() >= 3:
                var expected_normal := (vertices[1] - vertices[0]).cross(vertices[2] - vertices[0]).normalized()
                check(normals[0].dot(expected_normal) > 0.99999 and
                    normals[1].dot(expected_normal) > 0.99999 and
                    normals[2].dot(expected_normal) > 0.99999,
                    "human triangle normals match the native desktop face-normal calculation")
        check(adapter.find_children("*", "CollisionObject3D", true, false).is_empty(),
            "human presentation creates no Godot gameplay collision authority")
        var before: PackedVector3Array = vertices
        for _tick in range(30):
            runtime.submit_input(0, 0, 0, 0, false, false, false, false, false, false)
            runtime.tick()
        var after: PackedVector3Array = runtime.target_model_vertices(target)
        var changed := before.size() == after.size()
        if changed:
            changed = false
            for i in range(before.size()):
                if before[i].distance_to(after[i]) > 0.000001:
                    changed = true
                    break
        check(changed, "native animation and body state deform original model vertices over time")
        adapter.queue_free()

    var interaction = ClassDB.instantiate("ParentGameNode")
    interaction.start_target_interaction_fixture()
    var interaction_target := -1
    var player_position: Vector3 = interaction.player_position()
    var nearest_distance := INF
    for i in range(interaction.target_count()):
        if not interaction.target_alive(i):
            continue
        var candidate: Vector3 = interaction.target_position(i)
        var distance := Vector2(candidate.x - player_position.x, candidate.z - player_position.z).length_squared()
        if distance < nearest_distance:
            nearest_distance = distance
            interaction_target = i
    check(interaction_target >= 0, "interaction fixture has a native target for tether presentation")
    var tether_snapshot: Dictionary = {}
    var initial_shell_vertices: int = interaction.target_model_vertices(interaction_target).size()
    var found_native_cracks := false
    var found_digital_blood := false
    var found_shell_shards := false
    var verified_particle_mesh := false
    var verified_shell_to_cube_handoff := false
    var maximum_capture_pulse := 0.0
    var maximum_phone_light_pulse := 0.0
    var damage_adapter = HumanModelAdapter.new()
    damage_adapter.configure(interaction_target, interaction.target_model_color(interaction_target))
    get_root().add_child(damage_adapter)
    var soul_adapter = SoulLatticeAdapter.new()
    get_root().add_child(soul_adapter)
    var effects_adapter = load("res://scripts/native_particle_presentation_adapter.gd").new()
    get_root().add_child(effects_adapter)
    for step in range(1050):
        if step == 960:
            interaction.refill_target_interaction_battery()
        var melee := step > 300 and step < 1000 and step % 50 == 0
        interaction.submit_input(0, 0, 120.0 if step == 960 else 0.0, 0,
            step > 960, false, false, melee, false, false)
        interaction.tick()
        var phone_state: Dictionary = interaction.phone_presentation_snapshot()
        maximum_capture_pulse = maxf(maximum_capture_pulse, float(phone_state.capture_pulse))
        maximum_phone_light_pulse = maxf(maximum_phone_light_pulse,
            float(phone_state.screen_light_pulse))
        var damaged_target: Dictionary = interaction.target_presentation_snapshot(interaction_target)
        if float(damaged_target.shell_crack_amount) > 0.0:
            found_native_cracks = true
            var damaged_vertices: PackedVector3Array = interaction.target_model_vertices(interaction_target)
            check(damaged_vertices.size() < initial_shell_vertices,
                "native deterministic shell-crack mask removes triangles as original armor is damaged")
            damage_adapter.sync_from_native(damaged_target, damaged_vertices)
            if not bool(damaged_target.slurpable):
                check(damage_adapter.material.albedo_color.is_equal_approx(
                    damaged_target.damage_surface_color.lerp(Color(1.0, 0.28, 0.2),
                        clampf(float(damaged_target.hit_amount), 0.0, 1.0))),
                    "native wound surface color and hit flash drive Godot enemy material")
        if bool(damaged_target.slurpable) and bool(damaged_target.alive):
            var morphing_vertices: PackedVector3Array = interaction.target_model_vertices(interaction_target)
            damage_adapter.sync_from_native(damaged_target, morphing_vertices)
            var soul_morph := float(damaged_target.pose.soul_morph)
            var soul_snapshot: Dictionary = interaction.target_soul_presentation_snapshot(interaction_target)
            soul_adapter.sync_from_native(soul_snapshot, interaction.target_soul_lattice(interaction_target))
            check(float(damaged_target.shell_crack_amount) > 0.0,
                "broken enemy shell remains cracked instead of resetting intact at defeat")
            check(absf(damage_adapter.material.albedo_color.a - (1.0 - soul_morph * soul_morph * (3.0 - 2.0 * soul_morph))) < 0.00001,
                "native soul morph fades the cracked human shell continuously")
            check(absf(damage_adapter.position.y - (damaged_target.position.y +
                float(damaged_target.pose.root_bob) + 0.57 * soul_morph)) < 0.00001,
                "human shell collapses toward the native soul lattice center")
            var lattice_scale: Vector3 = soul_snapshot.scale * float(soul_snapshot.morph_scale)
            check(soul_adapter.visible and soul_adapter.scale.is_equal_approx(lattice_scale),
                "native soul lattice grows from the same collapse pivot")
            if soul_morph > 0.0 and soul_morph < 1.0:
                verified_shell_to_cube_handoff = true
        var native_particles: Array = interaction.particle_presentation_snapshot()
        if not native_particles.is_empty():
            effects_adapter.sync_from_native(native_particles)
            var native_mesh: MultiMeshInstance3D = effects_adapter.get_node("NativeDigitalParticles")
            check(native_mesh.multimesh.visible_instance_count == native_particles.size() and
                native_mesh.multimesh.instance_count >= native_particles.size(),
                "render adapter displays the current native particle count")
            verified_particle_mesh = true
            for particle_value in native_particles:
                var particle: Dictionary = particle_value
                if int(particle.material) == 1:
                    found_digital_blood = true
                    check(particle.color.is_equal_approx(Color(114.0 / 255.0,
                        22.0 / 255.0, 95.0 / 255.0, float(particle.remaining))),
                        "Flesh particle color matches native DigitalBlood material response")
                if int(particle.material) == 2:
                    found_shell_shards = true
        if interaction.frame() == 1020:
            tether_snapshot = interaction.target_presentation_snapshot(interaction_target)
    check(found_native_cracks, "native combat sequence reaches visible shell cracking before defeat")
    check(found_digital_blood, "native combat particles expose digital-blood impacts")
    check(found_shell_shards, "native shell break emits Environment material fragments")
    check(verified_shell_to_cube_handoff,
        "controlled native combat observes an intermediate cracked-shell-to-soul morph")
    check(verified_particle_mesh, "Godot particle adapter consumes native simulation snapshots")
    check(maximum_capture_pulse > 0.0 and maximum_phone_light_pulse >= maximum_capture_pulse,
        "native ingestion/capture activity drives the character phone light pulse")
    check(not tether_snapshot.is_empty() and bool(tether_snapshot.tether_visible),
        "native capture lifecycle produces an authoritative visible vacuum tether")
    if not tether_snapshot.is_empty():
        var tether_adapter := HumanModelAdapter.new()
        tether_adapter.configure(interaction_target, interaction.target_model_color(interaction_target))
        get_root().add_child(tether_adapter)
        await process_frame
        tether_adapter.sync_from_native(tether_snapshot,
            interaction.target_model_vertices(interaction_target))
        var tether := tether_adapter.get_node("NativeVacuumTether") as MeshInstance3D
        var anchor: Vector3 = tether_snapshot.tether_anchor
        var destination: Vector3 = tether_snapshot.tether_destination
        check(tether.visible, "Godot renders the native vacuum tether while capture is active")
        check(tether.global_position.distance_to((anchor + destination) * 0.5) < 0.00001,
            "vacuum tether is centered on native anchor and destination")
        check(tether.global_basis.y.normalized().dot((destination - anchor).normalized()) > 0.99999,
            "vacuum tether points along the native pull direction")
        check(absf(tether.scale.x - float(tether_snapshot.tether_width)) < 0.00001,
            "vacuum tether width is sourced from native state")
        tether_adapter.queue_free()
    effects_adapter.queue_free()
    damage_adapter.queue_free()
    soul_adapter.queue_free()
    interaction.free()
    runtime.free()
    if not failures.is_empty():
        print("RESULT: FAIL count=", failures.size())
        quit(2)
        return
    print("RESULT: PASS original native human model/skin and render-only enemy/vacuum presentation")
    quit(0)
