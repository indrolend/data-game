# Development-history visual proof audit

Audit date: 2026-10-22. This is a process diagnosis, not a claim that the
listed screenshots are matched frames.

## Historical trajectory

### Native renderer recovery

The native history contains repeated rounds of visual recovery rather than one
stable gold master. Representative commits include:

- `2106ef1` — restore readable scene lighting authority;
- `eb83790`, `8420f0a` — harden phone typography/layout;
- `0784f4b` — recover deterministic phone stencil reveal;
- `4a981d0`, `b309a55`, `52b6c71` — recover, centralize, and retune lighting;
- `d08ba57` — restore soul and Field presentation;
- `b6b0552`, `1b3d4ad` — broad visual-cohesion/convergence passes;
- `746f27e` — scene-wide shadows and City structure.

This sequence is evidence that native C++ itself had changing and sometimes
regressed presentation. Native behavior is authoritative, but an arbitrary
native screenshot is not automatically an approved visual target. A reference
must identify its revision, executable, fixture, and why that frame was chosen.

### Migration baseline and contract recovery

The migration sequence moved from baseline establishment (`e13c324`) through
phone presentation (`94ab70f`), lighting (`5e22482`), bridge expansion
(`2eb2b56`), celestial presentation (`06fc260`), and large presentation/gameplay
recovery checkpoints (`b908250`, `8a34ff3`, `842ba69`). This successfully
recovered a large amount of native state and deterministic behavior.

The original visual-parity gauntlet nevertheless recorded **zero strictly
matched native/Godot scenarios**. Later work often improved adapter contracts
without changing that fact. The recurring reporting defect was promoting
contract success into visual-completion language.

## Recurring failure chain

1. A visible defect was reported.
2. Source tracing found a real local cause.
3. A focused implementation and headless assertion proved that local cause was
   addressed.
4. A framebuffer was saved successfully.
5. Review concentrated on the intended local delta.
6. Unrelated or higher-level frame defects were not recorded.
7. The result was described as fixed/restored/verified.
8. A later human viewing the whole image immediately found the remaining bug.

Phone work demonstrates the chain: native menu semantics, pointer projection,
screen-light state, lens visibility, glyph scaling, and restart actions gained
real tests. Those are valuable results. They did not establish acceptable phone
typography, projected composition, screen-only light appearance, or a convincing
restart overlay. The current ledger therefore preserves their behavioral proof
while rejecting their visual acceptance.

## Structural contributors

- The migration matrix had one status vocabulary for several kinds of proof.
- `native-*` commonly meant native-driven Godot state, not a native C++
  framebuffer.
- `final`, `ok`, `verified`, and `readable` appeared in evidence directory names
  before independent visual acceptance.
- Tests emphasized source bytes, transforms, selection state, and process exit
  codes rather than projected size, occlusion, contrast, or visual salience.
- Successful frame capture was sometimes treated as successful graphical
  verification.
- Comparisons used different fixtures, ticks, cameras, revisions, and dirty
  states while prose shortened them to parity.
- Large evidence volume made provenance and negative evidence hard to find.
- A local correction did not require a cold whole-frame defect inventory.
- Native-source defects and deliberate Godot adaptations were not always
  separated from migration regressions.

## Enforced correction

`VISUAL_ACCEPTANCE.md` defines independent behavior, presentation-contract, and
visual statuses. `evidence/visual-acceptance/ledger.json` records the current
claims. `tests/check_visual_acceptance.ps1` rejects:

- unknown or collapsed statuses;
- verified/partial axes without evidence;
- missing evidence paths;
- accepted visuals without a target, framebuffer, reviewer, timestamp, and
  whole-frame review;
- acceptance with unresolved blockers;
- completion language on rejected or unverified cases.

`tests/prepush.ps1` invokes the validator. `AGENTS.md` makes the resulting claim
language part of the repository agent contract. `MIGRATION_MATRIX.md` now says
explicitly that its implementation status is not visual acceptance.

## Remaining system work

The ledger prevents unsupported claims but cannot judge pixels. The next
technical leverage point is one shared deterministic capture-state format that
both the selected C++ renderer and Godot can load. It should include room seed
and index, tick, player/entities, camera transform/FOV, time/lighting, viewport,
and quality settings. Until that exists, cross-renderer correspondence remains
approximate and must be reported that way.

Once exact capture state exists, add scenario-specific metrics rather than a
single similarity score:

- phone display: projected bounds, row baselines, text contrast, light centroid
  and spill outside the screen mesh;
- restart/HUD: safe-area bounds, projected text height, overlap and contrast;
- particles: maximum projected diameter, material family, depth and lifetime;
- humans: silhouette, crack-mask coverage, exposed-shell contrast and damage
  response through time;
- environments: lower-half luminance, subject/background separation, clipping,
  visible ground coverage and large unintended primitives.

Metrics are rejection guards, not automatic aesthetic approval. A human
whole-frame and temporal review remains required.
