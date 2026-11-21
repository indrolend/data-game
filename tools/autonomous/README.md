# Digital Breakdown autonomous gameplay experiments

Experimental native C++ controllers and audits, preserved from the ChatGPT sandbox on 2026-11-20. These files operate on the actual `game/Game.cpp` simulation. They are **not** integrated into the desktop renderer or CI and have **not** been rebuilt against the current remote main.

## Build

From the repository root:

```bash
g++ -std=c++17 -O2 -I game tools/autonomous/benchmark.cpp game/Game.cpp -o /tmp/db-benchmark
g++ -std=c++17 -O2 -I game tools/autonomous/controller.cpp game/Game.cpp -o /tmp/db-controller
mkdir -p /tmp/db-benchmark-results /tmp/db-controller-results
/tmp/db-benchmark 64 3600 /tmp/db-benchmark-results
/tmp/db-controller 64 3600 /tmp/db-controller-results
```

Both programs include `audit_support.cpp` as a translation-unit helper and compile it through the including source file. Do not compile it separately into the same executable.

## Policy identities

- Benchmark policy 0: deterministic random actions.
- Benchmark policy 1: fixed scripted open-loop actions.
- Benchmark policy 2: nearest-target feedback.
- Native controller: target acquisition, vacuum, soul storage, approximate capture-slot aiming, and shooting.

All use `Game::setTouchControls`, `Game::update`, and `debugStartGeneratedRoomFixture` to establish seeded initial conditions. No debug soul grants or room-clear commands are used during trials.

## Historical results (not rerun on remote main)

64 seeds, room 2, 3,600 ticks each:
- Three-policy benchmark: 192 trials / 691,200 ticks, 0 reported invariant violations, 0 objective completions.
- Nearest-target feedback: 10 enemy-death events, across 7 seeds.
- Native controller: 514 enemy-death events, 547 soul captures, 542 soul shots, 57 deposits, 0 room clears, 0 room transitions.
- Native controller: 58/64 trials ended with battery zero.
- Replay CSVs reproduced the selected state digests in independent runs.

Counts are event measurements, not necessarily distinct enemy identities. The native controller is a fourth policy and was not part of the original 192 trials. These results came from the uploaded source snapshot, not the current remote HEAD.

## Next acceptance test

1. Confirm compilation and reproducibility on this branch.
2. Trace projectile origin, aim, collision, capture-slot state, and battery consumption for one failing seed.
3. Fix the smallest demonstrated controller defect.
4. Complete one room using ordinary gameplay inputs and replay it.
5. Integrate synchronized state and original-renderer evidence via the existing desktop agent protocol.

See `docs/autonomous/` for the observed rules, contracts, and limits. Avoid merging this experiment into main until CI/build verification.
