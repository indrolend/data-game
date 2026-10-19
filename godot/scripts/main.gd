extends Node3D
# Godot owns presentation. The parent C++ Game owns simulation and collision.
# Temporary primitive meshes deliberately avoid claiming original visual parity.
const PhonePresentationAdapter = preload("res://scripts/phone_presentation_adapter.gd")
const RoomPresentationAdapter = preload("res://scripts/room_presentation_adapter.gd")
const HumanModelAdapter = preload("res://scripts/human_model_adapter.gd")
const SoulLatticeAdapter = preload("res://scripts/soul_lattice_adapter.gd")
const DeveloperConsole = preload("res://scripts/developer_console.gd")
const HudPresentationAdapter = preload("res://scripts/hud_presentation_adapter.gd")
var runtime: Node
var developer_console: Control
var hud_adapter: Control
var phone_adapter: Node
var room_adapter: Node
var camera: Camera3D
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
    var env := WorldEnvironment.new()
    var environment := Environment.new()
    environment.background_mode = Environment.BG_COLOR
    environment.background_color = Color(0.12, 0.16, 0.23)
    environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
    environment.ambient_light_color = Color(0.7, 0.75, 0.85)
    environment.ambient_light_energy = 0.8
    env.environment = environment
    add_child(env)
    var sun := DirectionalLight3D.new()
    sun.rotation_degrees = Vector3(-55, 35, 0)
    add_child(sun)
    room_adapter = RoomPresentationAdapter.new()
    room_adapter.name = "RoomPresentationAdapter"
    add_child(room_adapter)
    phone_adapter = PhonePresentationAdapter.new()
    phone_adapter.name = "PhonePresentationAdapter"
    add_child(phone_adapter)
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
    room_adapter.call("sync_snapshot", runtime.room_presentation_snapshot(-1))
    camera.position = runtime.camera_position()
    camera.look_at(runtime.camera_target(), Vector3.UP)
    camera.fov = runtime.camera_fov_degrees()
    phone_adapter.call("sync_snapshot", runtime.phone_presentation_snapshot(), camera)
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

func _phone_menu_active() -> bool:
    return runtime != null and bool(runtime.phone_menu_layout_snapshot().get("active", false))

func _upgrade_active() -> bool:
    return runtime != null and bool(runtime.upgrade_menu_active())
