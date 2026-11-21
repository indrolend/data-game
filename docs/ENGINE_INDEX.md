# Native engine index

This index describes the native authority at revision
`3d4fb0e93647a9aa1308d2e8c9d2f85a89753e02`. The machine-readable companion
is `docs/engine-index.json`. It is an explicit, source-reviewed index, not a
claim that every callback or indirect mutation was statically recovered.

## Ownership and order

`game/Game.hpp` and `game/Game.cpp` own gameplay. Desktop and Godot consume
native state and presentation snapshots. The fixed update runs input actions,
player motion, combat/targets, vacuum and souls, particles, camera, then
presentation-facing state. A change to shared state or ordering requires the
broad native suite plus affected presentation contracts; a renderer-only
change does not by itself establish a gameplay change.

## Three traced paths

### Particle lifecycle

Combat, shell fracture, soul capture, and movement effects request a slot from
`Game::allocateParticle`. It selects `particles[nextParticle]`, records whether
that slot is still live, advances the 256-slot ring, and leaves initialization
to the original caller. `Game::updateParticles` integrates motion, performs
environment/support collision and settling, applies SignalResidue vacuum
attraction and battery gain, and expires lifetime. `DesktopRenderer.cpp`
consumes the same state in particle drawing and residue-light contribution.

Runtime evidence shows no live overwrite in the single-enemy workload (1,090
allocations, peak 87), but 2,280 live overwrites in the six-enemy workload
(4,268 allocations, peak 256), including 192 SignalResidue overwrites. This
answers the allocation question but does not yet justify a policy change: the
next decision needs an outcome test for residue availability and battery gain.

### Camera

`lookX/lookY` enter through the bounded agent/input surface.
`Game::updateInputActions` writes player yaw and clamped pitch. Movement reads
the yaw-derived planar basis. `Game::updateCamera` derives position, target,
forward direction, collision constraints, first-person placement, and the
persistent air-lunge smoothing path. Desktop and Godot then consume the native
camera state.

`godot/tests/fixtures/agent_camera_turn.commands` reproduces the path without
debug commands. Two runs produced identical yaw `-0.36`, pitch `0.12`, camera
position `(-1.049223, 0.88, 18.287498)`, target
`(3.497409, 1.727122, 6.208336)`, and digest at tick 31.

### Soul capture

Damage passes through `Game::damageSoulShell`, exposing the target and making
its soul free. `Game::updateVacuum` selects an offer and advances the soul
through attraction, latch, and ingestion. The queued capture is processed by
`Game::processQueuedSoulCaptures` and `Game::captureSoul`, which stores the
soul, updates player/progression state, produces phone feedback, and emits
residue. HUD, phone, audio, particle, progression, desktop, and Godot
presentation paths consume the result.

The authentic Godot fixture uses ordinary player inputs and reached tick 930
with `captures=1`, `souls=1`, and its assertion passing. Schema 5 keeps the
authoritative digest beside scene telemetry and the captured PNG.

## Practical dependency answers

- Vacuum activation can change battery, vacuum lock and target, soul state,
  capture queue, stored souls, SignalResidue, phone/HUD feedback, audio, and
  rendered effects. Verify the authentic soul fixture plus vacuum/particle
  tests after changing this path.
- Camera orientation is written by input processing and consumed by player
  movement, camera placement/constraints, both render paths, and observations.
  Verify the camera fixture and presentation camera contracts; rerun gameplay
  tests if yaw/pitch or update order changes.
- Particle rendering consumes live `ParticleState` after simulation update;
  SignalResidue additionally participates in lighting and gameplay battery
  recovery. Renderer-only changes use render stress/visual evidence, while
  allocation or update changes require gameplay and deterministic checks.

## Microprimitive assessment

The five particle ring-allocation sites were a repeated implementation with
identical ownership semantics. They now share one diagnostic boundary without
changing slot order, caller initialization, random-number consumption, or
allocation policy. Interpolation, damping, support queries, and state
transitions already have reusable helpers in several paths; syntactically
similar camera, soul, and particle transitions remain domain behavior until a
measured change demonstrates that extraction preserves timing and semantics.

## Evidence receipts

| Investigation | INDRO run | Result |
| --- | --- | --- |
| Single-enemy particle stress | `20261111233506-b76d` | PASS; 0 live/residue overwrites |
| Six-enemy particle stress | `20261111233531-b73b` | PASS; 2,280 live, 192 residue overwrites |
| Camera fixture A | `20261111234035-6d5e` | PASS; tick 31 |
| Camera fixture B | `20261111234048-7de9` | PASS; identical observed state |
| Authentic soul capture | `20261111234052-b461` | PASS; one stored soul at tick 930 |

INDRO retains command logs under `D:/hud/runs/indrolend_data-game/<run-id>`;
frame evidence is under `data-game-tools/artifacts/evidence/microprim-*`.
