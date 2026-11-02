# Godot presentation

The Godot client is part of this repository and presents the same authoritative
`game/Game.cpp` simulation used by the desktop executable. It does not carry a
second copy of gameplay source.

Build the optional GDExtension with a matching `godot-cpp` checkout:

```powershell
cmake -S . -B build-godot -DDB_BUILD_GODOT_PRESENTATION=ON `
  "-DGODOT_CPP_DIR=C:/path/to/godot-cpp" `
  "-DGODOTCPP_DEFAULT_API_VERSION=4.7"
cmake --build build-godot --config Release --target DigitalBreakdownGodot --parallel
godot_console.exe --headless --editor --path godot --quit
godot_console.exe --headless --path godot --script res://tests/parent_smoke.gd
```

The editor-import step creates ignored `.godot/` metadata and registers the
GDExtension for a newly materialized checkout.

The continuous-integration macOS job builds and executes the native Apple
Silicon GDExtension from pinned `godot-cpp` commit
`2bb1acf6f8ae1ce96ac7b9ba3a10b499c59c49ec`, runs it with Godot 4.7.2, and
executes the native authority, playable encounter, and sound presentation
smoke tests. This keeps the Godot client covered separately from the native
desktop application release contract, which separately verifies the universal
`x86_64;arm64` desktop bundle.

## Reproducible scene telemetry capture

Run the capture without `--headless` so `render.png` comes from the graphical
viewport rather than being presented as headless GPU evidence:

```powershell
godot_console.exe --path godot --script res://tests/scene_telemetry_capture.gd
```

The deterministic 120-step evidence package is written to
`evidence/generated/scene-telemetry/`. The test enables the persistent skeleton
presentation slice for its own process and does not change native gameplay.
