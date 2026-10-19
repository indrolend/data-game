# Digital Breakdown → Godot migration

**Status: The default Godot scene runs the original C++ gameplay simulation through GDExtension, with Godot providing input and presentation.**

The default Godot scene forwards input to the original `Game` and advances it at its fixed 60 Hz simulation step. Godot presents native player, target, room, phone, and HUD state. The isolated `playable_runtime.tscn` is an exploratory Godot-owned encounter; it is not authoritative, not wired into the default launch, and must not be treated as a faithful port of original gameplay.

The complete subsystem ownership map and dependency order are maintained in `MIGRATION_MATRIX.md`. Native room presentation exposes deterministic room-plan metadata, structural records, material definitions, and stable computed-scene object records. Godot now renders the initial tile's native floor, door frames, selected traversal surface, and the native box-geometry lawn-fragment family. Other computed prop families and generated texture pixels remain unported.

## Build on Windows

1. Install Godot 4.x and CMake with a C++ compiler. Obtain the official `godot-cpp` source branch/tag matching your Godot version: https://github.com/godotengine/godot-cpp .
2. From this folder, run:

```powershell
cmake -S . -B build-godot47 "-DGODOT_CPP_DIR=C:/Users/indro/Projects/godot-cpp" "-DGODOTCPP_DEFAULT_API_VERSION=4.7"
cmake --build build-godot47 --config Release --parallel
```

3. Open the `godot/` directory in the Godot editor and run the main scene. If Godot fails to load the extension, check the compiled DLL, ABI compatibility, and the `.gdextension` library path.

## Play the game

Run `godot_console.exe --path godot` from PowerShell, or open `godot/project.godot` in Godot 4.7.2 and press F6/F5. Controls: WASD move, Shift sprint, Space jump, captured mouse-look, E vacuum, F melee, Q shoot, C camera toggle, and 1–4 communication signals. Escape pauses; click to resume. This launch path uses the original C++ simulation. Its current Godot visuals are incremental presentation work, not a verified pixel-for-pixel renderer port.

## Verification

Run `.\tests\verify.ps1` from PowerShell. It configures/builds the extension and independent direct-parent harness, runs native-driven Godot behavior and presentation tests, separately tests the exploratory encounter, checks default-scene startup, and compares structured parent/Godot snapshots. Logs are written under `evidence/`.

The default parent fixture has no stored souls. Parent-owned rally and room-inspector fixtures verify soul consumption, delayed projectile creation, 20 live targets, and deterministic target-state snapshots over neutral ticks. A controlled 1,050-tick parent-owned interaction also verifies pursuit, armor damage, shell defeat, ingestion, capture, and one stored soul. Communication-signal activation, persistence, and expiry are compared against the direct `Game` harness. These scenarios do not establish parity for every enemy, soul variant, upgrade, or room-progression outcome.
