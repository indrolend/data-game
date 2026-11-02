# GGG-PERF-003 investigation

## Decision

Do not change the playable game yet. Select **native-packed persistent vertex
buffers** as the next isolated implementation candidate. The retained
GGG-PERF-002 evidence rejects GDScript octahedral normal packing, while the new
isolated probe shows that Godot's persistent position-buffer upload mechanism is
fast enough to justify moving packing to native code and measuring it separately.

`Skeleton3D`/GPU skinning is not selected: it has a larger theoretical ceiling,
but the existing native deformation contract cannot be represented by ordinary
bone transforms without redesigning foot correction, flat normals, shell-crack
topology, and several expressive pose modifications.

## Complete current path

1. `HumanModelData::skin()` loads two animation poses for 33 bones, interpolates
   and modifies bone transforms for attacks and expressiveness, allocates world
   and skin matrices, skins 4,164 weighted vertices on CPU, and applies native
   per-vertex foot-contact correction.
2. `ParentGameNode::target_model_vertices()` creates a fresh native float output,
   creates a second filtered vector while removing shell-crack triangles, then
   allocates and fills a `PackedVector3Array` for GDExtension return.
3. `main.gd::_sync()` performs this call independently for every one of 32
   targets and passes each packed position array into `HumanModelAdapter`.
4. `HumanModelAdapter.sync_from_native()` allocates a normal array, recomputes
   one flat normal per triangle, creates a Godot arrays container and a new
   `ArrayMesh`, calls `add_surface_from_arrays()`, and replaces the instance mesh.
5. Godot validates/converts the arrays, octahedrally compresses normals, creates
   a RenderingServer surface and GPU buffers, uploads them, and retires the
   previous mesh resources. This repeats every synchronization.

The DBH1 source contains 4,164 vertices, 33 bones, and 60 animation frames.
Each source vertex contains bind position, four bone indices, and four weights.

## Approximate per-frame allocation/transfer inventory

Per target, excluding container metadata and allocator overhead:

| Current allocation | Approximate bytes |
|---|---:|
| Native world + skin matrices | 4,224 |
| Native skinned float output | 49,968 |
| Native filtered visible vertices | 49,968 |
| Returned `PackedVector3Array` | 49,968 |
| Godot flat-normal array | 49,968 |
| New GPU position + compressed-normal data | 66,624 |

Across 32 targets this is approximately 6.53 MiB of transient CPU payload plus
2.03 MiB of newly uploaded GPU vertex data per frame, before object/container
overhead. Some allocator capacity reuse inside Godot is possible, but source
code constructs fresh vectors, packed arrays, normals, meshes, and surfaces.

## Isolated mechanism prototype

`ggg_perf_003_vertex_upload_probe.gd` creates 32 visible meshes using real
4,164-vertex native human poses. It compares full surface reconstruction with
updating the persistent position region. Two precomputed pose byte arrays are
alternated; native skinning, GDExtension conversion, normal calculation, and
normal packing are outside the timed region. Normals are intentionally constant,
so this is not a visual/gameplay candidate.

Each row is the middle result from three interleaved trials of 180 warm-up and
600 measured frames. Worst is the maximum across the three trials.

| Mechanism | Operation median | p95 | Worst | Frame median | Frame p95 | GPU p95 | Memory median |
|---|---:|---:|---:|---:|---:|---:|---:|
| Rebuild 32 surfaces | 8.013 ms | 8.802 | 10.834 | 8.822 | 9.832 | 0.083 | 34.765 MiB |
| Update 32 position buffers | 0.113 ms | 0.134 | 0.268 | 0.733 | 1.208 | 0.046 | 34.765 MiB |

The position update transfers 49,968 bytes per target (1,598,976 bytes total)
per frame. Both modes increased reported static memory by the same 0.065 MiB
over the sample, providing no mechanism-specific growth signal.

This proves only that persistent uploads are inexpensive. It does not prove the
cost of native normal generation/packing, topology changes, AABB maintenance,
or the final playable integration.

## Architecture comparison

### 1. Native-packed persistent buffers — selected next

- **Mechanism:** keep one dynamic surface per target; have a native presentation
  helper produce Godot-compatible position and octahedrally packed flat-normal
  bytes plus vertex count/topology revision and bounds. Rebuild only when the
  shell-crack vertex count changes. Upload the packed region from Godot without
  per-vertex GDScript work.
- **Complexity:** medium-high. Requires a narrowly scoped native presentation
  packer, byte-layout contract tests, topology/AABB handling, and persistent
  adapter state. It does not alter simulation or physics.
- **Allocations:** can remove the GDScript normal array, per-frame `ArrayMesh`,
  surface, and associated GPU allocation. A first version may still allocate
  native skin output and one packed byte array per target; later scratch-buffer
  reuse must be measured separately.
- **Expected CPU:** current playable enemy bridge + adapter is 7.691 ms median.
  Native packing is expected to add 0.3–0.8 ms to native work, while the isolated
  position upload is 0.113 ms for all 32 targets. Allowing for full normal bytes,
  metadata, and Godot calls, the evidence supports a **2.5–3.8 ms median saving**,
  not the full 7.9 ms isolated delta.
- **Expected GPU:** neutral to slightly lower; persistent uploads had lower GPU
  p95 in isolation. Playable acceptance must require no p95 regression.
- **Parity risk:** medium. Exact byte encoding, current flat normals, topology
  revision, bounds, armor cracks, death/reuse, and OpenGL/Vulkan layout behavior
  need explicit tests.

### 2. Batched vertex transfer

- **Mechanism:** return all target presentation buffers/metadata in one native
  bridge call instead of 32 independent vertex calls.
- **Complexity:** medium.
- **Allocations:** may consolidate dictionaries/calls but does not by itself
  remove native skin outputs, packed arrays, Godot normals, mesh surfaces, or
  GPU allocation.
- **Expected CPU:** low standalone benefit, approximately **0.1–0.5 ms**, because
  measured bridge time is dominated by skinning/filtering/conversion rather than
  32 call dispatches. It becomes useful only as part of native-packed buffers.
- **Expected GPU:** unchanged.
- **Parity risk:** low-medium; ordering, stable IDs, dead targets, and topology
  revisions must remain exact.

### 3. `Skeleton3D` / GPU skinning

- **Mechanism:** upload bind vertices, bone indices/weights once and update bone
  poses for 32 skeletons. This could reduce per-frame transfer from megabytes of
  vertices to roughly 32 × 33 matrices.
- **Complexity:** very high. DBH1 contains the needed rig data, but the native
  code modifies poses beyond source animation and applies post-skin per-vertex
  foot corrections. Damage removes triangles dynamically, and current normals
  are recomputed flat from final deformed triangles.
- **Allocations:** potentially lowest steady-state CPU allocation after a full
  conversion, but new skeleton, skin, index/mask, and pose-buffer infrastructure
  would be required.
- **Expected CPU:** theoretical **5–7 ms saving**, but unmeasured.
- **Expected GPU:** likely higher due to 32 skinned models; estimated +0.2–0.8 ms,
  also unmeasured on the Compatibility renderer.
- **Parity risk:** high: foot planting, attack/expression deformation, exact flat
  normals, shell cracks, target reuse, and desktop/Godot parity could diverge.

## Next isolated candidate and gates

Implement a native presentation-only packer for **one target** that emits the
exact Godot vertex-buffer bytes, topology revision, and AABB. Do not connect it
to `main.gd` yet. Compare it against the current `target_model_vertices()` plus
Godot surface conversion using fixed native snapshots.

Required gates before playable integration:

1. Decode/round-trip positions within 1e-6 and normals with dot product >0.9999.
2. Match current vertex count and triangle ordering for intact and cracked armor.
3. Prove stable output size/allocation behavior across at least 10,000 updates.
4. Measure packer median/p95/worst and require projected total enemy cost below
   5.5 ms median before changing the playable adapter.
5. Exercise ordinary animation, hit, attack, foot contact, soul morph, death,
   target reuse, and topology transitions.
6. Only then add a disabled profiling variant and run three interleaved playable
   trials, visual captures, parity tests, and GPU/memory acceptance checks.

## Verification performed

- Unchanged playable benchmark: 12.599 ms median, 14.294 ms p95, 18.061 ms worst;
  native tick 0.080 ms, enemy bridge 3.403 ms, enemy adapter 4.145 ms, GPU p95
  1.881 ms.
- `performance_profile_parity.gd`: PASS.
- `human_model_presentation.gd`: PASS.
- Camera trauma, HUD, and phone presentation tests: PASS.
- Native suite: 57/57 PASS.

No playable runtime or native C++ code was changed by GGG-PERF-003.
