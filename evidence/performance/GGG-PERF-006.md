# GGG-PERF-006 — Invisible soul lattice bridge gate

## Decision

Promote. The change removes native-to-Godot conversion of 27 lattice points for
each invisible target while preserving the complete visible-soul path.

## Cause

`main.gd::_sync()` requested `target_soul_lattice(i)` for all 32 target slots on
every frame. `SoulLatticeAdapter.sync_from_native()` immediately discarded the
array unless `visible` was true, `morph_scale` exceeded 0.001, and the array had
exactly 27 points.

The retained path now reads the small soul snapshot first and requests the
lattice only under those same visibility/morph conditions. Simulation and the
snapshot remain native-authoritative.

## Measurements

Godot 4.7.2, OpenGL Compatibility, 1280×720, VSync disabled, RTX 3060. Each row
is a fresh process with 180 warmup and 600 measured playable frames.

| Condition | Trial | Median frame | P95 frame | Median sync | Enemy/soul total | Bridge only |
|---|---:|---:|---:|---:|---:|---:|
| Control | 1 | 4.51 ms | 5.17 ms | 2.92 ms | 1.86 ms | 1.62 ms |
| Control | 2 | 4.50 ms | 5.44 ms | 2.92 ms | 1.87 ms | 1.64 ms |
| Control | 3 | 4.33 ms | 5.29 ms | 2.77 ms | 1.83 ms | 1.60 ms |
| Gated | 1 | 4.15 ms | 4.66 ms | 2.60 ms | 1.74 ms | 1.52 ms |
| Gated | 2 | 4.22 ms | 4.70 ms | 2.65 ms | 1.76 ms | 1.54 ms |
| Gated | 3 | 4.12 ms | 4.76 ms | 2.62 ms | 1.74 ms | 1.53 ms |

Median-of-three comparison: frame 4.50→4.15 ms, P95 5.29→4.70 ms, sync
2.92→2.62 ms, enemy/soul total 1.86→1.74 ms, bridge 1.62→1.53 ms.

## Equivalence gates

- Godot editor parse: pass.
- `human_model_presentation.gd`: pass, including the intermediate cracked-shell
  to 27-point soul-lattice morph.
- `parent_smoke.gd`: pass, including target defeat, ingestion, and capture.
- `playable_encounter.gd`: pass.
- `performance_profile_parity.gd`: pass.
- `fundamental_loop_visual_capture.gd`: pass with rendered enemy-hit,
  shell-crack, capture-light, settled-residue, and phone-menu checkpoints.
