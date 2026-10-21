# Causal analysis

## Lighting/readability

Native path: `GameState` → `ProgressiveCandidate`/celestial and `RoomLighting` → `SceneLightingResponse` → `DesktopRenderer` fixed-function global ambient, fog and resolved lights → native framebuffer.

Godot path: the same retained parent `GameState` → `native/lighting_presentation_snapshot.hpp` → `ParentGameNode::lighting_presentation_snapshot()` → `main.gd::_sync_lighting()` → `Environment` ambient/fog, normalized directional lights and six `OmniLight3D` nodes → Godot StandardMaterial/shader output.

Classification: **rendering configuration plus renderer-response mismatch**. Source data exists and is parity-tested. `main.gd` maps RGB magnitude to `light_energy`, derives omni range only from quadratic attenuation, and leaves Godot exposure/tonemapping/default PBR behavior implicit. Desktop consumes fixed-function material/color/light equations and explicit attenuation terms. The Godot ground and humans therefore receive materially less useful separation even though the sky is bright. Narrowest owners: `godot/scripts/main.gd::_sync_lighting`, `_sync_directional_light`, `_sync_open_sky`, and material creation in `room_presentation_adapter.gd` / `human_model_adapter.gd`.

## Lawn slab

Native state/generation: `parent/game/RoomEnvironment.hpp::environmentProp()` returns a `LawnFragment` with center, thin Y size, footprint and yaw. `ComputedSceneObjects.hpp::objects()` emits `GeometryKind::Box`, stable ID, and `physical=false`.

Bridge: `native/room_presentation_snapshot.hpp::from()` copies computed objects; `native/parent_game_node.cpp::room_presentation_snapshot()` serializes the record.

Godot: `room_presentation_adapter.gd::_sync_lawn_objects()` filters primitive 2/box 0, creates `BoxMesh`, assigns record transform/size, and applies the floor material. Lifecycle is keyed by stable ID; stale IDs are freed.

Classification: **incorrect/incomplete visual interpretation**, not missing data. A source-authored ground-detail record is represented as a uniform raised box without a dedicated lawn visual contract, and darkness exaggerates the silhouette.

## Phone

Native state: `PhoneVisualState`, `PhoneTransformState`, `PhoneDisplayLayout`, camera mode/FOV. Bridge: `PhonePresentationSnapshot` and menu-layout dictionaries. Godot: `phone_presentation_adapter.gd::sync_snapshot()`.

Classification: **camera/projection and intentional renderer-specific adaptation**. Third-person accepts native world transform at 1×. First-person overrides placement and scale. No gameplay repair belongs here.

## Particles

Native state: `GameState::particles` with position, material color, size, velocity and lifetime. Bridge: particle presentation snapshot. Godot: `native_particle_presentation_adapter.gd::sync_from_native()` creates unshaded sphere instances, scales diameter to native size, then caps near-camera radius.

Classification: **renderer-specific deviation**, with the previously observed oversize failure already mitigated. Native simulation remains authoritative.

## Environment and HUD

Environment records and native texture bytes generally reach the bridge; missing bark/sand/sterile-tile/high-detail grass and exact desktop shadow/material behavior are **missing adapter implementation or explicitly unsupported**. HUD values/layout inputs reach `hud_presentation_adapter.gd`; sparse composition and contrast are **adapter/layout presentation** issues, not timer/state defects.
