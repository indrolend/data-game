# Phone menu renderer convergence (draft PR #153)

## Authority boundary

Native `PhoneDisplayMenuLayout` owns menu content, selection, logical geometry, and actions. Godot owns display geometry, typography, color, and screen composition. No renderer may independently decide the selected item or mutate gameplay state.

## Switching renderers without changing native behavior

The `PhonePresentationAdapter` exports:

- `menu_renderer = 0`: original native 720x1280 bitmap raster compatibility path (default). The existing GPU texture cache remains active.
- `menu_renderer = 1`: Godot semantic `Label3D` path using the same native `menu_layout` snapshot. Selected rows use the native logical visual rectangle for the highlight.
- `menu_font_path`: optional Godot resource path to an imported font (`.ttf`, `.otf`, or `Font` resource), loaded at node startup. An empty or unavailable path leaves the engine default font; this **does not** imply Source Sans visual parity.

Set these exported properties on the `PhonePresentationAdapter` scene node, or change `menu_renderer` on the adapter at runtime. Both modes preserve native menu action routing and the phone's native screen transform. The native raster is deliberately retained as a reversible compatibility fallback.

## Remaining parity gaps

- The source-native desktop's Source Sans stencil glyphs, horizontal metrics, glyph-by-glyph reveal, and paint effects are not recovered by selecting the semantic renderer.
- The semantic renderer uses Godot's `Label3D` glyph atlas and a selected-row quad. It does not yet reproduce native bitmap pixels or desktop compositor effects.
- Menu content opacity and per-row styling are derived from existing snapshot fields. Visual acceptance requires matching desktop and Godot captures in the same state.
- The Godot project currently references `res://../parent/models/phone.dbmesh`; packaging and source asset provenance require a separate review.

## Deferred integration boundary

Per the development decision, no tests or captures are executed in this batch. Before changing the default to semantic mode, run the existing phone presentation and navigation regressions, inspect GPU captures of pause/settings/long Controls pages, compare text alignment and clipping, and verify that switching back to mode 0 preserves behavior. Keep PR draft until then.

## Why this is staged

Changing the default renderer before the intended font and visual parity are available would silently replace the current native bitmap presentation with another unverified visual approximation. The two-mode switch makes comparison possible without duplicating menu gameplay authority or deleting the known compatibility path.
