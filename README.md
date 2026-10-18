# Digital Breakdown → Godot: first native integration spike

**Status: GDExtension build, headless Godot startup, deterministic input checks, and fixed-scenario parent parity are verified. Visual presentation remains unverified.**

This project keeps the original parent `Game.cpp` and `Game.hpp` and their gameplay headers unchanged. A tiny C++ `ParentBridge` calls `Game::setTouchControls`, `Game::update(1/60)`, and reads `Game::state()` directly. `ParentGameNode` exposes the original state to Godot. Godot draws *temporary primitive* phone/enemy meshes; this is not the parent OpenGL renderer or an original asset conversion.

## Build on Windows

1. Install Godot 4.x and CMake with a C++ compiler. Obtain the official `godot-cpp` source branch/tag matching your Godot version: https://github.com/godotengine/godot-cpp .
2. From this folder, run:

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DGODOT_CPP_DIR="C:/path/to/godot-cpp"
cmake --build build --config Release --parallel
```

3. Open the `godot/` directory in the Godot editor and run the main scene. If Godot fails to load the extension, check the compiled DLL, ABI compatibility, and the `.gdextension` library path.

## Initial controls

WASD movement, Shift sprint, Space jump, E vacuum, F melee, Q shoot, C camera toggle, and captured mouse-look. Original asset import, full HUD/audio, and scene-authoring parity are not implemented.

## Verification

Run `.\tests\verify.ps1` from PowerShell. It configures and builds the extension and direct-parent harness, runs headless behavior/startup checks, and compares structured parent-versus-Godot snapshots. Logs are written under `evidence/`.

The default parent fixture has no stored souls. Parent-owned rally and room-inspector fixtures verify soul consumption, delayed projectile creation, 20 live targets, and deterministic target-state snapshots over neutral ticks. The targets remained stationary in the tested scenario. Target defeat, capture, and soul ingestion remain unsupported because the current fixture does not align a controlled interaction.
