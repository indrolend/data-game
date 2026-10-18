# Phone presentation vertical slice

The native-only `PhonePresentationSnapshot` is built from `GameState` after each simulation tick. The Godot adapter renders the native phone position/orientation and dimensions in third person, plus native display mode/tint/brightness and battery/soul/vacuum indicators. In first person, it moves the same phone presentation to a camera-relative view-model pose; this is rendering only and does not affect native simulation.

This does not port the desktop phone renderer, pixel UI, menus, or assets.

## Captures

- `../phone-presentation-final/frame-001-tick-000001.png`: third person after the native transform has initialized.
- `../phone-presentation-final/frame-002-tick-000002.png`: first-person camera mode.
- `../phone-presentation.commands` records the exact agent-playtest sequence.
- `../phone-presentation-final-run.log` and `../phone-presentation-final.exit-code` preserve runner output and actual process result (`0`).

The images were captured by Godot 4.7.2 using OpenGL Compatibility on an NVIDIA GeForce RTX 3060 and inspected directly. They establish visible phone presentation in both modes, not visual parity with the original renderer.

## Deterministic checks

- `verification/parent-reference.csv` and `verification/godot-parity.log` include nine `PHONE` snapshots from both direct C++ and `ParentGameNode`, alongside the previous state/action/target/interaction rows.
- `verification/parity-result.log`: 29 structured rows matched within `1e-5`.
- `verification/phone-presentation.log`: headless adapter contract passed for dimensions, screen offset/status, third-person native transform, and first-person camera-relative transform.
- `verification/` retains build, smoke, startup, and parity logs. `verification/previous-canonical/` preserves the canonical evidence that existed before this verification run; it was restored after copying the new run outputs.
