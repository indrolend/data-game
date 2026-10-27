extends CanvasLayer

const DoorDatamoshShader = preload("res://shaders/door_datamosh.gdshader")

var overlay: ColorRect
var material: ShaderMaterial
var native_snapshot: Dictionary = {}

func _ready() -> void:
    layer = 5
    overlay = ColorRect.new()
    overlay.name = "NativeDoorDatamosh"
    overlay.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
    overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
    material = ShaderMaterial.new()
    material.shader = DoorDatamoshShader
    overlay.material = material
    add_child(overlay)
    overlay.visible = false

func sync_snapshot(snapshot: Dictionary, viewport_size: Vector2) -> void:
    native_snapshot = snapshot.duplicate(true)
    var active := bool(snapshot.get("active", false)) and float(snapshot.get("progress", 0.0)) > 0.018
    overlay.visible = active
    var progress := clampf(float(snapshot.get("progress", 0.0)), 0.0, 1.0)
    var motion: Vector3 = snapshot.get("frame_motion", Vector3.ZERO)
    var motion_pixels := Vector2(
        clampf(motion.x * viewport_size.x * 0.045, -20.0, 20.0),
        clampf(-motion.z * viewport_size.y * 0.030, -24.0, 24.0))
    material.set_shader_parameter("strength", progress)
    material.set_shader_parameter("motion_pixels", motion_pixels)
    material.set_shader_parameter("viewport_size", viewport_size)
    overlay.set_meta("native_active", active)
    overlay.set_meta("native_progress", progress)
    overlay.set_meta("native_motion_pixels", motion_pixels)
