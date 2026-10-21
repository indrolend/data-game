# Remaining work

Highest-leverage next repair: expose and materialize native tree trunk/crown/card geometry. The Field remains visibly sparse, and the migration inventory already marks trees/foliage incomplete. Acceptance should require stable native IDs/transforms, native seeded family/cluster records, bark/foliage material identity, no Godot collision, direct-parent snapshot parity, and matched deterministic Field GPU captures.

Other important limitations:

- Zero genuine current-native/Godot captures share a serialized simulation state, tick, camera, and environment.
- Native nighttime Field, synchronized City day/night, and GPU room-transition pairs remain missing.
- Fixed-function attenuation, fog, shadow, color-space, and Godot PBR equivalence remain unresolved.
- Grass blades remain a low-detail Godot adaptation; high-detail selection and desktop shadow behavior are absent.
- Bark, sand, and sterile-tile material hookups are still unported.
- Particle families, residue, phone geometry/display, HUD completeness, and transition presentation need later source-specific work.
