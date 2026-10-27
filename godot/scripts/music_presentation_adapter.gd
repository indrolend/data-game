extends Node

const TRACKS := {
	"title": "res://audio/title_gen.mp3",
	"menu-pensive": "res://audio/menu_pensive.mp3",
	"stage-original": "res://audio/game_music.mp3",
	"stage-turnt": "res://audio/stage_turnt.mp3",
	"stage-wacky": "res://audio/stage_wacky.mp3",
	"boss": "res://audio/boss_theme.mp3",
}

var player: AudioStreamPlayer
var current_key := ""
var last_snapshot: Dictionary = {}
var audio_output_enabled := true

func _ready() -> void:
	audio_output_enabled = DisplayServer.get_name() != "headless"
	player = AudioStreamPlayer.new()
	player.name = "NativeMusicPlayer"
	add_child(player)

func _exit_tree() -> void:
	_release_playback()

func _release_playback() -> void:
	if not is_instance_valid(player):
		return
	player.stop()
	player.stream = null

func sync_snapshot(snapshot: Dictionary) -> void:
	last_snapshot = snapshot.duplicate(true)
	var key := str(snapshot.get("key", ""))
	var should_play := bool(snapshot.get("playing", false)) and TRACKS.has(key)
	if not should_play:
		_release_playback()
		current_key = ""
		return
	if key != current_key:
		var stream := load(TRACKS[key])
		if stream == null:
			push_error("Missing native music asset: " + TRACKS[key])
			return
		if stream is AudioStreamMP3:
			(stream as AudioStreamMP3).loop = true
		_release_playback()
		player.stream = stream
		current_key = key
		if audio_output_enabled:
			player.play()
	var base_gain := 0.36 if bool(snapshot.get("menu", false)) else 0.52
	var linear_volume := clampf(float(snapshot.get("volume", 0.0)) * base_gain, 0.0, 1.0)
	player.volume_db = linear_to_db(maxf(linear_volume, 0.0001))

func track_path(key: String) -> String:
	return str(TRACKS.get(key, ""))
