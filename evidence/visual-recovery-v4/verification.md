# Visual Recovery V4 verification

## Commands and results

- `cmake --build build-godot47 --config Release --parallel` — exit 0.
- `godot_console.exe --headless --path godot --script res://tests/phone_presentation.gd` — exit 0.
- `godot_console.exe --path godot --resolution 1280x720 --script res://tests/phone_raster_visual_capture.gd` — exit 0 on Godot 4.7.2, OpenGL 3.3, NVIDIA GeForce RTX 3060.
- `.\tests\verify.ps1 -GodotExe godot_console.exe` — exit 0.
- Parent/Godot parity — 454 snapshots match within tolerance `1E-05`.

## Independently observed

- Raster size is exactly 3,686,400 RGBA bytes (720x1280x4).
- Changing native selection changes both the native revision and raster byte hash.
- Godot presents the raster texture and hides the prior Label3D menu substitute.
- Native Resume, Settings, Graphics, preset, and shadow actions still execute.
- All three 1280x720 GPU frames were inspected at original resolution. Text is readable, selection is distinct, and the complete handset is visible after the offset correction.

## Capture SHA-256

- `00-pause-resume-selected.png`: `E41654649F79ABAEFAF2CA5E2B47CB949229B66DBC8A7820AF5A2E8CA7B4AB19`
- `01-pause-settings-selected.png`: `8D8AA38462DCE8BCB6B9310C47D7FE77C62A2D1F02706FE78C92DE7BCE4A22C8`
- `02-settings-page.png`: `0A4714B26A4AC5E866B281C3A0C4FE8034B4EC39A9F21FB82E9DD0839707FAFF`

All three corrected Source Sans frames were reinspected at original resolution:
the handset and text are centered, readable, and unobstructed. Exact native
desktop parity remains unverified because state correspondence is approximate
and the animated stencil/paint path is not yet matched.
