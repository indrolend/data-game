# Discrepancy ledger

| Severity | Observed failure | Verified cause | Class | Status |
|---|---|---|---|---|
| P0 | Outdoor ground and actors were severely underlit against a bright sky | Native fixed-function OpenGL stores a directional vector toward the light; Godot emits DirectionalLight3D rays along local `-Z`. `look_at(direction)` applied the opposite illumination direction. | C — incorrect adapter | Repaired and regression-tested |
| P0 | Human triangle surfaces lacked a defined source-equivalent light response | Native `drawHumanModel` calculates one flat face normal for every skinned triangle; Godot supplied only `ARRAY_VERTEX`. | B/C — omitted presentation contract | Repaired and regression-tested |
| P0/P1 | Large flat LawnFragment read as a pale polygon/slab | Its native geometry was correct, but Godot reused the room-floor material. Desktop uses FieldGrass pixels, tint `(0.72, 0.84, 0.70)`, world scale `1.45`, and variant phase. | B/C — omitted material fields and wrong adapter material | Repaired and regression-tested |
| Open | Scene remains materially sparser than desktop | Trees/foliage and some surface geometry are not exposed/materialized. | E — incomplete feature | Not repaired in this milestone |
| Open | Fixed-function and Godot PBR output are not photometrically identical | Exposure, attenuation, fog, shadow maps, and material response have no established matched-frame target. | D/G | Requires matched native state evidence |

No claim is made that the lighting correction alone establishes native visual parity.
