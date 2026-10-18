# Agent contract — Digital Breakdown Godot migration

Goal: move infrastructure into Godot while retaining the supplied parent C++ gameplay simulation as the sole authority.

## Architecture

- `parent/game/` is the unmodified archive-derived parent gameplay reference truth. Do not edit it unless a concrete parent build/runtime blocker has first been reproduced and documented.
- `native/parent_bridge.hpp` is the only direct adapter used by Godot. It owns one `Game`, forwards semantic input, and advances exactly `1.0f / 60.0f` per `tick()`.
- `native/parent_game_node.*` exposes read-only `GameState` observations and forwards input/ticks. `native/phone_presentation_snapshot.hpp` is a bounded read-only phone projection built only from existing `GameState`; it owns no timers or gameplay state. Do not add gameplay state or Godot physics authority here.
- `godot/scripts/main.gd` forwards input and synchronizes scene presentation. `godot/scripts/phone_presentation_adapter.gd` translates the native phone snapshot into Godot body/screen nodes and indicators. Its first-person camera-relative view-model offset is render-only and never feeds simulation. This is not an original renderer or asset port.
- `tests/parent_reference.cpp` runs `Game` directly, without Godot. Its fixed scenario is the independent reference for `godot/tests/parent_parity.gd`.
- `godot/tests/parent_smoke.gd` explicitly frees every test-created Godot object. Failures must exit nonzero.
- `godot/tests/phone_presentation.gd` checks the native snapshot drives phone dimensions, screen offset/status, third-person transform, and first-person view-model placement; it explicitly frees the test scene and fails nonzero.
- `tests/compare_parity.ps1` compares structured reference and Godot snapshots with a default tolerance of `1e-5`.

## Verification

Run `.\tests\verify.ps1` from the repository root in PowerShell. Equivalent commands are:

```powershell
cmake -S . -B build-godot47 "-DGODOT_CPP_DIR=C:/Users/indro/Projects/godot-cpp" "-DGODOTCPP_DEFAULT_API_VERSION=4.7"
cmake --build build-godot47 --config Release --parallel
godot_console.exe --headless --path godot --script res://tests/parent_smoke.gd
godot_console.exe --headless --path godot --quit-after 2
godot_console.exe --headless --path godot --script res://tests/phone_presentation.gd
.\tests\compare_parity.ps1
```

Keep the `GODOTCPP_DEFAULT_API_VERSION=4.7` definition quoted in PowerShell. An unquoted form was observed to split `4.7` and incorrectly select `extension_api-4.json`.

Logs belong in `evidence/`. Do not report a pass unless the command returned exit code zero.

## Current scope and rules

- Verified headlessly: deterministic repeatability, movement, sprint, jump, mouse-look input semantics, camera toggle, vacuum activation while grounded, melee/battery coupling, frame progression, target observation, phone snapshot contracts, and direct-parent/Godot fixed-scenario parity.
- The default fixture begins with zero stored souls. The `start_rally_fixture()` test hook delegates to the original `Game::debugStartRallyLab()` and seeds one soul; headless checks and parity cover soul consumption and delayed projectile creation. The `start_target_fixture()` hook delegates to the original room-inspector enemy setup; headless checks and parity cover 20 live targets and deterministic target-state snapshots over neutral ticks. Those fixture targets remain stationary in that scenario. A separate 1,050-tick controlled interaction sequence covers shell defeat, ingestion, capture, and one soul entering inventory; it does not establish every enemy, soul variant, upgrade, or room-progression outcome.
- Godot synchronizes temporary player/target meshes, native camera state, a phone body/screen/indicator adapter, and a minimal native-state HUD. It does not own collision or gameplay simulation. GPU captures in `evidence/phone-presentation-final/` show the phone in both camera modes; the screen content/body remain a minimal Godot presentation, not original renderer parity.
- Preserve existing evidence and dirty files. Do not weaken assertions, invent success, or hardcode expectations derived only from the Godot-facing runtime.
- Do not claim original renderer/assets, audio, complete controls, or full gameplay parity. Headless startup is not visual verification; graphical claims require inspecting retained GPU captures and identifying the exact scenario.
