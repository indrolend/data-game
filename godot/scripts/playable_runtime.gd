extends Node3D
## Godot-authoritative first encounter. The native Game remains available in the
## parent-reference scene and is not advanced by this playable scene.

const PlayerScene = preload("res://scenes/player.tscn")
const EnemyScene = preload("res://scenes/enemy.tscn")

var player
var enemy
var status_label: Label
var health_label: Label
var enemy_label: Label
var pause_panel: ColorRect
var _paused := false
var _ending := ""

func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	_register_input_actions()
	_build_world()
	player = PlayerScene.instantiate()
	player.name = "Player"
	player.position = Vector3(0, 0, 4)
	player.process_mode = Node.PROCESS_MODE_PAUSABLE
	add_child(player)
	enemy = EnemyScene.instantiate()
	enemy.name = "Enemy"
	enemy.position = Vector3(0, 0, -5)
	enemy.process_mode = Node.PROCESS_MODE_PAUSABLE
	enemy.set_player(player)
	add_child(enemy)
	_build_hud()
	player.health_changed.connect(_on_player_health_changed)
	player.defeated.connect(_on_player_defeated)
	enemy.health_changed.connect(_on_enemy_health_changed)
	enemy.defeated.connect(_on_enemy_defeated)
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
	_update_hud()

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		player.add_look_input(event.relative)
	elif event is InputEventKey and event.pressed and not event.echo:
		if event.keycode == KEY_ESCAPE:
			_set_paused(not _paused)
		elif event.keycode == KEY_C:
			player.toggle_camera()
		elif event.keycode == KEY_R and not _ending.is_empty():
			restart_encounter()
	elif event is InputEventMouseButton and event.pressed and _paused:
		_set_paused(false)

func _process(_delta: float) -> void:
	_update_hud()

func _register_input_actions() -> void:
	_add_key_action("move_left", KEY_A)
	_add_key_action("move_right", KEY_D)
	_add_key_action("move_forward", KEY_W)
	_add_key_action("move_back", KEY_S)
	_add_key_action("jump", KEY_SPACE)
	_add_key_action("sprint", KEY_SHIFT)
	_add_key_action("attack", KEY_F)
	_add_key_action("camera_toggle", KEY_C)

func _add_key_action(action: StringName, key: Key) -> void:
	if InputMap.has_action(action):
		InputMap.erase_action(action)
	InputMap.add_action(action)
	var key_event := InputEventKey.new()
	key_event.physical_keycode = key
	InputMap.action_add_event(action, key_event)

func _build_world() -> void:
	var environment_node := WorldEnvironment.new()
	var environment := Environment.new()
	environment.background_mode = Environment.BG_COLOR
	environment.background_color = Color("#101b2a")
	environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environment.ambient_light_color = Color("#9bbed0")
	environment.ambient_light_energy = 0.72
	environment_node.environment = environment
	add_child(environment_node)
	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-52, -28, 0)
	sun.light_energy = 1.25
	sun.shadow_enabled = true
	add_child(sun)
	_add_box("ArenaFloor", Vector3(24, 0.5, 24), Vector3(0, -0.25, 0), Color("#3e5860"))
	_add_box("NorthWall", Vector3(24, 3.2, 0.5), Vector3(0, 1.35, -12), Color("#243543"))
	_add_box("SouthWall", Vector3(24, 3.2, 0.5), Vector3(0, 1.35, 12), Color("#243543"))
	_add_box("WestWall", Vector3(0.5, 3.2, 24), Vector3(-12, 1.35, 0), Color("#243543"))
	_add_box("EastWall", Vector3(0.5, 3.2, 24), Vector3(12, 1.35, 0), Color("#243543"))
	_add_box("NorthDoorGlow", Vector3(2.1, 2.35, 0.12), Vector3(0, 1.1, -11.69), Color("#164b59"), true)
	_add_box("PracticeBlockLeft", Vector3(1.0, 0.8, 1.0), Vector3(-6.6, 0.4, 1.0), Color("#53636a"))
	_add_box("PracticeBlockRight", Vector3(1.0, 0.8, 1.0), Vector3(6.6, 0.4, -1.0), Color("#53636a"))

func _add_box(node_name: String, size: Vector3, at: Vector3, tint: Color, unshaded := false) -> void:
	var body := StaticBody3D.new()
	body.name = node_name
	body.position = at
	var collision := CollisionShape3D.new()
	var shape := BoxShape3D.new()
	shape.size = size
	collision.shape = shape
	body.add_child(collision)
	add_child(body)
	var mesh_node := MeshInstance3D.new()
	mesh_node.name = node_name + "Visual"
	var mesh := BoxMesh.new()
	mesh.size = size
	mesh_node.mesh = mesh
	var material := StandardMaterial3D.new()
	material.albedo_color = tint
	material.roughness = 0.84
	if unshaded:
		material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
		material.emission_enabled = true
		material.emission = tint
		material.emission_energy_multiplier = 0.7
	mesh_node.material_override = material
	body.add_child(mesh_node)

func _build_hud() -> void:
	var layer := CanvasLayer.new()
	layer.name = "EncounterHUD"
	add_child(layer)
	var title := Label.new()
	title.text = "DIGITAL BREAKDOWN  /  SIGNAL 01"
	title.position = Vector2(24, 18)
	title.add_theme_color_override("font_color", Color("#8cecf0"))
	title.add_theme_font_size_override("font_size", 19)
	layer.add_child(title)
	health_label = Label.new()
	health_label.position = Vector2(24, 52)
	health_label.add_theme_font_size_override("font_size", 18)
	layer.add_child(health_label)
	enemy_label = Label.new()
	enemy_label.position = Vector2(24, 80)
	enemy_label.add_theme_font_size_override("font_size", 18)
	layer.add_child(enemy_label)
	var reticle := Label.new()
	reticle.text = "+"
	reticle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	reticle.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	reticle.set_anchors_preset(Control.PRESET_CENTER)
	reticle.position = Vector2(-10, -15)
	reticle.size = Vector2(20, 30)
	reticle.add_theme_color_override("font_color", Color("#aaf7f4"))
	reticle.add_theme_font_size_override("font_size", 24)
	layer.add_child(reticle)
	status_label = Label.new()
	status_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	status_label.set_anchors_preset(Control.PRESET_CENTER_BOTTOM)
	status_label.position = Vector2(-420, -80)
	status_label.size = Vector2(840, 32)
	status_label.add_theme_font_size_override("font_size", 17)
	layer.add_child(status_label)
	pause_panel = ColorRect.new()
	pause_panel.name = "PausePanel"
	pause_panel.color = Color(0.02, 0.05, 0.09, 0.82)
	pause_panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	pause_panel.visible = false
	pause_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	layer.add_child(pause_panel)
	var pause_text := Label.new()
	pause_text.text = "PAUSED\nClick to return"
	pause_text.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	pause_text.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	pause_text.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	pause_text.add_theme_font_size_override("font_size", 30)
	pause_panel.add_child(pause_text)
	var controls := Label.new()
	controls.text = "WASD move  •  Shift sprint  •  Space jump  •  F strike  •  C camera  •  Esc pause"
	controls.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
	controls.position = Vector2(24, -34)
	controls.add_theme_color_override("font_color", Color("#b8c9d0"))
	layer.add_child(controls)

func _update_hud() -> void:
	if not is_instance_valid(player) or not is_instance_valid(enemy):
		return
	health_label.text = "PHONE INTEGRITY  " + "■ ".repeat(player.health) + "□ ".repeat(player.MAX_HEALTH - player.health)
	enemy_label.text = "BREAKDOWN  %d / %d  •  %s" % [enemy.health, enemy.MAX_HEALTH, enemy.state_name()]
	if not _ending.is_empty():
		status_label.text = _ending + "  —  Press R to play again"
	elif enemy.state_name() == "WINDUP":
		status_label.text = "INCOMING STRIKE — evade or interrupt with F"
	elif enemy.state_name() == "STAGGERED":
		status_label.text = "Enemy staggered — keep pressure"
	else:
		status_label.text = "Close the distance and strike when the handset flashes"

func _on_player_health_changed(_current: int, _maximum: int) -> void:
	_update_hud()

func _on_enemy_health_changed(_current: int, _maximum: int) -> void:
	_update_hud()

func _on_player_defeated() -> void:
	_ending = "SIGNAL LOST — you were overwhelmed"
	_update_hud()

func _on_enemy_defeated() -> void:
	_ending = "ENCOUNTER CLEAR — the hostile is down"
	_update_hud()

func _set_paused(paused: bool) -> void:
	_paused = paused
	pause_panel.visible = paused
	get_tree().paused = paused
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE if paused else Input.MOUSE_MODE_CAPTURED

func restart_encounter() -> void:
	_ending = ""
	_set_paused(false)
	player.reset_player(Vector3(0, 0, 4))
	player.rotation.y = 0.0
	enemy.reset_enemy(Vector3(0, 0, -5), player)
