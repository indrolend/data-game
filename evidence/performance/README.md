# Digital Breakdown Godot performance investigation

Follow-up: [GGG-PERF-003](GGG-PERF-003.md) preserves the GGG-PERF-002 rejection,
measures persistent position uploads in isolation, compares native-packed
buffers, batched transfer, and GPU skinning, and selects a one-target native
presentation packer as the next isolated candidate. No playable code changed.

## Result

The playable Godot path is CPU-bound by presentation synchronization, not by
authoritative simulation or the GPU on this machine. Baseline median frame time
was 12.371 ms (80.83 FPS), p95 was 14.023 ms, and the worst sampled frame was
20.098 ms. `_sync()` consumed 9.287 ms median; `runtime.tick()` consumed only
0.079 ms median.

The dominant cost is the enemy/model path. Removing only that presentation work
while retaining identical native gameplay reduced median frame time by 7.860 ms
and `_sync()` from 9.287 ms to 2.069 ms. Split instrumentation measured the
enemy GDExtension bridge at 3.290 ms median and the Godot human-model adapter at
4.035 ms median. The combined path skins and transfers 32 models / 133,248
vertices, computes a fresh normal array, creates a new `ArrayMesh`, and replaces
each model mesh every synchronization.

## Controlled A/B results

Each row is the middle result from three independent trials. Each trial used
180 warm-up frames followed by 600 measured frames. `p95` is the middle of the
three trial p95s; `worst` is the worst frame across all three trials.

| Variant | Median ms | p95 ms | Worst ms | Median FPS | Median saving | GPU median ms | Draw calls |
|---|---:|---:|---:|---:|---:|---:|---:|
| Baseline | 12.371 | 14.023 | 20.098 | 80.83 | — | 0.764 | 292 |
| No enemy/model presentation | 4.511 | 5.736 | 9.472 | 221.68 | 7.860 ms | 0.673 | 284 |
| No shadows | 10.992 | 12.228 | 17.129 | 90.98 | 1.379 ms | 0.297 | 65 |
| No grass | 11.678 | 13.155 | 16.465 | 85.63 | 0.693 ms | 0.745 | 199 |
| No phone | 11.837 | 13.488 | 16.819 | 84.48 | 0.534 ms | 0.729 | 259 |
| No particles | 12.075 | 13.754 | 16.410 | 82.82 | 0.296 ms | 0.760 | 292 |
| No HUD | 12.080 | 13.562 | 17.804 | 82.78 | 0.291 ms | 0.745 | 287 |

The last three deltas are small enough to treat as secondary until a proposed
change is measured. They are not evidence that hiding those systems is a useful
product optimization.

## Baseline component timings

| Component | Median ms | p95 ms | Worst ms |
|---|---:|---:|---:|
| Native authoritative tick | 0.079 | 0.092 | 0.271 |
| Entire Godot `_sync()` | 9.287 | 10.400 | 14.164 |
| Enemy GDExtension bridge | 3.290 | 3.710 | 4.918 |
| Human model adapter | 4.035 | 4.406 | 5.685 |
| Combined enemy bridge + adapter | 7.406 | 8.104 | 9.922 |
| Room GDExtension bridge | 0.167 | 0.206 | 0.662 |
| Room adapter | 1.132 | 1.324 | 2.692 |
| HUD bridge + adapter | 0.253 | 0.347 | 0.899 |
| Phone bridge + adapter | 0.098 | 0.139 | 0.348 |
| Particle bridge + adapter | 0.010 | 0.142 | 0.350 |
| Viewport render CPU | 1.014 | 1.706 | 5.015 |
| Viewport render GPU | 0.764 | 1.728 | 2.749 |

Godot's viewport timing API reported valid GPU measurements on this OpenGL
Compatibility run. Static memory was 46.64 MiB median and Godot reported 2,469
objects median. The baseline rendered 147,169 primitives median, rising to
240,326 at p95 as the deterministic encounter evolved.

## Ranked bottlenecks and evidence-backed fixes

1. **Per-frame enemy skin/transfer/mesh reconstruction (7.860 ms A/B ceiling).**
   `ParentGameNode::target_model_vertices()` allocates native vectors and a
   `PackedVector3Array` for every target. `human_model_adapter.gd` then allocates
   the mesh arrays and normals and constructs a new `ArrayMesh` every frame.
   The first fix to prototype is persistent mesh storage with in-place vertex/
   normal buffer updates, measured separately from native skinning. A second
   controlled candidate is visibility/distance update cadence for presentation
   only; native target ticks must remain at full rate. Acceptance should recover
   several milliseconds without changing target snapshots or native hashes.

2. **Shadow rendering (1.379 ms frame delta; 0.467 ms median GPU delta).**
   The active dramatic spotlight shadow expands the run from 65 to 292 draw
   calls. Prototype a tighter shadow caster mask/range for that presentation
   light, then separately test disabling only its shadow. Preserve native
   lighting state and do not alter gameplay visibility or target logic.

3. **Grass node synchronization (0.693 ms frame delta; 93 draw-call delta).**
   The room holds 160 individual `MeshInstance3D` blades with duplicated
   materials and updates each transform every sync. Prototype one `MultiMesh`
   with per-instance transform/color updates. Validate identical native blade
   roots/tips and compare captures before adopting it.

4. **General room materialization (1.132 ms adapter median).**
   `RoomPresentationAdapter.sync_snapshot()` revisits every structure and
   assigns material overrides every frame. Add dirty/version evidence first,
   then prototype skipping immutable structure/material work while continuing
   dynamic grass/water/capture updates. Do not cache until room transition and
   presentation-state invalidation cases are covered.

5. **HUD, phone, and particles are currently secondary.** Their individual
   medians and A/B deltas are small relative to enemy and shadow costs. No
   optimization is justified from this run alone. Particle load was bursty but
   low in this deterministic encounter; a dedicated particle-saturation fixture
   would be required before changing it.

## Single highest-confidence optimization

> **GGG-PERF-002 result:** the first persistent-buffer candidate was implemented,
> measured, rejected, and reverted. GDScript-side octahedral normal packing made
> it 3.685–3.816 ms slower at the median and also regressed GPU p95. See
> `GGG-PERF-002.md` and the retained raw trials. The section below records the
> original hypothesis and must not be read as a currently accepted optimization.

**Prototype persistent enemy `ArrayMesh` storage with in-place vertex/normal
buffer updates in `HumanModelAdapter`; do not change native skinning, target
simulation, animation time, visibility rules, or the GDExtension contract in
the first experiment.**

This is the highest-confidence choice because it attacks a directly measured
4.035 ms median / 4.406 ms p95 / 5.685 ms worst Godot adapter cost whose source
is visible in one short code path: every live model allocates arrays, recomputes
normals, constructs an `ArrayMesh`, creates a surface, and replaces the mesh on
every `_sync()`. It is narrower and lower-risk than changing enemy update
cadence, and unlike the shadow candidate it addresses the primary CPU limit.

### Expected benefit and uncertainty

- **Measured hard ceiling:** 4.035 ms median from the adapter itself. The full
  enemy-disable ceiling is 7.860 ms median, but 3.290 ms median of that is the
  native/GDExtension bridge and is intentionally outside this first candidate.
- **Engineering expectation:** recover **3.000 ms median frame time**, reducing
  this workload from 12.371 ms to approximately **9.371 ms** (80.83 to about
  **106.71 FPS**, a 24.3% frame-time reduction). This is a target, not a measured
  result; no optimization has been implemented.
- **Acceptance floor:** at least **2.000 ms median** and **2.000 ms p95** saved
  against an interleaved three-trial baseline, with no statistically material
  GPU-time regression.
- **Uncertainty range:** **2.0–4.0 ms median**. Persistent buffers still require
  vertex uploads and some normal work. The existing timer covers normal
  generation, array construction, surface construction, and upload together,
  so it cannot prove that mesh allocation alone accounts for all 4.035 ms.
- Trial-to-trial full enemy-path median savings were 8.287, 7.613, and 7.872 ms.
  GPU clocks were not locked and one later split trial contained an unrelated
  123 ms frame outlier. Distribution targets, not a single worst frame, should
  decide acceptance.

### Verification plan

1. Keep the current harness and raw baseline files immutable. Add a new named
   candidate variant rather than replacing `baseline` or `no_enemies`.
2. Run baseline and candidate interleaved for three trials each, with the same
   180-frame warm-up, 600-frame sample, fixture, resolution, renderer, input
   sequence, VSync setting, and uncapped frame rate.
3. Retain median, p95, worst, `_sync`, bridge, adapter, render-CPU, render-GPU,
   draw-call, primitive, object-count, and memory results for every trial.
4. Require `performance_profile_parity.gd`, the existing camera/phone/HUD/human
   presentation tests, and all native tests to pass. Compare authoritative
   player, camera, FOV, battery, souls, gameplay-loop, phone, and HUD snapshots.
5. Add adapter tests proving the mesh RID/surface persists across syncs while
   current native vertices and normals reach the GPU. Exercise alive/dead,
   armor-crack triangle removal, hit deformation, locomotion, first/third person,
   obstruction fade, tether, room transition, restart, and target reuse.
6. Capture matched deterministic frames for idle, locomotion, hit, vacuum,
   collapse, and armor-crack states. Reject the candidate for topology errors,
   stale geometry, incorrect normals, or visible animation cadence changes.
7. Accept only if the 2.000 ms median and p95 floors are met without native
   divergence, visual regression, higher GPU p95, or unbounded memory growth.

## Method and settings

- Source baseline: `8e6824278f1226f0d74950688d6cf7badb736d30`, plus the
  uncommitted GGG-CAM-001 user work, which was preserved.
- Godot: `4.7.2.stable.official.ed1daf0bf`.
- Renderer: OpenGL 3.3 Compatibility (`opengl3`), NVIDIA driver 610.88.
- Resolution: 1280×720 windowed; VSync disabled; `Engine.max_fps = 0`.
- CPU: AMD Ryzen 9 5900X, 12 cores / 24 logical processors.
- GPU: NVIDIA GeForce RTX 3060, 12,288 MiB; observed P3, 832 MHz graphics and
  5001 MHz memory before the run. Clocks were not locked.
- RAM: 34,265,444,352 bytes reported (31.91 GiB).
- OS API report: Windows 10 Pro 2009, build 26200, 64-bit.
- Workload: deterministic target-interaction fixture; one native tick per
  rendered frame; repeatable movement, look, sprint, vacuum, and melee cycle.
- No framebuffer capture/readback was used. Rendering remained enabled.
- Presentation variants skip or hide only the named Godot subsystem. The native
  runtime continues to tick identically. `performance_profile_parity.gd` proves
  matching player, camera, FOV, battery, souls, gameplay-loop, phone, and HUD
  native snapshots for all variants.

The benchmark harness introduces microsecond timestamp calls while profiling.
Absolute figures therefore include small instrumentation overhead. A/B variants
use the same instrumentation. Background processes and GPU clocks were not
controlled, which is why three independent trials and distribution statistics
are retained instead of a single FPS number.

## Reproduction

```powershell
$out = (Resolve-Path evidence/performance).Path.Replace('\\','/') + '/baseline.json'
godot_console.exe --path godot --resolution 1280x720 --rendering-method gl_compatibility `
  --script res://../evidence/performance/profile_playable.gd -- `
  --variant=baseline "--output=$out"
```

Replace `baseline` with `no_enemies`, `no_grass`, `no_particles`, `no_shadows`,
`no_hud`, or `no_phone`. Raw JSON results and console logs for all three trials
are stored beside this report. `scene-inventory.json` records the measured scene
population.

## Unverified

- Forward+ and Vulkan were not measured; the playable project is pinned to
  Compatibility rendering.
- No macOS, Linux, integrated-GPU, 4K, or high-refresh display run was performed.
- No particle-saturation, City skyline, Coastal water, or Sterile-room benchmark
  was run; this evidence represents the deterministic Field encounter.
- The proposed fixes above were not implemented. They require their own parity,
  capture, and before/after performance evidence.
