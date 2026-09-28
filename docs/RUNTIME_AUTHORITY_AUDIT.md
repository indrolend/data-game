# Runtime Authority Audit

Baseline: `c558088988c1fa298b00e7f6c8ca171fde6b4845` (`origin/main` at the start of this pass).

This audit ranks observed smells by competing runtime authority, observable breakage,
misleading proof, visual inconsistency, and unnecessary complexity. Its governing
question is not whether a helper exists, but whether the executable uses it.

## 1. Vacuum offer and capture geometry had two authorities

- **Observed:** `native/game/gameplay/VacuumGeometry.hpp` described and tested offer/capture geometry, while `native/game/Game.cpp` performed separate calculations.
- **Runtime authority:** the local calculations in `Game::updateVacuum`.
- **Duplicate authority:** `insideVacuumOffer`, `insideCaptureCylinder`, and `VacuumGeometryConfig`.
- **Observable risk:** the two attraction defaults were already different: runtime used `20.0 / 3.25`; the helper used `15.5 / 2.35`.
- **Minimum convergence:** preserve the shipped runtime values in one `VACUUM_GEOMETRY` contract and route flowers and souls through its predicates. Implemented in this pass.

## 2. Runtime atmosphere displaced readable lighting without creating day/night

- **Observed:** commit `9b8a4c71` replaced the static background, ambient, sun, fill, and fog values with `sceneAtmosphere(state.time, ...)`.
- **Runtime authority:** `render_contract::sceneAtmosphere` supplies colors and fog; `DesktopSceneLighting.sun.direction` remains fixed.
- **Disconnected claim:** elapsed time drives `omenPulse`, not environmental time or a solar phase.
- **Observable risk:** an identical 1280x720 deterministic capture changed from bright blue daylight at `feb4c277` to an almost-black magenta/green scene at `c558088`; average luma fell from `118.253` to `33.0206`.
- **Minimum convergence:** decide the accepted presentation, then introduce one explicit environmental-time contract only if day/night is intended. Do not stack another pulse or multiplier onto the current function. Evidence only; no speculative lighting change in this pass.

## 3. Room dimensions had separate simulation and rendering owners

- **Observed:** `Game.cpp` and `DesktopRenderer.cpp` separately declared `30 / 42 / 7.2`; `RoomCoordinates` independently defaulted depth to `42`.
- **Runtime authority:** each consumer's local copy.
- **Observable risk:** collision, room seams, camera constraints, rendered walls, and periodic positions could silently disagree.
- **Minimum convergence:** own immutable dimensions in `world/RoomGeometry.hpp` and consume them from simulation, rendering, and coordinate code. Implemented in this pass.

## 4. Room periodic-coordinate behavior also had a shadow implementation

- **Observed:** `Game::getRoomTileIndex`, `getRoomTileOriginZ`, and `wrapZ` repeated the formulas in `world::RoomCoordinates`.
- **Runtime authority:** the `Game` methods.
- **Duplicate authority:** the tested coordinate helper.
- **Observable risk:** seam selection and wrapping could diverge from helpers used by vacuum geometry.
- **Minimum convergence:** retain the existing `Game` API but delegate its formulas to `RoomCoordinates`. Implemented in this pass.

## 5. `EnemyMotionFacts` claimed a producer-to-presentation flow that did not exist

- **Observed:** `native/gameplay/EnemyMotionFacts.hpp` was referenced only by `HumanVisual.hpp`; every runtime call used its default-empty value.
- **Runtime authority:** `TargetState` fields passed directly into `makeHumanVisualPose`.
- **Disconnected authority:** a documented locomotion-facts boundary with no runtime producer.
- **Observable risk:** dormant pose behavior suggested contact/slip/climb presentation that the game never delivered.
- **Minimum convergence:** remove the unused parameter and unreachable pose adjustments. Implemented in this pass without adding an enemy motor or changing the current pose result.

## 6. The gameplay state-contract test name overstated its proof

- **Observed:** `gameplay_state_contracts_test.cpp` only included `StateContracts.hpp` and returned success; the header contains compile-time layout/protocol assertions.
- **Runtime authority:** none; this is compile-time compatibility proof.
- **Misleading authority:** the target name implied behavioral state-transition coverage.
- **Observable risk:** a green test could be read as proof of gameplay transitions.
- **Minimum convergence:** rename it to `StateLayoutContractsTest` and state exactly what reaching `main` proves. Implemented in this pass.

## 7. Geometry proof was capable of certifying code the executable bypassed

- **Observed:** `GameplayGeometryAndConfigTest` exercised vacuum helpers while runtime used local predicates.
- **Runtime authority:** formerly the bypass in `Game.cpp`; now the tested predicates themselves.
- **Observable risk:** tests could stay green while shipped offer boundaries changed independently.
- **Minimum convergence:** make the runtime consume the helper, lock the preserved shipped configuration, and pair helper-boundary proof with `GameplayFlowProbe`'s real capture flow. Implemented in this pass.

## 8. `Game.cpp` remains an authority sink, but size is not the causal defect

- **Observed:** traversal, camera, vacuum, enemies, economy, flowers, melee, projectiles, progression, and presentation timing coexist in `Game.cpp`.
- **Runtime authority:** the `Game` update path is often correctly authoritative.
- **Duplicate authority:** only demonstrated contract/runtime overlaps should move.
- **Observable risk:** a mechanical split would add indirection without removing competing behavior.
- **Minimum convergence:** continue extracting only after finding a tested helper or coherent state boundary that runtime currently duplicates. Deferred.

## 9. Visual decisions are distributed across contracts and renderer-local policy

- **Observed:** `VisualIdentity.hpp`, `RenderContracts.hpp`, `MaterialResponse.hpp`, `EarlyBrowserVisuals.hpp`, and `DesktopRenderer.cpp` all make presentation decisions.
- **Runtime authority:** the renderer ultimately combines all of them.
- **Competing authority:** atmosphere and renderer-local colors/effects can override the declared identity and material response.
- **Observable risk:** individually intentional treatments combine into an incoherent scene, as the lighting A/B demonstrates.
- **Minimum convergence:** establish the order palette -> material response -> environment/lighting -> renderer, then remove only concrete contradictions. Begin with lighting in a separate visual PR.

## 10. Fixed directional light and time-varying atmosphere describe different worlds

- **Observed:** `sceneAtmosphere` varies background/ambient/sun color/fog with elapsed time, while `DesktopSceneLighting.sun.direction` never changes.
- **Runtime authority:** both values are applied together in `DesktopRenderer::draw`.
- **Observable risk:** apparent time variation has no solar geometry, shadow progression, or explicit cycle semantics.
- **Minimum convergence:** either name and scope this as threat ambience, or derive all environmental lighting properties from an explicit phase. Deferred pending an accepted baseline decision.

## 11. Represented traversal motifs are deliberately not physical systems

- **Observed:** `EarlyBrowserVisuals.hpp` explicitly distinguishes production-physical Playground/Funnel behavior from represented Orbit/Vertical motifs.
- **Runtime authority:** only implemented traversal capabilities have physical consequences.
- **Observable risk:** graph vocabulary can be mistaken for shipped mechanics.
- **Minimum convergence:** keep the distinction explicit and do not implement Orbit/Vertical during cleanup. No change required.

## 12. Gameplay path ownership was split for one disconnected type

- **Observed:** gameplay contracts live in `native/game/gameplay/`, while the sole file in `native/gameplay/` was `EnemyMotionFacts.hpp`.
- **Runtime authority:** no producer used that type.
- **Observable risk:** directory structure implied a second gameplay authority.
- **Minimum convergence:** resolve the type before moving paths. Removing the unused type removes the split without a cosmetic move. Implemented in this pass.

## 13. Archived enemy experiments are not current-main authority

- **Observed:** recent archived work covers recurrent enemy motors, perception probes, physical embodiment, and mature-human pose translation on isolated experimental branches/PRs.
- **Runtime authority:** this pass starts from current `origin/main`; those experiments remain separate unless merged deliberately.
- **Observable risk:** importing their concepts piecemeal would mix planner, motor, physics, and presentation authorities.
- **Minimum convergence:** do not cherry-pick or recreate experimental enemy behavior here. The removal of empty `EnemyMotionFacts` is intentionally behavior-neutral.

## 14. Test execution can be misclassified when only the game target is built

- **Observed:** archived build/playtest evidence records CTest `Not Run` results after building only `DigitalBreakdown`; test executables were missing rather than failing assertions.
- **Runtime authority:** CMake defines the test targets; CTest only runs built executables.
- **Observable risk:** missing binaries can be reported as gameplay regressions, or partial green results as full coverage.
- **Minimum convergence:** build the all/default target before the broad CTest pass and classify `Not Run` separately from failed assertions. Applied to validation for this pass.

## 15. Presentation captures can inherit persistent user progression

- **Observed:** capture startup loads `%LOCALAPPDATA%/DigitalBreakdown/progression.v1` unless the chosen mode overrides it.
- **Runtime authority:** persistent progression affects the initial game state.
- **Observable risk:** visual comparisons may not be reproducible across machines or profiles even with identical binaries.
- **Minimum convergence:** use the same profile for immediate A/B evidence now; later add an explicit deterministic capture seed/profile reset rather than silently deleting user saves. Deferred.

## Evidence and validation contract

The lighting A/B artifacts for this audit are intentionally outside Git:

- `evidence/lighting-ab/before-9b8a4c71.png`
- `evidence/lighting-ab/current-c558088.png`
- matching lossless `.ppm` captures

For the implemented convergence changes, correctness requires all three layers:

1. helper/config boundary tests (`GameplayGeometryAndConfigTest`),
2. compile-time protocol/layout proof (`StateLayoutContractsTest`), and
3. real `Game` flow (`GameplayFlowProbe` plus desktop runtime smoke and the full native suite).
