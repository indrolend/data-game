extends Control

# Godot rendering of the original DesktopRenderer::drawHud() state. All pulse,
# crosshair, target, and ticker timing is supplied by the native Game snapshot.
const CYAN := Color(0.470588, 0.835294, 0.882353)
const MAGENTA := Color(0.596078, 0.117647, 0.592157)
const CHARTREUSE := Color(0.831373, 0.925490, 0.556863)
const DEEP_PLUM := Color(0.376471, 0.247059, 0.356863)
const COPPER := Color(0.666667, 0.501961, 0.400000)
const SIGNAL_GREEN := Color(0.301961, 0.909804, 0.556863)
const WARM_GOLD := Color(0.882353, 0.721569, 0.498039)
const TARGET_GLYPHS := "01ABCDEFHIKMNPRSTXYZ+-/:"

var snapshot: Dictionary = {}
var restart_menu: Dictionary = {}
var target_snapshots: Array[Dictionary] = []
var gameplay_camera: Camera3D
var player_position := Vector3.ZERO
var comm_signal := 0
var comm_signal_timer := 0.0
var simulation_time := 0.0

func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_IGNORE

func sync_snapshot(next_snapshot: Dictionary, next_restart_menu: Dictionary,
        next_targets: Array[Dictionary], camera: Camera3D,
		native_player_position: Vector3, native_comm_signal: int, native_comm_signal_timer: float,
		native_time: float) -> void:
	snapshot = next_snapshot
	restart_menu = next_restart_menu
	target_snapshots = next_targets
	gameplay_camera = camera
	player_position = native_player_position
	comm_signal = native_comm_signal
	comm_signal_timer = native_comm_signal_timer
	simulation_time = native_time
	queue_redraw()

func _draw() -> void:
	if snapshot.is_empty():
		return
	if bool(restart_menu.get("active", false)):
		_draw_restart_menu()
		return
	if not bool(snapshot.get("started", false)):
		return
	# Match the original renderer's bounded logical HUD canvas.
	var hud_scale := clampf(minf(size.x / 1280.0, size.y / 720.0), 1.0, 2.5)
	draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE * hud_scale)
	var width := size.x / hud_scale
	var height := size.y / hud_scale
	var upgrade: Dictionary = snapshot.get("upgrade", {})
	if bool(upgrade.get("active", false)):
		_draw_upgrade_menu(width, height, upgrade)
		return
	if bool(snapshot.get("dead", false)) or bool(snapshot.get("ui_paused", false)):
		return
	_draw_target_markers(width, height, hud_scale)
	_draw_communication_signal(width, height, hud_scale)
	_draw_battery_and_objective(width, height)
	_draw_critical_pulse(width, height)
	_draw_energy_ticker(width)
	_draw_native_reticle(width, height)

func _draw_restart_menu() -> void:
	var ui_scale := float(restart_menu.get("ui_scale", 1.0))
	draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE * ui_scale)
	var width := float(restart_menu.get("canvas_width", size.x))
	var height := float(restart_menu.get("canvas_height", size.y))
	var fade := clampf(float(restart_menu.get("fade", 0.0)), 0.0, 1.0)
	draw_rect(Rect2(0.0, 0.0, width, height), Color(0.0, 0.0, 0.0, 0.18 * fade))
	var pulse := clampf(float(restart_menu.get("pulse", 0.5)), 0.0, 1.0)
	var selection := int(restart_menu.get("selection", 0))
	for row in restart_menu.get("rows", []):
		var label := str(row.get("label", ""))
		var selected := int(row.get("selection", -1)) == selection
		var text_scale := float(row.get("text_scale", 1.0))
		var center_x := float(row.get("center_x", width * 0.5))
		var baseline_y := float(row.get("baseline_y", height * 0.56))
		# DesktopRenderer advances every bitmap character by six source pixels,
		# including unsupported punctuation such as the question mark in Again?.
		var text_width := label.length() * 6.0 * text_scale
		var color := Color(1.0, 1.0, 1.0, (0.98 if selected else 0.54) * fade)
		if not selected:
			color = Color(0.70, 0.88, 1.0, 0.54 * fade)
		if selected:
			var hit: Rect2 = row.get("hit", Rect2())
			draw_rect(Rect2(hit.position.x + 10.0, baseline_y + 3.5 * text_scale,
				7.0, 7.0), Color(CYAN, (0.78 + 0.16 * pulse) * fade))
		_draw_native_bitmap_text(label,
			Vector2(center_x - text_width * 0.5, baseline_y), text_scale, color, fade)

func _native_bitmap_glyph(character: String) -> Array[int]:
	# Exact rows from game/BitmapFont.hpp for the restart vocabulary.
	# Unknown glyphs intentionally remain blank while retaining their advance.
	match character.to_upper():
		"A": return [14, 17, 17, 31, 17, 17, 17]
		"G": return [14, 17, 16, 23, 17, 17, 15]
		"I": return [31, 4, 4, 4, 4, 4, 31]
		"N": return [17, 25, 21, 19, 17, 17, 17]
		"Q": return [14, 17, 17, 17, 21, 18, 13]
		"T": return [31, 4, 4, 4, 4, 4, 4]
		"U": return [17, 17, 17, 17, 17, 17, 14]
		_: return [0, 0, 0, 0, 0, 0, 0]

func _draw_native_bitmap_text(value: String, origin: Vector2, glyph_scale: float,
		color: Color, overlay_alpha: float) -> void:
	var pen_x := origin.x
	for character in value:
		var rows := _native_bitmap_glyph(character)
		for row_index in range(7):
			var bits: int = rows[row_index]
			for column in range(5):
				if (bits & (1 << (4 - column))) == 0:
					continue
				var pixel := Rect2(pen_x + column * glyph_scale,
					origin.y + row_index * glyph_scale, glyph_scale, glyph_scale)
				if overlay_alpha < 0.999:
					var shadow := Color(0.0, 0.0, 0.0, color.a)
					draw_rect(Rect2(pixel.position + Vector2(-1.0, 0.0), pixel.size), shadow)
					draw_rect(Rect2(pixel.position + Vector2(1.0, 0.0), pixel.size), shadow)
					draw_rect(Rect2(pixel.position + Vector2(0.0, -1.0), pixel.size), shadow)
					draw_rect(Rect2(pixel.position + Vector2(0.0, 1.0), pixel.size), shadow)
				draw_rect(pixel, color)
		pen_x += 6.0 * glyph_scale

func _draw_upgrade_menu(width: float, height: float, upgrade: Dictionary) -> void:
	var reveal := smoothstep(0.0, 0.42, float(upgrade.get("presentation_time", 0.0)))
	draw_rect(Rect2(0.0, 0.0, width, height), Color(0.0, 0.012, 0.018, 0.54))
	var panel_width := minf(680.0, width - 24.0)
	var panel_height := 300.0
	var panel := Rect2((width - panel_width) * 0.5, (height - panel_height) * 0.5,
		panel_width, panel_height)
	draw_rect(panel, Color(0.008, 0.022, 0.030, 0.70 * reveal))
	draw_rect(Rect2(panel.position.x + 18.0, panel.position.y + 8.0,
		(panel_width - 36.0) * reveal, 2.0), Color(0.47, 0.84, 0.88, 0.66 * reveal))
	_draw_text("ROOM %d / RECOMPILE" % int(upgrade.get("room_index", 0)),
		Vector2(panel.position.x + 18.0, panel.position.y + 35.0), 18, Color(0.82, 0.97, 1.0, 0.96 * reveal))
	var host_text := "WAITING FOR HOST" if bool(upgrade.get("remote_choice", false)) else "RUN MUTATION / SELECT ONE"
	_draw_text(host_text, Vector2(panel.position.x + 18.0, panel.position.y + 57.0),
		12, Color(0.62, 0.88, 0.94, 0.88 * reveal))
	var labels := ["SHOT", "LUNGE", "ATTACK"]
	var temporary: Array = upgrade.get("temporary_levels", [])
	var permanent: Array = upgrade.get("permanent_levels", [])
	var temporary_available: Array = upgrade.get("temporary_available", [])
	var permanent_available: Array = upgrade.get("permanent_available", [])
	var cell_width := (panel_width - 24.0) / 3.0
	for item in range(6):
		var track := item % 3
		var is_permanent := item >= 3
		var row_top := 66.0 if not is_permanent else 184.0
		var card_height := 76.0 if not is_permanent else 66.0
		var x := panel.position.x + 12.0 + float(track) * cell_width
		var card := Rect2(x, panel.position.y + row_top, cell_width - 4.0, card_height)
		var selected := int(upgrade.get("selection", 0)) == item
		var available := bool(permanent_available[track]) if is_permanent else bool(temporary_available[track])
		var color := Color(0.83, 0.93, 0.56) if is_permanent else Color(0.47, 0.84, 0.88)
		if not available:
			color = Color(0.22, 0.39, 0.43)
		draw_rect(card, Color(color, (0.15 if selected else 0.035) * reveal))
		draw_rect(Rect2(card.position.x + 9.0, card.end.y - 2.0,
			card.size.x - 18.0, 1.5), Color(color, (0.82 if selected else 0.24) * reveal))
		var label := str(labels[track])
		var label_width := ThemeDB.fallback_font.get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1.0, 18).x
		_draw_text(label, Vector2(card.get_center().x - label_width * 0.5, card.position.y + 29.0),
			18, Color(0.94, 1.0, 1.0, (0.98 if selected else 0.72) * reveal))
		var level := int(permanent[track]) if is_permanent else int(temporary[track])
		var detail := "PERM %d/5" % level if is_permanent else "RUN %d / NEXT %d" % [level, mini(12, level + 1)]
		var detail_width := ThemeDB.fallback_font.get_string_size(detail, HORIZONTAL_ALIGNMENT_LEFT, -1.0, 11).x
		_draw_text(detail, Vector2(card.get_center().x - detail_width * 0.5, card.position.y + 53.0),
			11, Color(color, (0.88 if available else 0.48) * reveal))
	_draw_text("PERMANENT MEMORY / TOKENS %d" % int(upgrade.get("tokens", 0)),
		Vector2(panel.position.x + 18.0, panel.position.y + 174.0), 12,
		Color(0.83, 0.93, 0.56, 0.86 * reveal))
	_draw_text("ARROWS MOVE   ENTER / SPACE SELECT   1-6 QUICK SELECT",
		Vector2(panel.position.x + 18.0, panel.position.y + 291.0), 10,
		Color(0.58, 0.82, 0.88, 0.72 * reveal))

func _draw_text(value: String, position: Vector2, font_size: int, color: Color) -> void:
	draw_string(ThemeDB.fallback_font, position, value, HORIZONTAL_ALIGNMENT_LEFT, -1.0, font_size, color)

func _draw_battery_and_objective(width: float, height: float) -> void:
	var battery := clampf(float(snapshot.get("battery_fill", 0.0)), 0.0, 1.0)
	var low := bool(snapshot.get("low_battery", false))
	var fill_color := COPPER if low else CYAN
	var bar_width := 220.0
	var bar_height := 5.0
	var x := (width - bar_width) * 0.5
	draw_rect(Rect2(x, 22.0, bar_width, bar_height), Color(DEEP_PLUM, 0.54))
	draw_rect(Rect2(x, 22.0, maxf(bar_height, bar_width * battery), bar_height),
		Color(fill_color, 0.96 if low else 0.68))

	var deposited := int(snapshot.get("deposited_souls", 0))
	var room_clear := bool(snapshot.get("room_clear", false))
	if room_clear or deposited > 0:
		var objective := "PATH OPEN" if room_clear else "%d / %d" % [deposited,
			maxi(1, int(snapshot.get("required_souls", 1)))]
		var objective_color := CHARTREUSE if room_clear else CYAN
		draw_string(ThemeDB.fallback_font,
			Vector2(width - 22.0 - _text_width(objective, 1.0), height - 28.0),
			objective, HORIZONTAL_ALIGNMENT_LEFT, -1.0, 12, Color(objective_color, 0.74))

func _draw_native_reticle(width: float, height: float) -> void:
	var center := Vector2(width * 0.5, height * 0.5)
	var spread := float(snapshot.get("crosshair_spread_pixels", 0.0))
	var angle := deg_to_rad(float(snapshot.get("crosshair_rotation_degrees", 0.0)))
	var joining := float(snapshot.get("shoot_join_timer", 0.0)) > 0.0
	var color := Color.WHITE if joining else Color(0.498, 0.906, 1.0)
	color.a = 0.98 * clampf(float(snapshot.get("crosshair_opacity", 0.0)), 0.0, 1.0)
	var offsets := [Vector2(0.0, -spread), Vector2(0.0, spread),
		Vector2(-spread, 0.0), Vector2(spread, 0.0)]
	var sizes := [Vector2(3.0, 14.0), Vector2(3.0, 14.0),
		Vector2(14.0, 3.0), Vector2(14.0, 3.0)]
	for i in range(offsets.size()):
		var translated := _rotate_vector(offsets[i], angle)
		_draw_rotated_rect(center + translated, sizes[i], angle, color)

func _draw_target_markers(width: float, height: float, hud_scale: float) -> void:
	if gameplay_camera == null:
		return
	var pulse := clampf(float(snapshot.get("critical_hit_pulse", 0.0)), 0.0, 1.0)
	var room_index := int(snapshot.get("room_index", 0))
	for target in target_snapshots:
		if not bool(target.get("alive", false)) or bool(target.get("slurpable", false)):
			continue
		var attack_active := bool(target.get("attack_active", false))
		var hit_amount := float(target.get("hit_amount", 0.0))
		if not attack_active and hit_amount <= 0.08:
			continue
		var attack_phase := float(target.get("attack_phase", -1.0)) if attack_active else -1.0
		var variant := int(target.get("attack_variant", 0))
		var head: Vector3 = target.get("head_center", Vector3.ZERO)
		if attack_active and variant >= 2:
			head.y += sin(attack_phase * PI) * 0.035
		if gameplay_camera.is_position_behind(head):
			continue
		var viewport_point := gameplay_camera.unproject_position(head) / hud_scale
		if viewport_point.x < -width * 0.04 or viewport_point.x > width * 1.04 or \
				viewport_point.y < -height * 0.04 or viewport_point.y > height * 1.04:
			continue
		var view_forward := -gameplay_camera.global_basis.z
		var depth := (head - gameplay_camera.global_position).dot(view_forward)
		if depth <= 0.18 or depth > 16.0:
			continue
		var maximum_armor := 4.0 if bool(target.get("brute", false)) else 2.0
		var damage := 1.0 - clampf(float(target.get("armor", 0.0)) / maximum_armor, 0.0, 1.0)
		var perfect_ready := attack_phase >= 0.22 and attack_phase <= 0.46
		var color := Color.from_hsv(fposmod(0.51 + damage * 0.38, 1.0), 1.0, 1.0)
		color = color.lerp(MAGENTA, clampf(pulse * 0.75 + (0.18 if perfect_ready else 0.0), 0.0, 1.0))
		var scale := clampf(8.0 / depth, 0.82, 1.55)
		var marker := (11.0 + damage * 4.0 + (3.0 if perfect_ready else 0.0)) * scale
		var glyph_scale := (1.35 + damage * 0.28 + (0.18 if perfect_ready else 0.0)) * scale
		var alpha := 0.72 + damage * 0.20 + (0.08 if perfect_ready else 0.0)
		var spin := simulation_time * 0.9 + float(target.get("index", 0)) * 0.37
		_draw_rotated_rect(viewport_point, Vector2(marker, marker), PI * 0.25 + spin,
			Color(color, 0.10 + damage * 0.08))
		_draw_rotated_rect(viewport_point + Vector2(-marker, 0), Vector2(marker * 0.52, 2),
			spin * 0.08, Color(color, alpha))
		_draw_rotated_rect(viewport_point + Vector2(marker, 0), Vector2(marker * 0.52, 2),
			spin * 0.08, Color(color, alpha))
		_draw_rotated_rect(viewport_point + Vector2(0, -marker), Vector2(2, marker * 0.52),
			spin * 0.08, Color(color, alpha))
		_draw_rotated_rect(viewport_point + Vector2(0, marker), Vector2(2, marker * 0.52),
			spin * 0.08, Color(color, alpha))
		var glyph_index := int(simulation_time * 10.0 + float(target.get("index", 0)) * 7.0 + room_index * 3) % TARGET_GLYPHS.length()
		var glyph := TARGET_GLYPHS.substr(glyph_index, 1)
		var font := ThemeDB.fallback_font
		var font_size := maxi(1, roundi(7.0 * glyph_scale))
		var text_position := viewport_point - Vector2(font.get_string_size(glyph, HORIZONTAL_ALIGNMENT_LEFT,
			-1.0, font_size).x * 0.5, -font_size * 0.28)
		draw_string(font, text_position + Vector2(1, 1), glyph,
			HORIZONTAL_ALIGNMENT_LEFT, -1.0, font_size, Color(0, 0, 0, alpha * 0.85))
		draw_string(font, text_position, glyph,
			HORIZONTAL_ALIGNMENT_LEFT, -1.0, font_size, Color(color, alpha))

func _draw_communication_signal(width: float, height: float, hud_scale: float) -> void:
	if gameplay_camera == null or comm_signal < 1 or comm_signal > 4 or comm_signal_timer <= 0.0:
		return
	var world_position := player_position + Vector3(0, 1.05, 0)
	if gameplay_camera.is_position_behind(world_position):
		return
	var delta := world_position - gameplay_camera.global_position
	var depth := delta.dot(-gameplay_camera.global_basis.z)
	if depth <= 0.18 or depth > 24.0:
		return
	var point := gameplay_camera.unproject_position(world_position) / hud_scale
	if absf(point.x / width * 2.0 - 1.0) > 1.08 or absf(point.y / height * 2.0 - 1.0) > 1.08:
		return
	var labels := ["", "HELP", "PING", "GROUP", "OK"]
	var colors := [CYAN, MAGENTA, CYAN, CHARTREUSE, WARM_GOLD]
	var label: String = labels[comm_signal]
	var color: Color = colors[comm_signal]
	var fade := clampf(comm_signal_timer / 0.35, 0.0, 1.0)
	var text_scale := clampf(9.0 / depth, 1.15, 2.15)
	var font_size := maxi(1, roundi(7.0 * text_scale))
	var text_width := ThemeDB.fallback_font.get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT,
		-1.0, font_size).x
	var panel_width := text_width + 18.0 * text_scale
	var panel_height := 13.0 * text_scale
	draw_rect(Rect2(point - Vector2(panel_width, panel_height) * 0.5,
		Vector2(panel_width, panel_height)), Color(DEEP_PLUM * 0.12, 0.52 * fade))
	var pulse := 0.5 + 0.5 * sin(simulation_time * 8.0)
	draw_rect(Rect2(point.x - panel_width * 0.5, point.y - panel_height * 0.5,
		panel_width, 1.4 * text_scale), Color(color, (0.58 + 0.18 * pulse) * fade))
	draw_string(ThemeDB.fallback_font, point - Vector2(text_width, -font_size * 0.28) * 0.5,
		label, HORIZONTAL_ALIGNMENT_LEFT, -1.0, font_size, Color(color, 0.96 * fade))

func _draw_critical_pulse(width: float, height: float) -> void:
	var charge := clampf(float(snapshot.get("headshot_kill_charge", 0.0)), 0.0, 1.0)
	var pulse := maxf(float(snapshot.get("headshot_pulse", 0.0)),
		float(snapshot.get("critical_hit_pulse", 0.0)))
	if pulse <= 0.001:
		return
	var eased := pulse * pulse
	var thickness := 12.0 + charge * 9.0 + float(snapshot.get("perfect_pulse", 0.0)) * 4.0
	var violet := Color(0.58, 0.34, 0.92)
	var core := violet.lerp(MAGENTA, 0.72)
	var accent := CYAN.lerp(MAGENTA, 0.46)
	var veil := eased * (0.035 + charge * 0.070)
	draw_rect(Rect2(0, 0, width, thickness), Color(core, veil))
	draw_rect(Rect2(0, height - thickness, width, thickness), Color(accent, veil))
	draw_rect(Rect2(0, 0, thickness, height), Color(accent, veil * 0.90))
	draw_rect(Rect2(width - thickness, 0, thickness, height), Color(core, veil * 0.90))

func _draw_energy_ticker(width: float) -> void:
	var ticker := str(snapshot.get("energy_ticker", ""))
	if ticker.is_empty() or simulation_time >= float(snapshot.get("energy_ticker_until", 0.0)):
		return
	var type := int(snapshot.get("energy_ticker_type", 2))
	var color := COPPER if type == 1 else SIGNAL_GREEN if type == 0 else CYAN
	var font := ThemeDB.fallback_font
	var font_size := 16
	var text_width := font.get_string_size(ticker, HORIZONTAL_ALIGNMENT_LEFT, -1.0, font_size).x
	var panel_width := maxf(118.0, text_width + 16.0)
	var x := (width - panel_width) * 0.5
	draw_rect(Rect2(x, 72.0, panel_width, 18.0), Color(0, 0, 0, 0.54))
	draw_rect(Rect2(x, 72.0, panel_width, 1.0), Color(color, 0.72))
	draw_string(font, Vector2((width - text_width) * 0.5, 85.0), ticker,
		HORIZONTAL_ALIGNMENT_LEFT, -1.0, font_size, color)

func _draw_rotated_rect(center: Vector2, dimensions: Vector2, angle: float, color: Color) -> void:
	var half := dimensions * 0.5
	var corners := PackedVector2Array([
		Vector2(-half.x, -half.y), Vector2(half.x, -half.y),
		Vector2(half.x, half.y), Vector2(-half.x, half.y)])
	for i in range(corners.size()):
		corners[i] = center + _rotate_vector(corners[i], angle)
	draw_colored_polygon(corners, color)

func _rotate_vector(value: Vector2, angle: float) -> Vector2:
	return Vector2(value.x * cos(angle) - value.y * sin(angle),
		value.x * sin(angle) + value.y * cos(angle))

func _text_width(value: String, scale: float) -> float:
	return float(value.length()) * 6.0 * scale
