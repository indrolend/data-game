# Repository convergence reconnaissance — 2026-11-02

## Audited runtime identity

- Product authority: `indrolend/data-game`
- Revision: `4788e050d81b507f4f02b8adf51d63aabc7ffbb0`
- Branch: `codex/product-convergence-20261024`
- Worktree at investigation start: clean; `HEAD`, `origin/main`, and the branch
  remote all matched
- Workstation source key: `80c91579`
- Godot: `4.7.2.stable.official.ed1daf0bf`
- Project renderer: OpenGL Compatibility, 1280 x 720
- Loaded extension descriptor: `godot/parent.gdextension`
- Windows extension: `godot/bin/digital_breakdown_parent.dll`
- Extension SHA-256:
  `E44F4EF82426F1AF587DB1DE3AC3AEC61C0DE4A09294821BFE25CEE8E88A6F4C`

This investigation preserves the accepted PC baseline and does not claim macOS
acceptance.

## System map

Native `Game` state remains authoritative. `ParentBridge` and
`ParentGameNode` expose snapshots and presentation payloads; `main.gd::_sync()`
consumes them and dispatches to narrow Godot adapters.

| Area | Native authority / bridge | Godot consumer |
|---|---|---|
| Simulation and input | `Game`, `ParentBridge::tick()` | `main.gd::_process()` / `_sync()` |
| Enemy behavior and physical response | perception, behavior, motor, locomotion, physical body, expressiveness | target snapshots plus `HumanModelAdapter` |
| Enemy deformation | `HumanModelData::skin()`, native foot targets and shell-crack filtering | `target_model_vertices()` then per-frame normals and `ArrayMesh` construction |
| Phone | native phone/menu snapshot and raster revision/pixels | `PhonePresentationAdapter` |
| Room and grass | native room/grass snapshots | `RoomPresentationAdapter` |
| Souls | native lifecycle and 27-point lattice | `SoulLatticeAdapter` |
| Player melee visuals | native action timer, origin, direction, impact and hit state | `MeleeArcAdapter` |
| HUD/audio/particles/projectiles/lighting | native snapshots | dedicated presentation adapters in `_sync()` |

The enemy physical chain is not bypassed by Godot. Native animation time,
attack modifiers, hit response, body pitch/roll, expressive scale, foot targets,
foot contact, shell cracks, vacuum tether, and soul morph all reach the default
presentation path. The principal risk is loss or distortion while translating
that state into render geometry, not absence of the native systems.

## Ranked findings

### 1. Enemy vertex presentation is the dominant measured CPU cost

**Severity: high. Confidence: high.**

The accepted PC run measured approximately 3.088 ms in the enemy bridge and
3.868 ms in the adapter, about 6.96 ms per median frame. The path is:

1. `HumanModelData::skin()` evaluates 33 bones, expressive/attack modifiers,
   4,164 weighted vertices, and post-skin foot correction.
2. `ParentGameNode::target_model_vertices()` filters shell-crack triangles and
   converts the result to a new `PackedVector3Array`.
3. `main.gd::_sync()` repeats the call across 32 target slots.
4. `HumanModelAdapter.sync_from_native()` computes flat normals in GDScript,
   creates a new `ArrayMesh` surface, uploads it, and replaces the old mesh.

The representative fixture processes 133,248 enemy vertices per sync. Earlier
ablation measured a 7.86 ms median frame-time ceiling when enemy presentation
was removed while native gameplay continued unchanged. This is an opportunity,
not permission to remove or simplify the presentation.

### 2. Neither existing persistent-mesh nor skeleton experiments is shippable

**Severity: high design constraint. Confidence: high.**

GGG-PERF-002's GDScript persistent-buffer candidate regressed median frame time
by 3.685–3.816 ms because octahedral normal packing, AABB work, and buffer
updates cost more than surface reconstruction. It was correctly reverted.

The current one-enemy `Skeleton3D` slice was remeasured in three interleaved
fresh-process pairs. Median microseconds were:

| Trial | Vertex sync | Skeleton sync | Vertex enemy | Skeleton enemy |
|---:|---:|---:|---:|---:|
| 1 | 7,711 | 7,403 | 6,722 | 6,405 |
| 2 | 7,715 | 7,488 | 6,715 | 6,488 |
| 3 | 7,762 | 7,397 | 6,734 | 6,401 |

The slice saves roughly 0.23–0.37 ms of total sync for one enemy, but does not
represent native post-skin foot correction, final-deformed flat normals,
shell-crack triangle removal, or the soul-collapse deformation. Promoting it
would trade visible behavior for an extrapolated performance gain and is
rejected.

### 3. Native-packed persistent buffers remain the best next isolated candidate

**Opportunity: estimated 2.5–3.8 ms median. Confidence: medium.**

GGG-PERF-003 proved that persistent position uploads for 32 real-size meshes
cost only 0.113 ms median when packing is excluded. The useful next experiment
is therefore a presentation-only native packer for one target that produces
Godot-compatible position and normal bytes, topology revision, and bounds. It
must remain disconnected from the playable adapter until it passes round-trip,
topology, allocation, pose, foot-contact, crack, death/reuse, and timing gates.

This is deliberately narrower than a skeleton rewrite and directly avoids the
GDScript packing failure observed in GGG-PERF-002.

### 4. Grass is a secondary, bounded presentation cost

**Severity: medium-low. Confidence: high.**

Room/grass construction is not repeated every frame: `main.gd` and
`RoomPresentationAdapter` key rebuilds by room seed/index/tile/grass state.
However, the Field scene retains 160 individual `MeshInstance3D` blades and
updates blade transforms during synchronization. The earlier no-grass ablation
saved 0.693 ms median and 93 draw calls. A matched `MultiMesh` experiment is
reasonable only after the enemy candidate, with root/tip placement and captures
as equivalence gates.

### 5. Phone raster allocation is already revision-gated

**Severity: low in current evidence. Confidence: high.**

`PhonePresentationAdapter` calls `phone_display_raster_pixels()` and creates a
720 x 1280 `ImageTexture` only when the native raster revision changes while a
menu is shown. It does not create a texture every synchronization. The accepted
phone bridge/adapter median is about 0.1 ms, and the earlier no-phone ablation
was only 0.534 ms for the entire subsystem. A persistent `ImageTexture.update()`
candidate needs a menu-navigation-specific allocation/upload profile before it
is justified.

### 6. Soul and melee mesh rebuilding is event-limited

**Severity: presently unproven. Confidence: medium-high.**

The soul adapter reconstructs a small surface only for a visible, valid
27-point lattice. The melee adapter reconstructs three small meshes only while
the native attack visual timer is active. Both are real allocations, but neither
has evidence of material frame cost. Dedicated multi-soul capture and repeated
melee profiles should precede changes; timing, color, hit direction, topology,
and visibility remain acceptance requirements.

## Experimental decision

No playable source change is justified by the first experiment. The skeleton
slice reproduces a small CPU benefit but fails the required visual-equivalence
boundary. It remains opt-in and disabled by default.

The next implementation experiment should follow the six gates already defined
in `evidence/performance/GGG-PERF-003.md`: one-target native packing, exact
position/normal round-trip, intact/cracked topology parity, stable 10,000-update
behavior, projected enemy cost below 5.5 ms median, and complete physical pose
coverage before any playable integration.

## Reproduction

Runtime identity:

```powershell
& C:\Users\indro\Projects\data-game-tools\Invoke-DataGame.ps1 `
  -GameRoot C:\Users\indro\Projects\data-game -Action status
git status --short --branch
git rev-parse HEAD
git rev-parse origin/main
Get-FileHash godot\bin\digital_breakdown_parent.dll -Algorithm SHA256
```

One-enemy boundary comparison (run each condition in a fresh process and
interleave conditions):

```powershell
Remove-Item Env:DB_ENABLE_SKELETON_ENEMY_SLICE -ErrorAction SilentlyContinue
godot_console.exe --headless --path godot `
  --script res://tests/skeleton_enemy_benchmark.gd

$env:DB_ENABLE_SKELETON_ENEMY_SLICE = '1'
godot_console.exe --headless --path godot `
  --script res://tests/skeleton_enemy_benchmark.gd
Remove-Item Env:DB_ENABLE_SKELETON_ENEMY_SLICE
```

## Remaining gaps

- No native-packed one-target implementation or playable A/B exists yet.
- No menu-navigation raster allocation/upload profile exists.
- No multi-soul, repeated-melee, dense-environment, or room-transition spike
  profile exists.
- No packaged-Windows full-loop acceptance has been completed from this exact
  revision.
- macOS and hardware-specific acceptance remain intentionally separate.
