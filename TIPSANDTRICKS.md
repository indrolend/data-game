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

## PowerShell

Copy concise diagnostic output while preserving terminal visibility:

    $out = <command> | Out-String
    $out | Set-Clipboard
    $out
    '✓ copied'

## Historical references

- docs/archive/STRUCTURAL_POLISH.md
