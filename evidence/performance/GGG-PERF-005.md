# GGG-PERF-005: all-target persistent enemy buffers

## Decision

**Accepted and promoted to the default Godot enemy presentation path.**

All 32 target slots now use the packed adapter; live targets use native-packed
position/normal buffers and persistent dynamic `ArrayMesh` surfaces. Dead slots
do not generate or transfer model geometry. Set `DB_DISABLE_PACKED_ENEMIES=1`
to restore the original mesh-reconstruction control for diagnostics and
comparisons while retaining the dead-slot guard.

## Critical correction from the one-target experiment

Godot 4.7 reports this surface layout for the enemy mesh:

- position stride: 12 bytes;
- normal/tangent stride: 8 bytes;
- normal offset: `vertex_count * 12`;
- tangent offset: normal offset + 4;
- total payload: 20 bytes per vertex.

The original GGG-PERF-004 prototype supplied only 16 bytes per vertex and wrote
normal words contiguously. CPU decoding tests passed, but the GPU interpreted
the malformed normal/tangent region as an 8-byte interleaved stream. Expanding
the candidate to every enemy made the resulting orange/black faceting obvious.

This finding invalidates GGG-PERF-004's earlier visual classification. The test
now asserts the runtime-reported strides and offsets directly.

## Candidate sequence

### Malformed 16-byte stream

The first all-target run appeared to improve median frame time by 1.479 ms, but
rendered output showed severe faceting. It was rejected regardless of timing.

### Correct 20-byte stream with dynamic tangents

The corrected stream generated the same tangent vectors Godot creates when a
normal array is supplied. All six rendered checkpoints became byte-identical,
but the added native tangent computation failed performance acceptance:

| Measurement | Control | Dynamic tangents | Change |
|---|---:|---:|---:|
| Frame median | 9.908 ms | 10.092 ms | +0.184 ms |
| Frame p95 | 10.882 ms | 11.874 ms | +0.992 ms |
| Enemy bridge median | 3.086 ms | 6.725 ms | +3.639 ms |
| Enemy adapter median | 3.899 ms | 0.213 ms | -3.686 ms |
| GPU p95 | 2.641 ms | 2.751 ms | +0.110 ms |

This candidate was rejected.

### Correct 20-byte stream with fixed valid tangents

`HumanModelAdapter` uses `StandardMaterial3D` without a normal map. Its shader
does not consume tangent-space data, although Godot reserves the tangent word in
the buffer. Replacing unused per-frame tangent derivation with the stable valid
encoding `0xffffffff` produced byte-identical rendered output and retained the
correct 20-byte surface layout.

## Accepted performance result

Three fresh interleaved control/candidate processes used 180 warm-up frames,
600 samples, the deterministic full encounter, 1280 x 720 OpenGL Compatibility,
VSync disabled, and uncapped frame rate. Values are the middle of three trial
medians/p95 values; worst is the maximum across trials.

| Measurement | Control | Packed all targets | Change |
|---|---:|---:|---:|
| Frame median | 9.960 ms | 8.580 ms | **-1.380 ms (-13.9%)** |
| Frame p95 | 10.825 ms | 9.419 ms | **-1.406 ms (-13.0%)** |
| Worst sampled frame | 15.424 ms | 14.611 ms | **-0.813 ms** |
| Scene sync median | 8.382 ms | 6.979 ms | **-1.403 ms** |
| Enemy bridge + adapter | 7.044 ms | 5.662 ms | **-1.382 ms** |
| Enemy bridge median | 3.105 ms | 5.409 ms | +2.304 ms |
| Enemy adapter median | 3.904 ms | 0.213 ms | **-3.691 ms** |
| GPU median | 2.338 ms | 2.093 ms | **-0.245 ms** |
| GPU p95 | 2.712 ms | 2.353 ms | **-0.359 ms** |
| Static memory | 46.933 MiB | 46.937 MiB | +0.004 MiB |

The result improved every accepted frame distribution, reduced rather than
shifted work onto the GPU, and showed no meaningful memory growth.

## Dead-slot elimination and final promoted result

The all-target regression test exposed that the representative fixture has nine
live targets but the old loop skinned all 32 slots before adapters hid the dead
ones. `_sync()` now reads the authoritative target snapshot first and skips
model generation for dead targets. Snapshot, HUD, soul, tether, visibility,
death, and reuse synchronization continue; a newly alive target constructs its
persistent surface on its first live frame.

A final three-pair interleaved A/B enabled this guard in both the reconstruction
control and packed candidate:

| Measurement | Dead-skip control | Promoted packed default | Change |
|---|---:|---:|---:|
| Frame median | 8.175 ms | 4.607 ms | **-3.568 ms (-43.6%)** |
| Frame p95 | 8.733 ms | 5.263 ms | **-3.470 ms (-39.7%)** |
| Worst sampled frame | 13.854 ms | 10.970 ms | **-2.884 ms** |
| Scene sync median | 6.612 ms | 3.038 ms | **-3.574 ms** |
| Enemy bridge + adapter | 5.288 ms | 1.764 ms | **-3.524 ms** |
| Enemy bridge median | 1.378 ms | 1.543 ms | +0.165 ms |
| Enemy adapter median | 3.869 ms | 0.187 ms | **-3.682 ms** |
| GPU median | 2.053 ms | 1.002 ms | **-1.051 ms** |
| GPU p95 | 2.306 ms | 1.177 ms | **-1.129 ms** |
| Static memory | 46.934 MiB | 46.937 MiB | +0.003 MiB |

Relative to the original no-guard reconstruction control in the immediately
preceding experiment, the promoted default improves median frame time from
9.960 to 4.607 ms and p95 from 10.825 to 5.263 ms. This combined comparison is
informative but the final acceptance decision uses the isolated dead-skip A/B
above.

Raw final isolated trials:

- `dead-skip-control-{1,2,3}.json`
- `dead-skip-packed-{1,2,3}.json`
- `default-promoted.json`

Raw final trials:

- `control-final-{1,2,3}.json`
- `packed-all-fixed-tangent-{1,2,3}.json`

Intermediate malformed and dynamically generated tangent trials are retained
in `evidence/generated/packed-buffer-all-targets/` so the rejected conclusions
remain auditable.

## Visual parity

Control and accepted candidate captures are byte-identical at all six rendered
checkpoints:

1. gameplay phone and active enemies;
2. enemy hit and phone damage response;
3. shell crack and melee effects;
4. capture phone light;
5. settled signal residue;
6. phone menu.

The sequence exercises intact enemies, physical poses, hit materials, crack
topology, soul/capture transition, particles, and menu presentation. Exact
buffer tests separately verify animation, bounds, persistent surface identity,
and crack-triggered surface reconstruction.

## Work eliminated

For ordinary frames, Godot no longer performs these operations for every target:

- allocate and fill a `PackedVector3Array` bridge result;
- allocate and compute a GDScript normal array;
- allocate a mesh-array container and `ArrayMesh`;
- create a rendering surface and replace the mesh resource;
- retire and recreate GPU buffers.

Native code still performs authoritative skinning, expressive deformation,
post-skin foot correction, shell filtering, final flat-normal generation, and
packing. The remaining 5.409 ms enemy bridge cost is now the largest measured
portion of this path; it should not be optimized further without another
isolated experiment.

## Control and rollback

The original `HumanModelAdapter.sync_from_native()` implementation remains in
the repository. `DB_DISABLE_PACKED_ENEMIES=1` selects it for every target, so
the accepted path is reversible without source changes.
