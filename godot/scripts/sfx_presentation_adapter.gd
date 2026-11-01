extends Node

const CUE_PATHS := [
	"res://audio/vc_ended.mp3",
	"res://audio/vc_invitation.mp3",
	"res://audio/connect_power.mp3",
	"res://audio/low_power.mp3",
	"res://audio/negative_ack.mp3",
	"res://audio/received_message.mp3",
	"res://audio/sent_message.mp3",
	"res://audio/phone_attack.mp3",
	"res://audio/payment_success.mp3",
	"res://audio/payment_failure.mp3",
	"res://audio/end_call_tone.mp3",
	"res://audio/slurp_ringtone.mp3",
	"res://audio/slurp_ringtone.mp3",
	"res://audio/capture_1.mp3",
	"res://audio/capture_2.mp3",
	"res://audio/capture_3.mp3",
	"res://audio/capture_4.mp3",
	"res://audio/capture_5.mp3",
	"res://audio/headshot.mp3",
	"res://audio/headshot_critical.mp3",
	"res://audio/reward_woah.mp3",
	"res://audio/reward_nice.mp3",
]
const SLURP_RINGTONE_START := 11
const SLURP_RINGTONE_STOP := 12
const VOICE_COUNT := 16

var voices: Array[AudioStreamPlayer] = []
var ringtone: AudioStreamPlayer
var streams: Array[AudioStream] = []
var next_voice := 0
var last_serial := 0
var sfx_volume := 0.0
var audio_output_enabled := true
var played_serials: Array[int] = []

func _ready() -> void:
	audio_output_enabled = DisplayServer.get_name() != "headless"
	for path in CUE_PATHS:
		var stream := load(path) as AudioStream
		if stream == null:
			push_error("Missing native SFX asset: " + path)
		streams.append(stream)
	for index in range(VOICE_COUNT):
		var voice := AudioStreamPlayer.new()
		voice.name = "NativeSfxVoice_%02d" % index
		add_child(voice)
		voices.append(voice)
	ringtone = AudioStreamPlayer.new()
	ringtone.name = "NativeSlurpRingtone"
	add_child(ringtone)

func _exit_tree() -> void:
	stop_all()

func sync_snapshot(snapshot: Dictionary) -> void:
	sfx_volume = clampf(float(snapshot.get("volume", 0.0)), 0.0, 1.0)
	var newest := int(snapshot.get("newest_serial", 0))
	if newest < last_serial:
		last_serial = 0
		stop_all()
	for value in snapshot.get("events", []):
		var event := value as Dictionary
		var serial := int(event.get("serial", 0))
		if serial <= last_serial or serial > newest:
			continue
		_play_event(event)
		last_serial = serial
	if newest > last_serial:
		last_serial = newest

func _play_event(event: Dictionary) -> void:
	var cue := int(event.get("cue", -1))
	if cue < 0 or cue >= streams.size():
		return
	played_serials.append(int(event.get("serial", 0)))
	if cue == SLURP_RINGTONE_STOP:
		ringtone.stop()
		ringtone.stream = null
		return
	var linear_volume := clampf(float(event.get("volume", 0.0)) * sfx_volume, 0.0, 1.0)
	if cue == SLURP_RINGTONE_START:
		ringtone.stop()
		ringtone.stream = streams[cue]
		if ringtone.stream is AudioStreamMP3:
			(ringtone.stream as AudioStreamMP3).loop = true
		ringtone.volume_db = linear_to_db(maxf(linear_volume, 0.0001))
		if audio_output_enabled and linear_volume > 0.0:
			ringtone.play()
		return
	var voice := voices[next_voice % voices.size()]
	next_voice += 1
	voice.stop()
	voice.stream = streams[cue]
	voice.volume_db = linear_to_db(maxf(linear_volume, 0.0001))
	if audio_output_enabled and linear_volume > 0.0:
		voice.play()

func stop_all() -> void:
	for voice in voices:
		voice.stop()
		voice.stream = null
	if is_instance_valid(ringtone):
		ringtone.stop()
		ringtone.stream = null

func cue_path(cue: int) -> String:
	return CUE_PATHS[cue] if cue >= 0 and cue < CUE_PATHS.size() else ""
