extends Node3D
# Godot owns presentation. The parent C++ Game owns simulation and collision.
# Temporary primitive meshes deliberately avoid claiming original visual parity.
const PhonePresentationAdapter = preload("res://scripts/phone_presentation_adapter.gd")
const RoomPresentationAdapter = preload("res://scripts/room_presentation_adapter.gd")
const HumanModelAdapter = preload("res://scripts/human_model_adapter.gd")
const SkeletonHumanModelAdapter = preload("res://scripts/skeleton_human_model_adapter.gd")
const PackedHumanModelAdapter = preload("res://scripts/packed_human_model_adapter.gd")
const NativeParticlePresentationAdapter = preload("res://scripts/native_particle_presentation_adapter.gd")
const MeleeArcAdapter = preload("res://scripts/melee_arc_adapter.gd")
const SoulLatticeAdapter = preload("res://scripts/soul_lattice_adapter.gd")
const DeveloperConsole = preload("res://scripts/developer_console.gd")
const HudPresentationAdapter = preload("res://scripts/hud_presentation_adapter.gd")
const DoorTransitionPresentation = preload("res://scripts/door_transition_presentation.gd")
const MusicPresentationAdapter = preload("res://scripts/music_presentation_adapter.gd")
const SfxPresentationAdapter = preload("res://scripts/sfx_presentation_adapter.gd")
const SkyDomeShader = preload("res://shaders/open_sky_dome.gdshader")
const MoonSurfaceShader = preload("res://shaders/moon_surface.gdshader")
var runtime: Node
var developer_console: Control
var hud_adapter: Control
var door_transition_adapter: CanvasLayer
var music_adapter: Node
var sfx_adapter: Node
var phone_adapter: Node
var particle_adapter: Node
var melee_arc_adapter: Node3D
var room_adapter: Node
var camera: Camera3D
var world_environment: WorldEnvironment
var scene_environment: Environment
var sky_resource: Sky
var sky_material: ProceduralSkyMaterial
var sky_dome: MeshInstance3D
var sky_dome_material: ShaderMaterial
var moon: MeshInstance3D
var moon_material: ShaderMaterial
var moon_albedo: Texture2D
var moon_elevation: Texture2D
var _sky_update_bucket := -1
var _sky_update_setting := -1
var sun_light: DirectionalLight3D
var fill_light: DirectionalLight3D
var drama_spotlight: SpotLight3D
var native_point_lights: Array[OmniLight3D] = []
var _grass_snapshot_key := ""
var targets: Array[Node3D] = []
var souls: Array[Node3D] = []
var projectile_nodes := {}
var camera_obstruction_fade_enabled := true
var camera_trauma_enabled := false
var _camera_base_transform := Transform3D.IDENTITY
var _camera_base_target := Vector3.ZERO
var _camera_base_fov := 0.0
var _camera_base_valid := false
var _camera_trauma_strength := 0.0
var _camera_trauma_phase := 0
var performance_profile_enabled := false
var performance_profile_variant := "baseline"
var performance_profile_last := {}
var skeleton_enemy_slice_enabled := false
var skeleton_enemy_slice_index := 0
var packed_enemy_slice_enabled := false
var packed_enemy_slice_index := 0
var packed_enemies_enabled := false
var pending_look := Vector2.ZERO
var pending_jump := false
var pending_melee := false
var pending_shoot := false
var pending_camera_toggle := false
var pending_comm_signal := 0
var agent_playtest := false
const WORLD_AND_PHONE_RECEIVER_LAYERS := 3
const CAMERA_TRAUMA_MAX_TRANSLATION := 0.12
const CAMERA_TRAUMA_MAX_ROTATION_RADIANS := deg_to_rad(1.5)
const CAMERA_TRAUMA_DECAY_PER_SYNC := 0.08
func _ready() -> void:
    agent_playtest = "--agent-playtest" in OS.get_cmdline_user_args()
    camera_obstruction_fade_enabled = OS.get_environment("DB_DISABLE_CAMERA_OBSTRUCTION_FADE") != "1"
    # GGG-CAM-001 is an opt-in presentation experiment. It never mutates the
    # ParentGameNode or any value used to submit authoritative input.
    camera_trauma_enabled = OS.get_environment("DB_ENABLE_CAMERA_TRAUMA") == "1"
    skeleton_enemy_slice_enabled = OS.get_environment("DB_ENABLE_SKELETON_ENEMY_SLICE") == "1"
    skeleton_enemy_slice_index = clampi(OS.get_environment("DB_SKELETON_ENEMY_INDEX").to_int(),
        0, 31)
    packed_enemy_slice_enabled = OS.get_environment("DB_ENABLE_PACKED_ENEMY_SLICE") == "1"
    packed_enemy_slice_index = clampi(OS.get_environment("DB_PACKED_ENEMY_INDEX").to_int(), 0, 31)
    packed_enemies_enabled = OS.get_environment("DB_DISABLE_PACKED_ENEMIES") != "1"
    runtime = ClassDB.instantiate("ParentGameNode")
    add_child(runtime)
    world_environment = WorldEnvironment.new()
    scene_environment = Environment.new()
    scene_environment.background_mode = Environment.BG_COLOR
    scene_environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
    scene_environment.ambient_light_energy = 1.0
    scene_environment.fog_enabled = true
    sky_material = ProceduralSkyMaterial.new()
    sky_material.use_debanding = true
    sky_material.sky_curve = 0.24
    sky_material.ground_curve = 0.18
    sky_resource = Sky.new()
    sky_resource.sky_material = sky_material
    sky_resource.process_mode = Sky.PROCESS_MODE_INCREMENTAL
    sky_resource.radiance_size = Sky.RADIANCE_SIZE_64
    world_environment.environment = scene_environment
    add_child(world_environment)
    get_viewport().world_3d.environment = scene_environment
    sky_dome = MeshInstance3D.new()
    sky_dome.name = "NativeOpenSkyPresentation"
    var sky_sphere := SphereMesh.new()
    sky_sphere.radius = 400.0
    sky_sphere.height = 800.0
    sky_sphere.radial_segments = 32
    sky_sphere.rings = 16
    sky_dome.mesh = sky_sphere
    sky_dome_material = ShaderMaterial.new()
    sky_dome_material.shader = SkyDomeShader
    sky_dome_material.render_priority = -128
    sky_dome.material_override = sky_dome_material
    add_child(sky_dome)
    moon = MeshInstance3D.new()
    moon.name = "NASA_LRO_Moon_Presentation"
    var moon_sphere := SphereMesh.new()
    moon_sphere.radius = 1.67
    moon_sphere.height = 3.34
    moon_sphere.radial_segments = 64
    moon_sphere.rings = 32
    moon.mesh = moon_sphere
    moon_albedo = load("res://models/moon_lroc_color_2k.jpg")
    moon_elevation = load("res://models/moon_lola_elevation_1k.jpg")
    moon_material = ShaderMaterial.new()
    moon_material.shader = MoonSurfaceShader
    moon_material.set_shader_parameter("moon_albedo", moon_albedo)
    moon_material.set_shader_parameter("moon_elevation", moon_elevation)
    moon.material_override = moon_material
    add_child(moon)
    sun_light = DirectionalLight3D.new()
    sun_light.name = "NativeSun"
    sun_light.light_cull_mask = WORLD_AND_PHONE_RECEIVER_LAYERS
    add_child(sun_light)
    fill_light = DirectionalLight3D.new()
    fill_light.name = "NativeSkyFill"
    fill_light.light_cull_mask = WORLD_AND_PHONE_RECEIVER_LAYERS
    add_child(fill_light)
    # Render-only theatrical follow spot: it consumes native day/event state and
    # never feeds back into the authoritative simulation.
    drama_spotlight = SpotLight3D.new()
    drama_spotlight.name = "GodotDramaSpotlight"
    drama_spotlight.light_cull_mask = WORLD_AND_PHONE_RECEIVER_LAYERS
    drama_spotlight.light_color = Color(1.0, 0.91, 0.72)
    drama_spotlight.light_energy = 3.2
    drama_spotlight.spot_range = 18.0
    drama_spotlight.spot_angle = 24.0
    drama_spotlight.shadow_enabled = true
    drama_spotlight.visible = false
    add_child(drama_spotlight)
    for light_index in range(6):
        var point_light := OmniLight3D.new()
        point_light.name = "NativeResolvedLight_%d" % light_index
        point_light.light_cull_mask = WORLD_AND_PHONE_RECEIVER_LAYERS
        add_child(point_light)
        native_point_lights.append(point_light)
    room_adapter = RoomPresentationAdapter.new()
    room_adapter.name = "RoomPresentationAdapter"
    add_child(room_adapter)
    room_adapter.call("set_native_field_grass_pixels", runtime.field_grass_texture_pixels())
    room_adapter.call("set_native_city_asphalt_pixels", runtime.city_asphalt_texture_pixels())
    phone_adapter = PhonePresentationAdapter.new()
    phone_adapter.name = "PhonePresentationAdapter"
    add_child(phone_adapter)
    music_adapter = MusicPresentationAdapter.new()
    music_adapter.name = "NativeMusicPresentation"
    add_child(music_adapter)
    sfx_adapter = SfxPresentationAdapter.new()
    sfx_adapter.name = "NativeSfxPresentation"
    add_child(sfx_adapter)
    particle_adapter = NativeParticlePresentationAdapter.new()
    particle_adapter.name = "NativeParticlePresentationAdapter"
    add_child(particle_adapter)
    melee_arc_adapter = MeleeArcAdapter.new()
    melee_arc_adapter.name = "NativeMeleeArcPresentation"
    add_child(melee_arc_adapter)
    for i in range(runtime.target_count()):
        var enemy: Node3D
        if _uses_packed_enemy(i):
            enemy = PackedHumanModelAdapter.new()
            enemy.call("configure", i, runtime.target_model_color(i))
        elif skeleton_enemy_slice_enabled and i == skeleton_enemy_slice_index:
            enemy = SkeletonHumanModelAdapter.new()
            enemy.call("configure_from_native", i, runtime.target_model_color(i),
                runtime.human_model_rig())
        else:
            enemy = HumanModelAdapter.new()
            enemy.call("configure", i, runtime.target_model_color(i))
        add_child(enemy)
        targets.append(enemy)
        var soul := SoulLatticeAdapter.new()
        soul.name = "NativeSoul_%d" % i
        add_child(soul)
        souls.append(soul)
    camera = Camera3D.new()
    camera.current = true
    camera.keep_aspect = Camera3D.KEEP_HEIGHT
    camera.environment = scene_environment
    add_child(camera)
    door_transition_adapter = DoorTransitionPresentation.new()
    door_transition_adapter.name = "DoorTransitionPresentation"
    add_child(door_transition_adapter)
    var hud := CanvasLayer.new()
    hud.layer = 10
    add_child(hud)
    hud_adapter = HudPresentationAdapter.new()
    hud_adapter.name = "NativeHudPresentation"
    hud.add_child(hud_adapter)
    developer_console = DeveloperConsole.new()
    developer_console.configure(runtime)
    hud.add_child(developer_console)
    _sync_mouse_mode()
    _sync()
func _input(event: InputEvent) -> void:
    if event is InputEventKey and event.pressed and not event.echo:
        if event.keycode == KEY_QUOTELEFT and not developer_console.visible:
            pending_look = Vector2.ZERO
            pending_jump = false
            pending_melee = false
            pending_shoot = false
            pending_camera_toggle = false
            pending_comm_signal = 0
            developer_console.open_console()
            get_viewport().set_input_as_handled()

func _unhandled_input(event: InputEvent) -> void:
    if developer_console.visible:
        return
    if _restart_menu_active():
        if event is InputEventKey and event.pressed and not event.echo:
            match event.keycode:
                KEY_UP, KEY_W:
                    runtime.menu_move_selection(-1)
                KEY_DOWN, KEY_S:
                    runtime.menu_move_selection(1)
                KEY_ENTER, KEY_KP_ENTER, KEY_SPACE:
                    runtime.menu_confirm()
                KEY_ESCAPE, KEY_BACKSPACE:
                    runtime.menu_back()
                _:
                    return
            _sync_mouse_mode()
            _sync()
            get_viewport().set_input_as_handled()
        elif event is InputEventMouseMotion:
            var viewport_size := get_viewport().get_visible_rect().size
            var item := int(runtime.restart_menu_item_at(event.position.x, event.position.y,
                viewport_size.x, viewport_size.y))
            if item >= 0:
                runtime.set_menu_selection(item)
                _sync()
            get_viewport().set_input_as_handled()
        elif event is InputEventMouseButton and event.pressed:
            if event.button_index == MOUSE_BUTTON_LEFT:
                var viewport_size := get_viewport().get_visible_rect().size
                var item := int(runtime.restart_menu_item_at(event.position.x, event.position.y,
                    viewport_size.x, viewport_size.y))
                if item >= 0:
                    runtime.set_menu_selection(item)
                    runtime.menu_confirm()
            elif event.button_index == MOUSE_BUTTON_RIGHT:
                runtime.menu_back()
            _sync_mouse_mode()
            _sync()
            get_viewport().set_input_as_handled()
        return
    if _upgrade_active():
        if event is InputEventKey and event.pressed and not event.echo:
            match event.keycode:
                KEY_LEFT, KEY_A:
                    runtime.move_upgrade_selection(-1, 0)
                KEY_RIGHT, KEY_D:
                    runtime.move_upgrade_selection(1, 0)
                KEY_UP, KEY_W:
                    runtime.move_upgrade_selection(0, -1)
                KEY_DOWN, KEY_S:
                    runtime.move_upgrade_selection(0, 1)
                KEY_ENTER, KEY_KP_ENTER, KEY_SPACE, KEY_F:
                    runtime.activate_upgrade_selection()
                KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6:
                    runtime.set_upgrade_selection(int(event.keycode - KEY_1))
                    runtime.activate_upgrade_selection()
                _:
                    return
            Input.mouse_mode = Input.MOUSE_MODE_VISIBLE if _upgrade_active() else Input.MOUSE_MODE_CAPTURED
            _sync()
            get_viewport().set_input_as_handled()
        return
    if _phone_menu_active():
        if event is InputEventKey and event.pressed and not event.echo:
            match event.keycode:
                KEY_UP, KEY_W:
                    runtime.menu_move_selection(-1)
                KEY_DOWN, KEY_S:
                    runtime.menu_move_selection(1)
                KEY_LEFT, KEY_A:
                    runtime.menu_adjust(-1)
                KEY_RIGHT, KEY_D:
                    runtime.menu_adjust(1)
                KEY_ENTER, KEY_KP_ENTER, KEY_SPACE:
                    runtime.menu_confirm()
                KEY_ESCAPE, KEY_BACKSPACE:
                    runtime.menu_back()
                _:
                    return
            _sync_mouse_mode()
            _sync()
            get_viewport().set_input_as_handled()
        elif event is InputEventMouseMotion:
            var logical = phone_adapter.call("viewport_to_phone_logical", event.position, camera, runtime.phone_menu_layout_snapshot())
            if logical != null:
                var item := int(runtime.menu_item_at(logical.x, logical.y))
                if item >= 0:
                    runtime.set_menu_selection(item)
                    _sync()
            get_viewport().set_input_as_handled()
        elif event is InputEventMouseButton and event.pressed:
            if event.button_index == MOUSE_BUTTON_LEFT:
                var logical = phone_adapter.call("viewport_to_phone_logical", event.position, camera, runtime.phone_menu_layout_snapshot())
                if logical != null:
                    var item := int(runtime.menu_item_at(logical.x, logical.y))
                    if item >= 0:
                        runtime.set_menu_selection(item)
                        runtime.menu_confirm()
            elif event.button_index == MOUSE_BUTTON_RIGHT:
                runtime.menu_back()
            _sync_mouse_mode()
            _sync()
            get_viewport().set_input_as_handled()
        return
    if event is InputEventMouseMotion and _gameplay_pointer_active():
        pending_look += event.relative
    elif event is InputEventKey and event.pressed and not event.echo:
        match event.keycode:
            KEY_TAB:
                Input.mouse_mode = Input.MOUSE_MODE_VISIBLE if Input.mouse_mode == Input.MOUSE_MODE_CAPTURED else Input.MOUSE_MODE_CAPTURED
                runtime.set_paused(Input.mouse_mode != Input.MOUSE_MODE_CAPTURED)
            KEY_ESCAPE:
                runtime.set_paused(true)
                Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
            KEY_SPACE:
                pending_jump = true
            KEY_F:
                pending_melee = true
            KEY_Q:
                pending_shoot = true
            KEY_C:
                pending_camera_toggle = true
            KEY_1, KEY_2, KEY_3, KEY_4:
                pending_comm_signal = int(event.keycode - KEY_1) + 1
    elif event is InputEventMouseButton and event.pressed:
        if _gameplay_pointer_active() and event.button_index == MOUSE_BUTTON_RIGHT:
            pending_melee = true
func _physics_process(_delta: float) -> void:
    if agent_playtest:
        return
    if _upgrade_active():
        runtime.submit_input(0.0, 0.0, 0.0, 0.0, false, false, false, false, false, false)
        runtime.tick()
        Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
        _sync()
        return
    if _phone_menu_active():
        runtime.submit_input(0.0, 0.0, 0.0, 0.0, false, false, false, false, false, false)
        runtime.tick()
        _sync()
        return
    if developer_console.visible:
        pending_look = Vector2.ZERO
        pending_jump = false
        pending_melee = false
        pending_shoot = false
        pending_camera_toggle = false
        pending_comm_signal = 0
        runtime.submit_input(0.0, 0.0, 0.0, 0.0, false, false, false, false, false, false)
        runtime.tick()
        _sync()
        return
    var mx := float(Input.is_key_pressed(KEY_D)) - float(Input.is_key_pressed(KEY_A))
    var mz := float(Input.is_key_pressed(KEY_W)) - float(Input.is_key_pressed(KEY_S))
    var look := pending_look
    pending_look = Vector2.ZERO
    if pending_comm_signal > 0:
        runtime.set_comm_signal(pending_comm_signal)
    runtime.submit_input(mx, mz, look.x, look.y, Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT),
        Input.is_key_pressed(KEY_SHIFT), pending_jump, pending_melee, pending_shoot,
        pending_camera_toggle)
    pending_jump = false
    pending_melee = false
    pending_shoot = false
    pending_camera_toggle = false
    pending_comm_signal = 0
    runtime.tick()
    if _upgrade_active():
        Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
    _sync()
func agent_step(frames: int, move_x: float, move_z: float, look_x: float, look_y: float,
        vacuum: bool, sprint: bool, jump: bool, melee: bool, shoot: bool, camera_toggle: bool) -> void:
    var tick_started := Time.get_ticks_usec() if performance_profile_enabled else 0
    for frame_index in range(frames):
        runtime.submit_input(move_x, move_z, look_x if frame_index == 0 else 0.0,
            look_y if frame_index == 0 else 0.0, vacuum, sprint,
            jump and frame_index == 0, melee and frame_index == 0,
            shoot and frame_index == 0, camera_toggle and frame_index == 0)
        runtime.tick()
    if performance_profile_enabled:
        performance_profile_last["native_tick_us"] = Time.get_ticks_usec() - tick_started
    _sync()
func agent_reset() -> void:
    runtime.start_target_interaction_fixture()
    pending_look = Vector2.ZERO
    pending_jump = false
    pending_melee = false
    pending_shoot = false
    pending_camera_toggle = false
    pending_comm_signal = 0
    _sync()

func _uses_packed_enemy(index: int) -> bool:
    return packed_enemies_enabled or (packed_enemy_slice_enabled and index == packed_enemy_slice_index)

func _sync() -> void:
    var sync_started := Time.get_ticks_usec() if performance_profile_enabled else 0
    _restore_base_camera_transform()
    var lighting_snapshot: Dictionary = runtime.lighting_presentation_snapshot()
    _sync_lighting(lighting_snapshot)
    var room_bridge_started := Time.get_ticks_usec() if performance_profile_enabled else 0
    var room_snapshot: Dictionary = runtime.room_presentation_snapshot(-1)
    var grass_key := "%d:%d:%d:%d" % [int(room_snapshot.room_seed), int(room_snapshot.room_index),
        int(room_snapshot.tile_index), int(room_snapshot.grass)]
    if grass_key != _grass_snapshot_key:
        room_snapshot["grass_blades"] = runtime.grass_blade_presentation_snapshot(
            int(room_snapshot.tile_index))
        _grass_snapshot_key = grass_key
    if performance_profile_variant == "no_grass":
        room_snapshot["grass"] = false
        room_snapshot["grass_blades"] = []
        room_snapshot["grass_tips"] = []
    else:
        room_snapshot["grass_tips"] = runtime.grass_blade_tip_presentation_snapshot(
            int(room_snapshot.tile_index))
    if performance_profile_enabled:
        performance_profile_last["room_bridge_us"] = Time.get_ticks_usec() - room_bridge_started
    var room_adapter_started := Time.get_ticks_usec() if performance_profile_enabled else 0
    room_adapter.call("sync_snapshot", room_snapshot)
    if performance_profile_enabled:
        performance_profile_last["room_adapter_us"] = Time.get_ticks_usec() - room_adapter_started
    camera.position = runtime.camera_position()
    camera.look_at(runtime.camera_target(), Vector3.UP)
    camera.fov = runtime.camera_fov_degrees()
    sky_dome.position = camera.position
    sky_dome_material.set_shader_parameter("sky_origin", camera.position)
    _sync_moon(lighting_snapshot)
    var phone_started := Time.get_ticks_usec() if performance_profile_enabled else 0
    if performance_profile_variant == "no_phone":
        phone_adapter.visible = false
    else:
        phone_adapter.visible = true
        phone_adapter.call("sync_snapshot", runtime.phone_presentation_snapshot(), camera, runtime)
    if performance_profile_enabled:
        performance_profile_last["phone_bridge_adapter_us"] = Time.get_ticks_usec() - phone_started
    music_adapter.call("sync_snapshot", runtime.music_presentation_snapshot())
    sfx_adapter.call("sync_snapshot", runtime.audio_presentation_snapshot())
    var particles_started := Time.get_ticks_usec() if performance_profile_enabled else 0
    if performance_profile_variant == "no_particles":
        particle_adapter.visible = false
    else:
        particle_adapter.visible = true
        particle_adapter.call("sync_from_native", runtime.particle_presentation_snapshot(), camera)
    if performance_profile_enabled:
        performance_profile_last["particles_bridge_adapter_us"] = Time.get_ticks_usec() - particles_started
    melee_arc_adapter.call("sync_snapshot", runtime.player_action_snapshot())
    var hud_targets: Array[Dictionary] = []
    var enemies_started := Time.get_ticks_usec() if performance_profile_enabled else 0
    var enemy_bridge_us := 0
    var enemy_adapter_us := 0
    for i in range(targets.size()):
        var enemy_bridge_started := Time.get_ticks_usec() if performance_profile_enabled else 0
        var target_snapshot: Dictionary = runtime.target_presentation_snapshot(i)
        hud_targets.append(target_snapshot)
        if performance_profile_variant == "no_enemies":
            targets[i].visible = false
        else:
            var model_vertices := PackedVector3Array()
            var model_pose := []
            var model_packed: Dictionary = {}
            if bool(target_snapshot.alive):
                if _uses_packed_enemy(i):
                    model_packed = runtime.target_model_packed_buffer(i)
                    var packed_count := int(model_packed.get("vertex_count", 0))
                    if targets[i].call("needs_surface_rebuild", packed_count):
                        model_vertices = runtime.target_model_vertices(i)
                elif skeleton_enemy_slice_enabled and i == skeleton_enemy_slice_index:
                    model_pose = runtime.target_model_pose(i)
                else:
                    model_vertices = runtime.target_model_vertices(i)
            if performance_profile_enabled:
                enemy_bridge_us += Time.get_ticks_usec() - enemy_bridge_started
            var enemy_adapter_started := Time.get_ticks_usec() if performance_profile_enabled else 0
            if _uses_packed_enemy(i):
                targets[i].call("sync_packed_from_native", target_snapshot, model_packed,
                    model_vertices, camera.global_position, runtime.camera_target(),
                    camera_obstruction_fade_enabled and not runtime.first_person())
            elif skeleton_enemy_slice_enabled and i == skeleton_enemy_slice_index:
                targets[i].call("sync_pose_from_native", target_snapshot,
                    model_pose, camera.global_position, runtime.camera_target(),
                    camera_obstruction_fade_enabled and not runtime.first_person())
            else:
                targets[i].call("sync_from_native", target_snapshot,
                    model_vertices, camera.global_position, runtime.camera_target(),
                    camera_obstruction_fade_enabled and not runtime.first_person())
            if performance_profile_enabled:
                enemy_adapter_us += Time.get_ticks_usec() - enemy_adapter_started
            enemy_bridge_started = Time.get_ticks_usec() if performance_profile_enabled else 0
        var soul_snapshot: Dictionary = runtime.target_soul_presentation_snapshot(i)
        var soul_lattice := PackedVector3Array()
        if bool(soul_snapshot.get("visible", false)) and \
                float(soul_snapshot.get("morph_scale", 0.0)) > 0.001:
            soul_lattice = runtime.target_soul_lattice(i)
        if performance_profile_enabled:
            enemy_bridge_us += Time.get_ticks_usec() - enemy_bridge_started
        souls[i].call("sync_from_native", soul_snapshot, soul_lattice)
    if performance_profile_enabled:
        performance_profile_last["enemies_bridge_adapter_us"] = Time.get_ticks_usec() - enemies_started
        performance_profile_last["enemy_bridge_us"] = enemy_bridge_us
        performance_profile_last["enemy_adapter_us"] = enemy_adapter_us
    _sync_projectiles(runtime.projectile_presentation_snapshot())
    var viewport_size := get_viewport().get_visible_rect().size
    door_transition_adapter.call("sync_snapshot",
        runtime.door_transition_presentation_snapshot(), viewport_size)
    var hud_started := Time.get_ticks_usec() if performance_profile_enabled else 0
    if performance_profile_variant == "no_hud":
        hud_adapter.visible = false
    else:
        hud_adapter.visible = true
        hud_adapter.call("sync_snapshot", runtime.hud_presentation_snapshot(),
            runtime.restart_menu_presentation_snapshot(viewport_size.x, viewport_size.y), hud_targets,
            camera, runtime.player_position(), runtime.comm_signal(),
            runtime.comm_signal_timer(), runtime.simulation_time())
    if performance_profile_enabled:
        performance_profile_last["hud_bridge_adapter_us"] = Time.get_ticks_usec() - hud_started
    # All existing consumers above observe the native camera. Only the final
    # rendered view receives trauma, and the next sync restores the base first.
    _capture_base_camera_transform()
    _apply_render_camera_trauma()
    if performance_profile_enabled:
        performance_profile_last["sync_total_us"] = Time.get_ticks_usec() - sync_started

func configure_performance_profile(variant: String) -> void:
    performance_profile_enabled = true
    performance_profile_variant = variant
    var shadows_enabled := variant != "no_shadows"
    sun_light.shadow_enabled = shadows_enabled
    fill_light.shadow_enabled = shadows_enabled
    drama_spotlight.shadow_enabled = shadows_enabled
    for point_light in native_point_lights:
        point_light.shadow_enabled = shadows_enabled

func performance_profile_snapshot() -> Dictionary:
    return performance_profile_last.duplicate()

func _capture_base_camera_transform() -> void:
    _camera_base_transform = camera.global_transform
    _camera_base_target = runtime.camera_target()
    _camera_base_fov = camera.fov
    _camera_base_valid = true

func _restore_base_camera_transform() -> void:
    if camera != null and _camera_base_valid:
        camera.global_transform = _camera_base_transform
        camera.fov = _camera_base_fov

func _apply_render_camera_trauma() -> void:
    if not camera_trauma_enabled or _camera_trauma_strength <= 0.0:
        return
    var strength := clampf(_camera_trauma_strength, 0.0, 1.0)
    var phase := float(_camera_trauma_phase)
    var local_offset := Vector3(sin(phase * 2.17), cos(phase * 1.63), sin(phase * 2.83 + 0.4))
    if local_offset.length_squared() > 1.0:
        local_offset = local_offset.normalized()
    local_offset *= CAMERA_TRAUMA_MAX_TRANSLATION * strength
    var local_rotation := Vector3(cos(phase * 1.91), sin(phase * 2.41), sin(phase * 1.37 + 0.7))
    local_rotation *= CAMERA_TRAUMA_MAX_ROTATION_RADIANS * strength
    var rendered := _camera_base_transform
    rendered.origin += _camera_base_transform.basis * local_offset
    rendered.basis = (_camera_base_transform.basis * Basis.from_euler(local_rotation)).orthonormalized()
    camera.global_transform = rendered
    _camera_trauma_phase += 1
    _camera_trauma_strength = maxf(0.0, _camera_trauma_strength - CAMERA_TRAUMA_DECAY_PER_SYNC)

func set_camera_trauma_enabled_for_test(enabled: bool) -> void:
    camera_trauma_enabled = enabled
    if not enabled:
        _camera_trauma_strength = 0.0
        _restore_base_camera_transform()

func inject_camera_trauma_for_test(amplitude: float) -> void:
    # Deterministic and bounded: tests may request any value, but presentation
    # strength is always clamped to [0, 1].
    _camera_trauma_strength = clampf(amplitude, 0.0, 1.0) if camera_trauma_enabled else 0.0
    _camera_trauma_phase = 0

func _sync_projectiles(snapshots: Array) -> void:
    var active := {}
    for snapshot: Dictionary in snapshots:
        var stable_id := str(snapshot.stable_id)
        active[stable_id] = true
        if not projectile_nodes.has(stable_id):
            var node := MeshInstance3D.new()
            node.name = stable_id
            var mesh := SphereMesh.new()
            mesh.radius = 0.18
            mesh.height = 0.36
            node.mesh = mesh
            var material := StandardMaterial3D.new()
            material.albedo_color = Color(0.35, 0.9, 1.0)
            material.emission_enabled = true
            material.emission = Color(0.2, 0.75, 1.0)
            material.emission_energy_multiplier = 4.0
            node.material_override = material
            add_child(node)
            projectile_nodes[stable_id] = node
        var projectile: MeshInstance3D = projectile_nodes[stable_id]
        projectile.visible = true
        projectile.global_position = snapshot.position
        projectile.rotation.y = float(snapshot.spin)
    for stable_id in projectile_nodes:
        if not active.has(stable_id):
            projectile_nodes[stable_id].visible = false

func presentation_observation_snapshot() -> Dictionary:
    var target_rows: Array[Dictionary] = []
    for index in range(targets.size()):
        target_rows.append({
            "stable_id": "native-target-%d" % index,
            "node_name": targets[index].name,
            "position": targets[index].global_position,
            "visible": targets[index].visible,
            "camera_alpha": float(targets[index].camera_visibility_alpha),
        })
    var projectile_rows: Array[Dictionary] = []
    for stable_id in projectile_nodes:
        var projectile: MeshInstance3D = projectile_nodes[stable_id]
        if projectile.visible:
            projectile_rows.append({"stable_id": stable_id, "node_name": projectile.name,
                "position": projectile.global_position, "visible": true})
    var base_transform := _camera_base_transform if _camera_base_valid else camera.global_transform
    var rendered_transform := camera.global_transform
    var target_distance := maxf(base_transform.origin.distance_to(_camera_base_target), 0.001)
    var rendered_target := rendered_transform.origin - rendered_transform.basis.z * target_distance
    return {
        # Preserve the original observation contract as the native/base camera.
        "camera_position": base_transform.origin,
        "camera_target": _camera_base_target,
        "camera_fov": _camera_base_fov,
        "base_camera_position": base_transform.origin,
        "base_camera_target": _camera_base_target,
        "base_camera_fov": _camera_base_fov,
        "rendered_camera_position": rendered_transform.origin,
        "rendered_camera_target": rendered_target,
        "rendered_camera_fov": camera.fov,
        "camera_trauma_enabled": camera_trauma_enabled,
        "camera_trauma_strength": _camera_trauma_strength,
        "targets": target_rows,
        "projectiles": projectile_rows,
        "camera_obstruction_fade_enabled": camera_obstruction_fade_enabled,
        "door_transition": {
            "stable_id": "native-door-transition",
            "visible": door_transition_adapter.overlay.visible,
            "progress": float(door_transition_adapter.overlay.get_meta("native_progress", 0.0)),
            "motion_pixels": door_transition_adapter.overlay.get_meta("native_motion_pixels", Vector2.ZERO),
        },
    }

func _sync_lighting(snapshot: Dictionary) -> void:
    var open_sky := bool(snapshot.open_sky)
    sky_dome.visible = open_sky
    scene_environment.background_mode = Environment.BG_SKY if open_sky else Environment.BG_COLOR
    scene_environment.sky = sky_resource if open_sky else null
    if open_sky:
        _sync_open_sky(snapshot)
    else:
        scene_environment.background_color = snapshot.background
    scene_environment.ambient_light_color = snapshot.ambient
    scene_environment.fog_light_color = snapshot.fog_color
    scene_environment.fog_density = float(snapshot.fog_density)
    _sync_directional_light(sun_light, snapshot.sun)
    _sync_directional_light(fill_light, snapshot.fill)
    _sync_drama_spotlight(snapshot)
    var sources: Array = snapshot.local_lights
    # The screen-attached light is materialized by PhonePresentationAdapter.
    # Adding the older resolved `snapshot.phone` here creates a second omni
    # source and makes the entire fallback phone appear emissive.
    sources.append(snapshot.shot)
    sources.append(snapshot.exit)
    for index in range(native_point_lights.size()):
        var point: OmniLight3D = native_point_lights[index]
        if index >= sources.size():
            point.visible = false
            continue
        var source: Dictionary = sources[index]
        point.visible = bool(source.enabled)
        if not point.visible:
            continue
        point.position = source.position
        var source_color: Color = source.color
        var energy := maxf(source_color.r, maxf(source_color.g, source_color.b))
        if energy <= 0.0001:
            point.visible = false
            continue
        point.light_color = source_color / energy
        point.light_energy = energy
        var quadratic := float(source.quadratic_attenuation)
        point.omni_range = clampf(1.0 / sqrt(quadratic), 1.5, 30.0) if quadratic > 0.0001 else 4.5

func _sync_drama_spotlight(snapshot: Dictionary) -> void:
    var loop: Dictionary = runtime.gameplay_loop_snapshot()
    var special_event := int(loop.projectiles) > 0 or int(loop.room_captures) > 0 or bool(loop.upgrade_active)
    var night := bool(snapshot.open_sky) and float(snapshot.daylight) < 0.22
    drama_spotlight.visible = night or special_event
    if not drama_spotlight.visible:
        return
    var player: Vector3 = runtime.player_position()
    drama_spotlight.position = player + Vector3(0.0, 10.5, 1.5)
    drama_spotlight.look_at(player + Vector3(0.0, 0.8, 0.0), Vector3.FORWARD)
    drama_spotlight.light_energy = 4.2 if special_event else 2.6

func _sync_open_sky(snapshot: Dictionary) -> void:
    # The shader has no engine TIME dependency and updates its uniforms only
    # once per native simulation second. This keeps the radiance cubemap on
    # Godot's incremental 64px path instead of invalidating it every frame.
    var setting := int(snapshot.setting)
    var time_bucket := int(floor(float(snapshot.time)))
    if setting == _sky_update_setting and time_bucket == _sky_update_bucket:
        return
    _sky_update_setting = setting
    _sky_update_bucket = time_bucket
    sky_material.sky_top_color = snapshot.background
    sky_material.sky_horizon_color = snapshot.fog_color
    sky_material.ground_horizon_color = snapshot.fog_color
    sky_material.ground_bottom_color = snapshot.ambient
    sky_material.sky_energy_multiplier = lerpf(0.55, 1.0, float(snapshot.daylight))
    sky_material.ground_energy_multiplier = lerpf(0.45, 1.0, float(snapshot.daylight))
    sky_dome_material.set_shader_parameter("zenith_color", snapshot.background)
    sky_dome_material.set_shader_parameter("horizon_color", snapshot.fog_color)
    sky_dome_material.set_shader_parameter("ground_color", snapshot.ambient)
    sky_dome_material.set_shader_parameter("sun_color", snapshot.sun.color)
    sky_dome_material.set_shader_parameter("sun_direction", snapshot.sun.direction.normalized())
    sky_dome_material.set_shader_parameter("daylight", float(snapshot.daylight))
    sky_dome_material.set_shader_parameter("cloud_seed", float(snapshot.room_index) * 0.37 + float(setting))
    sky_dome_material.set_shader_parameter("cloud_phase", float(snapshot.time) * 0.003)
    sky_dome_material.set_shader_parameter("sky_origin", camera.position)
    moon_material.set_shader_parameter("sun_direction", snapshot.sun.direction.normalized())
    match setting:
        0: # Field: thin, high cirrus.
            sky_material.sky_curve = 0.26
            sky_dome_material.set_shader_parameter("cloud_form", 0.0)
            sky_dome_material.set_shader_parameter("cloud_coverage", 0.40)
            sky_dome_material.set_shader_parameter("cloud_opacity", 0.27)
        1: # City: more broken cloud and urban haze.
            sky_material.sky_curve = 0.34
            sky_dome_material.set_shader_parameter("cloud_form", 1.0)
            sky_dome_material.set_shader_parameter("cloud_coverage", 0.46)
            sky_dome_material.set_shader_parameter("cloud_opacity", 0.23)
        3: # Coastal: broader cloud shapes over the open horizon.
            sky_material.sky_curve = 0.22
            sky_dome_material.set_shader_parameter("cloud_form", 2.0)
            sky_dome_material.set_shader_parameter("cloud_coverage", 0.54)
            sky_dome_material.set_shader_parameter("cloud_opacity", 0.34)

func _sync_moon(snapshot: Dictionary) -> void:
    var open_sky := bool(snapshot.open_sky)
    var day_seconds := 480.0
    var synodic_days := 29.53059
    var sun_angle := TAU * (float(snapshot.time) / day_seconds + 0.20)
    var lunar_offset := TAU * float(snapshot.time) / (day_seconds * synodic_days)
    var nodal_phase := TAU * (float(snapshot.time) / (day_seconds * 173.31) + 0.23)
    var orbital_inclination := sin(nodal_phase) * 8.0
    var moon_angle := sun_angle - lunar_offset
    var moon_direction := Vector3(cos(moon_angle) * 54.0,
        sin(moon_angle) * 62.0, cos(lunar_offset) * 28.0 + orbital_inclination).normalized()
    moon.visible = open_sky and moon_direction.y > 0.0
    moon.position = camera.position + moon_direction * 360.0

func _sync_directional_light(light: DirectionalLight3D, source: Dictionary) -> void:
    light.visible = bool(source.enabled)
    if not light.visible:
        return
    var direction: Vector3 = source.direction.normalized()
    # OpenGL's directional-light position stores the direction toward the
    # light. Godot emits a DirectionalLight3D along local -Z, so its emitted
    # ray must point in the opposite direction.
    light.look_at(-direction, Vector3.UP)
    var source_color: Color = source.color
    var energy := maxf(source_color.r, maxf(source_color.g, source_color.b))
    light.light_color = source_color / maxf(energy, 0.0001)
    light.light_energy = energy

func _sync_mouse_mode() -> void:
    if runtime == null:
        return
    if _restart_menu_active() or _phone_menu_active() or _upgrade_active():
        Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
    elif bool(runtime.hud_presentation_snapshot().get("started", false)):
        Input.mouse_mode = Input.MOUSE_MODE_CAPTURED

func _gameplay_pointer_active() -> bool:
    return runtime != null and bool(runtime.hud_presentation_snapshot().get("started", false)) \
        and not _restart_menu_active() and not _phone_menu_active() and not _upgrade_active()

func _restart_menu_active() -> bool:
    return runtime != null and bool(runtime.hud_presentation_snapshot().get("dead", false))

func _phone_menu_active() -> bool:
    if runtime == null or not bool(runtime.phone_menu_layout_snapshot().get("active", false)):
        return false
    # Native menu semantics, rather than a one-tick-old presentation mode,
    # decide whether pointer input still belongs to the phone. Resume clears
    # ui_paused synchronously, so camera capture is restored on that click.
    var hud: Dictionary = runtime.hud_presentation_snapshot()
    return not bool(hud.get("started", false)) or bool(hud.get("ui_paused", false))

func _upgrade_active() -> bool:
    return runtime != null and bool(runtime.upgrade_menu_active())
