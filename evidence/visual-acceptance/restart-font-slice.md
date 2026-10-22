# Restart overlay bitmap-font slice

Base revision: `842ba69988299246ab52fda92764ef02e13a5546`, dirty working tree.

The native renderer draws the death choices with `bitmapGlyph()` as 5x7 pixel
cells and advances every character by six scaled pixels. Godot previously used
`ThemeDB.fallback_font`, which changed glyph shape, width, baseline, centering,
and perceived weight. The native table has no question-mark glyph: `Again?`
reserves the final advance but renders that cell blank.

The HUD adapter now draws the exact source rows needed by `Again?` and `Quit`,
preserves the native blank punctuation behavior, and retains the native row
centers, scales, colors, fade, pulse, and selection-block geometry. Native menu
state and hit testing remain authoritative.

## Proof

- Headless `godot/tests/restart_menu.gd`: exit 0.
- RTX 3060 graphical invocation: exit 0.
- Captured framebuffer:
  `../godot-842ba69-dirty-restart-menu/godot-842ba69-dirty-restart-menu-death-tick-000045.png`.
- The test directly checks the native `A` bitmap rows and blank `?` rows.

## Whole-frame review

The bitmap typography is visibly clearer and source-equivalent. Visual status
remains rejected: the composition is small, the character is directly behind
the menu, the scene is dark, and the large foreground lawn geometry competes
with the overlay. There is no exact matched-state native framebuffer. This
slice therefore improves the presentation contract without claiming complete
visual restoration.
