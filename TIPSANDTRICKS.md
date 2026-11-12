# Tips and Tricks

## Native development

- Entry point: Invoke-DataGame.ps1
- Build configuration: CMakeLists.txt
- Native application: desktop/main.cpp
- Simulation: game/Game.cpp and game/Game.hpp
- Renderer: desktop/DesktopRenderer.cpp

### Developer Mode

- Configure with `DB_ENABLE_DEVELOPER_CONSOLE=ON` and keep storefront builds at the default `OFF`.
- Open the in-game console with the grave accent key.
- `lighting show` reports lighting-control overrides; `lighting input time <seconds>` freezes time and `lighting input time live` releases it.
- `shadow show` reports requested/effective shadow paths and mapped readiness.
- Isolate paths with `shadow mode off|cheap|normal|pretty|auto`.
- Tune mapped shadows with `shadow strength <0..3>`, `shadow resolution 0|256|512|1024|2048|4096`, `shadow bias <factor> <units>`, and `shadow coverage <0.25..4>`.
- Inspect the depth texture with `shadow depth-map on` and caster geometry with `shadow geometry on`; use the corresponding `off` commands to return to gameplay.
- `shadow reset` restores renderer-owned defaults without changing authoritative gameplay state.
- Deterministic captures can apply the same controls before the first frame with repeatable `--shadow-command "shadow ..."` arguments.
- Inspect or load deterministic rooms with `environment show`, `environment catalog`, `environment premise <name|0..7>`, and `environment seed <seed> [room]`; `environment next` and `environment reroll` use the existing room debug controls.

## Verification

- Inspect Git status before changing files.
- Prefer existing native tests and performance traces.
- Compare identical fixtures when measuring performance.
- Distinguish simulation failures from rendering performance.
- Preserve generated evidence under ignored artifact directories.
- For an authentic native-to-Godot soul-capture check, run
  `godot/scripts/agent_playtest.gd` with
  `godot/tests/fixtures/agent_authentic_soul_capture.commands` and
  `--authentic-playthrough`. The fixture uses only player inputs, asserts one
  stored soul, emits an authoritative digest at every observation, and writes
  a synchronized PNG.
- Compare schema 5 `state_digest` sequences before comparing screenshots. The
  first digest mismatch is an authoritative gameplay divergence; matching
  digests with different frames or transforms isolates presentation behavior.

## PowerShell

Copy concise diagnostic output while preserving terminal visibility:

    $out = <command> | Out-String
    $out | Set-Clipboard
    $out
    '✓ copied'

## Historical references

- docs/archive/STRUCTURAL_POLISH.md
