# Godot/native convergence: enemy vertical slice

## Prioritized convergence plan

1. Finish one production enemy as a persistent Godot `ArrayMesh`, `Skin`, and
   `Skeleton3D`, with compact native pose/state snapshots and visual parity.
2. Once parity is proven, convert the DBH source asset to an editor-imported
   Godot scene/resource and delete the runtime rig construction and legacy
   per-frame vertex path.
3. Move other purely presentational systems toward authored scenes/resources
   only when profiling shows value; retain native deterministic gameplay,
   procedural systems, combat, progression, and specialized math.
4. Replace remaining broad polling with narrow state/revision signals where it
   reduces work without obscuring the deterministic tick boundary.

## Implemented slice

`DB_ENABLE_SKELETON_ENEMY_SLICE=1` switches target 0 only to a persistent Godot
rig. The GDExtension transfers bind vertices, weights, hierarchy, and inverse
binds once, then transfers 33 final local pose transforms per sync. Native C++
still chooses animation time and applies attack and expressiveness mathematics.
Godot owns the mesh, material, `Skin`, `Skeleton3D`, obstruction alpha, scene
transform, and GPU skinning. Other targets and the default game use the existing
adapter, so incomplete visual parity cannot silently change the shipped game.

For the opted-in target, the executing path no longer transfers 4,164 deformed
vertices, regenerates 4,164 flat normals, allocates an `ArrayMesh`, or recreates
its surface every frame. Legacy code has not yet been deleted because it is the
validated fallback and still supplies three effects the slice does not match.

## Playable performance

Existing profiler, Compatibility/OpenGL 3.3, 1280x720, VSync off, 180 warm-up
and 600 measured frames. Trials were interleaved baseline then candidate.

| Trial | Baseline median / p95 / worst | Slice median / p95 / worst | Enemy-sync median change | GPU p95 change |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 14.147 / 17.027 / 20.118 ms | 13.308 / 16.057 / 22.220 ms | -0.693 ms | +0.080 ms |
| 2 | 13.454 / 16.177 / 24.295 ms | 12.941 / 15.017 / 17.855 ms | -0.372 ms | -0.027 ms |
| 3 | 13.202 / 15.395 / 17.715 ms | 12.807 / 14.929 / 17.419 ms | -0.354 ms | +0.222 ms |

Median frame time improves by 0.395–0.839 ms and p95 by 0.466–1.160 ms from
converting only one target. GPU p95 is noisy and changes by -0.027 to +0.222 ms;
this does not establish a GPU improvement or regression. Raw JSON and logs are
stored in this directory. `skeleton-slice.png` is the production-scene capture.

## Validation and remaining gate

- GDExtension Release build: PASS.
- Persistent-rig test, including 120-frame native-state comparison: PASS.
- Performance parity, human model, camera trauma, HUD, phone, and gameplay-loop
  Godot tests: PASS.
- Native CTest suite: 57/57 PASS.
- `git diff --check`: PASS.

Unverified and therefore not enabled by default: exact native post-skin foot
correction, shell-crack triangle removal, and soul-collapse vertex motion.
Attack and expressiveness are included in native pose transforms. Bind-pose
normals are visually acceptable in the capture but are not pixel-equivalent to
the legacy path's post-deformation flat normals. AnimationTree authoring and an
editor-imported model scene remain future work because the current source asset
contains one baked clip in the proprietary DBH format. Enabling the slice for
all targets or deleting the fallback requires visual checkpoints for these
states and three new full-playable trials.

