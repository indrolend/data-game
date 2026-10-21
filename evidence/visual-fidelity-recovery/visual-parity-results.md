# Visual results

This milestone supplies Godot before/after evidence, not genuine native/Godot pixel-matched pairs.

- Original dark Field: `../visual-parity-gauntlet/godot-environments-ok/frame-001-tick-000006.png`
- Direction-only Field: `experiments/directional-sign/frame-001-tick-000010.png`
- Final Field with corrected lawn material: `after-initial-milestone/frame-001-tick-000011.png`
- Direction-only combat sequence: `experiments/directional-sign-combat/`
- Direction plus human normals: `experiments/directional-sign-flat-normals/`
- Final adjacent room/camera gauntlet: `after-initial-milestone/` (9 frames: Field, Coastal, Sterile; third- and first-person)

Observed result: ground, props, and humans separate visibly from the background after the directional correction. LawnFragment remains at its authoritative size and transform but no longer inherits the generic floor material. Human faces exhibit per-face directional response rather than relying on missing normals.

Unsupported claims: exact native exposure, shadow parity, matched camera projection, desktop texture-coordinate equivalence, and native/Godot pixel parity.
