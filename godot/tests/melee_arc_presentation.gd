extends SceneTree

const MeleeArcAdapter = preload("res://scripts/melee_arc_adapter.gd")

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func material_color(effect: MeshInstance3D) -> Color:
    return (effect.material_override as StandardMaterial3D).albedo_color

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    var adapter := MeleeArcAdapter.new()
    root.add_child(adapter)
    await process_frame
    var active := {
        "visual_timer": 0.24,
        "visual_duration": 0.30,
        "origin": Vector3(1.0, 0.8, 2.0),
        "direction": Vector3(0.0, 0.0, 1.0),
        "impact": Vector3(1.0, 0.9, 3.4),
        "visual_hit": true,
        "locomotion_lunge": false,
    }
    adapter.sync_snapshot(active)

    check(adapter.visible, "active native melee timer shows the presentation")
    check(adapter.swing_arc.mesh.get_surface_count() == 1 and
        adapter.swing_arc.mesh.surface_get_array_len(0) == 24 * 6,
        "swing arc preserves the original 24-segment ribbon geometry")
    check(adapter.directional_streak.mesh.get_surface_count() == 1 and
        adapter.directional_streak.mesh.surface_get_array_len(0) == 12 * 6,
        "directional streak preserves the original 12-segment tapered geometry")
    check(adapter.impact_ring.mesh.get_surface_count() == 1 and
        adapter.impact_ring.mesh.surface_get_array_len(0) == 24 * 6,
        "impact ring preserves the original 24-segment ribbon geometry")
    check(adapter.swing_arc.position.is_equal_approx(Vector3(1.0, 0.8, 2.704)),
        "swing arc follows native origin, direction, and timer progress")
    check(adapter.directional_streak.position.is_equal_approx(Vector3(1.0, 0.846, 2.644)),
        "streak midpoint follows the native impact vector")
    check(adapter.impact_ring.position.is_equal_approx(active.impact),
        "impact ring uses the native impact position")

    var magenta := material_color(adapter.swing_arc)
    var cyan := material_color(adapter.directional_streak)
    var chartreuse := material_color(adapter.impact_ring)
    check(magenta.is_equal_approx(Color(0x98 / 255.0, 0x1e / 255.0, 0x97 / 255.0, 0.528)),
        "swing arc uses native ElectricMagenta and native fade")
    check(cyan.is_equal_approx(Color(0x78 / 255.0, 0xd5 / 255.0, 0xe1 / 255.0, 0.352)),
        "streak uses native ElectricCyan and native fade")
    check(chartreuse.is_equal_approx(Color(0xd4 / 255.0, 0xec / 255.0, 0x8e / 255.0, 0.656)),
        "hit ring uses native AcidChartreuse and hit opacity")

    var lunge := active.duplicate()
    lunge["locomotion_lunge"] = true
    adapter.sync_snapshot(lunge)
    check(not adapter.visible, "native locomotion lunge suppresses melee effects")
    var expired := active.duplicate()
    expired["visual_timer"] = 0.0
    adapter.sync_snapshot(expired)
    check(not adapter.visible, "expired native visual timer hides melee effects")

    adapter.free()
    if not failures.is_empty():
        print("RESULT: FAIL count=", failures.size())
        quit(2)
        return
    print("RESULT: PASS native-timed melee arc, streak, hit ring, colors, fade, and lunge suppression")
    quit(0)
