# Prioritized repair plan

## 1. P0 — restore Field subject/surface readability

- Observed: ground and humans collapse toward black while sky remains bright; lower-half luma is ~33–34/255.
- Native expectation: open-room subjects remain separable under the resolved atmosphere/light response; do not equate Sterile's intentionally dark progressive profile with black Field materials.
- Root cause: confirmed mapping mismatch, exact contribution split unresolved (ambient/PBR response, directional energy, attenuation, exposure/color-space).
- Location: `godot/scripts/main.gd::_sync_lighting`, `_sync_directional_light`; material builders in `room_presentation_adapter.gd` and `human_model_adapter.gd`.
- Smallest repair: first add a diagnostic capture that records native light dictionary plus Godot environment/light/material effective values, then adjust only the proven mismatched mapping. Do not add a new shader or gameplay light.
- Regression: repeat Field fixture at tick 6 and controlled human frame around tick 1038; require lower-half luma and subject/background contrast thresholds while preserving `LIGHTING` structured parity.
- Risk: global energy/exposure changes can wash out Coastal, City windows, phone glow and Sterile atmosphere.

## 2. P0 — make LawnFragment read as ground detail without deleting it

- Observed: thin native lawn records read as bright/floating slabs.
- Expected: ground-attached lawn variation, nonphysical.
- Root cause: dedicated source record is flattened to a generic `BoxMesh` with floor material; lighting contrast amplifies it.
- Location: `room_presentation_adapter.gd::_sync_lawn_objects`; contract source `RoomEnvironment.hpp::environmentProp`.
- Smallest repair: preserve stable ID/transform/footprint and render-only status, but use a ground-conforming presentation whose height/normal/material treatment is explicitly checked against native desktop output. If desktop has no lawn draw path, document omission instead of inventing a decorative slab.
- Regression: fixture assertion for primitive/id/transform/no collision plus a camera-space bound showing the fragment remains ground-attached; paired capture review.
- Risk: changing size-axis interpretation may invalidate existing snapshot/material tests or conceal real traversal geometry if primitive filtering regresses.

## 3. P1 — reconcile phone screen-space composition

- Observed: third-person phone is too small; first-person uses a hard-coded 1.5× adaptation.
- Expected: match native screen coverage/readability at the same camera mode and FOV.
- Root cause: camera/projection and adapter offsets, not missing native data.
- Location: `phone_presentation_adapter.gd::sync_snapshot` lines 80–95 and `main.gd` camera FOV sync.
- Smallest repair: measure native phone bounding box for a controlled view, then tune only render-only view placement/scale per camera mode.
- Regression: fixed-resolution bounding-box ratio and screen-center offset checks at 1280×720, alongside existing phone snapshot tests.
- Risk: menu text clipping, near-plane clipping, and misleading world-scale changes.

## 4. P1 — finish particle presentation parity

- Observed: catastrophic oversize is mitigated, but spheres and depth behavior differ from desktop.
- Expected: native size/material identity without full-screen occlusion.
- Root cause: primitive/renderer deviation.
- Location: `native_particle_presentation_adapter.gd`.
- Smallest repair: capture and measure one native particle burst before selecting a Godot primitive/billboard mapping; retain the camera cap as presentation-only until replacement is proven.
- Regression: maximum screen-space diameter at controlled distances plus native particle snapshot equality.
- Risk: transparent overdraw and reintroducing near-camera occlusion.

## 5. P1 — port only evidenced environment gaps

Add bark, sand, sterile tile and high-detail grass only after locating the native visual contracts/assets. Maintain analytical Coastal water and native City/room records. Each addition needs a fixture, material/byte assertion where possible, and a graphical paired capture.

## 6. P2 — improve HUD/upgrade contrast and hierarchy

Keep snapshot ownership unchanged. Tune adapter anchors, backing opacity and type scale against native captures. Add 1280×720 safe-area/contrast checks. Avoid adding gameplay timers or state.

## First recommended minimal repair

Instrument and correct the Field lighting/material mapping, not the lawn mesh. Acceptance criteria:

1. Same Godot Field fixture/camera at tick 6 and same controlled enemy frame are captured at 1280×720.
2. Lower-half luma increases from the recorded ~34/255 baseline to at least 50/255, or a reviewed native-derived threshold if the native paired Field capture establishes another value.
3. Median human-to-background luma separation is at least 15/255 for the nearest intact human.
4. Sky highlights do not clip more than 2% of pixels and phone glow remains localized.
5. `tests/verify.ps1`, `lighting_presentation.gd`, room/human/phone tests, and direct-parent parity all return zero.
6. Coastal, City and Sterile comparison captures show no new washout.
