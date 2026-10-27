extends SceneTree

const WARMUP_FRAMES := 60
const SAMPLE_FRAMES := 300

func _initialize() -> void:
    call_deferred("_run")

func _percentile(values: Array[float], percentile: float) -> float:
    var sorted := values.duplicate()
    sorted.sort()
    if sorted.is_empty():
        return 0.0
    var index := mini(int(ceil(percentile * float(sorted.size()))) - 1, sorted.size() - 1)
    return sorted[maxi(index, 0)]

func _summary(values: Array[float]) -> Dictionary:
    var total := 0.0
    for value in values:
        total += value
    return {
        "samples": values.size(),
        "mean_us": total / float(values.size()) if not values.is_empty() else 0.0,
        "median_us": _percentile(values, 0.5),
        "p95_us": _percentile(values, 0.95),
        "max_us": values.max() if not values.is_empty() else 0.0,
    }

func _run() -> void:
    var world: Node3D = load("res://scenes/parent_reference.tscn").instantiate()
    root.add_child(world)
    world.agent_playtest = true
    await process_frame
    world.runtime.start_target_interaction_fixture()
    world.configure_performance_profile("baseline")
    var metrics := {
        "sync_total_us": [] as Array[float],
        "native_tick_us": [] as Array[float],
        "enemies_bridge_adapter_us": [] as Array[float],
        "enemy_bridge_us": [] as Array[float],
        "enemy_adapter_us": [] as Array[float],
    }
    for frame in range(WARMUP_FRAMES + SAMPLE_FRAMES):
        var cycle := frame % 60
        world.agent_step(1, 0.3 if cycle < 30 else -0.3, 0.7, 0.1, 0.02,
            cycle >= 15 and cycle < 40, cycle < 25, false, cycle == 10, false, false)
        if frame < WARMUP_FRAMES:
            continue
        var snapshot: Dictionary = world.performance_profile_snapshot()
        for metric in metrics:
            (metrics[metric] as Array[float]).append(float(snapshot.get(metric, 0.0)))
    var output := {
        "schema": "digital-breakdown.godot-enemy-boundary-benchmark/v1",
        "variant": "skeleton" if world.skeleton_enemy_slice_enabled else "vertices",
        "warmup_frames": WARMUP_FRAMES,
        "sample_frames": SAMPLE_FRAMES,
        "final_frame": world.runtime.frame(),
        "metrics": {},
    }
    for metric in metrics:
        output.metrics[metric] = _summary(metrics[metric])
    print("BENCHMARK_JSON ", JSON.stringify(output))
    quit(0)
