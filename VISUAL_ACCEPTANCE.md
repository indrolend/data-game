# Visual acceptance contract

Digital Breakdown has three independent proof axes. Never collapse them into a
single `verified`, `fixed`, `restored`, or `parity` claim.

1. **Behavior** — the authoritative native simulation or menu operation does
   the correct thing.
2. **Presentation contract** — the renderer receives and interprets the
   intended state, geometry, transforms, materials, and timing.
3. **Visual acceptance** — an identified human reviewer inspected the actual
   framebuffer as a whole and accepted it against an identified target.

A passing headless test can establish axes 1 or 2. It cannot establish axis 3.
Saving a PNG establishes only that rendering and capture occurred. Image
measurements can support axis 3, but do not replace whole-frame inspection.

## Claim vocabulary

Use the weakest accurate claim:

- `behavior verified; presentation unverified`
- `presentation implemented; visual acceptance unverified`
- `visual acceptance rejected: <blocking defects>`
- `visually accepted against <target> by <reviewer>`

`fixed`, `restored`, `final`, `verified visual parity`, and similar completion
language are permitted only when the ledger's visual status is `accepted`.
Acceptance does not imply pixel parity unless the case also records an exact
matched state and a native-renderer reference.

## Evidence identity

New visual evidence should use:

`<renderer>-<commit>-<scenario>-<state-id>-<frame>.<ext>`

`renderer` names the framebuffer producer (`cpp-desktop`, `godot`, etc.), not
the simulation authority. In particular, a Godot frame driven by native state
is still a `godot` frame. Do not add `final`, `fixed`, `ok`, `verified`, or
`readable` to evidence names.

Each accepted case must record:

- repository revision and dirty state in its capture manifest;
- executable/library identity where available;
- renderer, viewport, scenario, tick/state ID, camera, and FOV;
- reference kind: `native_renderer`, `approved_design`, or
  `intentional_improvement`;
- state correspondence: `exact`, `approximate`, or `not_applicable`;
- before/reference/after paths where applicable;
- a whole-frame review and zero unresolved blocking defects.

Temporal behavior requires a sequence, not one selected frame.

## Required review order

Review a capture without relying on the implementation goal. Record the most
obvious defects first:

1. unintended primitives, clipping, and occlusion;
2. silhouette, projected scale, placement, and composition;
3. text hierarchy, centering, and contrast;
4. light origin, reach, exposure, and material response;
5. texture/material identity and scene depth;
6. temporal continuity, animation, and state transitions.

Any unresolved P0/P1 defect makes visual status `rejected`, even when behavior
and presentation-contract tests pass.

## Tooling

The tracked ledger is `evidence/visual-acceptance/ledger.json`.

Run:

```powershell
.\tests\check_visual_acceptance.ps1
```

The validator is also called by `tests/prepush.ps1`. It checks the schema,
status separation, evidence paths, whole-frame review requirements, and the
language permitted by each visual status. Historical evidence directories are
not renamed; they are legacy inputs and their names confer no status.
