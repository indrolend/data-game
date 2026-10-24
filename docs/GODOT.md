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
