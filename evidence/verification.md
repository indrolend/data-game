# Verification record — 2026-10-17

Environment: Windows, Godot 4.7.2, MSVC via Visual Studio 18 2026, `godot-cpp` at `C:/Users/indro/Projects/godot-cpp`.

## Results

- Native GDExtension and direct-parent harness: PASS.
- Headless Godot startup: PASS. This proves startup only, not visible rendering.
- Deterministic repeatability: PASS for two independent 120-tick movement runs.
- Supported behavior checks: PASS for movement, sprint, jump, look deltas, camera switching, grounded vacuum activation, melee pose/battery use, communication-signal activation/persistence/expiry, native solo pause/resume with simulation-clock and movement freezing, native room metadata and stable computed-scene objects, frame progression, stored-soul consumption, delayed projectile creation, live target observation, deterministic target-state snapshots over neutral ticks, and a controlled parent-owned target interaction.
- Target interaction: PASS. The controlled fixture resets the original `Game`, selects its nearest live target, and sends only semantic input plus the existing `debugFillBattery()` test hook. By frame 600 the target has moved in pursuit; by frame 900 melee has reduced armor from `2.0` to `1.18`; by frame 960 armor is zero and the target is slurpable; by frame 1020 vacuum ingestion is `0.3319208`; and at frame 1050 the target is no longer alive, the live-target count has changed from 9 to 8, and one soul is stored. The snapshot retains the final pre-removal ingestion value (`0.9246550`) because capture commits once the parent threshold is reached.
- Parent-versus-Godot parity: PASS for 56 structured records: 9 movement/action snapshots and 9 phone snapshots (frames 0 through 240 at 30-frame intervals), 3 rally projectile lifecycle records, 2 room-inspector target records, 6 interaction records, 3 communication-signal records, 3 native pause/resume records, 1 room-plan record, all 8 structural records, and all 12 computed object records for the seeded room. Stable structure/object IDs compare exactly; numeric fields compare at `1e-5`.
- Test-created object lifetime: corrected with explicit `free()` calls; current headless logs contain no leak warning.
- Room presentation adapter: PASS. It creates one render-only node for each native structural record, preserves stable IDs and dimensions, applies native base colors, and retains generated-texture identity/world scale without inventing replacement images. GPU capture `native-room-structures/` was inspected; floor, both door frames, and the selected traversal block are visible. This does not verify generated grass pixels or computed props.

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

## 2026-10-18 — restore native-driven default scene

The normal launch (`godot/scenes/main.tscn`) now points to the existing
`main.gd` adapter so play starts in the original C++ `Game` simulation. The
Godot-only `playable_runtime.tscn` remains isolated as an exploratory scene and
its encounter test targets that scene directly. This is a source-informed
reuse of existing gameplay, not a claim that its visuals match the desktop
renderer. No graphical captures were made in this verification run.

- `godot/tests/playable_encounter.gd`: PASS after correcting the isolated
  prototype camera offset; the existing behind-player assertion remains.
- `tests/verify.ps1`: PASS — configure, native build, headless behaviors,
  default Godot startup, and parent parity.
- Parent/Godot numeric comparison: PASS under the script's `1e-5` tolerance.
- Detailed current logs: `cmake-configure.log`, `native-build.log`,
  `headless-smoke.log`, `phone-presentation.log`, `room-presentation.log`,
  `human-model-presentation.log`, `playable-encounter.log`,
  `godot-startup.log`, `parent-reference.csv`, `godot-parity.log`, and
  `parity-result.log`.

## 2026-10-18 — native lawn-fragment presentation slice

The room adapter now materializes only the `LawnFragment`/`Box` family from
`RoomPresentationSnapshot::objects`. Each render-only mesh preserves the native
stable ID, center, dimensions, and yaw and reuses the native room floor material.
Other computed object families remain intentionally skipped. This check was
headless; no GPU screenshot was captured, so visible appearance is not yet
verified.

- Focused room presentation test: PASS (one or more source-native lawn records,
  exact mesh transforms, shared floor material, and no collision object).
- Full `tests/verify.ps1`: PASS after the adapter change.
- Parent/Godot parity: PASS, 66 snapshot records within `1e-5`.
- Detailed room adapter output: `room-presentation.log`.

## 2026-10-18 — combat and vacuum presentation cues

The parent-driven default scene now presents native melee action/hit state,
enemy attack windup/commit/hit state, and vacuum ingestion percentage in its HUD.
The native human adapter applies native hit amount as a reversible red damage
tint and renders the parent's target tether using its authoritative anchor,
destination, and width. This adds no Godot-side attack, damage, or ingestion
simulation. The tether is checked during the existing controlled 1,050-tick
capture sequence and its state is included in direct-parent/Godot parity.

- Focused human/tether presentation test: PASS (native model deformation,
  capture lifecycle, visible adapter node, midpoint, orientation, and width).
- Full `tests/verify.ps1`: PASS (native build, all headless contracts, startup,
  and direct-parent parity).
- Parent/Godot parity: PASS, 78 structured snapshot rows within `1e-5`; tether
  and damage/reaction rows agree at frames 0, 600, 900, 960, 1020, and 1050.
- `human-model-presentation.log` reports no test failures or object leak.
- No graphical capture method/window was available in the current desktop
  session. The tether/HUD appearance and native enemy attack animation are
  therefore not visually verified; prior captures are not evidence for this
  changed presentation.

## 2026-10-18 — original HUD subset and live capture

Source mapping for this slice: `data-game/desktop/DesktopRenderer.cpp`'s
`DesktopRenderer::drawHud()` supplies the reference. The Godot adapter consumes
the native HUD/target snapshots plus native camera, player position, signal,
and simulation time. It currently renders the native four-arm reticle, top
battery strip, conditional room objective, projected target and communication
markers, energy ticker, and critical-hit edge pulse. The previously visible
diagnostic header, invented bottom-left battery/soul panel, always-on controls,
and independent static `+` reticle were removed from the default scene.

This is a mapped subset only. The fallback Godot font, original menu and phone
screen interaction, death/restart and contextual HUD states, and exact original
pixel/layout/timing fidelity are not established. The original native capture
`capability-reconciliation/native-soul-lifecycle-start.png` is useful visual
reference but is not a matching-state comparison with the Godot captures.

- `godot/tests/hud_presentation.gd`: PASS; checks native battery/soul values,
  native vertical camera FOV application, snapshot synchronization, and a
  single HUD adapter with no diagnostic/static duplicate elements.
- `tests/verify.ps1`: PASS; native configure/build, headless contracts,
  startup, exploratory encounter checks, and parent/Godot parity. The final
  parity comparison reports 78 rows within `1e-5` (`parity-result.log`).
- `hud-presentation.log` and `human-model-presentation.log`: PASS.
- Non-headless Godot playtest captures exited 0 (`hud-restored.exit-code`,
  `hud-combat.exit-code`) on RTX 3060 / OpenGL 3.3. Inspected
  `ui-restoration/hud-restored/frame-000-tick-000000.png`,
  `frame-004-tick-000121.png`, and `frame-006-tick-000181.png`; also inspected
  `ui-restoration/hud-combat/frame-015-tick-000900.png`,
  `frame-017-tick-000960.png`, and `frame-019-tick-001050.png`. These show the
  default scene's third-person and first-person phone presentation, native HUD
  cleanup, and interaction marker/ticker during the controlled combat/vacuum
  sequence. The sequence reaches one stored soul at tick 1050. They do not
  establish visual parity with the original renderer.
- The Godot world remains visibly sparse (flat green floor, simple background,
  and limited props) compared with the original renderer capture. Environment
  geometry/material/lighting composition is the largest visible gap.

## 2026-10-18 — native phone menu crossover

The original `PhoneMenuModel.hpp` and `PhoneDisplayLayout.hpp` are mature,
reusable native counterparts to the Godot phone menu. The adapter now renders
their rows/title/selection and semantic commands route through `ParentBridge`
to the original native menu/settings and pause state. Godot does not own a
second menu selection or settings model. The desktop renderer's exact phone
screen composition/effects are not claimed as ported.

- `godot/tests/phone_presentation.gd`: PASS for native row/layout projection,
  Settings → Graphics navigation, quality adjustment, shadow toggle, back,
  pause resume, and first-person view-model transform. Test exits 0.
- `tests/compare_parity.ps1`: PASS, 78 structured rows within `1e-5`, including
  phone and menu layout/row records from the independent direct-parent harness.
- Full `tests/verify.ps1`: PASS after integration (see current verification
  logs); the script checks native build, focused headless contracts, startup,
  and direct-parent parity.
- Non-headless agent playtest exited 0 on RTX 3060 / OpenGL 3.3. Exact command
  sequence is `phone-menu-playtest.commands`; stdout/stderr and actual exit
  status are retained alongside captures in `evidence/phone-menu-playtest/`.
  Inspected `frame-003-tick-000002.png` (pause),
  `frame-005-tick-000002.png` (Settings), and
  `frame-008-tick-000002.png` (Graphics). The phone is visible in first-person
  and native menu contents change as commanded. At full 1280x720 capture scale,
  phone text is very small; legibility and visual fidelity are not established.
- Remaining menu gaps: keyboard rebinding capture, application exit, online
  host/join flows, and desktop-equivalent font/screen treatment. Do not add
  corresponding Godot-owned state; map these only after their native contracts
  are understood.

## 2026-10-18 — native faceted-rock geometry crossover

The room snapshot now carries render-only vertices/normals generated by the
existing `faceted_rock::makeVisualMesh` for `FacetedMesh` object records. The
Godot room adapter turns those exact records into an `ArrayMesh`; native
room-seed/room-index/prop-index remain the mesh-generation inputs. The source
desktop renderer's stone texture is not ported; the Godot material uses the
same source-derived room-substrate base color and retains the native texture
world-scale as metadata. No collision or Godot support physics was added.

- `godot/tests/room_presentation.gd`: PASS; verifies native object count/ID,
  triangle surface, native vertex positions, normalized face normals, base
  transform, and render-only node. Godot normalizes normal vectors on mesh
  upload (observed maximum difference `0.000091218`), so the adapter test uses
  `1e-6` vertex tolerance and `1e-4` normal tolerance; direct snapshot parity
  compares the unmodified transferred values at the suite's `1e-5` tolerance.
- `tests/compare_parity.ps1`: PASS, 172 records within `1e-5`; includes 90
  ordered rock vertex/normal rows compared between direct `Game` generation
  and the Godot-facing snapshot, in addition to existing gameplay/menu/room
  records.
- Full `tests/verify.ps1`: PASS (native build, focused headless tests, startup,
  and parity). GPU agent playtest exited 0 on RTX 3060/OpenGL 3.3; its command
  sequence, log, exit code, and captures are in `evidence/native-rock-presentation/`.
  Inspected `frame-002-tick-000001.png`. The default view shows room structures
  but does not isolate the faceted rock clearly enough to claim visual fidelity;
  its geometry is verified structurally/parity-wise, not by a useful matched
  visual comparison. The frame and phone-readable captures also confirm the
  first-person view-model presentation change; menu text remains small in the
  full gameplay frame.
- Next native-first room slice: materialize house parts from existing
  `HouseGeometry` records with the original renderer's surface color mapping,
  retaining missing stone/metal textures as explicit gaps. Trees/foliage,
  ruins, marker pillars, city facades, grass, and coastal water still need their
  own bounded presentation contracts and must not be improvised from collision.

## 2026-10-19 — native HouseGeometry crossover

The Godot room snapshot now carries the original renderer's house-part color
mapping alongside `ComputedSceneObjects` records. House part geometry remains
from `HouseGeometry::parts`; the Godot adapter materializes those exact centers,
dimensions, yaw, stable IDs, physical metadata, and semantic surface colors as
render-only box meshes. Surface-zero `stone`/`metal` texture identity and world
scale are retained, but their original pixels are not substituted or claimed.
A test-only city fixture delegates to `Game::debugStartGeneratedRoomFixture`
using the parent `CityCourtyard` representative seed, because CityCorridor
intentionally produces no computed house props.

- `godot/tests/room_presentation.gd`: PASS for native CityCourtyard house count,
  stable part ID, dimensions/position/yaw, original surface color mapping,
  retained texture identity, and no Godot collision.
- `tests/compare_parity.ps1`: PASS, 192 records within `1e-5`, including every
  generated CityCourtyard house part's position, size, yaw, physical flag,
  source-mapped color, surface index, and texture identity.
- Full `tests/verify.ps1`: PASS, actual exit code 0. The current
  `evidence/parity-result.log` reports 192 parent-vs-Godot records within
  `1e-5`; the increase from 172 is the 20 HouseGeometry part records.
- GPU agent playtest exited 0 on RTX 3060/OpenGL 3.3. Commands, process log,
  exit code, and captures are retained in `evidence/native-house-presentation/`.
  Inspected `frame-002-tick-000000.png`: it shows generated house masses at
  the room edges, but the distance and missing textures/models do not establish
  visual parity. House decorations, authored city models, and stone/metal
  texture pixels remain unsupported.

## 2026-10-19 — native MarkerPillarGeometry crossover

Added a test fixture that finds a SterileChamber seed using the original
`roomPlan`/`matchesInspectorPremise` contract, then delegates room setup to
`Game::debugStartGeneratedRoomFixture`. `ParentGameNode` exposes the original
renderer marker body/cap colors and metal identity as read-only presentation
data. The room adapter renders each native part as a box with its native
transform and stable ID; it adds no collider. Original texture pixels and
renderer shading are not imported.

- `godot/tests/room_presentation.gd`: PASS; 8 marker parts across four props,
  correct native dimensions/centers/colors/texture IDs, render-only nodes.
- `tests/compare_parity.ps1`: PASS; 200 parent-vs-Godot records within
  `1e-5`, including all 8 direct-parent marker part rows.
- GPU runner command: `evidence/native-marker-presentation.commands`.
  Initial attempt with a project-relative command-file path exited 2 and is
  preserved in `native-marker-presentation.log`/`.exit-code`; retry with
  absolute paths exited 0 and captured frames in
  `evidence/native-marker-presentation-retry/`.
- Inspected `frame-002-tick-000006.png`: pillar bodies/cyan caps are visible at
  the sterile-room edges. This verifies this Godot rendering slice only, not
  exact original visual parity; room backdrop/materials and source metal texture
  pixels remain absent or different.

## 2026-10-19 — native endless-run upgrade boundary

Read-only history review of `indrolend/data-game/main` confirmed several
relevant design decisions: room grammar and deterministic capacity were
established in `6f2fe2d`/`3cdab50`; placeholder generated-surface colliders were
later removed (`f917635`) and enemy pursuit was routed through generated
surface contacts (`5faed4f`); the unstable projected-shadow path was replaced
(`b9d43c8`) and later quality-mode shadow behavior was corrected (`4f5e3a5`).
This supports keeping native generation, support, and enemy collision contracts
authoritative while letting Godot render snapshots; it argues against importing
placeholder colliders or treating the old shadow pass as a stable contract.
The migration repository itself has only the baseline and phone-presentation
commits in its visible history, so the detailed experiments live in data-game.

The audit found a functional Godot gap: native `updateRoomTopology()` opens a
six-choice run/permanent upgrade panel after a cleared-room transition, but the
Godot path had no upgrade selection or activation input. Added a read-only
upgrade snapshot, native grid movement and activation calls, an HUD overlay,
and keyboard handling. The game still owns room index/seed progression, target
scaling, token economy, upgrade caps, and level changes. Arrow keys match the
original three-column/two-row grid; Enter/Space/F activates, 1–6 quick-select.

- `godot/tests/upgrade_presentation.gd`: PASS; checks native snapshot values,
  availability, Godot key routing, permanent token spend, temporary level
  increase, and native resume behavior.
- `tests/compare_parity.ps1`: PASS; 208 rows within `1e-5`, including four
  direct-parent/Godot upgrade states (initial, grid selection, permanent
  purchase, and temporary run choice).
- GPU playtest `evidence/native-room-upgrade-flow.commands`: exit code 0.
  Inspected `evidence/native-room-upgrade-flow/frame-004-tick-000033.png`;
  the recompile panel and selected Lunge purchase are visible and legible.
  This is an upgrade UI verification capture, not proof of an entire endless
  run or exact desktop visual parity.

## 2026-10-19 — native RuinGeometry crossover

Added an original-plan fixture selector for a non-recovery Field room with the
existing ruin composition. The snapshot carries the renderer's per-part weather
tint, stone texture identity/scale/phase, and source-sized scar-box record. The
Godot adapter materializes `RuinGeometry` blocks and scar accents as render-only
meshes; native collision and room rules remain unchanged.

- `godot/tests/room_presentation.gd`: PASS for four ruin parts, source transforms,
  weather tint/stone phase, scar geometry, and no Godot collision.
- `tests/compare_parity.ps1`: PASS; 204 rows within `1e-5`, including four
  direct-parent `RUINPART` records.
- GPU runner: `evidence/native-ruin-presentation.commands` and
  `evidence/native-ruin-presentation-detail.commands`; both exited 0. Inspected
  `evidence/native-ruin-presentation-detail/frame-003-tick-000023.png`: the ruin
  blocks/scar accents render, but the camera shows them at a distance and lacks
  source stone pixels/lighting. This is not original-renderer visual parity.

## 2026-10-19 — native cleared-room transition

Added a test-only rally fixture that places the original player immediately
before a cleared-room doorway. The Godot test submits ordinary movement input
to `ParentGameNode`; no room advancement or upgrade state is synthesized by
Godot. Fourteen fixed ticks cross the seam, advance room 1 to room 2, and open
the native paused upgrade choice through `Game::updateRoomTopology()`.

- `godot/tests/room_transition.gd`: PASS; native transition occurs in 14 ticks,
  advances the room, and activates the native upgrade panel.
- `tests/verify.ps1`: PASS; full configure/build, headless behavior/presentation
  tests, startup, and parent parity.
- `tests/compare_parity.ps1`: PASS; 209 rows within `1e-5`. The new
  `ROOMCROSS` parent and Godot records both report frame 14, room 2, player
  position `(0, 0.08, -21.0122757)`, upgrade active, paused, battery 100, and
  souls 0.
- This is headless gameplay-path evidence only. No graphical multi-room
  transition capture was made, and visual parity remains unestablished.

## 2026-10-19 — native room lighting crossover

Replaced the fixed Godot ambient/background/directional-light values with a
read-only `LightingPresentationSnapshot` resolved from the mature static
`RenderContracts`, `RoomLighting`, and `SceneLightingResponse` APIs. Godot maps
the native atmosphere to `Environment`, global sun/fill to directional lights,
and native ceiling/phone/shot/exit lights to render-only omni lights. No new
Godot time cycle, shadow map, or gameplay response was added.

- `godot/tests/lighting_presentation.gd`: PASS for native environment values,
  City sun direction, Sterile global-light disablement, and ceiling fixtures.
- `tests/compare_parity.ps1`: PASS; the new `LIGHTING` record matches direct
  C++ for room, colors, fog, sun/fill directions and colors, and local-light
  count within `1e-5`.
- GPU runner `evidence/native-lighting-playtest.commands`: exit code 0 on
  OpenGL 3.3 / NVIDIA RTX 3060. Inspected
  `evidence/native-lighting-playtest/frame-000-tick-000000.png`,
  `frame-002-tick-000000.png`, and `frame-003-tick-000000.png`. The Sterile
  fixture visibly presents a more enclosed cool-lit scene; this is a Godot
  presentation check, not a matched-state desktop comparison or visual-parity
  claim. Original shadow maps and material texture pixels remain unsupported.
