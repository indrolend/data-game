# Verification record — 2026-10-17

Environment: Windows, Godot 4.7.2, MSVC via Visual Studio 18 2026, `godot-cpp` at `C:/Users/indro/Projects/godot-cpp`.

## Results

- Native GDExtension and direct-parent harness: PASS.
- Headless Godot startup: PASS. This proves startup only, not visible rendering.
- Deterministic repeatability: PASS for two independent 120-tick movement runs.
- Supported behavior checks: PASS for movement, sprint, jump, look deltas, camera switching, grounded vacuum activation, melee pose/battery use, frame progression, stored-soul consumption, delayed projectile creation, live target observation, deterministic target-state snapshots over neutral ticks, and a controlled parent-owned target interaction.
- Target interaction: PASS. The controlled fixture resets the original `Game`, selects its nearest live target, and sends only semantic input plus the existing `debugFillBattery()` test hook. By frame 600 the target has moved in pursuit; by frame 900 melee has reduced armor from `2.0` to `1.18`; by frame 960 armor is zero and the target is slurpable; by frame 1020 vacuum ingestion is `0.3319208`; and at frame 1050 the target is no longer alive, the live-target count has changed from 9 to 8, and one soul is stored. The snapshot retains the final pre-removal ingestion value (`0.9246550`) because capture commits once the parent threshold is reached.
- Parent-versus-Godot parity: PASS for 20 structured records: 9 movement/action snapshots (frames 0 through 240 at 30-frame intervals), 3 rally projectile lifecycle records, 2 room-inspector target records (fixture startup and after 120 neutral ticks), and 6 interaction records (frames 0, 600, 900, 960, 1020, and 1050). Numeric tolerance is `1e-5`. Interaction comparison includes selected target index, position, armor, slurpable/ingestion/alive state, aggregate live targets, stored souls, and battery.
- Test-created object lifetime: corrected with explicit `free()` calls; current headless logs contain no leak warning.

## Commands

```powershell
.\tests\verify.ps1
```

The script runs:

```powershell
cmake -S . -B build-godot47 "-DGODOT_CPP_DIR=C:/Users/indro/Projects/godot-cpp" "-DGODOTCPP_DEFAULT_API_VERSION=4.7"
cmake --build build-godot47 --config Release --parallel
godot_console.exe --headless --path godot --script res://tests/parent_smoke.gd
godot_console.exe --headless --path godot --quit-after 2
.\tests\compare_parity.ps1
```

The first configure attempt used an unquoted API definition; PowerShell split the value, and godot-cpp attempted to open nonexistent `gdextension/extension_api-4.json`. Quoting the complete definition selected `extension_api-4-7.json`. No parent source edit was needed.

Generated logs: `cmake-configure.log`, `native-build.log`, `headless-smoke.log`, `godot-startup.log`, `parent-reference.csv`, `godot-parity.log`, and `parity-result.log`.

No file under `parent/game/` was modified. The fixture adapter only delegates to existing public `Game` methods; pursuit, damage, defeat, vacuum motion, capture, and soul storage remain parent-owned.
