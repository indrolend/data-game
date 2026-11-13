# Computational convergence: sterile chamber

Source baseline: `abacebdf62d35f28a6b4f696850c21dc58fea245`.

## Ownership map

| Fact | Authority | Derivation / consumer | Persistence decision |
| --- | --- | --- | --- |
| Room seed, index, topology, actors | `GameState` | `roomPlan`, renderer, replay and digest | Authoritative simulation state |
| Sterile setting and chamber form | `RoomEnvironment.hpp` from seed/index | `RoomLightRig`, geometry and evidence | Derived; do not copy |
| Fixture position, size, color, radius | `RoomLighting.hpp::RoomLightRig` | Visible fixture geometry, fixed-function local lights, local-shadow source | Derived each frame; do not copy |
| Fixture world position | `resolveSceneLighting` / `primaryLocalShadowSource` + authoritative tile origin | GPU light and shadow view | Derived; do not persist |
| Shadow projection and depth texture | `DesktopRenderer` | GPU-only observation of the world | Necessary render cache, never gameplay authority |
| Requested/effective shadow state | Runtime shadow control / renderer observation | Console and capability evidence | Presentation control and observation only |

No independently mutable duplicate fixture or room definition was found in the
current native path. The missing behavior was a capability gate: mapped shadows
required `ResolvedSunState::castsDirectionalShadows`, while sterile rooms
intentionally disable the directional sun and use ceiling fixtures.

## Historical engine archaeology

| Revision / era | World authority | Rendering relationship | Transparency and verified tradeoff |
| --- | --- | --- | --- |
| `d844c6c` (2026-07-06) | `WorldState` | `buildRenderFrame` directly emitted a bounded array of boxes | Most intrinsically transparent equivalent path: one explicit world-to-command transformation. Limited geometry, lighting, and no present evidence tooling. |
| `87f7f44` (2026-07-13) | `GameState` | Desktop renderer read state and issued immediate-mode GL directly | Shortest input-to-pixel causal path and easiest visual mutation, but room constants were repeated in the renderer and behavior was not headlessly observable. |
| `e13c324` to `8e68242` (2026-10-18 to 2026-10-24) | Native `GameState` | Native presentation snapshots were materialized into cached Godot nodes | Determinism and presentation experimentation improved. `RoomPresentationSnapshot` remained derived, but the bridge plus node caches introduced the longest synchronization and diagnosis path. Adapter comments explicitly prohibit feedback into simulation. |
| `53f1cde` / `c8dbea9` (2026-11-11) | Native `GameState` plus canonical digest | Renderer-free digest and Data Engine command/observation loop | Greatest behavioral reproducibility and first-divergence power. These interfaces add observations, not a second gameplay authority. |
| `abacebd` current baseline | Native `GameState`; generated room and lighting policies | Native renderer derives geometry, lights, shadow maps and typed observations | Best current combination of authority and observability, but renderer complexity makes receiver/caster and GL-state causality less direct than the early command frame. |

The Godot migration demonstrably added transformation and cache obligations;
it did not create an authoritative second room model in the inspected path.
The valuable early property to recover is explicit, typed world-to-render
derivation. Headless simulation, canonical digests, presentation-only camera
experiments, and GPU ownership of shadow rasterization should remain.

## Bounded product change

Sterile chambers now select the central fixture from `RoomLightRig` and derive
one perspective shadow projection from its world position and radius. The
existing depth texture/FBO is reused, so the change adds no second shadow-map
resource and never runs beside the directional mapped path. Only the walkable
floor receives the local compositor: fixed-function local lighting already
accounts for walls and props, and darkening them again reproduced the earlier
screen-staining failure mode.

This is a **product improvement with justified computation**, not an
architectural convergence claim. It adds one depth pass in mapped modes for a
sterile chamber. It consolidates no authority because source tracing found no
safe duplicate authority to remove. Corridors retain their previously reported
unsupported local-shadow result; extending every fixture remains a separate
cost/selection decision.

## Required conclusions

1. The `d844c6c` render-command era was intrinsically easiest to debug for its supported behavior because derivation was explicit and bounded.
2. The same era had the greatest computational transparency; the current engine has greater observability and reproducibility.
3. Native authority, headless execution, state digests, deterministic replay, typed observations, and retained evidence demonstrably improved verification.
4. Godot materialization added derived snapshots, node caches, and a longer causal path. No inspected current native fixture value has competing mutable authority.
5. Recover explicit typed derivations and small renderer contracts, not the old feature set.
6. Preserve simulation/render execution independence, renderer-owned GPU state, and presentation-only camera experiments.
7. The smallest verified present change is one rig-derived central-fixture projection using the existing shadow resource and an explicit floor receiver.

