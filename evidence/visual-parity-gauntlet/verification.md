# Verification record

## Commands and outcomes

- Native: `Invoke-DataGame.ps1 -Action test -Configuration Release` — build succeeded; CTest completed 55/55 tests, zero failures. The streaming wrapper stopped displaying before the final summary, so `Testing/Temporary/LastTest.log` was checked through test 55 (`SignalResidueLifecycleTest`, passed).
- Godot: `./tests/verify.ps1` — exit 0, reporting configure, native build, headless behaviors, Godot startup, and parent parity PASS.
- Native graphical evidence, lighting: `Invoke-DataGame.ps1 -Action evidence -Scenario lighting-comparison -SkipBuild` — exit 0; 9 real renderer frames, 600 ticks, result classification pass.
- Native graphical evidence, soul lifecycle: same command with `-Scenario soul-lifecycle` — exit 0; 20 frames, 383 ticks, classification pass.
- Godot graphical evidence: four `godot_console.exe --path godot --script res://scripts/agent_playtest.gd` invocations with absolute command paths — all exit 0. Renderer/device were reported and viewport PNGs were saved.
- Four earlier Godot invocations with relative command paths exited 1 after saving only initial frame zero; retained under directories without `-ok` and excluded from successful counts.

## What is proven

Compilation, native tests, Godot headless/structured parity, graphical renderer initialization, actual framebuffer capture, and deterministic simulation progression are separately evidenced. The native capture manifests contain camera, room, quality, lighting and tick metadata. Godot stdout contains fixed ticks and player/camera-mode gameplay fields.

## Limitations

- No current interface serializes one complete current-native `GameState` and loads it into the migration, so there are zero genuine cross-runtime matched frames.
- Native product scenarios cover Sterile lighting and soul lifecycle, not all ten requested rooms/UI states.
- Godot agent telemetry does not print room seed/index, camera transform/FOV, lighting snapshot, particle count or full entity list, even though those values exist internally. Existing tests verify many of those contracts, but they are not attached to each PNG.
- Field nighttime and a synchronized room-transition framebuffer remain unverified.
- Numerical luma measurements are diagnostic only; they do not establish visual parity.
- No feature or shader changes were made.
