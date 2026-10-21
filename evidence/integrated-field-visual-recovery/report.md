# Integrated Field and visual recovery — 2026-10-21

## Baseline

- Native authority: `C:\Users\indro\Projects\data-game`, `main` at `18b19807333be6b1c83f0a39ae08084978285aeb` plus the uncommitted Field-island change set listed below.
- Godot migration: `C:\Users\indro\Projects\DigitalBreakdown-Godot-Migration`, `main` at `06fc26045269dcdec8b934cd147aa0345651d8be` plus the existing dirty visual-recovery work and this uncommitted integration slice.
- Godot: 4.7.2 stable, OpenGL 3.3 Compatibility renderer, NVIDIA GeForce RTX 3060 for retained captures.
- Built native executable SHA-256: `391399DE841F1C5B8AD34344DC40E7C7CFB642E68B566379BC6E3EA86AC7579C` (`data-game/build/bin/Release/DigitalBreakdown.exe`).
- Built Godot extension SHA-256: `BE8E5109430E9BB76BF6427EDCD965B62FBF8946F08B79C05BD5583797E02E50` (`godot/bin/digital_breakdown_parent.dll`).
- Built direct-parent reference SHA-256: `D6C4C4FD11206C46012964F8C78DB9060B53C3E0A47AF41E74AD35221637C8BB` (`build-godot47/Release/parent_reference.exe`).
- No reset, clean, stash, merge, commit, or push was performed.

## Native physics change

The prior player support contract always began with a ground height of `0.08`, so unsupported space could not exist. `updatePlayer()` also clamped X to the room boundary. The new `FieldIsland` contract applies only to production room 1 when its generated setting is Field. It defines a 26 x 35 main footprint plus 4.8-wide doorway bridges. Outside those areas, base-ground support is unavailable, while collider, slope, and generated-rock support remains valid.

The native player keeps existing horizontal velocity and gravity after leaving support. Below Y=-8, recovery clears transient traversal/combat actions and moves only the player traversal state to `(0, 0.08, 15.5)`. Battery, souls, capture completion, enemies, run upgrades, seed, and room index remain intact. Other rooms and native lab fixtures retain the previous floor/clamp behavior.

## Godot presentation change

The room snapshot exposes the exact native footprint and recovery state. Godot materializes render-only main/bridge grass tops, dark island cores/sides, and a simple abyss. There are no Godot collision nodes or duplicate fall/respawn rules.

The original desktop capture-point assembly is now exposed from `GameState::captures`: a 0.72 surround, 0.52 recess, and a rotating 0.36 soul when filled. Stable IDs, positions, filled state, dimensions, and native semantic materials cross the bridge; Godot owns no capture rules.

## Verification

- Native `ctest --test-dir build -C Release --output-on-failure`: 56/56 passed, including the new `FieldIslandTraversalTest` and existing slope, gameplay-flow, and room-progression coverage.
- Migration `tests/verify.ps1`: exit 0, including build, parent smoke, room/phone/human/HUD/lighting/upgrade/transition tests, the new Field-island traversal test, startup, and direct-parent parity.
- Focused Godot tests: `parent_smoke.gd`, `room_presentation.gd`, and `field_island_traversal.gd` all exit 0.
- Actual GPU evidence: `../field-island-vertical-slice-v3/` and `../field-island-vertical-slice-v3.log`. Frames 001 and 003 show edge departure and descent; frame 004 shows recovered gameplay. The log records X=13.121672/Y=0.041111 at departure, X=15.494081/Y=-3.598890 during descent, and the safe spawn at `(0,0.08,15.5)`.

## Remaining visual recovery

- Native phone menu/state/light bridging and readable first-/third-person framing were already present in the preserved dirty work and remain covered by the full suite; exact desktop font/stencil/pixel parity is still unsupported.
- The HUD remains the documented native-driven subset. No source-backed persistent world/location header or footer was found in `DesktopRenderer::drawHud()`, so none was invented.
- Filled capture-point lifecycle needs a retained GPU sequence through a real soul deposit.
- Bullets, flowers, signal residue, exact native particle primitive families, audio, full death/context UI, and matched desktop screenshots remain incomplete.
- The island is intentionally a minimal rectangular vertical slice with doorway bridges. Rounded/beveled terrain would be a presentation refinement, not a gameplay requirement.
