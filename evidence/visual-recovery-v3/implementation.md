# V3 implementation

The selected discrepancy was the absent Godot doorway datamosh. Native C++ already owns activation, progress, distance, and frame motion; Godot now only presents those values.

## Vertical slice

- Added a bounded read-only native snapshot with stable identity `native-door-transition`.
- Added a render-only CanvasLayer adapter below the HUD and above the 3D scene.
- Added a Compatibility-renderer canvas shader using the source 48×27 grid, source travel range, motion clamps, strength/alpha relationship, and deterministic cell displacement.
- Kept the native simulation as the only transition authority.
- Extended schema 4 with command/tick-correlated native transition state and Godot visibility/progress error.
- Added unified object evidence for active transition observations.
- Extended the existing room-transition contract and protocol hardening contract.
- Added a graphical capture script covering inactive, strength-1, and mid-peel checkpoints.

## Intentional differences

The mature desktop renderer retains a framebuffer from before the room switch. Godot's Compatibility canvas implementation samples the current screen buffer. The characteristic block peel and motion smear are recovered, but exact historical-frame persistence is not claimed. No GLB export applies to a screen-space effect.

## Captures

- `captures/00-before-door-transition.png` — clean pre-transition framebuffer.
- `captures/01-native-transition-strength-1.png` — maximum native transition strength.
- `captures/02-native-transition-peel.png` — native mid-strength block peel after upgrade resume.

The three frames were visually inspected as a sequence. The mid-peel is clear and intentional. The strength-1 frame is dark and lacks an exact desktop matched-state reference, so visual acceptance remains unverified.
