# Living Worlds: City vertical slice

## Runtime truth

- Starting branch: `codex/masterpiece-vertical-slice-mission`
- Starting commit: `b6b0552cb8144c0dacf9570931bd2e2f4f2885f5`
- Configuration: Release, Windows x64
- Executable: `C:/Users/indro/Projects/data-game-tools/artifacts/build/b6b0552cb814-Release/bin/Release/DigitalBreakdown.exe`
- Fixture: seed 2, room 2, `CITY/CORRIDOR`, 1280x720, HUD hidden
- The active desktop path is the fixed-function renderer in `desktop/DesktopRenderer.cpp`; the existence of asset or grammar files alone was not treated as runtime evidence.

## Baseline diagnosis and priority

| Rank | Observed problem | Active path | Decision |
| --- | --- | --- | --- |
| 1 | City reused the enclosed Sterile room shell. A large terminal wall cut off the street, erased depth, and made the city read as an indoor runway. | `DesktopRenderer::drawRoomTile` | Keep City exterior continuous; let buildings and the street define its edges. |
| 2 | The ground was one broad textured slab with a single center stripe. There were no curbs, gutters, intersections, crossings, or drainage cues. | `drawCityGround` and room-tile decoration | Add a deterministic, quality-aware street section without changing collision. |
| 3 | Buildings had repeated windows but insufficient construction hierarchy and authored edge detail. | `CityFacadeGrammar.hpp` and City obstacle presentation | Assimilate a small set of actual CC0 facade modules into selected eye-level bays; retain the cheaper grammar above and at distance. |

The implementation is hybrid: traversal, collision, street topology, building mass, and family selection remain deterministic; selected architectural surface modules retain authored geometry. This avoids both arbitrary shape stacking and an undirected asset-pack appearance.

## Public reference asset and derivatives

Source: [Poly Haven — Modular Urban Apartments Facade](https://polyhaven.com/a/modular_urban_apartments_facade)

- Author: James Ray Cock
- License: CC0
- Acquired: 2026-10-14 through the Poly Haven API
- Source package kept under ignored `artifacts/reference-assets/modular_urban_apartments_facade/`
- Source glTF SHA-256: `1A5A17DFFD27FB9E1236DEA7E51C4E0393A9D88ED0885A0621E67A37F80B27EB`
- Source buffer SHA-256: `F041E71B6BD31864F20DD64D3A82519454A603C300686161E0349FF44F838BF2`

Selected authored vocabulary:

- `window_centered_large_01`
- `window_centered_small_01`
- `door_centered_large_01`
- `door_window_small_01`
- `cornice_standard_standard_01`

The converter now accepts GLB or external-buffer glTF and can extract one exact named node with `--include`. It writes the same compact DBM1 runtime format plus optional inspectable structural metadata. Runtime derivatives live in `models/environment/city/`; provenance, source bounds, triangle counts, selected node names, and normalized bounds live in `models/environment/structure/`.

The source textures and mesh hierarchy were inspected rather than blindly copied. The plaster bitmap is a generic surface, while the separate window and door nodes contain the useful authored frames, mullions, glass, and proportions. The current renderer has no UV channel in DBM1, so this slice keeps those assembly meshes and their semantic object/glass batches while retaining Digital Breakdown's existing facade colors. Copying the generic plaster bitmap onto every building would have reduced material variation rather than improved it.

## Implemented slice

1. City no longer draws the shared enclosed-room wall shell.
2. `CityStreetGrammar.hpp` provides deterministic road width, sidewalks, raised curbs, gutters, an intersection, crossing marks, drains, lamps/service clusters, and distant skyline bands. Low/Normal/Pretty preserve the same critical layout and reduce optional repetition only.
3. Normal/Pretty render actual CC0 window, door, door-window, and cornice modules on deterministic eye-level facade selections. Upper and distant storeys use the existing cheaper grammar. The same assemblies replace box-built openings on the existing City House landmark primitive.
4. Imported tree families now receive deterministic width/depth, lean, yaw, and root-spread variation instead of uniform scale variation. Their gameplay collision and climb contracts remain unchanged.
5. Continuous deterministic capture now honors `--capture-room-seed`, `--capture-room-index`, and `--capture-graphics-preset`, allowing this exact City fixture to be recorded through a kill and soul capture.

## Cross-environment audit

| World | Current strength | Largest remaining weakness | Next small experiment |
| --- | --- | --- | --- |
| City | Facade grammar, authored modular vocabulary, continuous street section, skyline depth | Sparse street-level occupation; windows remain uniformly dark and the road foreground is broad | Add one deterministic street-corner/service family using existing modular door/corner/utility pieces, plus sparse lit-window occupancy driven by room/time state. |
| Field | Four authored CC0 tree silhouettes, photo grass/bark, wind grass, day/night atmosphere | Vegetation distribution and terrain silhouette still read as placed props on a floor | Use structural tree variants in two ecologically plausible clusters and add one worn-ground transition around traversal routes. |
| Coastal | Existing photo sand, deterministic animated water grid, shallow/deep color and foam, faceted rocks | Sand repetition/flatness and the hard land-water boundary; source texture is not the primary failure | Keep Ground054; adjust world UV scale/tint and add a narrow damp-sand/foam transition before changing assets. |
| Sterile | Institutional shell, tile material, fixtures/panels, controlled lighting | Repeated corridor composition and wear that does not yet follow plumbing, thresholds, or traffic | Author one service-room threshold kit from existing panels/fixtures and place localized moisture/corrosion by causal anchors. |

### Follow-through: existing Field and Coastal primitives

- Field backdrop trees now use the same deterministic structural variation as climbable authored trees: nonuniform width/depth, lean, yaw history, species-specific root counts, and grounded root flares. The four imported CC0 families remain the source silhouettes; collision is unchanged.
- Coastal keeps ambientCG Ground054 as its only sand source. A narrow darker, finer-scale saturated strip now bridges dry sand and computed water instead of introducing another beach material.
- Coastal collider obstacles now reuse the existing deterministic faceted-rock vocabulary and ambientCG stone material instead of rendering as moss-textured boxes. The renderer uses dominant-axis face projection so the photograph retains sensible scale on both top and side faces.
- Rock presentation is deliberately separate from rock support authority. `makeMesh` remains the established physics/AI surface; `makeVisualMesh` supplies the broader irregular plateau silhouette inside the same bounds. This separation was required after traversal evidence caught an initial shared-mesh regression.

## Evidence and validation

- Baseline: `artifacts/living-worlds/baseline/city-seed2-room2.png`
- Final matched frame: `artifacts/living-worlds/after/city-seed2-room2-v6.png`
- Low-quality identity frame: `artifacts/living-worlds/after/city-seed2-room2-low.png`
- Field before/after: `artifacts/living-worlds/baseline/field-seed2-room1-before-grounding.png`, `artifacts/living-worlds/after/field-seed2-room1-grounded.png`
- Coastal before/after: `artifacts/living-worlds/baseline/coastal-seed11-room12.png`, `artifacts/living-worlds/after/coastal-seed11-room12-final.png`
- Continuous gameplay: `artifacts/living-worlds/gameplay/city-seed2-room2-assemblies.mp4`
- Continuous capture includes an active enemy, melee defeat, soul exposure, vacuum capture, particles/fragments, player movement, and recovery; it completed with one stored soul.
- Release build succeeded.
- CTest: 52/52 passed, including multiplayer, traversal, environment, soul economy, City street grammar, and tree structural variation.
- `git diff --check`: clean.

The capture-instrumented 640x360 run reported roughly 43 ms mean renderer time, but this path synchronously reads frames back from the GPU every other update and produced 150 PPM frames. It is evidence of deterministic continuity, not representative gameplay frame time. No trustworthy before/after GPU timing is available from the current harness, so this pass does not claim a performance win. To bound imported cost, authored high-density bays are restricted to selected eye-level facades; Low renders the same city composition with the procedural facade fallback.

## Honest remaining weakness

The City is materially more coherent and has real depth, but it is not yet a convincing inhabited metropolis. Street-level frontage is too empty, most windows are black, and the broad foreground road still dominates the image. The next highest-leverage experiment is not another skyline layer: it is one detailed, reusable street-corner/service family assembled from the same CC0 kit, with entrances, a termination/corner module, utility hardware, and sparse state-driven window light. That would put authored information where the player can actually inspect it while preserving current performance and traversal contracts.
