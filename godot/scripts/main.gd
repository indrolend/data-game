extends Node3D
# Godot owns presentation. The parent C++ Game owns simulation and collision.
# Temporary primitive meshes deliberately avoid claiming original visual parity.
const PhonePresentationAdapter = preload("res://scripts/phone_presentation_adapter.gd")
const RoomPresentationAdapter = preload("res://scripts/room_presentation_adapter.gd")
const HumanModelAdapter = preload("res://scripts/human_model_adapter.gd")
const NativeParticlePresentationAdapter = preload("res://scripts/native_particle_presentation_adapter.gd")
const SoulLatticeAdapter = preload("res://scripts/soul_lattice_adapter.gd")
const DeveloperConsole = preload("res://scripts/developer_console.gd")
const HudPresentationAdapter = preload("res://scripts/hud_presentation_adapter.gd")
var runtime: Node
var developer_console: Control
var hud_adapter: Control
var phone_adapter: Node
var particle_adapter: Node
var room_adapter: Node
var camera: Camera3D
var world_environment: WorldEnvironment
var scene_environment: Environment
var sun_light: DirectionalLight3D
var fill_light: DirectionalLight3D
var native_point_lights: Array[OmniLight3D] = []
var _grass_snapshot_key := ""
var targets: Array[Node3D] = []
var souls: Array[Node3D] = []
var pending_look := Vector2.ZERO
var pending_jump := false
var pending_melee := false
var pending_shoot := false
var pending_camera_toggle := false
var pending_comm_signal := 0
var agent_playtest := false
func _ready() -> void:
    agent_playtest = "--agent-playtest" in OS.get_cmdline_user_args()
    runtime = ClassDB.instantiate("ParentGameNode")
    add_child(runtime)
    world_environment = WorldEnvironment.new()
    scene_environment = Environment.new()
    scene_environment.background_mode = Environment.BG_COLOR
    scene_environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
    scene_environment.ambient_light_energy = 1.0
    scene_environment.fog_enabled = true
    world_environment.environment = scene_environment
    add_child(world_environment)
    sun_light = DirectionalLight3D.new()
    sun_light.name = "NativeSun"
    add_child(sun_light)
    fill_light = DirectionalLight3D.new()
    fill_light.name = "NativeSkyFill"
    add_child(fill_light)
    for light_index in range(6):
        var point_light := OmniLight3D.new()
        point_light.name = "NativeResolvedLight_%d" % light_index
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
    particle_adapter = NativeParticlePresentationAdapter.new()
    particle_adapter.name = "NativeParticlePresentationAdapter"
    add_child(particle_adapter)
    for i in range(runtime.target_count()):
        var enemy := HumanModelAdapter.new()
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
    add_child(camera)
    var hud := CanvasLayer.new()
    add_child(hud)
    hud_adapter = HudPresentationAdapter.new()
    hud_adapter.name = "NativeHudPresentation"
    hud.add_child(hud_adapter)
    developer_console = DeveloperConsole.new()
    developer_console.configure(runtime)
    hud.add_child(developer_console)
    Input.mouse_mode = Input.MOUSE_MODE_VISIBLE if _phone_menu_active() or _upgrade_active() else Input.MOUSE_MODE_CAPTURED
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
            if not _phone_menu_active() and bool(runtime.hud_presentation_snapshot().get("started", false)):
                Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
            _sync()
            get_viewport().set_input_as_handled()
        elif event is InputEventMouseButton and event.pressed:
            if event.button_index == MOUSE_BUTTON_LEFT:
                runtime.menu_confirm()
            elif event.button_index == MOUSE_BUTTON_RIGHT:
                runtime.menu_back()
            if not _phone_menu_active() and bool(runtime.hud_presentation_snapshot().get("started", false)):
                Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
            _sync()
            get_viewport().set_input_as_handled()
        return
    if event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
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
        if Input.mouse_mode != Input.MOUSE_MODE_CAPTURED:
            runtime.set_paused(false)
            Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
        elif event.button_index == MOUSE_BUTTON_RIGHT:
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
    for frame_index in range(frames):
        runtime.submit_input(move_x, move_z, look_x if frame_index == 0 else 0.0,
            look_y if frame_index == 0 else 0.0, vacuum, sprint,
            jump and frame_index == 0, melee and frame_index == 0,
            shoot and frame_index == 0, camera_toggle and frame_index == 0)
        runtime.tick()
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
func _sync() -> void:
    _sync_lighting(runtime.lighting_presentation_snapshot())
    var room_snapshot: Dictionary = runtime.room_presentation_snapshot(-1)
    var grass_key := "%d:%d:%d:%d" % [int(room_snapshot.room_seed), int(room_snapshot.room_index),
        int(room_snapshot.tile_index), int(room_snapshot.grass)]
    if grass_key != _grass_snapshot_key:
        room_snapshot["grass_blades"] = runtime.grass_blade_presentation_snapshot(
            int(room_snapshot.tile_index))
        _grass_snapshot_key = grass_key
    room_snapshot["grass_tips"] = runtime.grass_blade_tip_presentation_snapshot(
        int(room_snapshot.tile_index))
    room_adapter.call("sync_snapshot", room_snapshot)
    camera.position = runtime.camera_position()
    camera.look_at(runtime.camera_target(), Vector3.UP)
    camera.fov = runtime.camera_fov_degrees()
    phone_adapter.call("sync_snapshot", runtime.phone_presentation_snapshot(), camera)
    particle_adapter.call("sync_from_native", runtime.particle_presentation_snapshot(), camera)
    var hud_targets: Array[Dictionary] = []
    for i in range(targets.size()):
        var target_snapshot: Dictionary = runtime.target_presentation_snapshot(i)
        hud_targets.append(target_snapshot)
        targets[i].call("sync_from_native", target_snapshot,
            runtime.target_model_vertices(i))
        souls[i].call("sync_from_native", runtime.target_soul_presentation_snapshot(i),
            runtime.target_soul_lattice(i))
    hud_adapter.call("sync_snapshot", runtime.hud_presentation_snapshot(), hud_targets,
        camera, runtime.player_position(), runtime.comm_signal(),
        runtime.comm_signal_timer(), runtime.simulation_time())

func _sync_lighting(snapshot: Dictionary) -> void:
    scene_environment.background_color = snapshot.background
    scene_environment.ambient_light_color = snapshot.ambient
    scene_environment.fog_light_color = snapshot.fog_color
    scene_environment.fog_density = float(snapshot.fog_density)
    _sync_directional_light(sun_light, snapshot.sun)
    _sync_directional_light(fill_light, snapshot.fill)
    var sources: Array = snapshot.local_lights
    sources.append(snapshot.phone)
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

func _sync_directional_light(light: DirectionalLight3D, source: Dictionary) -> void:
    light.visible = bool(source.enabled)
    if not light.visible:
        return
    var direction: Vector3 = source.direction.normalized()
    light.look_at(direction, Vector3.UP)
    var source_color: Color = source.color
    var energy := maxf(source_color.r, maxf(source_color.g, source_color.b))
    light.light_color = source_color / maxf(energy, 0.0001)
    light.light_energy = energy

func _phone_menu_active() -> bool:
    return runtime != null and bool(runtime.phone_menu_layout_snapshot().get("active", false))

func _upgrade_active() -> bool:
    return runtime != null and bool(runtime.upgrade_menu_active())
