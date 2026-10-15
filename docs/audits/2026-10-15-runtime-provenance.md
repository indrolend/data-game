# Runtime provenance audit — 2026-10-15

## Executive diagnosis

The principal confirmed failure is provenance, not a single renderer defect. The repository, build cache, running Windows process, and downloaded macOS artifact represented different commits.

Status vocabulary follows the handoff: **VERIFIED**, **PARTIAL**, **UNVERIFIED**, **NOT FIXED**, and **SUPERSEDED**.

### Source → build → executable → runtime identity

| Role | Identity | Evidence | Status |
|---|---|---|---|
| Audit worktree | `codex/structural-polish-shadow-geometry` at `4f5e3a5c717ea28c1cb1105d026e9f26148057bd` | `git status`, `git rev-parse`, upstream ref | VERIFIED |
| Remote main | `137d08e192e3cd8e083208494673060b3af1e0fa` | fetched `origin/main`; merge commit for PR #147 | VERIFIED |
| Canonical local checkout | `C:\Users\indro\Projects\data-game`, local `main` at `52b6c713…` (3 behind `origin/main`), with modified `game/RoomLighting.hpp` | direct status inspection and `rev-list --left-right --count` | VERIFIED; stale and dirty |
| Running Windows executable | `C:\Users\indro\Documents\Codex\2026-10-14\digital-breakdown-master-engineering-handoff-mission\work\data-game\build\bin\Release\DigitalBreakdown.exe` | live process path and embedded build identity | VERIFIED |
| Running Windows build SHA | `d08ba5753e4d7501233551a1b0b00e341f012f90` | executable build identity | VERIFIED; older than branch HEAD |
| Latest structural Windows artifact | `C:\Users\indro\Projects\data-game-tools\artifacts\build\315a2676ab45-Release\bin\Release\DigitalBreakdown.exe` | embedded identity reports `4f5e3a5`; directory label is stale | VERIFIED |
| User-downloaded macOS artifact | workflow run `38506296494`, artifact `desktop-macos-universal` | GitHub Actions run metadata | VERIFIED as source/build SHA `137d08e`; runtime behavior UNVERIFIED |

The build-cache directory name is not authoritative: the `315a2676ab45-Release` directory contains an executable whose embedded identity is `4f5e3a5`. Future tooling must key and validate artifacts by embedded build identity, not directory name.

## Build and renderer configuration

- Desktop uses the shared native `DesktopRenderer` with an OpenGL 2.1 compatibility context (`desktop/main.cpp`). Windows and macOS therefore share the principal renderer source, but driver capability resolution and bundle paths remain platform-specific.
- At `4f5e3a5`, directional shadow-map initialization reports either `supported` or `compatibility fallback` at runtime. Windows evidence reports `supported`. No structural-branch macOS run exists, so macOS support is UNVERIFIED.
- `desktopAssetRoot()` resolves beside the executable on Windows and to `Contents/Resources` inside a macOS app bundle.
- CMake copies the complete `models`, `desktop/audio`, `desktop/fonts`, and `tv` trees into the runtime asset directory and installs the model tree. This is correct source-side staging logic; successful CI packaging does not prove that a particular mesh loaded during a Mac frame.
- Runtime authored-city logging on the structural Windows build reports `wall-window`, `wall-door`, `corner`, and `crown` modules loaded. These are CC0-derived DBM1 meshes recorded in the project provenance documents.

## Runtime evidence and tests

The structural branch has deterministic native captures under `artifacts/structural-polish`, including City shadow off/Normal/Pretty, sunset, night, Field, Coastal, Sterile, and continuous video. The captured Windows log reports directional shadow-map support and authored module loading.

The branch previously completed 54/54 tests, desktop smoke, deterministic lighting replay, and room-inspector sweep. Measured render capture time rose from roughly 6.6–8.1 ms in the pre-shadow baseline to roughly 12.1–12.5 ms after the optimized shadow pass; the initial unoptimized pass was roughly 25–27 ms. These measurements are Windows capture timings, not cross-platform GPU benchmarks.

## Provenance controls required

1. Print SHA, renderer path, asset root, shadow backend, graphics preset, and loaded authored modules in every evidence log.
2. Refuse to label an artifact current when its embedded SHA differs from the requested SHA.
3. Publish a macOS artifact from the same reviewed SHA used for Windows evidence.
4. Store a manifest beside each packaged executable containing SHA, configuration, asset hashes, and workflow run ID.
5. Add a one-command `Invoke-DataGame.ps1` provenance check before launch and capture.

No branch was merged, rebased, reset, or pushed during this audit.
