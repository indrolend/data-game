# Data Phone UI and Text Effects Handoff

## Objective

Preserve the existing `PhoneMenuModel -> PhoneDisplayLayout -> DesktopRenderer` architecture while making the Data phone UI readable, text-safe, and visually distinctive. The intended character is a restrained digital spray/stencil arrival: each string should feel freshly sprayed for a fraction of a second, then settle into clean, bold, readable text.

This work does not redesign gameplay, add menu features, or intentionally change world lighting.

## Work completed

### Typography hardening

- Centralized the small set of phone typography and spacing constants in `PhoneDisplayLayout.hpp`.
- Made title width, label width, value width, minimum two-column gap, and available text width explicit layout constraints.
- Added a renderer measurement hook so layout uses the same text geometry that is actually drawn.
- Fit titles and row text to their available regions instead of allowing clipping or label/value collisions.
- Increased table-row and section-label spacing slightly for readability.
- Preserved the existing semantic menu model and fixed-footer behavior.

### Text effect

- Added a horizontally condensed stencil treatment in the CPU phone renderer.
- Bound the one-shot reveal to the authoritative phone transition progress instead of a fragile per-string animation registry.
- Kept directional underprint, edge variation, and small flecks only during the short arrival.
- Removed permanent bridge cuts, double-image residue, and lingering breakup from settled text.
- Settled text is deliberately clean; effect character comes from motion, not damaged letterforms.

### Tests and verification

- Extended `phone_menu_layout_test.cpp` with text-safe invariants for current menu pages.
- Verified `PhoneMenuLayoutTest`, `PhoneDisplayStateTest`, and `RenderContractsTest`.
- Verified the desktop smoke test.
- Verified `git diff --check` before commits.

## Visual direction

The Data phone should read as a physical-digital object: teal/metallic presentation, restrained acid-chartreuse emphasis, strong hierarchy, and legibility at phone/video scale. Text should evoke a tasteful, directional spray-stencil application without becoming graffiti noise. Variation belongs in the first few frames of appearance; the resting interface must remain effortless to read.

The reported lighting regression and missing day/night behavior remain a separate acceptance concern. This UI convergence branch makes no intentional lighting or day/night changes. Before merging, evaluate it against the desired lighting reference and reject any build whose world lighting, day/night cycle, or presentation baseline is wrong.

## Git trajectory

- `codex/phone-ui-hardening` is the narrow pass based directly on current `origin/main`. Commit `2080861dd90092fe1a17bdf02153db6766bc77bf`.
- `codex/phone-text-effects` preserves the text-effect implementation on the presentation/lighting baseline used during visual iteration. Commit `80595150c5d2caa7e4a15dc3581d41267d7102af`.
- `codex/phone-ui-convergence` is the recommended review branch. It starts at current `origin/main`, includes the hardening pass, and then applies the restrained spray-on renderer effect.

## Recommended convergence sequence

1. Review and build `codex/phone-ui-convergence` against `main`.
2. Check every current phone menu at native resolution and in a downscaled phone/video presentation.
3. Confirm titles never clip and two-column labels/values maintain a visible gap.
4. Confirm the reveal is brief and directional, while settled text is fully readable.
5. Run the targeted tests and desktop smoke test.
6. Visually verify world lighting and the day/night cycle against the accepted reference before merging.
7. Merge the convergence pull request if both UI and lighting acceptance checks pass.

## Repository hygiene note

The generated Windows ZIP, PNG previews, and MP4 showcases are local review artifacts and are intentionally not committed to source control. Other dirty worktrees on the workstation contain separate enemy, zombie, specification, runtime-mirror, and predator-probe experiments. They were inventoried and preserved, but were not bundled into this phone-UI branch because their ownership and convergence requirements are independent.
