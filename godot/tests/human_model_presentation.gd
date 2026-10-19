extends SceneTree

const HumanModelAdapter = preload("res://scripts/human_model_adapter.gd")
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
        adapter.sync_from_native(snapshot, vertices)
        var model := adapter.get_node("SkinnedModel") as MeshInstance3D
        check(model.mesh is ArrayMesh, "Godot builds an ArrayMesh from native-skinned vertices")
        if model.mesh != null:
            check(model.mesh.get_surface_count() == 1, "human model has one native triangle surface")
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
    for step in range(1050):
        if step == 960:
            interaction.refill_target_interaction_battery()
        var melee := step > 300 and step < 1000 and step % 50 == 0
        interaction.submit_input(0, 0, 120.0 if step == 960 else 0.0, 0,
            step > 960, false, false, melee, false, false)
        interaction.tick()
        if interaction.frame() == 1020:
            tether_snapshot = interaction.target_presentation_snapshot(interaction_target)
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
    interaction.free()
    runtime.free()
    if not failures.is_empty():
        print("RESULT: FAIL count=", failures.size())
        quit(2)
        return
    print("RESULT: PASS original native human model/skin and render-only enemy/vacuum presentation")
    quit(0)
