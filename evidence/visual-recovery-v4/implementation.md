# Visual Recovery V4 — native phone raster

Baseline: `95cc63d6365b2e624b22800f5eb1d153da8c0f55` on `data-main/godot-migration`.

## Recovered slice

The V3 inventory ranked phone typography and pause composition first. Godot previously reconstructed native menu rows as separate `Label3D` nodes. V4 adds two read-only bridge calls:

- `phone_display_raster_revision()` hashes the current native display mode, selection, title, hints, rows, visibility, and values.
- `phone_display_raster_pixels()` generates a 720x1280 RGBA image from `PhoneDisplayMenuLayout`, `PhoneMenuModel`, native palette semantics, and the mature desktop Source Sans 3 font rasterizer.

`phone_presentation_adapter.gd` uploads the image to a nearest-filtered emissive `QuadMesh` only when the revision changes. Native navigation and settings actions remain authoritative. The camera-only handset offset changed from `(0, 0.05, -0.70)` to `(0, -0.04, -0.70)` after whole-frame review found the handset top clipped.

No native gameplay mechanics, menu state, collision, or timing changed.

The follow-up correction replaces the interim 5x7 bitmap glyphs with the
original Source Sans 3 Semibold asset, centers the menu composition, and keeps
the camera-presented screen depth-independent. The checked-in font retains its
SIL Open Font License in `godot/fonts/LICENSE.txt`.

## Explicit boundary

This recovers the native Source Sans raster and removes the Godot font
substitution for pause/settings. It does not yet duplicate the desktop
renderer's animated stencil reveal or per-letter paint splatter. Therefore
exact desktop visual parity is not claimed.
