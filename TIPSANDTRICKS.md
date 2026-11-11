# Tips and Tricks

## Native development

- Entry point: Invoke-DataGame.ps1
- Build configuration: CMakeLists.txt
- Native application: desktop/main.cpp
- Simulation: game/Game.cpp and game/Game.hpp
- Renderer: desktop/DesktopRenderer.cpp

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
