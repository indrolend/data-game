extends SceneTree

func _initialize() -> void:
    call_deferred("_run")

func _run() -> void:
    var world: Node3D = load("res://scenes/parent_reference.tscn").instantiate()
    root.add_child(world)
    world.agent_playtest = true
    await process_frame
    var runtime = world.runtime
    runtime.start_damage_fixture()
    world._sync()
    var initial_battery: float = runtime.battery()
    var minimum_battery := initial_battery
    var maximum_pulse := float(runtime.phone_presentation_snapshot().screen_light_pulse)
    var maximum_damage_pulse := 0.0
    var attack_hit := false
    var presented_damage_light := false
    for _frame in range(600):
        runtime.submit_input(0, 0, 0, 0, false, false, false, false, false, false)
        runtime.tick()
        minimum_battery = minf(minimum_battery, runtime.battery())
        maximum_pulse = maxf(maximum_pulse,
            float(runtime.phone_presentation_snapshot().screen_light_pulse))
        maximum_damage_pulse = maxf(maximum_damage_pulse,
            float(runtime.phone_presentation_snapshot().damage_pulse))
        attack_hit = attack_hit or bool(runtime.target_presentation_snapshot(0).attack_hit)
        if float(runtime.phone_presentation_snapshot().damage_pulse) > 0.9 and not presented_damage_light:
            world._sync()
            var snapshot: Dictionary = runtime.phone_presentation_snapshot()
            var light: OmniLight3D = world.get_node("PhonePresentationAdapter/PhoneRoot/NativePhoneScreenLight")
            var expected := float(snapshot.screen_light_intensity) * \
                (1.0 + clampf(float(snapshot.screen_light_pulse), 0.0, 1.0) * 0.25)
            presented_damage_light = absf(light.light_energy - expected) < 0.00001
    print("DAMAGE_PROBE initial=%.6f minimum=%.6f attack_hit=%d pulse=%.6f damage_pulse=%.6f" % [
        initial_battery, minimum_battery, int(attack_hit), maximum_pulse, maximum_damage_pulse])
    world.free()
    quit(0 if attack_hit and minimum_battery < initial_battery and
        maximum_damage_pulse > 0.9 and presented_damage_light else 2)
