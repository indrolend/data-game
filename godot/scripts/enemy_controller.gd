extends CharacterBody3D

signal health_changed(current: int, maximum: int)
signal defeated

enum FightState { APPROACH, WINDUP, STRIKE, RECOVER, STAGGERED, DEFEATED }
const MAX_HEALTH := 3
const GRAVITY := 18.0
const APPROACH_SPEED := 1.65
const ATTACK_DISTANCE := 1.55
const WINDUP_TIME := 0.62
const STRIKE_TIME := 0.24
const RECOVER_TIME := 0.72

@onready var attack_area: Area3D = $AttackArea
@onready var visuals: Node3D = $Visuals
@onready var torso: MeshInstance3D = $Visuals/Torso
@onready var head: MeshInstance3D = $Visuals/Head
@onready var left_arm: MeshInstance3D = $Visuals/LeftArm
@onready var right_arm: MeshInstance3D = $Visuals/RightArm
@onready var left_leg: MeshInstance3D = $Visuals/LeftLeg
@onready var right_leg: MeshInstance3D = $Visuals/RightLeg

var health := MAX_HEALTH
var state := FightState.APPROACH
var state_time := 0.0
var attack_cooldown := 0.45
var attack_hit_player := false
var _player: CharacterBody3D
var _knockback := Vector3.ZERO
var _base_torso_material: StandardMaterial3D

func _ready() -> void:
	add_to_group("playable_enemy")
	attack_area.monitoring = true
	attack_area.monitorable = false
	attack_area.set_collision_layer_value(1, false)
	attack_area.set_collision_layer_value(2, false)
	attack_area.set_collision_mask_value(1, true)
	attack_area.set_collision_mask_value(2, false)
	_base_torso_material = torso.material_override as StandardMaterial3D
	for part in [head, left_arm, right_arm, left_leg, right_leg]:
		var material := StandardMaterial3D.new()
		material.albedo_color = Color("#93a2aa")
		material.roughness = 0.78
		part.material_override = material
	var torso_material := StandardMaterial3D.new()
	torso_material.albedo_color = Color("#80909a")
	torso_material.roughness = 0.82
	torso.material_override = torso_material
	_base_torso_material = torso_material

func set_player(player: CharacterBody3D) -> void:
	_player = player

func _physics_process(delta: float) -> void:
	if state == FightState.DEFEATED:
		velocity.x = move_toward(velocity.x, 0.0, 6.0 * delta)
		velocity.z = move_toward(velocity.z, 0.0, 6.0 * delta)
		velocity.y -= GRAVITY * delta
		move_and_slide()
		_update_visual_reaction()
		return
	attack_cooldown = maxf(0.0, attack_cooldown - delta)
	state_time = maxf(0.0, state_time - delta)
	var planar := Vector3.ZERO
	if _player != null and not _player.is_defeated():
		var offset: Vector3 = _player.global_position - global_position
		offset.y = 0.0
		var distance := offset.length()
		if distance > 0.01:
			look_at(global_position + offset, Vector3.UP)
		if state == FightState.APPROACH:
			if distance > ATTACK_DISTANCE:
				planar = offset.normalized() * APPROACH_SPEED
			elif attack_cooldown <= 0.0:
				state = FightState.WINDUP
				state_time = WINDUP_TIME
				attack_hit_player = false
		elif state == FightState.WINDUP:
			if state_time <= 0.0:
				state = FightState.STRIKE
				state_time = STRIKE_TIME
				attack_hit_player = false
		elif state == FightState.STRIKE:
			left_arm.rotation.x = -1.05
			right_arm.rotation.x = -1.05
			if not attack_hit_player and attack_area.overlaps_body(_player):
				attack_hit_player = true
				_player.take_damage(1, global_position)
			if state_time <= 0.0:
				state = FightState.RECOVER
				state_time = RECOVER_TIME
				attack_cooldown = 1.0
		elif state == FightState.RECOVER:
			if state_time <= 0.0:
				state = FightState.APPROACH
		elif state == FightState.STAGGERED:
			if state_time <= 0.0:
				state = FightState.APPROACH
				attack_area.monitoring = true
		elif distance > ATTACK_DISTANCE + 0.1:
			state = FightState.APPROACH
	else:
		state = FightState.APPROACH
		left_arm.rotation.x = 0.0
		right_arm.rotation.x = 0.0
	velocity.x = planar.x + _knockback.x
	velocity.z = planar.z + _knockback.z
	_knockback = _knockback.move_toward(Vector3.ZERO, 13.0 * delta)
	if not is_on_floor():
		velocity.y -= GRAVITY * delta
	else:
		velocity.y = maxf(velocity.y, -0.2)
	move_and_slide()
	_update_visual_reaction()

func receive_hit(source_position: Vector3, damage: int) -> void:
	if state == FightState.DEFEATED:
		return
	health = maxi(0, health - damage)
	var away := global_position - source_position
	away.y = 0.0
	_knockback = away.normalized() * 4.7 + Vector3.UP * 0.85
	state = FightState.DEFEATED if health == 0 else FightState.STAGGERED
	state_time = 0.55
	attack_area.monitoring = false
	attack_cooldown = 0.7
	health_changed.emit(health, MAX_HEALTH)
	if health == 0:
		collision_layer = 0
		collision_mask = 1
		defeated.emit()

func reset_enemy(at_position: Vector3, player: CharacterBody3D) -> void:
	global_position = at_position
	velocity = Vector3.ZERO
	_knockback = Vector3.ZERO
	health = MAX_HEALTH
	state = FightState.APPROACH
	state_time = 0.0
	attack_cooldown = 0.45
	attack_hit_player = false
	_player = player
	collision_layer = 2
	collision_mask = 3
	attack_area.monitoring = true
	health_changed.emit(health, MAX_HEALTH)
	_update_visual_reaction()

func state_name() -> String:
	return FightState.keys()[state]

func is_defeated() -> bool:
	return state == FightState.DEFEATED

func _update_visual_reaction() -> void:
	var stagger := state == FightState.STAGGERED
	if state == FightState.STRIKE:
		visuals.rotation.z = move_toward(visuals.rotation.z, -0.22, 8.0 * get_physics_process_delta_time())
	elif stagger:
		visuals.rotation.z = -0.32 if _knockback.x > 0.0 else 0.32
	elif state == FightState.DEFEATED:
		visuals.rotation.z = move_toward(visuals.rotation.z, PI * 0.42, 3.0 * get_physics_process_delta_time())
	else:
		visuals.rotation.z = move_toward(visuals.rotation.z, 0.0, 2.5 * get_physics_process_delta_time())
		left_arm.rotation.x = move_toward(left_arm.rotation.x, 0.0, 5.0 * get_physics_process_delta_time())
		right_arm.rotation.x = move_toward(right_arm.rotation.x, 0.0, 5.0 * get_physics_process_delta_time())
	if _base_torso_material != null:
		_base_torso_material.albedo_color = Color("#e84d38") if stagger else Color("#80909a")
