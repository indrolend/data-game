# Regression results

- `cmake --build build-godot47 --config Release --parallel`: exit 0.
- `godot_console.exe --headless --path godot --script res://tests/human_model_presentation.gd`: exit 0.
- `godot_console.exe --headless --path godot --script res://tests/lighting_presentation.gd`: exit 0.
- `godot_console.exe --headless --path godot --script res://tests/room_presentation.gd`: exit 0.
- `./tests/verify.ps1`: exit 0; configure, native build, headless behaviors, Godot startup, and parent parity passed.
- Graphical 9-frame Field/Coastal/Sterile command gauntlet: exit 0 on RTX 3060; actual framebuffer PNGs saved under `after-initial-milestone/`.

The existing modified `tests/verify.ps1`, untracked menu test, and their retained evidence predate this work and were not overwritten.
