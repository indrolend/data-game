# GGG-PERF-002: persistent enemy mesh buffers

## Decision: rejected and reverted

The candidate retained one dynamic `ArrayMesh` surface while native vertex
count/topology remained unchanged, rebuilt only when armor-crack topology
changed, updated positions in place, encoded and updated current flat normals,
and refreshed the mesh AABB. Native simulation and C++ were untouched.

The implementation was rejected because it missed both required CPU thresholds
and regressed GPU p95. Candidate code was reverted; raw evidence is retained.

## Three interleaved trials

Each run used the existing 180-frame warm-up and 600-frame measurement window,
1280x720 OpenGL Compatibility rendering, VSync disabled, and the deterministic
target-interaction workload. Runs were ordered baseline/candidate for each trial.

| Trial | Variant | Median ms | p95 ms | Worst ms | Adapter median ms | GPU p95 ms | Static MiB median |
|---:|---|---:|---:|---:|---:|---:|---:|
| 1 | Baseline | 12.230 | 14.021 | 17.109 | 4.063 | 1.969 | 46.361 |
| 1 | Persistent mesh | 15.990 | 17.947 | 21.546 | 7.369 | 2.337 | 46.347 |
| 2 | Baseline | 12.216 | 13.953 | 16.649 | 4.069 | 1.823 | 46.361 |
| 2 | Persistent mesh | 15.901 | 17.850 | 20.493 | 7.357 | 2.341 | 46.347 |
| 3 | Baseline | 12.466 | 14.040 | 17.604 | 4.176 | 1.967 | 46.361 |
| 3 | Persistent mesh | 16.282 | 17.976 | 22.712 | 7.430 | 2.376 | 46.347 |

Candidate minus baseline deltas:

| Trial | Median delta | p95 delta | GPU p95 delta |
|---:|---:|---:|---:|
| 1 | +3.760 ms | +3.926 ms | +0.368 ms |
| 2 | +3.685 ms | +3.897 ms | +0.518 ms |
| 3 | +3.816 ms | +3.936 ms | +0.409 ms |

## Acceptance result

- At least 2 ms median improvement: **FAIL**; median regressed in all trials.
- At least 2 ms p95 improvement: **FAIL**; p95 regressed in all trials.
- No native-state divergence: profiling variant changed presentation only; the
  retained parity harness covers this boundary, but no candidate is shipping.
- No visual/topology regression: the existing human-model test passed during
  the candidate smoke, including native triangle and flat-normal contracts.
- No GPU p95 regression: **FAIL** in all trials.
- No unbounded memory growth: no growth signal; median static memory was stable.
- Relevant and native tests: not used to override the failed performance gate;
  the candidate was reverted before final verification.

## Cause

Godot stores vertex positions as contiguous 12-byte `Vector3` data but stores
normals in an octahedrally compressed region. Updating persistent buffers from
GDScript required encoding every current flat normal into that representation.
That per-frame GDScript packing, plus explicit dynamic buffer updates and AABB
maintenance, increased the measured enemy adapter from roughly 4.1 ms to 7.4 ms.
Avoiding surface construction did not compensate for this added work.

## Preserved files

- `ggg-perf-002-baseline-{1,2,3}.json` and `.log`
- `ggg-perf-002-persistent_mesh-{1,2,3}.json` and `.log`
- `candidate-persistent-smoke.json`
- `ggg-perf-002-summary.csv`

No persistent-mesh runtime implementation remains in the working tree.
