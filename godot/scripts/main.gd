extends Node3D
# Godot owns presentation. The parent C++ Game owns simulation and collision.
# Temporary primitive meshes deliberately avoid claiming original visual parity.
const PhonePresentationAdapter = preload("res://scripts/phone_presentation_adapter.gd")
var runtime: Node
var phone_adapter: Node
var camera: Camera3D
var targets: Array[MeshInstance3D] = []
var label: Label
var reticle: Label
var controls: Label
var pending_look := Vector2.ZERO
var agent_playtest := false
const TARGET_HEIGHT := 1.16
const TARGET_RADIUS := 0.17
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
    var floor := MeshInstance3D.new()
    var floor_mesh := PlaneMesh.new()
    floor_mesh.size = Vector2(90, 90)
    floor.mesh = floor_mesh
    add_child(floor)
    phone_adapter = PhonePresentationAdapter.new()
    phone_adapter.name = "PhonePresentationAdapter"
    add_child(phone_adapter)
    for i in range(runtime.target_count()):
        var enemy := MeshInstance3D.new()
        var shape := CapsuleMesh.new()
        shape.radius = TARGET_RADIUS
        shape.height = TARGET_HEIGHT
        enemy.mesh = shape
        add_child(enemy)
        targets.append(enemy)
    camera = Camera3D.new()
    camera.current = true
    add_child(camera)
    var hud := CanvasLayer.new()
    add_child(hud)
    label = Label.new()
    label.position = Vector2(18, 18)
    hud.add_child(label)
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
    controls.text = "WASD move  |  Shift sprint  |  Space jump  |  Mouse look  |  C camera  |  E vacuum  |  F melee  |  Q shoot"
    controls.add_theme_color_override("font_color", Color(0.95, 0.97, 1.0, 0.85))
    controls.add_theme_font_size_override("font_size", 13)
    controls.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
    controls.position = Vector2(18, -34)
    controls.size = Vector2(1050, 24)
    hud.add_child(controls)
    Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
    _sync()
func _unhandled_input(event: InputEvent) -> void:
    if event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
        pending_look += event.relative
    elif event is InputEventKey and event.pressed and event.keycode == KEY_ESCAPE:
        Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
    elif event is InputEventMouseButton and event.pressed:
        Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
func _physics_process(_delta: float) -> void:
    if agent_playtest:
        return
    var mx := float(Input.is_key_pressed(KEY_D)) - float(Input.is_key_pressed(KEY_A))
    var mz := float(Input.is_key_pressed(KEY_W)) - float(Input.is_key_pressed(KEY_S))
    var look := pending_look
    pending_look = Vector2.ZERO
    runtime.submit_input(mx, mz, look.x, look.y, Input.is_key_pressed(KEY_E),
        Input.is_key_pressed(KEY_SHIFT), Input.is_key_pressed(KEY_SPACE),
        Input.is_key_pressed(KEY_F), Input.is_key_pressed(KEY_Q), Input.is_key_pressed(KEY_C))
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
    _sync()
func _sync() -> void:
    camera.position = runtime.camera_position()
    camera.look_at(runtime.camera_target(), Vector3.UP)
    phone_adapter.call("sync_snapshot", runtime.phone_presentation_snapshot(), camera)
    for i in range(targets.size()):
        targets[i].visible = runtime.target_alive(i)
        if targets[i].visible:
            targets[i].position = runtime.target_position(i) + Vector3.UP * (TARGET_HEIGHT * 0.5)
    label.text = "Parent GameState | tick %d | souls %d | battery %.1f" % [runtime.frame(),runtime.souls(),runtime.battery()]
