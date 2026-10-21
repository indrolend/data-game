# Repair log

1. `godot/scripts/main.gd`: convert native OpenGL vector-toward-light state to Godot's emitted-ray convention by aiming local `-Z` at the negated native vector.
2. `godot/scripts/human_model_adapter.gd`: populate `Mesh.ARRAY_NORMAL` with one flat normal per triangle, matching desktop `triangle_geometry::faceNormal` behavior.
3. `native/parent_game_node.cpp`: expose the already-authoritative desktop LawnFragment material parameters.
4. `godot/scripts/room_presentation_adapter.gd`: apply FieldGrass texture, tint, world scale, and phase to LawnFragment rather than reusing the floor material.
5. Added focused direction, normal, and lawn-material assertions; updated `MIGRATION_MATRIX.md` only for verified status changes.

No gameplay state, collision, movement, combat, time, procedural placement, or camera simulation was moved into Godot.
