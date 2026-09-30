# Main maturity convergence ledger

Starting authority: `f5468af9aa7b2346138066f009fa79a4ccfb1787` (`origin/main`, observed 2026-09-30).

This ledger classifies recovered work by semantics. Historical paths and branches remain provenance, not source authority.

| Responsibility / recovery unit | Source provenance | Classification | Current authoritative representation | Verification / reason |
| --- | --- | --- | --- | --- |
| Frame battery demand computation | `origin/refactor/simulation-computation-slice`, `b6ef581` | RECOVERED | `game/SoulEconomy.hpp`; delegation from `game/Game.cpp` | Exact-revision Release build; 36/36 CTest passed at candidate `4ed6cfe` |
| Run progression timers and accuracy decay | `origin/refactor/simulation-computation-slice`, `74f999c` | RECOVERED | `game/gameplay/RunProgression.hpp`; delegation from `game/Game.cpp` | `RunProgressionTest`; 36/36 CTest passed at candidate `4ed6cfe` |
| Enemy perception state and pure update contract | newest enemy preservation bundle `data-enemy-recovery-20260930-103824`; untracked `native/game/gameplay/EnemyPerception.hpp` and `native/tests/enemy_perception_test.cpp` | PARTIAL | `game/gameplay/EnemyPerception.hpp`, `tests/enemy_perception_test.cpp` | Focused Release test passes. Runtime consumption and removal of omniscient solo pursuit remain required before RECOVERED. |
| Faceted-rock perception bounds | dirty enemy preservation patch over `b8ab454` | RECOVERED | `faceted_rock::meshBounds` in `game/FacetedRock.hpp` | Enemy perception contract proves generated-mesh crown occlusion rather than an obsolete center box |
| Historical `native/`, `native-desktop/`, and `native/tests/` source layout | pre-`0bdaedad6206` branches and dirty worktrees | SUPERSEDED | Current `game/`, `desktop/`, and `tests/` tree | No legacy source path has been restored |
| `tools/autonomous-playtest.ps1` | newest enemy preservation bundle | REJECTED | None; bounded product protocol remains to be recovered, orchestration belongs to `data-game-tools` | Script is provenance only and would duplicate orchestration authority |
| Progressive atmosphere as default lighting policy | historical progressive lighting lineage | EXPERIMENTAL | Main's readable static lighting remains authoritative pending controlled comparison | No aesthetic winner has been established |

## Preserved dirty inputs left untouched

- `C:\Users\indro\Projects\data-game` on `restore/progressive-atmosphere`, including current-path lighting/control edits and suspicious staged legacy-path additions.
- `C:\Users\indro\Projects\db-enemy-motor-runtime` on `prototype/physical-enemies` at `b8ab454283dc28e7c253385070189cba214bd976` plus its uncommitted delta.
- `C:\Users\indro\data-dirty-preservation-20260930-103604`.
- `C:\Users\indro\data-enemy-recovery-20260930-103824`.

These locations are evidence sources. Their preservation is not authorization to clean, reset, stash, or repurpose them.
