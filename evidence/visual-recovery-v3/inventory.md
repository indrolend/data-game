# Visual recovery inventory V3

Baseline: `de26438d2639536741c05d2511f8b2ba65edb7db` on `checkpoint/godot-latest-20261022`, matching `data-main/godot-migration` before this slice. The working tree was already dirty; unrelated files were preserved. Status labels describe the strongest available evidence, not aspiration.

| Area | Status | Native authority and Godot evidence | Finding |
| --- | --- | --- | --- |
| Phone geometry/display/animations/HUD | Implemented but visually unverified | `parent/models/phone.dbmesh`, `parent/game/PhoneDisplay.hpp`, `native/phone_presentation_snapshot.hpp`, `godot/scripts/phone_presentation_adapter.gd`, `godot/tests/phone_presentation.gd`; commits `94ab70f`, `f38a2de` | Native geometry, state-derived substrate, light, menu layout, first-person placement, damage/capture pulses, and partial HUD are adapted. Exact desktop typography/composition and a matched native/Godot sequence remain absent. |
| Camera placement/first/third person | Recovered and independently verified | `parent/game/Game.cpp` camera state, `ParentGameNode::camera_*`, schema-4 observations, `godot/tests/human_model_presentation.gd`; commits `6775630`, `de26438` | Native camera position/target/FOV drive Godot. Presentation-only camera-ray fading is ablation-tested. This does not prove pixel-identical framing. |
| Vacuum/target/tether/ingestion/souls | Recovered and independently verified | `parent/game/Game.cpp::updateVacuum`, target/soul snapshots, `human_model_adapter.gd`, `soul_lattice_adapter.gd`, `human_model_presentation.gd` | Native target selection, lock, ingestion, tether endpoints, soul lattice, capture pulse, and stored-soul transition are contract-tested and captured. |
| Enemy model/skin/damage/cracking/destruction | Implemented but visually unverified | `parent/models/human.dbhuman`, native skin/crack/damage fields, `human_model_adapter.gd`, `native_particle_presentation_adapter.gd` | Original skinned vertices and native crack/damage state are adapted. Simplified particle spheres and lack of exact native-renderer sequence keep visual parity unverified. |
| Lighting/atmosphere/day-night | Implemented but visually unverified | `native/lighting_presentation_snapshot.hpp`, `main.gd`, `open_sky_dome.gdshader`, `moon_surface.gdshader`, `lighting_presentation.gd`; commits `5e22482`, `06fc260` | Native lighting and celestial inputs drive Godot; sky silhouettes, Moon model, and dramatic spotlight are render adaptations. Original shadow/material and weather parity are unsupported. |
| Projectiles/combat feedback/particles | Recovered and independently verified | Native bullet slots, `projectile_presentation_snapshot`, stable Godot projectile nodes, schema-4 transform error, `projectile_observation.gd`; commit `6775630` | Projectile identity, position, velocity, visibility, and zero transform error are observed. Generated sphere geometry and particle appearance remain adaptations, not GLB or desktop parity. |
| Room transitions/objectives/environment | Behaviorally divergent at baseline; presentation implemented in V3 | Native `DoorTransitionState` and `DesktopRenderer::drawDoorDataMosh`; baseline Godot only changed room/upgrade state. V3 adds `door_transition_presentation.gd` and `door_datamosh.gdshader`. | Before V3, native room progression worked but the characteristic doorway datamosh was missing. V3 correlates native progress/motion to a Godot screen-space block peel. Whole-frame captures were inspected, but exact renderer parity remains unverified. |

## Highest-value next slices

1. Phone typography and exact pause composition using a matched native/Godot state pair.
2. Replace shared particle spheres with native-material-specific shapes while preserving native particle state.
3. Coastal foam/ocean extent and source-renderer comparison.
4. Sterile tile, bark, and sand source texture hookups.
5. A native/Godot matched-state doorway transition sequence to measure temporal and pixel divergence.
