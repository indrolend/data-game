# Phone, character, and playable-loop runtime review

Date: 2026-10-20 (America/Chicago)

## Runtime provenance

- Native source: `C:\Users\indro\Projects\data-game`, `main` at `18b19807333be6b1c83f0a39ae08084978285aeb`.
- Native graphical reference: `C:\Users\indro\Projects\data-game-tools\artifacts\build\18b19807333b-80c91579-Release\bin\Release\DigitalBreakdown.exe`, SHA-256 `56d9f603955fbf398aa5d0fb65c295a940d20552c703385f536d946d78ab410b`.
- Godot source baseline: `06fc26045269dcdec8b934cd147aa0345651d8be`, with preserved uncommitted fidelity/test work.
- Loaded GDExtension: `godot/bin/digital_breakdown_parent.dll`, SHA-256 `4e46864f768a839683ab8b3aebd46dfca9557903ba440e41a101e954b565ccc9`.
- Godot `4.7.2.stable.official.ed1daf0bf`, OpenGL Compatibility renderer, NVIDIA GeForce RTX 3060.

## Direct play result

The native GPU runtime completed its deterministic vertical slice in 216 ticks at 60 Hz: traversal, shell defeat, soul exposure, vacuum pull, soul storage, all five objective deposits, room clear, room transition, upgrade presentation, selection/resume. The run returned success for traversal, combat, soul storage, objective completion, transition, and resume. See `native-playthrough/manifest.json`, its timeline/assertions, and `native-playthrough/frames/video/00-room-start.png` through `07-next-room-ready.png`.

The Godot default parent-driven runtime could not execute the same visible loop. The native simulation still owns capture-point state, but neither the bridge nor a Godot presentation adapter exposes/renders capture-point records. The only graphical fixture that reaches a room transition starts with the objectives pre-filled, so it proves transition synchronization but bypasses the objective interaction. This is a missing presentation contract, not evidence that the underlying native rules are absent.

There are zero cross-runtime frames with identical serialized simulation state, tick, camera, and environment. Native-versus-Godot image comparisons below are therefore functional/visual comparisons, not pixel-parity claims.

## Phone and menu

The native phone is stateful. `Game::updatePhoneDisplay()` resolves display mode and transitions, battery-dependent brightness and warning pulse, vacuum-dependent noise/brightness, damage/capture/power/critical-hit pulses, tint, emission, and local-light color/intensity/radius/offset. `DesktopRenderer::renderPhoneDisplayPixels()` produces a real 720x1280 display image, including semantic menu layout and feedback, and `drawPhoneDisplayTexture()` uploads it to the handset. The world-lighting calculation also consumes the resolved phone illumination.

Godot preserves considerably more native state than the current image suggests: `phone_presentation_snapshot.hpp` exposes screen and light values, and `phone_presentation_adapter.gd` assigns the native light to an `OmniLight3D`. However, it replaces the native phone mesh with boxes and the native raster display with `Label3D` title/rows/status elements. It also applies a Godot-only `1 + pulse * 0.25` light-energy curve. The result is semantically connected but visually primitive and not the native display contract.

Concrete capture comparison:

- Native pause phone: `native-phone-pause.png`. The centered handset is approximately 566 px tall in a 720 px frame (78.6% of frame height); Resume, Settings, and Exit Run are clearly readable.
- Godot current phone/menu sequence: `godot-phone-menu-current/frame-000-tick-000013.png` through `frame-013-tick-000141.png`. The phone is approximately 227 px tall (31.5% of frame height), about 2.49 times smaller than the native reference, displaced to the right, and the menu labels collapse into unreadable glyph/block stacks.
- Navigation changes native menu state, but the GPU captures do not demonstrate a usable phone UI. The existing headless checks establish snapshot/state plumbing only.

A diagnostic attempt to replace the labels with a SubViewport texture rendered black on the actual GPU path despite passing the headless contract. It was completely reverted. Its frames under `godot-phone-menu-subviewport/` and `godot-phone-pause-repaired*/` are failed diagnostics, not improvements.

## Character damage, shell, and particles

- Native evidence `native-playthrough/frames/video/02-soul-exposed.png` shows an angular, materially distinct shell-break/soul presentation. Native rendering branches by particle material and includes fragments/residue behaviors.
- Godot's `native_particle_presentation_adapter.gd` creates one shared `SphereMesh` for all native particle records. The simulation values and lifetime remain native, but the primitive erases the visual distinction between digital blood, flesh, environment fragments, and residue. The screen-filling purple/gray spheres are therefore a confirmed adapter-level representation error, not a native particle-state error.
- Godot receives native shell-triangle removal, armor, wound tint, hit amount, tether endpoints, and skinned vertices. Its tests confirm those data contracts. The current renderer nevertheless does not establish native-equivalent crack readability: mesh/material/opacity and particle accompaniment differ. This is a presentation-equivalence gap; it is not evidence that shell damage rules are missing.

## Ranked repairs

1. **Recover the phone display contract and composition.** Extract/reuse the native 720x1280 phone-display rasterization as a bounded presentation output, expose its pixels or texture-ready buffer through the bridge, and upload it to the Godot phone screen. Use the native view composition/mesh bounds as the projection target. Do not recreate menu ownership in Godot.
2. **Expose and render capture points.** Add stable read-only records containing ID, world transform, and filled state; render the native backplate/inner/filled-box composition without collision. This is the smallest missing link that makes the authoritative level objective visible and allows the normal loop to be played instead of bypassed.
3. **Split particle presentation by native material family.** Keep native positions, sizes, lifetimes, and gameplay untouched, but map demonstrated families to their native-equivalent primitive (fragment/box, residue stain, sprite/disk, etc.) rather than the universal sphere. Re-evaluate shell-crack readability with the correct accompanying fragments before changing shell gameplay or masks.

Phone-light work should follow a controlled state series, not arbitrary energy changes. Capture idle/full battery, low battery, vacuum, damage pulse, capture pulse, and pause/menu states; compare the native snapshot, Godot node readback, and rendered local light pool. The bridge already contains resolved values, so the first question is visibility/material response and the extra Godot pulse curve, not missing gameplay intelligence.

## Smallest first repair and pass/fail test

The first phone-focused repair is the native phone-display raster bridge plus corrected menu camera composition.

Pass only if one deterministic GPU run demonstrates all of the following:

1. The native and bridge phone display buffers have identical dimensions and byte checksum for fixed Pause, Settings, low-battery, vacuum, damage, and capture states.
2. In the 1280x720 pause capture, the complete handset remains visible and occupies 70-85% of frame height; no menu row is clipped or overlaps another row.
3. Resume, Settings, and Exit Run are visually readable in the saved frame, and semantic navigation changes the highlighted row while native menu state remains authoritative.
4. Godot `OmniLight3D` color, base energy, range, position, and pulse are read back and agree with the resolved native lighting snapshot, except for any explicitly documented renderer conversion.
5. Exiting the menu restores the scene camera and the native run continues without a Godot-owned gameplay state change.

Any black/blank screen, Label3D-only substitute, checksum mismatch, clipped handset/menu, or headless-only success is a failure.

After that repair, the required end-to-end acceptance test is one non-prefilled graphical run that stores a soul, visibly fills capture points, clears the room, crosses the doorway, chooses an upgrade, and resumes. Until capture-point presentation exists, Godot cannot honestly pass that test.
