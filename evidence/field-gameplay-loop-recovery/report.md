# Field gameplay-loop recovery

## Result

The parent-driven Godot runtime now exposes and verifies the complete controlled Field progression loop while retaining the original C++ `Game` as the only gameplay authority:

1. room time raises native room heat;
2. stored souls are fired through the original input and projectile path;
3. five native capture goals receive those projectiles;
4. each deposit consumes one soul, costs 7 battery, and awards one permanent token;
5. the fifth deposit marks the room clear and opens the path;
6. native doorway traversal advances room 1 to room 2;
7. the original room-recompile upgrade pause opens;
8. a native temporary upgrade is selected and the simulation resumes.

The deterministic headless test and the graphical evidence both exercise that sequence. Godot does not calculate deposits, battery costs, token awards, room clear, topology, or upgrade effects.

## Native ownership map

| Contract | Native source | Presentation/verification boundary |
|---|---|---|
| Heat and elapsed time | `GameState::progression.run.roomHeat/roomElapsed/roomCaptures`; `Game::updateHumanRespawns()` | `ParentGameNode::gameplay_loop_snapshot()` read-only observation |
| Soul firing and battery | `Game::shootStoredSoul()` and `Game::processPendingShots()` | ordinary `shootPressed` input; no Godot projectile/economy rule |
| Goal deposits and rewards | native bullet/capture processing and progression state | capture-point adapter consumes native position/filled snapshots |
| Room clear and doorway | `Game::updateRoomTopology()` | Godot synchronizes the resulting room and HUD state |
| Difficulty progression | `Game::advanceRunRulesForRoom()` | rule stacks are read-only fields in the loop snapshot |
| Upgrade economy | `Game::chooseTemporaryUpgrade()` / `purchasePermanentUpgrade()` | HUD forwards selection/activation to the native API |

The new `Game::debugPrepareCaptureShot()` proof hook only positions the player and camera toward the next unfilled native goal. The existing native developer commands provide a stored soul, refill battery, and disable enemies. The actual shot, flight, collision/deposit, economy, room clear, topology transition, and upgrade remain production C++ behavior.

## Verification

- `godot/tests/gameplay_loop.gd`: passes the five-deposit, room-clear, doorway, upgrade, reset, and resume contract.
- Controlled graphical command stream: `evidence/field-gameplay-loop-v2.commands`.
- Runtime log: `evidence/field-gameplay-loop-v2.log` (Godot 4.7.2, OpenGL 3.3, NVIDIA RTX 3060, exit code 0).
- Ordered captures: `evidence/field-gameplay-loop-v2/`.
- Room-clear frame: `frame-026-tick-000402.png`; log state is room 1, deposited 5/5, clear 1, tokens 5.
- Upgrade frame: `frame-027-tick-000505.png`; log state is room 2, upgrade active, tokens 5.
- Resumed frame: `frame-029-tick-000526.png`; log state is room 2, upgrade inactive.

The first graphical attempt is intentionally retained at `evidence/field-gameplay-loop/`. It did not complete: enemies attacked the stationary proof setup until the battery reached zero. That run is evidence of an unsuitable capture fixture, not proof that progression succeeded.

## Arena-growth finding

`RoomScale` (`Compact`, `Standard`, `Large`, `Arena`) currently changes environment/encounter placement and formation bias. Room index and run rules also increase enemy budget, required goals, respawn pressure, and slurp behavior. They do **not** grow the physical room or Field-island footprint. Physical arena growth is therefore new design work, not a disconnected presentation feature waiting to be enabled.

## Remaining limits

- The controlled GPU proof uses native developer hooks rather than earning all five souls through live combat and ingestion.
- Existing controlled interaction evidence separately proves one native enemy damage/defeat/ingestion/capture lifecycle, but the two sequences have not yet been combined into one unassisted graphical level completion.
- The frame taken on the exact fifth-deposit simulation observation can precede the HUD's next draw; the subsequent retained frame shows `PATH OPEN`. The authoritative log already reports 5/5 and clear at the earlier observation.
- Filled capture presentation, room-recompile UI, and resumed room are graphically visible, but exact mature-desktop pixel parity is not claimed.
- A spherical Moon stage, spherical gravity, and phase-preserving transfer are intentionally deferred until the Field loop milestone is accepted. The current Moon remains render-only and driven by native celestial time/light state.
