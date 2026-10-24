# Visual Recovery V5 — native particle geometry

The V3 inventory identified the shared Godot particle sphere as a remaining
presentation substitution. The native simulation remains authoritative and
continues to supply position, lifetime, size, material, color, settled state,
and surface normal.

`native_particle_presentation_adapter.gd` now mirrors the mature desktop
renderer's geometry contract:

- Airborne particles are rotating unit-box primitives scaled by native size and
  remaining lifetime.
- Settled signal residue is projected onto the native support normal as four
  deterministic dark stain lobes.
- Each residue fragment contains three restrained native-color glints.
- The existing near-camera cap remains presentation-only.

No gameplay state, particle integration, collision, lifetime, material color,
or capture behavior moved into Godot.
