# Branch reconciliation — 2026-10-15

All counts were refreshed after fetching and pruning `origin`. `origin/main` is `137d08e192e3cd8e083208494673060b3af1e0fa`.

## Git integration matrix

| Branch | Relative to main | Unique functionality | Risk | Classification and recommendation |
|---|---:|---|---|---|
| `codex/structural-polish-shadow-geometry` | 15 ahead, 0 behind | Living Worlds, deterministic evidence, graphics controls, music variation, atmosphere, scene-wide directional shadow map, expanded authored City modules, legacy-shadow correction | Medium: large renderer diff; Mac path unverified | **RECOVERABLE; strongest verified baseline.** Review/integrate as an ordered stack, then build both platforms from the resulting SHA. |
| `codex/master-engineering-handoff-20261014` | 12 ahead, 0 behind | Same stack through `315a267`: evidence harness, gameplay/readability, audio, graphics, atmosphere | Medium | **RECOVERABLE.** Ancestor of structural branch; do not merge separately if structural stack is selected. |
| `codex/masterpiece-vertical-slice-mission` / PR #148 | 2 ahead plus main's merge divergence | Living Worlds City grammar, tree structures, coastal water, DBM converter | Medium | **RECOVERABLE, not integrated.** `git cherry` shows both commits unique despite PR #147 using the same branch name earlier. Close/update PR ambiguity only after preserving review context. |
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

- The same Git branch name backed both merged PR #147 and later open PR #148. PR #147 merged only `6b8ae98`; the branch then advanced to `b6b0552` and `8765062`. This makes the GitHub UI look integrated when the later work is not.
- The 12-commit engineering stack is linear on top of the two Living Worlds commits; the three structural commits are linear on top of that. Integrating all three branch names would duplicate review effort and invite conflict.
- The structural branch changes high-conflict files (`DesktopRenderer.cpp/.hpp`, `desktop/main.cpp`, `DesktopAudio.cpp`, `Game.cpp`, render contracts, packaging, tests, and binary assets). Select the strongest stack once; do not independently reimplement it on main.
- `main` is not the best verified playable baseline today. The structural branch is the strongest Windows-verified baseline, but it cannot be called cross-platform verified until a same-SHA macOS build is run and captured.

## Recommended integration sequence

1. Protect the dirty canonical checkout; do not use it for integration.
2. Review PR #148's two commits as the foundation.
3. Review the ten later engineering commits in dependency order.
4. Review `746f27e`, `f51b321`, and `4f5e3a5` together as the shadow/geometry completion.
5. Build Windows and macOS artifacts from the exact candidate merge SHA.
6. Run matched captures and audio/daylight/camera checks before merging to main.

Branches with zero unique commits are archive candidates, but this audit did not delete or archive anything.
