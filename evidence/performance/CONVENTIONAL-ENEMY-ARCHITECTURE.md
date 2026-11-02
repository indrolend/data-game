# Conventional enemy architecture probe

Date: 2026-10-24

## Decision

Do not replace all enemies yet. The smallest architecture worth advancing is a
static `ArrayMesh` with bind positions/weights, a `Skeleton3D`, and one material,
while the native simulation remains authoritative and exports only final bone
poses plus explicitly retained exceptional presentation state. This removes the
normal case's per-frame 4,164-vertex conversion, flat-normal regeneration, and
surface reconstruction. It is substantially simpler than optimizing each stage
of the existing vertex pipeline.

The isolated representative supports a one-enemy integration pilot, not a
wholesale migration. It reproduces the base DBH clip and silhouette, but it does
not yet reproduce attack/expression bone modifiers, post-skin foot correction,
shell-crack topology removal, soul-collapse morphing, or exact flat normals.
Those are visible parity requirements and must be resolved or deliberately
retained as small exceptional paths before replacement.

## Pipelines compared

| Existing custom path, every sync | Conventional representative |
| --- | --- |
| Native interpolates the clip, applies attack/expression modifications, CPU-skins 4,164 vertices, and applies per-vertex foot correction. | Native gameplay remains unchanged; a bridge would provide 33 final local bone transforms and small exceptional state. |
| GDExtension allocates/converts a `PackedVector3Array` and removes triangles for shell cracks. | Bind vertices, weights, indices, topology, material, and `Skin` are built once. |
| GDScript regenerates 4,164 flat normals and recreates the `ArrayMesh` surface. | Godot updates `Skeleton3D` poses; GPU skinning reuses the static mesh. |

The probe parses the production `human.dbhuman` asset (4,164 vertices, 33
bones, 60 frames), creates one conventional Godot rig, and renders it beside the
unchanged `HumanModelAdapter`. The conventional timing covers interpolation and
33 `set_bone_pose` calls. The custom timing covers the real native
`target_model_vertices` bridge call plus `HumanModelAdapter.sync_from_native`.
The two paths run in the same frame after 180 warm-up frames for 600 samples.

## Results

Windows 11 Pro build 26200; AMD Ryzen 9 5900X (12 cores/24 threads); NVIDIA
GeForce RTX 3060, driver 32.0.16.1088; Godot 4.7.2; Compatibility/OpenGL 3.3;
1280x720; VSync off; uncapped; one shadowed directional light.

| Trial | Custom median / p95 / worst (ms) | Skeleton median / p95 / worst (ms) | GPU p95 (ms) | Static-memory growth (MiB) |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 0.542 / 0.672 / 0.887 | 0.059 / 0.071 / 0.128 | 0.118 | 0.0811 |
| 2 | 0.544 / 0.674 / 0.718 | 0.061 / 0.071 / 0.108 | 0.118 | 0.0811 |
| 3 | 0.543 / 0.673 / 0.744 | 0.060 / 0.071 / 0.102 | 0.118 | 0.0811 |

Across trials, the representative mechanism reduces median CPU submission by
0.483 ms per enemy (88.9%, about 9.1x) and p95 by 0.602 ms (89.5%, about 9.5x).
This is an isolated one-enemy result, not an expected whole-frame saving: the
playable baseline includes multiple enemies, bridge/snapshot work, other scene
synchronization, and features omitted by the conventional prototype. Memory
rose once during warm-up/sampling and then plateaued at the same 0.0811 MiB in
all three runs; no continuing growth was observed over 600 measured frames.

## Recommendation and verification gate

Build one playable, feature-complete pilot behind a presentation-only switch.
Add a native read-only final-bone-pose snapshot; keep physics, collision, AI,
damage, targeting, animation clocks, and all authoritative state untouched.
Represent shell damage with a small fixed set of prebuilt surfaces or masks;
first determine whether foot correction can be expressed as final bone poses.
If it cannot, retain only that correction as an exceptional GPU-compatible
deformation rather than restoring the general CPU vertex pipeline.

Replace the pipeline only if the pilot:

- matches attack, expression, feet, shell cracks, soul collapse, material,
  silhouette, and animation captures at representative states;
- produces identical native snapshots and passes all parity tests;
- retains the measured CPU advantage in three interleaved playable trials with
  no GPU-p95 or unbounded-memory regression; and
- deletes more bridge/adapter processing than it adds in exception handling.

The expected benefit for one enemy is 0.48 ms median and 0.60 ms p95 CPU time,
with high confidence for the isolated base-animation mechanism and low
confidence for end-to-end playable savings until parity features are included.

## Evidence and commands

Raw outputs are `conventional-enemy-trial-{1,2,3}.json`, `.log`, and `.png`.
`conventional_enemy_probe.gd` is the reproducible probe. The images are evidence
of base silhouette/animation only, not pixel-equivalence proof.

```powershell
godot_console.exe --path godot --resolution 1280x720 --rendering-method gl_compatibility --script res://../evidence/performance/conventional_enemy_probe.gd -- --output-stem=conventional-enemy-trial-1
godot_console.exe --headless --path godot --script res://tests/performance_profile_parity.gd
godot_console.exe --headless --path godot --script res://tests/human_model_presentation.gd
godot_console.exe --headless --path godot --script res://tests/camera_trauma_presentation.gd
godot_console.exe --headless --path godot --script res://tests/hud_presentation.gd
godot_console.exe --headless --path godot --script res://tests/phone_presentation.gd
ctest --test-dir build -C Release --output-on-failure
```

All five Godot tests passed. Native tests passed 57/57. No playable code was
changed by this investigation.

