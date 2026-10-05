# Data best-work convergence ledger

Audit date: 2026-09-29  
Canonical product authority: `indrolend/data-game/main`  
Canonical product revision audited: `f5468af9aa7b2346138066f009fa79a4ccfb1787`  
External tooling authority audited: `indrolend/data-game-tools/main` at `1fdb95cc02515701d25d05bcba984204fdd5c2e7`

## Scope and method

This is a capability ledger, not a commit inventory. Git commits, paths, symbols, tests, assets, and runtime entry points are evidence for capabilities. Status does not follow from ancestry alone.

FACT: `git fetch --all --prune` completed before analysis. The reachable Data DAG contains 1,001 commits: 620 reachable from `main`, 381 outside `main`, and 54 merge commits. It exposes 36 local branches, 92 remote refs (including the remote HEAD alias), and 3 tags.

FACT: the working checkout was `restore/progressive-atmosphere` at the same commit as `main`, with three staged additions and no unstaged changes: `native-desktop/DesktopRenderer.cpp`, `native/game/RenderContracts.hpp`, and `native/tests/render_contracts_test.cpp` (1,459 added lines). They were preserved. This report is the only audit write.

FACT: current-main validation passed all 35 CTest tests through Data Tools. The build was revision-keyed to `f5468af9aa7b`, and Data Tools correctly reported the product checkout as dirty.

The audit used `git log --all`, branch containment, patch and tree inspection, blob identity, symbol searches, historical path searches, and comparison of historical implementations with current runtime wiring. Subjective visual equivalence remains explicitly unresolved where no same-scenario render comparison exists.

## 1. Current Git authority/state

| Authority | Branch/revision | State |
|---|---|---|
| Data product | `main` / `f5468af9` | Canonical; synchronized enemy evidence harness is current tip |
| Audit checkout | `restore/progressive-atmosphere` / `f5468af9` | Dirty only by preserved staged atmosphere experiment |
| Data Tools | `main` / `1fdb95cc` | Clean; exact-revision build/evidence orchestration |

The path migration in `0bdaeda` moved the canonical runtime from `native-*` to `desktop/`, `game/`, `tests/`, `models/`, and `network/`. Historical path absence is therefore not treated as feature loss.

## 2. Reachable history inspected

- 1,001 reachable commits, including all 381 commits outside `main`.
- 131 branch-like refs (36 local plus 92 remote refs plus 3 tags).
- 54 merge commits and the main convergence/removal points `c558088`, `3bf72eb`, `2106ef1`, and `0bdaeda`.
- Historical tests, docs, scripts, assets, artifacts, and runtime code were searched independently of commit messages.

## 3. Capability categories discovered

Gameplay; player/DATA phone; enemy behavior; enemy locomotion; enemy physics; enemy visual presentation; combat; melee; vacuum; souls; capture; progression; rooms; procedural generation; traversal; camera; rendering; lighting; atmosphere; materials; visual effects; HUD; phone UI; audio/music; Secret TV; multiplayer; networking; controller/input; platform support; saves; performance; diagnostics; developer labs; automation; agent playtesting; recording; telemetry; evidence capture; determinism; soak testing; assets/provenance; and release/build tooling.

## 4. Feature ledger

| Capability | Category | Best-known implementation / commits | Current-main implementation and concrete evidence | Status | Validation / runtime evidence | Disposition |
|---|---|---|---|---|---|---|
| Core gameplay lifecycle and room flow | Gameplay | Main lineage through `0162eda`, `c558088`, `3bf72eb` | `game/Game.cpp`, `game/Game.hpp`; `tests/gameplay_flow_probe.cpp`, `tests/room_progression_probe.cpp`, lifecycle/state contract tests | PRESENT | CTest passes | Retain |
| Original DATA device body | Player / assets | `a7d61da`, main-equivalent `9454fa2`, later `b272962` | `models/phone.dbmesh`, `game/gameplay/PhoneBody.hpp`, `tests/phone_body_contract_test.cpp` | SUPERSEDED | Geometry contract passes; current blob differs from early `a7d61da` because it was refined | Document final lineage; retain current asset |
| Phone display and menus | Phone UI / HUD | Main menu/display convergence plus historical `8059515`, `2080861`, `2d5c61d` | `game/PhoneDisplay.hpp`, `PhoneDisplayLayout.hpp`, `PhoneMenuModel.hpp`; three focused tests | PRESENT | Contract tests pass; no current rendered typography A/B | Add deterministic menu capture set before adopting any off-main typography polish |
| Spray text reveal / typography hardening | Phone UI | `eb83790`/`8059515`, `2080861` on `codex/phone-*` | No matching symbols or tests established on main | STRANDED | Historical source only | Inspect rendered phone handoff before port; isolated candidate |
| Melee, vacuum, and target lifecycle | Combat | Main lineage through `c19efd9`, modular helpers, `3bf72eb` | `game/gameplay/MeleeConfig.hpp`, `VacuumGeometry.hpp`, `TargetLifecycle.hpp`; geometry/lifecycle tests | PRESENT | CTest passes | Retain |
| Soul motion, storage, economy, and projectile identity | Souls | Main lineage including `3281fa4`, `05634ca` | `SoulMotion.hpp`, `SoulEconomy.hpp`, `tests/soul_*`, role/lifecycle tests | PRESENT | Deterministic tests pass | Retain |
| Soul deformation during extraction | Souls / VFX | `e5d78a6` | Main has `SoulVisualState`, `drawSoulFlesh`, morph/cube/visibility paths, but historical deformation delta and rendered equivalence were not proven | PARTIAL | State contract exists; no same-scenario render A/B | Add soul-lifecycle evidence scenario and A/B historical render |
| Flower theme and capture polish | Capture / audio / VFX | `bd37f70` | Flower glyph/shell rendering in `desktop/DesktopRenderer.cpp`; flower music mixing in `DesktopAudio.cpp`; `desktop/audio/flowertheme.wav` survives but blob differs | PARTIAL | Gameplay tests; no capture/audio evidence contract | Compare capture/flower scene and audio trigger against `bd37f70` |
| Wall-mounted capture goals | Capture | `6daef95`, `1a2e94e`, `203fe29` | Current capture targets render in `desktop/DesktopRenderer.cpp`; current room flow requires/deposits souls | SUPERSEDED | Room flow and rendered evidence infrastructure | Retain current implementation; document historical native-port lineage |
| Progression, upgrades, and saves | Progression / saves | Main convergence | Permanent/run progression in `Game`; save round-trip/recovery entry point and CTest | PRESENT | `DesktopSaveRecoveryTest`, flow/soak tests pass | Retain |
| Deterministic procedural room grammar | Rooms / generation | `5807fcf`, `b781bdb`, later main room convergence | `game/RoomEnvironment.hpp`, `world/RoomGeometry.hpp`; `RoomInspectorReport`; deterministic seed/premise reproduction | PRESENT | `room_environment_test`, `traversal_calibration_test`, room probe pass | Retain |
| Room inspector and review records | Diagnostics / rooms | `5807fcf` plus later instrumentation | `Game::debugStartRoomInspector`, `debugStepRoomInspector`, `debugRoomReviewLine`; developer codec and CLI wiring in `desktop/main.cpp` | PRESENT | Tests assert seed reproduction, report fields, and review record | Retain; expose capture orchestration only from Data Tools |
| Traversal graph, slopes, rocks, trees, generated surfaces | Traversal / physics | Main line `511b4b9`–`844cd81`, `c84b81c`; older `f50fc010` | Shared geometry/support code and extensive fixture tests | PRESENT | Deterministic controller sweeps and traversal tests pass | Retain |
| Playground traversal evidence | Traversal / diagnostics | `f50fc010` | Current inspector materializes Playground/Funnel surfaces; tests exercise projectile and player/enemy traversal, but no named current evidence bundle | PARTIAL | Strong headless contracts, weak rendered evidence | Add Data Tools traversal evidence scenario rather than restore old script |
| Secure automation playtest policy | Automation / input | `e445f6e1`, `0d9773a8` | Current `desktop/DesktopPlaytestPolicy.hpp` and test are blob-identical to `0d9773a8`; CLI remains wired | PRESENT | Focus/input policy test passes | Retain |
| Deterministic input soak | Determinism / soak | `77cf89d`, `4b87903`, current main | `tests/deterministic_input_soak.cpp`, 180,000 fixed-step frames with state bounds and deterministic summary hash | PRESENT | Passes in 4.57 s in this audit | Retain |
| Gameplay compute timing folded into soak | Performance | `9382508`, consolidation `9aabfda` | Current soak retained longevity/hash but removed `chrono`, ticks/sec, realtime factor, and microseconds/tick output | STRANDED | Historical output contract only | Restore measurement fields without imposing a machine-dependent pass threshold |
| Runtime/render performance traces | Performance | Main runtime stress/profiling paths and historical `0edba56` | `desktop/main.cpp` contains perf CSV, render/combat stress modes | PRESENT | Executable diagnostics exist; not run in this source-only audit | Preserve; add documented Data Tools actions if routinely needed |
| Static readable lighting authority | Rendering / lighting | `2106ef1` | `game/RenderContracts.hpp::DesktopSceneLighting`; runtime reference A; render test covers exact state | PRESENT | Deterministic rendered reference A plus telemetry and replay | Preserve as a reproducible region; do not select as final default in the control experiment |
| Progressive scene atmosphere | Atmosphere / lighting | lineage `3e5c9c6`, `8f0bb87`, `3791e28`, `d567a7c`, `50bf6fe`, strongest compact contract `64febd7` | Runtime reference B preserves the staged time/room/phone atmosphere function behind the actual renderer's lighting control | CONFLICTING | Deterministic rendered reference B plus telemetry and replay | Preserve as a reproducible region; treat its modulation as controllable state, not a forced replacement |
| Room-setting lighting, visible-source anchoring, contact occlusion, shared response lights | Lighting | `3e5c9c6` through `50bf6fe` | Main keeps common lighting/material/shadow machinery but semantic equivalence of per-setting profiles, source anchors, occlusion, and response lights is not established | PARTIAL | Historical contract tests; current static contract | Compare each component separately; do not cherry-pick the chain wholesale |
| Hypercore atmospheric lighting | Lighting / atmosphere | `82150c6` / `fad1c78` | Main contains later lighting authority, but visual equivalence is unknown | UNKNOWN | Requires runtime A/B | Use same deterministic room/camera; reject if it harms readability |
| Semantic material response | Materials | `f1c8109`, main-equivalent `ce64e81` | `game/MaterialResponse.hpp`, `tests/material_response_test.cpp` | PRESENT | Test passes | Retain |
| Blob/soul-lattice impact visual | VFX | `76f103a` restoration script points to earlier blob-tracking work | Current has soul shells, flesh, particles, melee ribbons, but no proven blob-tracking/lattice semantic match | UNKNOWN | No surviving focused test/evidence located | Identify source commit from script payload, then render A/B; do not run restoration script blindly |
| Expressive elastic combat and soul ingestion | Enemy presentation / combat VFX | `e387c14` | Some current hit/vacuum/soul reaction fields and renderer paths survive; expressive test and full historical behavior do not | PARTIAL | Current generic tests only | Extract independent pose/VFX invariants and add rendered combat evidence |
| Current enemy obstruction/contact behavior | Enemy behavior / physics | Main fixes through generated-surface line; harness `bde8a44`, `3fbf6ca`, `f5468af` | `game/diagnostics/EnemyEvidenceScenario.hpp`, runtime `--evidence-scenario`, continuous frames | PRESENT | Headless test plus synchronized bundles through Data Tools | Retain as current behavioral baseline |
| Recurrent enemy motor | Enemy behavior | `403bcfc`, `9038165`, integration `b58b338`/`1de1184`, telemetry `e60c158` | `EnemyMotor.hpp`, runtime integration test, and motor probe are absent; main uses direct product movement/routing | STRANDED | Historical deterministic unit/integration tests and compact probe | Port only behind a behavior comparison boundary; do not replace current routing blindly |
| Persistent physical enemy foot plants and gait | Enemy locomotion / physics | `af92829`–`54a5149`, proof `32a84d5`; later `ff8317d`, `5b27ad1` | Main retains supported enemy traversal but not `EnemyLocomotion.hpp`, persistent step phases, foot-load ownership, or locomotion proof artifact | STRANDED | Historical unit tests, probe, and telemetry CSV | High-value but higher-risk recovery after A/B harness exists |
| Physical/recurrent/relentless enemy alternatives | Enemy behavior / developer labs | recurrent motor line; physical humanoid lab `5e63c1f`; Zombie V1 `f4eb6bc`–`0838672` | Main baseline differs; three alternative authorities overlap and are not behaviorally ranked | CONFLICTING | Branch tests/benchmarks, no common current evidence protocol | Normalize candidates to the current obstruction/contact scenario before selection |
| Compact enemy motor telemetry | Telemetry | `e60c158` | Current enemy evidence emits scenario telemetry, but the motor-specific probe fields (social lateral, vacuum/individuality deltas, brace, attack commitment, speed scale) are absent | PARTIAL | Current scenario telemetry plus historical test output | Add selected motor internals to product-owned observation only if motor recovery proceeds |
| Live enemy physics tuning lab / in-game developer mode | Developer labs | `7b3c74f`, convergence `0838672` | Main has developer codec and room/traversal labs, not the full live enemy-physics controls | PARTIAL | Codec tests; historical lab source | Recover only controls needed to compare shortlisted motor/gait candidates |
| General frame-stepped agent playtest bridge | Agent playtesting | `abd97e1` | No `--agent-playtest`, stdin `step/observe/reset`, or `AGENT_STATE` protocol on main. Scenario evidence is deterministic but not general interactive control | STRANDED | Historical real-simulation/real-renderer bridge | Highest-confidence tooling recovery: re-expose narrow product boundary; orchestration belongs in Data Tools |
| Agent demonstration recording | Recording | `ab299df` | Current evidence records fixed scenarios continuously and Data Tools encodes H.264, but interactive `record start/status/stop` during agent play is absent | PARTIAL | Current scenario videos; historical recording docs/tool | Put recording orchestration in Data Tools atop recovered bridge; keep capture in product |
| Autonomous room playtest script | Automation | `1a4b170:tools/autonomous-playtest.ps1` | Removed from product; depended on the absent agent bridge and old `dbdev.ps1` | REJECTED | Historical CSV controller only | Do not restore verbatim. If needed, implement a thin Data Tools client after bridge recovery |
| Synchronized gameplay evidence harness | Evidence capture | `bde8a44`, `3fbf6ca`, `f5468af` | Product emits synchronized scenario state/frames; Data Tools validates, encodes, and writes result/timeline/events/assertions/log/video | PRESENT | Runtime and headless validation pass | Canonical evidence path; extend by scenarios, not parallel frameworks |
| Multiplayer protocol, simulation determinism, peer isolation | Multiplayer / networking | Main lineage including edge-latching and convergence work | `network/`, `desktop/DesktopMultiplayer.*`, server tests; determinism, protocol, peer isolation tests | PRESENT | C++ and server test surfaces | Retain |
| Historical network input edge repair | Networking / input | `0fe11b7` | Main multiplayer determinism suite contains extensive edge/lifecycle cases; exact old characterization file does not survive | SUPERSEDED | Current deterministic suite passes | Document mapping; no old helper restoration |
| Controller support and hot-plug recovery | Controller / platform | `001fc99`, `8d7234b` | `desktop/ControllerInput*`, rumble platform files; Windows/macOS/stub implementations | PRESENT | Build validation; no hardware run in this audit | Add manual hardware smoke checklist |
| Windows/macOS/Linux desktop release | Platform / release | `ba86e3f`, `0eab94a`, current workflows | `CMakeLists.txt`, `.github/workflows/release.yml`, build identity and manifest tools | PRESENT | Windows release build succeeded here | Retain |
| Android/phone-control tooling | Platform / tooling | historical `tools/phone-control`, Android branches | `366ab1c` intentionally removed abandoned Android/browser targets; current authority is desktop | REJECTED | Deliberate convergence/removal evidence | Keep as history unless platform scope changes |
| Pass 7 oracle and asset-generation machinery | Assets / diagnostics | `tools/pass7-oracle/*`, phone lineage | Removed by `7e277ba` as stale browser reference; later `0bdaeda` removed remaining product-owned generators/source assets | REJECTED | Final assets and contracts survive; generators do not | Do not restore as gameplay authority; document provenance separately if needed |
| Audio/music and Secret TV | Audio / gameplay | Main recurring convergence including `3a2f9b8`; flower/audio recovery line | Current audio engine/assets; `tests/secret_tv_policy_test.cpp`; TV assets | PRESENT | Policy test and build pass; subjective mix not audited | Add deterministic cue-state telemetry plus manual audio capture checklist |
| External exact-revision build/evidence orchestration | Release / diagnostics | Data Tools `1fdb95cc` | `Invoke-DataGame.ps1` owns dirty reporting, revision builds, unique bundles, validation, H.264, `result.json` | PRESENT | Used successfully for this audit's build/test | Preserve authority split |

## 5. STRANDED capabilities

FACT:

- General frame-stepped agent playtest bridge (`abd97e1`).
- Recurrent enemy motor and its runtime integration (`403bcfc`, `9038165`, `b58b338`/`1de1184`).
- Persistent physical foot-plant/gait lineage and deterministic locomotion proof (`af92829` through `32a84d5`).
- Gameplay compute timing output inside the deterministic soak (`9382508`; `9aabfda` removed only the redundant standalone benchmark).
- Phone spray-text/typography hardening from the `codex/phone-*` branches.

INFERENCE: the agent bridge and compute timing are the cleanest recoveries because they are narrow, observable, and do not change gameplay. Enemy locomotion is potentially more valuable but cannot be ranked safely without runtime comparison against current obstruction/contact behavior.

## 6. PARTIAL capabilities

- Agent recording: fixed-scenario continuous video is present, interactive agent-play recording is absent.
- Flower/capture polish: core visuals and audio survive, but blob identity changed and no current visual/audio survival evidence exists.
- Soul deformation, elastic combat, and ingestion expression: data/renderer pieces survive, focused expressive contracts do not.
- Playground evidence: strong headless behavior survives, one-command rendered evidence does not.
- Room-setting/source/occlusion/response lighting: pieces of the rendering substrate survive, semantic equivalence does not.
- Motor telemetry: scenario telemetry survives, compact motor-internal telemetry does not.
- Developer labs: generic codec/room/traversal modes survive, full enemy-physics tuning does not.

## 7. CONFLICTING capabilities

- Progressive atmosphere (`64febd7`) versus readable static lighting (`2106ef1`). The current staged experiment itself demonstrates active work here and was not treated as audited main.
- Enemy movement authorities: current direct pursuit/routing versus recurrent motor, physical support locomotion, and Relentless Zombie V1.
- Historical Hypercore lighting versus both progressive and current static profiles, pending render comparison.

## 8. SUPERSEDED capabilities worth documenting

- Early DATA phone asset work (`a7d61da`) led to the refined current asset and phone-body contract.
- Old wall-mounted/native capture-goal ports led to the current renderer/room-flow implementation.
- Historical network edge characterization was absorbed into broader current multiplayer determinism coverage.
- Old in-repository build/evidence scripts were superseded operationally by the `data-game-tools` authority. Gameplay implementations must not be copied there.

## 9. Highest-confidence recovery candidates

1. **Restore compute metrics to `DeterministicInputSoak`.** Source: `9382508`. Low risk, one file, no gameplay effect. Preserve informational metrics; avoid a hardware-sensitive threshold.
2. **Reintroduce a narrow frame-stepped product bridge.** Source concept: `abd97e1`; target current `desktop/main.cpp` plus a product-owned protocol/parser test. Keep policy/save bypass and real renderer capture in Data; put process orchestration in Data Tools.
3. **Add interactive recording orchestration in Data Tools.** Source concept: `ab299df`; depend on candidate 2. Reuse current encoder/result conventions rather than the old standalone encoder script.
4. **Add deterministic render comparison scenarios for atmosphere, soul lifecycle, capture/flower, and combat impact.** No gameplay change. These unblock safe decisions on visual lineages.
5. **Evaluate, then selectively port enemy gait/motor components.** Start with persistent foot-plant state and observation, not a wholesale motor replacement.

## 10. Areas requiring runtime evidence

- Static readable lighting versus progressive atmosphere and Hypercore lighting.
- Per-setting/source/contact/shared-response lighting equivalence.
- Flower/capture appearance and flower-theme trigger/mix.
- Soul deformation, blob/lattice appearance, elastic hit and ingestion expression.
- Recurrent versus physical versus relentless versus current enemy movement under identical obstruction, contact, slope, crowd, vacuum, and long-horizon scenarios.
- Phone typography and spray reveal.
- Controller hardware behavior and subjective audio mix.

## 11. Proposed survival contracts

| Capability | Survival signal |
|---|---|
| Progressive/readable lighting | Deterministic room/camera/time/phone-power scenario; render-state telemetry; paired frames/video for human A/B judgment |
| Flower/capture polish | Deterministic capture cycle with flower active; event timeline, cue-state telemetry, and rendered video |
| Soul deformation/ingestion | Existing lifecycle assertions plus synchronized morph/ingest/scale telemetry and close deterministic render |
| Enemy motor/gait | Common product-owned scenario schema; position/support/contact/foot-plant/motor-output telemetry; deterministic hash; rendered video |
| Agent bridge | Parser bounds test, fixed-step determinism test, observation schema version, save-bypass assertion, real-render smoke |
| Agent recording | Manifest/frame-count/monotonic-tick validation in Data Tools; no overwrite; H.264 output |
| Compute soak | Existing state/hash contract plus informational elapsed/ticks-per-second/us-per-tick fields |
| Phone UI polish | Deterministic page/state/font capture matrix; layout assertions; human visual review |
| Audio | Cue-state assertions and a manual synchronized capture checklist, not waveform aesthetics as a fake unit test |
| Assets | Canonical asset checksum/provenance record and geometry contract where behavior depends on shape |

## 12. Lighting runtime-control result and next operation

FACT: the deterministic comparison found two valid, substantially different regions of the current lighting space rather than establishing a winner. Reference A and reference B now remain reproducible through one product-owned runtime lighting control consumed by the real renderer. The same boundary permits per-channel overrides and fixed/live time, room, and phone inputs; synchronized evidence records resolved state and control configuration. Replaying the emitted control state reproduced the complete rendered evidence video byte-for-byte.

FACT: machine experiments now enter through repeatable `--lighting-command` arguments or `--lighting-control-state`. Both use the same parser, mutation function, serialized state, renderer-owned control object, and resolver as the interactive developer console. Data Tools only forwards those inputs to the real executable and retains normal evidence bundles. A command-driven primary experiment and serialized-state replay reproduced all nine frames and the encoded video byte-for-byte; three additional bounded mutations produced distinct rendered evidence without rebuilding.

FACT: historical lighting commits discovered separable concepts: room-setting and seed-conditioned bases (`3e5c9c6`), visible source ownership (`8f0bb87`), unified profiles and source-linked shadow direction (`3791e28`), profile-driven contact/corner occlusion (`d567a7c`), gameplay-response lights (`50bf6fe`), and time/room/phone modulation (`64febd7`). Only the concrete atmosphere channels and existing time/room/phone modulators are in the first control slice. The other concepts remain evidence for later investigation and were not restored.

PROPOSAL: the next smallest operation is to add a compact machine-readable batch index beside the retained lighting bundles, containing bundle path, exported control-state path, resolved first-sample state, and evidence hash. Do not add search/ranking policy or expand controls.

Architectural interpretation: historical conflicts may represent discovered regions or independent dimensions of controllable runtime space. This is not assumed universally; lighting validates the pattern because A, B, and an independently manipulated state all traverse the same resolver, renderer, telemetry, capture, and replay path without a second rendering authority.

## Recovery sequence after the next operation

1. Lighting A/B evidence scenario.
2. Informational compute metrics in soak.
3. Frame-stepped agent bridge plus protocol tests.
4. Data Tools interactive recording client.
5. Soul/capture/combat rendered evidence scenarios.
6. Common enemy-motion comparison schema.
7. Selective enemy motor/gait recovery only after evidence ranks the alternatives.
