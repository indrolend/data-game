# GGG-PERF-004: native-packed persistent enemy buffer

## Decision

**Retain the one-enemy candidate as an opt-in experiment and promote it to the
next all-target A/B. Do not make it the default presentation path yet.**

The candidate preserves the native deformation contract and produced a
repeatable complete-scene improvement for one enemy. It does not yet establish
all-target topology-transition behavior, total scaling, or cross-renderer
behavior.

## Function-level change

The control path remains the default:

`HumanModelData::skin()` → `ParentGameNode::target_model_vertices()` →
`HumanModelAdapter.sync_from_native()` → new normals, `ArrayMesh`, surface and
GPU upload every synchronization.

The opt-in candidate, selected only with
`DB_ENABLE_PACKED_ENEMY_SLICE=1`, is:

1. `ParentGameNode::build_target_model_vertices()` shares the existing native
   skinning, foot correction, and shell-crack filtering between the control and
   candidate bridge methods.
2. `ParentGameNode::target_model_packed_buffer()` computes the same final flat
   face normals and writes Godot's surface vertex-buffer layout directly:
   contiguous 32-bit XYZ positions followed by one 32-bit octahedrally encoded
   normal per vertex. It also returns vertex count and final-deformed bounds.
3. `PackedHumanModelAdapter.sync_packed_from_native()` keeps one dynamic
   `ArrayMesh` surface and calls `surface_update_vertex_region()` for ordinary
   animation updates.
4. A vertex-count change triggers one control-geometry fetch and one persistent
   surface rebuild. This preserves the native shell-crack triangle list rather
   than forcing stale topology.
5. `HumanModelAdapter.sync_presentation_state()` factors the existing material,
   hit flash, soul fade, obstruction fade, transform, expressive scale, and
   tether state so both render paths consume identical presentation state.

Native gameplay, collision, combat, locomotion, physical response, animation
selection, foot targets, damage, and soul state are unchanged.

## Why GGG-PERF-002 failed and this candidate differs

GGG-PERF-002 packed octahedral normals and maintained bounds in GDScript. That
raised adapter median from roughly 4.1 ms to 7.4 ms and regressed GPU p95.

GGG-PERF-004 moves normal generation and byte packing into the existing native
presentation bridge. Godot receives one `PackedByteArray` and performs one
persistent buffer update. No per-vertex GDScript packing occurs during ordinary
updates.

## Exact geometry contract

`godot/tests/packed_enemy_buffer.gd` compares candidate output against
`target_model_vertices()` for intact, animated, and cracked states.

- Position round-trip tolerance: `1e-6`.
- Decoded normal requirement: dot product greater than `0.9999` against the
  control flat face normal.
- Bounds: every final-deformed vertex contained.
- Ordinary animation: `ArrayMesh` identity remains unchanged for 120 updates.
- Shell crack: native vertex count decreases and exactly one topology rebuild
  is requested before the new surface becomes persistent.

Result: **PASS**.

## Complete playable-scene performance

Three interleaved control/candidate trials used the existing representative
Field encounter: 180 warm-up frames, 600 samples, 1280 x 720 OpenGL
Compatibility, VSync disabled, uncapped frame rate, identical deterministic
input, and one packed enemy out of 32. Values are the middle of three trial
medians/p95 values; worst is the maximum across the three trials.

| Measurement | Control | One packed enemy | Change |
|---|---:|---:|---:|
| Frame median | 9.906 ms | 9.549 ms | **-0.357 ms (-3.60%)** |
| Frame p95 | 10.594 ms | 10.368 ms | **-0.226 ms (-2.13%)** |
| Worst sampled frame | 20.334 ms | 15.265 ms | -5.069 ms |
| Scene sync median | 8.335 ms | 7.972 ms | **-0.363 ms** |
| Enemy bridge + adapter median | 7.027 ms | 6.649 ms | **-0.378 ms** |
| Enemy bridge median | 3.074 ms | 3.100 ms | +0.026 ms |
| Enemy adapter median | 3.907 ms | 3.504 ms | **-0.403 ms** |
| GPU median | 2.296 ms | 2.318 ms | +0.022 ms |
| GPU p95 | 2.620 ms | 2.633 ms | +0.013 ms |
| Static memory median | 46.930 MiB | 46.930 MiB | effectively unchanged |

Raw JSON is retained in
`evidence/generated/packed-buffer-experiment/{control,packed}-{1,2,3}.json`.
The candidate improved all three frame medians and p95 values. The 0.013 ms GPU
p95 difference is below the observed run-to-run noise and is not a meaningful
regression in this experiment.

## Rendered parity

The graphical fundamental-loop sequence passed for both control and candidate:
enemy hit, phone damage response, shell cracking, melee effects, capture light,
settled residue, and phone menu.

Matched capture comparison:

| Checkpoint | Changed pixels | Mean absolute channel delta | Visual result |
|---|---:|---:|---|
| Gameplay phone | 13 | <0.001 / 255 | equivalent |
| Enemy hit | 2,908 | 0.02 / 255 | equivalent pose, silhouette and material |
| Shell crack | 263 | 0.01 / 255 | equivalent topology and effects |
| Capture phone light | 0 | 0 | byte-identical |
| Settled residue | 0 | 0 | byte-identical |
| Phone menu | 0 | 0 | byte-identical |

The small non-identical regions are sub-visible lighting/timing differences;
manual inspection found no changed silhouette, pose, crack mask, material,
effect placement, or gameplay cue. Captures are retained under
`evidence/generated/packed-buffer-experiment/visual-{control,packed}/`.

## Validation

- Godot 4.7.2 editor parse and GDExtension load: **PASS**.
- Native suite: **57/57 PASS**.
- `packed_enemy_buffer.gd`: **PASS**.
- `human_model_presentation.gd`: **PASS** in control and packed modes.
- `performance_profile_parity.gd`: **PASS** in control and packed modes.
- `parent_smoke.gd`: **PASS** in control and packed modes.
- `playable_encounter.gd`: **PASS** in control and packed modes.
- `fundamental_loop_visual_capture.gd`: **PASS** graphically in both modes.

## Promotion boundary and next experiment

The mechanism has passed the one-enemy acceptance gate. It should next be
enabled for all targets behind a separate opt-in mode and measured in three
interleaved complete-scene trials. That experiment must specifically exercise:

- simultaneous intact and cracked topologies;
- multiple targets changing topology in one window;
- hit, attack, foot-contact, falling/recovery, vacuum, soul collapse, death,
  restart, and target reuse;
- surface identity except at topology transitions;
- total bridge allocation, static memory, GPU p95, and frame variance.

Only a successful all-target playable and visual result should replace the
default control path. No linear extrapolation from the one-enemy saving is used
as evidence.

## Reproduction

```powershell
# Control
Remove-Item Env:DB_ENABLE_PACKED_ENEMY_SLICE -ErrorAction SilentlyContinue
godot_console.exe --path godot --resolution 1280x720 `
  --rendering-method gl_compatibility `
  --script res://../evidence/performance/profile_playable.gd -- `
  --variant=baseline --output=C:/absolute/path/control.json

# Candidate
$env:DB_ENABLE_PACKED_ENEMY_SLICE = '1'
godot_console.exe --path godot --resolution 1280x720 `
  --rendering-method gl_compatibility `
  --script res://../evidence/performance/profile_playable.gd -- `
  --variant=baseline --output=C:/absolute/path/packed.json
Remove-Item Env:DB_ENABLE_PACKED_ENEMY_SLICE
```
