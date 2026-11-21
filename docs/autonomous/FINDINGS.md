# Experimental findings and limitations — 2026-11-20

## Observed
- 64 seeds × 3 policies × 3,600 ticks = 691,200 benchmark ticks.
- 192 benchmark trials reproduced their selected final-state digests.
- No room objective completions or room transitions in the three-policy benchmark.
- The fourth, rules-aware native controller achieved 547 captures, 542 shots, and 57 deposits across 64 seeds, but still no room clears.
- 58/64 controller trials ended with battery at zero.

## Inference, not established cause
- Projectile aiming and resource scheduling likely limit progression. Shot/deposit aggregate ratios alone cannot identify exact misses or collisions.

## Unfinished
- Controller-to-desktop-agent integration.
- Synchronized renderer observations for native controller actions.
- Full causal trace of a missed soul shot.
- First ordinary room completion and independently replayed proof.
- Coverage-guided exploration, delta debugging, automated source repair.

## Evidence provenance
Original sandbox outputs: `digital-breakdown-benchmark-64x3600.zip`, `digital-breakdown-native-controller-rules.zip`, `digital-breakdown-audit-v2.zip`. These binary archives and full CSVs were **not uploaded to this Git branch**. The repository contains source and concise historical findings, not a complete byte-for-byte archive of the sandbox.

The branch intentionally leaves production game source unchanged.
