# Observed visual differences

## P0 — world surfaces and humans are under-readable

Current Godot renders show a bright sky but nearly black ground and low-contrast humans. The full-frame luma is not itself low because the sky dominates: Field is 69.48/255 and the combat frame is 68.66/255. Cropping to the lower half gives 34.42 and 33.32 respectively; first-person phone/menu is 35.71. This confirms a material/world-lighting contrast problem rather than a uniformly underexposed framebuffer.

Native current-main Sterile progressive-candidate frames are also deliberately dark (about 25/255 full-frame luma), while its readable-static reference is 53–54/255. This prevents claiming that all darkness is a Godot regression. The confirmed Godot regression is loss of subject/surface separation under the mapped native lighting, especially in open Field scenes.

## P0 reclassified — large flat polygon

The conspicuous slab in Field frames is a native `EnvironmentPrimitive::LawnFragment`, not an unexplained transient polygon. `ComputedSceneObjects` emits the native record; `ParentGameNode::room_presentation_snapshot()` copies it; `RoomPresentationAdapter._sync_lawn_objects()` creates a stable-ID thin `BoxMesh`, applies record center/yaw/size, and reuses the floor material. The object is intentionally nonphysical.

The failure is presentation: a large, almost uniform thin box is viewed edge-on/obliquely and contrasts strongly against the very dark floor. It reads as a floating opaque polygon. Removing it would violate the source record. The repair must preserve its stable native record while restoring the native lawn visual treatment or suppressing only unsupported geometry under an explicit omission contract.

## P1 — phone composition differs by mode

Third-person uses the native transform at unit scale and is tiny in the wide composition. First-person is a deliberate Godot adaptation at camera-local `(0.22,-0.17,-0.72)` and 1.5× scale, making the body readable but still leaving native menu text small. This is not a missing snapshot: body/screen sizes, transform, screen state, FOV, and camera mode already cross the bridge. The difference is camera/view-model composition in `phone_presentation_adapter.gd`.

## P1 — particles are improved but still renderer-specific

The migration used native particle size directly on a sphere, producing screen-filling circles near the lens. Current code caps only presentation size to `distance * 0.035` while retaining native simulation values. Current combat frames no longer show the catastrophic full-frame circles, so the provisional report describes a historical regression that is mitigated in this checkout. Shape remains wrong relative to desktop point/sprite-like presentation: shaded geometry was replaced with unshaded spheres and close-depth behavior remains renderer-specific.

## P1 — environment remains materially incomplete

Field has the native 64×64 grass texture, 160 native blade placements and tip reactions, but sparse low-detail cross-triangles, weak ground response, and no high-detail selection. City has native asphalt bytes, street grammar, obstacle masses, skyline and facade modules, but reads as blockout architecture with limited fixture depth. Coastal uses native analytical height/wake inputs, but the bounded mesh edge and subtle wave detail remain obvious. Bark, sand, sterile tile hookups, original shadow/material pixels, and exact desktop projections are unported. These are genuine omissions, distinct from the lighting failure.

## P2 — HUD and upgrade readability

HUD anchors are stable and native-state-driven, but the subset is visually sparse. The upgrade overlay is extremely low contrast over the already dark Field frame (`godot-upgrade-ok/frame-001-tick-000005.png`, full-frame luma 41.23); labels are hard to read. Phone-menu text remains constrained by world-space screen resolution and composition. Interaction state is correct; feedback/readability is the defect.
