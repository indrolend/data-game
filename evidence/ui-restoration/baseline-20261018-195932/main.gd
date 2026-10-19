extends Node3D
# Godot owns presentation. The parent C++ Game owns simulation and collision.
# Temporary primitive meshes deliberately avoid claiming original visual parity.
const PhonePresentationAdapter = preload("res://scripts/phone_presentation_adapter.gd")
const RoomPresentationAdapter = preload("res://scripts/room_presentation_adapter.gd")
const HumanModelAdapter = preload("res://scripts/human_model_adapter.gd")
const SoulLatticeAdapter = preload("res://scripts/soul_lattice_adapter.gd")
const DeveloperConsole = preload("res://scripts/developer_console.gd")
var runtime: Node
var developer_console: Control
var phone_adapter: Node
var room_adapter: Node
var camera: Camera3D
var targets: Array[Node3D] = []
var souls: Array[Node3D] = []
var label: Label
var combat_feedback: Label
var reticle: Label
var controls: Label
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
    add_child(camera)
    var hud := CanvasLayer.new()
    add_child(hud)
    label = Label.new()
    label.position = Vector2(18, 18)
    hud.add_child(label)
    combat_feedback = Label.new()
    combat_feedback.position = Vector2(18, 46)
    combat_feedback.add_theme_font_size_override("font_size", 18)
    hud.add_child(combat_feedback)
    reticle = Label.new()
    reticle.text = "+"
    reticle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
    reticle.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
    reticle.add_theme_color_override("font_color", Color(0.95, 0.97, 1.0, 0.85))
    reticle.add_theme_font_size_override("font_size", 22)
    reticle.set_anchors_preset(Control.PRESET_CENTER)
    reticle.position = Vector2(-10, -14)
    reticle.size = Vector2(20, 28)
    hud.add_child(reticle)
    controls = Label.new()
    controls.text = "WASD move  |  Shift sprint  |  Space jump  |  Mouse look  |  C camera  |  E vacuum  |  F melee  |  Q shoot  |  1-4 signal"
    controls.add_theme_color_override("font_color", Color(0.95, 0.97, 1.0, 0.85))
    controls.add_theme_font_size_override("font_size", 13)
    controls.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
    controls.position = Vector2(18, -34)
    controls.size = Vector2(1050, 24)
    hud.add_child(controls)
    developer_console = DeveloperConsole.new()
    developer_console.configure(runtime)
    hud.add_child(developer_console)
    Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
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
    phone_adapter.call("sync_snapshot", runtime.phone_presentation_snapshot(), camera)
    for i in range(targets.size()):
        targets[i].call("sync_from_native", runtime.target_presentation_snapshot(i),
            runtime.target_model_vertices(i))
        souls[i].call("sync_from_native", runtime.target_soul_presentation_snapshot(i),
            runtime.target_soul_lattice(i))
    label.text = "Parent GameState | tick %d | souls %d | battery %.1f" % [runtime.frame(),runtime.souls(),runtime.battery()]
    _sync_combat_feedback()

func _sync_combat_feedback() -> void:
    var message := ""
    var color := Color(0.95, 0.97, 1.0, 0.9)
    var action: Dictionary = runtime.player_action_snapshot()
    if float(action.visual_timer) > 0.0:
        message = "STRIKE %d" % (int(action.combo_index) + 1)
        if bool(action.visual_hit):
            message += "  HIT"
            color = Color(1.0, 0.68, 0.4)
    if bool(runtime.vacuum_active()):
        var ingest := 0.0
        for i in range(targets.size()):
            ingest = maxf(ingest, float(runtime.target_ingest_progress(i)))
        message = "VACUUM  %.0f%%" % (ingest * 100.0)
        color = Color(1.0, 0.72, 0.52)
    for i in range(targets.size()):
        var enemy: Dictionary = runtime.target_presentation_snapshot(i)
        if not bool(enemy.attack_active) or int(enemy.attack_target_player_id) != 0:
            continue
        var phase := float(enemy.attack_phase)
        var commit := float(enemy.attack_commit_phase)
        var end_phase := float(enemy.attack_end_phase)
        if phase < commit:
            message = "ENEMY WINDUP"
            color = Color(1.0, 0.82, 0.42)
        elif phase <= end_phase:
            message = "ENEMY STRIKE"
            color = Color(1.0, 0.35, 0.28)
        if bool(enemy.attack_hit):
            message = "PLAYER HIT"
            color = Color(1.0, 0.22, 0.18)
    combat_feedback.text = message
    combat_feedback.add_theme_color_override("font_color", color)
    if reticle != null:
        reticle.add_theme_color_override("font_color", color)



