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

- `00-pause-resume-selected.png`: `98D5F1A14AA1B3C72E326B4508DDF8FA4FEF4910BB32B3DFD6A35D4C6CD8B188`
- `01-pause-settings-selected.png`: `E448CE1C5202CF6AB6E236E372BFEF117A5876FC014C58C3B0C8937444EB641E`
- `02-settings-page.png`: `CA7B8A8FC91B30DEC75ACA9C31D2E48FB3AE6C9DB4EC5E16B956FBF75A39F275`

Visual readability is inspected and accepted for this bitmap slice. Exact native desktop parity remains unverified because state correspondence is approximate and the desktop uses a different font/stencil path.
