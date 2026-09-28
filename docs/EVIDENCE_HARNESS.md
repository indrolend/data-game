# Evidence harness

The evidence harness joins deterministic simulation facts to images from the real
desktop renderer. It is intended for humans and automated coding agents. The
native executable remains authoritative for both gameplay and presentation.

Run the first scenario from the repository root:

```powershell
.\tools\dbdev.ps1 evidence -Scenario enemy-obstruction
```

Available scenarios:

- `enemy-obstruction`: pursuit, detour, collision clearance, and stall behavior.
- `enemy-contact`: one windup, committed swing, hit, knockback, battery cost,
  and recovery cycle.

Use `-Output PATH` to select a bundle directory. Without it, `dbdev` creates a
unique directory under `artifacts/evidence/<scenario>/` using the timestamp,
commit, and dirty state. Existing manifests are never overwritten.

## Bundle contract

An evidence run contains:

- `manifest.json`: identity, scenario, classification, and relative evidence paths.
- `timeline.ndjson`: one runtime observation for every deterministic tick.
- `events.ndjson`: event-linked visual checkpoints.
- `assertions.json`: behavioral checks and summary measurements.
- `frames/clean`: player-facing frames from the real renderer.
- `frames/diagnostic`: the same ticks with a collider overlay; measurements are
  carried by the matching `timeline.ndjson` row.
- `stdout.log` and `stderr.log`: native process output.
- `evidence.mp4`: a compact checkpoint video when FFmpeg is available.

Every frame filename contains its simulation tick. Each checkpoint event records
both the clean and diagnostic frame paths, so an agent can inspect only the
frames associated with a transition or failure and then read the matching
timeline row.

Evidence runs neither load nor write the user's persistent save. The scenario
owns its complete deterministic fixture.

`timeline.ndjson` currently records enemy position, velocity, goal, goal distance,
speed, progress, support source and normal, obstruction clearance, maximum
detour, stalled ticks, collision overlap, attack state, and finite-value health.

## Exit and failure behavior

Native evidence classifications are:

- `pass`
- `behavior_assertion_failure`
- `visual_capture_failure`
- `protocol_failure`

Encoding runs after native evidence collection. An encoding failure is reported
separately and never discards native telemetry or frames. `dbdev` prints the
bundle path before returning a failure.

The `EnemyEvidenceScenarioTest` target consumes the same scenario setup and
observation code as the executable. This prevents the test fixture from becoming
a parallel description of runtime behavior.

## Adding a scenario

Prefer one small deterministic scenario at a time:

1. Put shared setup and observation logic under `native/game/diagnostics/`.
2. Add a focused CTest target that consumes that exact logic.
3. Add a native evidence runner that advances `Game::update` at 60 Hz.
4. Capture only start, transition, failure, and completion checkpoints.
5. Expose the closed-set scenario name through `dbdev.ps1`.

Do not add a second renderer, simulation, input authority, database, or scripting
language. The value of the harness is synchronized evidence from code that ships.
