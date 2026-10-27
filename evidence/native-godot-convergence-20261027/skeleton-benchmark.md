# Persistent skeleton boundary experiment

Source base: `8e6824278f1226f0d74950688d6cf7badb736d30`, with the preserved presentation experiment enabled only for target zero through `DB_ENABLE_SKELETON_ENEMY_SLICE=1`.

Each process warmed up for 60 frames and measured 300 deterministic frames. Five fresh processes were run per condition. Values below are medians across the five per-process medians.

| Metric | Full vertex path | Persistent skeleton path | Change |
|---|---:|---:|---:|
| Total `_sync()` | 7,923 us | 7,683 us | -3.03% |
| Enemy bridge + adapter | 6,907 us | 6,662 us | -3.55% |
| Enemy adapter | 3,783 us | 3,383 us | -10.57% |
| Native tick | 55 us | 56 us | effectively unchanged |

The candidate transfers the 4,164-vertex bind mesh and 33-bone rig once, preserves persistent mesh identity, and sends compact poses for the first enemy. Other enemies retain the full-vertex path, so this is a bounded vertical-slice result rather than a projected full-scene speedup.

Verification:

- `skeleton_enemy_slice.gd`: pass.
- `performance_profile_parity.gd`: pass for all ablations.
- `camera_trauma_presentation.gd`: pass; disabled by default and no native divergence.
- Native CTest: 57/57 pass.
- Existing Godot contracts: 18/18 pass.

The experiment supports continuing the persistent-resource direction. It does not yet establish GPU-frame improvement or visual parity; promotion beyond target zero requires matched GPU captures and all-target measurements.
