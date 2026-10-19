extends CharacterBody3D

signal health_changed(current: int, maximum: int)
signal defeated

const PhoneAvatar = preload("res://scripts/phone_avatar.gd")
const WALK_SPEED := 4.2
const SPRINT_SPEED := 6.2
const JUMP_SPEED := 5.0
const GRAVITY := 18.0
const ATTACK_COOLDOWN := 0.46
const ATTACK_WINDOW := 0.17
const MAX_HEALTH := 5

@onready var camera_pivot: Node3D = $CameraPivot
@onready var camera_arm: SpringArm3D = $CameraPivot/CameraArm
@onready var camera: Camera3D = $CameraPivot/CameraArm/Camera3D
@onready var melee_hitbox: Area3D = $MeleeHitbox

var health := MAX_HEALTH
var first_person := false
var invulnerability_time := 0.0
var _yaw := 0.0
var _pitch := -0.28
var _attack_cooldown := 0.0
var _attack_time := 0.0
var _attack_hit_ids: Dictionary = {}
var _knockback := Vector3.ZERO
var _defeated := false
var _world_phone: Node3D
var _view_phone: Node3D
var _body_visuals: Node3D

func _ready() -> void:
	_world_phone = PhoneAvatar.new()
	_world_phone.name = "WorldPhone"
	_world_phone.position = Vector3(0, 0.72, 0)
	add_child(_world_phone)
	_view_phone = PhoneAvatar.new()
	_view_phone.name = "PhoneViewModel"
	_view_phone.position = Vector3(0.29, -0.29, -0.62)
	_view_phone.rotation_degrees = Vector3(-7, -12, -7)
	camera.add_child(_view_phone)
	_body_visuals = Node3D.new()
	_body_visuals.name = "BodyVisuals"
	add_child(_body_visuals)
	melee_hitbox.monitoring = true
	melee_hitbox.monitorable = false
	melee_hitbox.set_collision_mask_value(1, false)
	melee_hitbox.set_collision_mask_value(2, true)
	melee_hitbox.set_collision_layer_value(1, false)
	melee_hitbox.set_collision_layer_value(2, false)
	camera_arm.add_excluded_object(get_rid())
	_configure_camera()

func add_look_input(relative: Vector2) -> void:
	_yaw -= relative.x * 0.0024
	_pitch = clampf(_pitch - relative.y * 0.002, -0.75, 0.45)

func toggle_camera() -> void:
	first_person = not first_person
	_configure_camera()

func _physics_process(delta: float) -> void:
	if _defeated:
		velocity.x = move_toward(velocity.x, 0.0, 8.0 * delta)
		velocity.z = move_toward(velocity.z, 0.0, 8.0 * delta)
		if not is_on_floor():
			velocity.y -= GRAVITY * delta
		move_and_slide()
		_world_phone.rotation.z = move_toward(_world_phone.rotation.z, 1.1, 2.5 * delta)
		return
	_attack_cooldown = maxf(0.0, _attack_cooldown - delta)
	_attack_time = maxf(0.0, _attack_time - delta)
	invulnerability_time = maxf(0.0, invulnerability_time - delta)
	if Input.is_action_just_pressed("jump") and is_on_floor():
		velocity.y = JUMP_SPEED
	if Input.is_action_just_pressed("attack") and _attack_cooldown <= 0.0:
		_attack_cooldown = ATTACK_COOLDOWN
		_attack_time = ATTACK_WINDOW
		_attack_hit_ids.clear()
		_world_phone.rotation.z = -0.48
	var axis := Input.get_vector("move_left", "move_right", "move_forward", "move_back")
	var forward := Vector3(sin(_yaw), 0.0, -cos(_yaw))
	var right := Vector3(cos(_yaw), 0.0, sin(_yaw))
	var direction := (right * axis.x - forward * axis.y).normalized()
	var speed := SPRINT_SPEED if Input.is_action_pressed("sprint") else WALK_SPEED
	var desired := direction * speed
	velocity.x = move_toward(velocity.x, desired.x, 22.0 * delta)
	velocity.z = move_toward(velocity.z, desired.z, 22.0 * delta)
	velocity.x += _knockback.x
	velocity.z += _knockback.z
	_knockback = _knockback.move_toward(Vector3.ZERO, 16.0 * delta)
	if not is_on_floor():
		velocity.y -= GRAVITY * delta
	else:
		velocity.y = maxf(velocity.y, -0.2)
	rotation.y = _yaw
	move_and_slide()
	_update_attack_contacts()
	if _attack_time <= 0.0:
		_world_phone.rotation.z = move_toward(_world_phone.rotation.z, 0.0, 5.0 * delta)
	if invulnerability_time > 0.0:
		_world_phone.call("set_damage_flash", int(Time.get_ticks_msec() / 90) % 2 == 0)
	else:
		_world_phone.call("set_damage_flash", false)
	_configure_camera()

func _update_attack_contacts() -> void:
	if _attack_time <= 0.0:
		return
	for body in melee_hitbox.get_overlapping_bodies():
		if body.is_in_group("playable_enemy") and body.has_method("receive_hit"):
			var instance_id: int = body.get_instance_id()
			if not _attack_hit_ids.has(instance_id):
				_attack_hit_ids[instance_id] = true
				body.receive_hit(global_position, 1)

func _configure_camera() -> void:
	camera_pivot.rotation.x = _pitch
	camera.current = true
	if first_person:
		camera_arm.spring_length = 0.01
		camera.position = Vector3.ZERO
		camera_arm.rotation.y = 0.0
		camera.rotation.y = 0.0
		_world_phone.visible = false
		_view_phone.visible = true
	else:
		# Use the authored offset for this prototype: the spring-arm query was
		# clipping against the player's own body in headless runs.
		camera_arm.spring_length = 0.01
		camera.position = Vector3(0.0, 0.0, 4.2)
		camera_arm.rotation.y = 0.0
		camera.rotation.y = 0.0
		_world_phone.visible = true
		_view_phone.visible = false

func take_damage(amount: int, source_position: Vector3) -> void:
	if _defeated or invulnerability_time > 0.0:
		return
	health = maxi(0, health - amount)
	invulnerability_time = 0.72
	var away := global_position - source_position
	away.y = 0.0
	_knockback = away.normalized() * 5.0 + Vector3.UP * 1.4
	health_changed.emit(health, MAX_HEALTH)
	if health == 0:
		_defeated = true
		_world_phone.rotation.z = 0.0
		defeated.emit()

func reset_player(at_position: Vector3) -> void:
	global_position = at_position
	velocity = Vector3.ZERO
	_knockback = Vector3.ZERO
	health = MAX_HEALTH
	_defeated = false
	_attack_cooldown = 0.0
	_attack_time = 0.0
	invulnerability_time = 0.0
	health_changed.emit(health, MAX_HEALTH)

func is_defeated() -> bool:
	return _defeated
