# Visual parity gauntlet baseline

Run date: 2026-10-20 (America/Chicago).

## Selected native authority

`C:\Users\indro\Projects\data-game` at `18b19807333be6b1c83f0a39ae08084978285aeb` (`main`, clean before the run) is the most mature native reference. `origin/main` and GitHub `refs/heads/main` resolved to the same SHA. The October lighting, material, field/coastal, City, shadow, and structural-polish branches inspected are ancestors of this commit, so choosing a newer-dated worktree would discard integrated behavior or select an experiment. The product maturity ledger also identifies current `main` as the converged authority.

Native executable:

- Path: `C:\Users\indro\Projects\data-game-tools\artifacts\build\18b19807333b-80c91579-Release\bin\Release\DigitalBreakdown.exe`
- Configuration: Release, CMake 4.4.0, Visual Studio/MSBuild 18.7.8, Windows SDK 10.0.26100.0
- Size: 2,249,728 bytes
- SHA-256: `56d9f603955fbf398aa5d0fb65c295a940d20552c703385f536d946d78ab410b`

## Godot comparison target

`C:\Users\indro\Projects\DigitalBreakdown-Godot-Migration` at `06fc26045269dcdec8b934cd147aa0345651d8be`, tracking `data-main/godot-migration`. GitHub `refs/heads/godot-migration` resolved to the same SHA.

The checkout was intentionally dirty before the run: modified `tests/verify.ps1`; untracked `godot/tests/menu_navigation.gd`; and untracked menu/primary-loop evidence. These were preserved. The verification run also updated the tracked timing-only `evidence/cmake-configure.log`, and this gauntlet added only `evidence/visual-parity-gauntlet/`.

- Godot: `4.7.2.stable.official.ed1daf0bf`
- Renderer observed in graphical runs: OpenGL 3.3 Compatibility, NVIDIA GeForce RTX 3060, driver string 610.88
- GDExtension: `godot/bin/digital_breakdown_parent.dll`
- GDExtension size: 1,015,808 bytes
- GDExtension SHA-256: `e0793ea8350d5a91e33a670bf09b5c093b011e83f6085c52ec26bc8fe96e0ab7`

## Authority boundary

The native product and migration do not share a cross-runtime capture-state file. Native `main` has product-owned evidence scenarios and renderer telemetry; Godot has semantic fixtures and a fixed-step `ParentGameNode` capture driver over its archived parent source. Therefore no frame in this package is claimed as pixel-matched or as an identical serialized simulation state. Structured parent/Godot parity inside the migration passes, but it compares the migration bridge against its retained parent source, not current native `main`.
