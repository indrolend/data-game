# Environment identity and rare-material pass

Native Release captures from source revision `1b3d4ad14a1047edb8b8cbae145d8fbd0f09c0bd` plus the dirty visual working tree.

- `coastal.png`: Coastal Shore inspector fixture. Photogrammetric sand, mossy stone, and the computed wave mesh are active.
- `sterile.png`: Sterile Corridor inspector fixture. Stained institutional tile is active on floor, walls, and obstacle masses under the ceiling-fixture lighting.
- `wireframe.png`: seed 1, room 44. Rare dark prismatic wireframe corruption.
- `gummy.png`: seed 1, room 9. Rare gummy-material substitution.
- `festive.png`: seed 1, room 66. Rare pale festive/snow material substitution.

Rare material mutations begin after room 8, are deterministic from the run seed and room index, are mutually exclusive, and change rendering only. Collision, traversal, enemy behavior, soul accounting, and room generation remain authoritative and unchanged.

The coastal wave surface is computed every rendered frame from two intersecting sine fields. This pass deliberately does not claim water physics: the water occupies the non-route side of the shore and currently provides animated presentation, normals, transparency, and moving foam, not buoyancy or a swimming state.

Validation: the native executable reported all six photographed material families loaded (`grass/bark/stone/metal/sand/sterile`). Direct CTest passed 51/51 tests.
