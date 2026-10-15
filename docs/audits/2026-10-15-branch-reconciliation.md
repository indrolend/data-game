# Branch reconciliation — 2026-10-15

All counts were refreshed after fetching and pruning `origin`. The current integrated baseline is `origin/main` at `24d1f4bff1a41759aa6cf603d77f643e4ae56c3b` (PR #150), including the structural stack merged by PR #149 and agent-playtest enemy observation schema v2.

## Git integration matrix

| Branch | Relative to main | Unique functionality | Risk | Classification and recommendation |
|---|---:|---|---|---|
| `codex/structural-polish-shadow-geometry` | 0 unique after PR #149 | Living Worlds, deterministic evidence, graphics controls, music variation, atmosphere, scene-wide directional shadow map, expanded authored City modules, legacy-shadow correction | Residual platform-validation risk | **PRESENT.** Integrated by PR #149 at `6fd5a23`; retain the branch as provenance until matched macOS runtime evidence exists. |
| `codex/master-engineering-handoff-20261014` | 0 unique after PR #149 | Evidence harness, gameplay/readability, audio, graphics, atmosphere | Low | **PRESENT.** Integrated as an ancestor of the structural stack; do not merge separately. |
| `codex/masterpiece-vertical-slice-mission` / PR #148 | 0 unique after PR #149 | Living Worlds City grammar, tree structures, coastal water, DBM converter | Low | **PRESENT.** PR #149 included the branch; GitHub consequently records PR #148 as merged. |
| `convergence/zombie-v1` | 34 commits ahead of the old lineage, 91 behind current baseline at audit start | Relentless direct pursuit, physics lab, gait/support variants, herd behavior, pressure experiments | High: old paths and overlapping authority | **PARTIALLY INTEGRATED.** Physical support, gait, perception, behavior, motor memory, and evidence scenarios have modern equivalents. Lab step/reset/scenario control is superseded by the agent protocol. Direct pursuit and live tuning remain candidates; evaluate independently. |
| `prototype/physical-enemies` | 30 commits ahead of the old lineage, 91 behind current baseline at audit start | Physical bodies, traction, persistent foot plants, articulated locomotion, identity | High: ancestor/overlap with later convergence | **EQUIVALENT/PARTIALLY INTEGRATED.** Modern main contains the core physical embodiment and telemetry. Inventory remaining identity/vehicle deltas separately; do not merge the branch. |
| PR #147 merge on `main` | merged at `137d08e` | Commit `6b8ae98`: visual humor, photographic materials, environment identity | Low | **INTEGRATED.** It does not include later `b6b0552`/`8765062`, despite the recycled head branch name. |
| `copilot/inspect-renderer-lighting-contract` | 0 ahead, 283 behind | No unique commit | Low | **SUPERSEDED.** Safe archive candidate after human confirmation. |
| `copilot/finish-lighting-laboratory-product-side` | 0 ahead, 21 behind | No unique commit | Low | **SUPERSEDED.** Safe archive candidate after human confirmation. |
| `agent/ownership-contracts-settings` | 89 ahead, 279 behind | Broad authority/settings experiment | Very high | **EXPERIMENTAL/UNRESOLVED.** Never merge wholesale; mine individual patches only with a named defect and tests. |
| `codex/phone-ui-convergence` | 4 ahead, 90 behind | Earlier phone/UI convergence | High | **UNRESOLVED, likely substantially superseded.** Compare individual commits against current phone/menu behavior before recovery. |
| `codex/phone-text-effects` | 19 ahead, 91 behind | Directional text/effect experiments | High | **EXPERIMENTAL.** Recover only a demonstrated missing behavior, not the branch. |
| `codex/physical-enemy-locomotion` | 17 ahead, 91 behind | Earlier enemy locomotion work | High | **UNRESOLVED.** Later main contains physical attitude/foot-plant work; use range-diff before selective recovery. |
| `codex/lighting-richness`, `scene-response-lighting` | 0 ahead | No remaining unique commits | Low | **SUPERSEDED.** |
| `codex/shared-normal-lighting-pass-5` | 2 ahead, 23 behind | Small normal/lighting delta | Medium | **UNRESOLVED.** Check patch equivalence with October normal fixes before cherry-picking. |
| `recovery/local-lighting-control` | 1 ahead, 80 behind | Preserved local lighting comparison | Medium | **EXPERIMENTAL.** Keep as reference, not product baseline. |
| iOS PR branch / PR #142 | 5 ahead, 24 behind | GL shim + Metal and touch continuation | High, separate platform path | **EXPERIMENTAL.** Not evidence for macOS desktop parity; rebase/reconcile only as a dedicated iOS task. |

## Overlap and overwrite findings

- The reused PR #147/#148 branch identity previously obscured two later commits. PR #149 resolved that ambiguity by integrating the complete descendant stack once.
- The structural and engineering branch names are now provenance, not newer product authorities. Cross-platform runtime validation is still required, but no source recovery remains for that stack.
- The zombie and physical-enemy branches are not additive feature bundles. They combine capabilities now present on main with obsolete runtime authority and developer interfaces. Recovery must be capability-by-capability.
- The historical relentless policy demonstrably bypasses evidence: with no perception the modern behavior rests, while the historical policy pursues whenever given the authoritative player vector. It is incompatible with the default perception contract and remains viable only as an explicitly designed archetype or pressure mode.

## Historical enemy capability disposition

| Capability | Historical source | Current source/evidence | Status | Recommended action |
|---|---|---|---|---|
| Physical support region | `convergence/zombie-v1:native/game/gameplay/EnemySupport.hpp` | `game/gameplay/EnemySupport.hpp` has the same Git blob hash (`a8d62c3`) | **PRESENT** | Retain current tests; no recovery. |
| Persistent foot locomotion | `convergence/zombie-v1:native/game/gameplay/EnemyLocomotion.hpp` | `game/gameplay/EnemyLocomotion.hpp` has the same Git blob hash (`4486f5e`) and is exercised by locomotion/evidence tests | **PRESENT** | No recovery; validate rendered stance through synchronized captures. |
| Physical body, perception, behavior, motor memory | Both historical enemy branches | Modern files exist and have continued changes; 55-test baseline covers their contracts and the v2 agent schema exposes their runtime outputs | **PRESENT/CHANGED** | Treat modern implementations as authority; investigate named outcome regressions only. |
| Relentless authoritative pursuit | `RelentlessZombieMotor.hpp` | Evaluation-only reconstruction plus `HistoricalPursuitComparisonTest` | **INCOMPATIBLE AS DEFAULT** | Consider only as an explicit archetype/pressure rule after navigation and visual evidence tests. |
| Pause, bounded stepping, reset, scenarios | Zombie physics lab | Agent protocol commands, deterministic evidence scenarios, traversal/slope/room labs | **SUPERSEDED** | Keep the command-driven interface; do not restore the old lab shell. |
| Live physics tuning | Zombie physics lab | No modern runtime tuning surface identified | **MISSING** | Recover only a small, versioned set of safe parameters with reset/default reporting and evidence provenance. |
| Herd awareness and social steering | Zombie convergence commits | Ally activity contributes vague awareness in `Game.cpp`; nearest-ally direction/distance feeds `EnemyMotor` | **EQUIVALENT/PARTIAL** | Add a multi-enemy outcome scenario before considering old protection rules. |
| Run pressure | `RunPressure.hpp` | `roomHeat` rises with time/captures and affects spawning, regeneration, and motor room pressure | **EQUIVALENT/PARTIAL** | Prefer extending the authoritative run model only when a concrete population/complexity/environment consumer is approved. |
| Soul identity | `SoulIdentity.hpp` | `SoulRecord` preserves ID, brute type, and origin room through storage/projectiles/networking; deterministic names, age, occupation, and visual signature are absent | **PARTIALLY INTEGRATED** | Evaluate presentation value independently; identity metadata must not become new lifecycle authority. |
| Rolling shopping cart | `RollingVehicle.hpp`, `ShoppingCartGeometry.hpp` | No modern vehicle implementation identified | **MISSING EXPERIMENT** | Keep separate from enemy reconciliation; require a playable room/use case and capture before recovery. |

`git cherry` reports the historical commits as unique because the repository was
reorganized from `native/game` and `native-desktop` to `game` and `desktop` and
because later code evolved. It is not evidence that every commit still contains
a missing capability. File equivalence and observable behavior take precedence.

## Recommended integration sequence

1. Protect the dirty canonical checkout; do not use it for integration.
2. Build and capture the integrated `24d1f4b` baseline on Windows and macOS from the exact SHA.
3. Use observation schema v2 to evaluate one historical enemy capability at a time.
4. Treat relentless pursuit as an opt-in archetype candidate, not a replacement for evidence-aware behavior.
5. Evaluate live physics tuning as a thin control layer over modern state; do not restore the old full-screen lab wholesale.
6. Inventory herd protection, run pressure, soul identity, and vehicles independently with outcome evidence.

Branches with zero unique commits are archive candidates, but this audit did not delete or archive anything.
