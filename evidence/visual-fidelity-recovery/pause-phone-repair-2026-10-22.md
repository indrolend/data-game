# Third-person pause phone repair — 2026-10-22

## Observed regression

The prior verified Godot menu capture at
`godot-phone-menu-current/frame-000-tick-000013.png` showed a tiny world-space
phone. The abandoned `godot-phone-pause-repaired*` SubViewport experiment was
also retained: it rendered a black screen and is not evidence of a repair.

The native comparison reference is `native-phone-pause.png`. It displays a
complete, centered, readable handset approximately 566 pixels high in a 720px
frame (78.6%).

## Repair

`godot/scripts/phone_presentation_adapter.gd` now treats an *active native
phone menu* as a camera-presented, render-only phone in addition to native
first-person mode. It does not alter native phone transform, pause state,
selection, menu layout, collision, movement, or camera simulation. The existing
native menu snapshot still supplies every row and native semantic actions remain
the sole interaction authority.

The camera composition is `position=(0.0, 0.05, -0.70)` relative to the native
camera basis and scale `3.4`. It was tuned only against captured 1280x720
frames.

## Evidence

Command file:
`third-person-pause-camera-anchored.commands`

Final graphical command:

```powershell
godot_console.exe --path godot --script res://scripts/agent_playtest.gd -- --commands=<repo>\evidence\visual-fidelity-recovery\third-person-pause-camera-anchored.commands --capture-dir=res://../evidence/visual-fidelity-recovery/godot-third-person-pause-final2
```

Graphical runtime: Godot 4.7.2 Compatibility renderer, NVIDIA GeForce RTX 3060.
The command exited `0`; state logs record `first_person=0`, `ui` pause state,
and three successful framebuffer PNG writes. Inspected final frame:
`godot-third-person-pause-final2/frame-002-tick-000002.png`.

The complete handset is visible at approximately 560/720px (77.8%) and menu
rows Resume, Settings, and Exit Run are readable. This satisfies the initial
third-person visibility/occupancy target, though it is not a pixel-identical
cross-engine comparison.

## Regression coverage

`godot/tests/menu_navigation.gd` now explicitly pauses from native third-person
gameplay and asserts the PhoneRoot becomes camera-anchored; it then resumes and
continues the existing first-person keyboard/mouse navigation proof.

`godot/tests/phone_presentation.gd` asserts the updated camera-relative
first-person composition. `tests/verify.ps1` passed after these changes:
native build, native pointer contract, all headless Godot behavior tests,
startup, and direct parent/Godot parity.

## Remaining limitation

This fixes composition and readability, not raster identity. The Godot screen
still renders native menu layout with Label3D rather than the exact desktop
720x1280 raster/paint/stencil implementation. The next phone slice should
recover that raster contract without revisiting native menu authority.
