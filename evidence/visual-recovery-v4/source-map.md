# Visual Recovery V4 source map

| Responsibility | Authority | V4 consumer or evidence |
| --- | --- | --- |
| Menu state and actions | `parent/game/Game.cpp`, `PhoneMenuModel.hpp` | Existing `ParentBridge`; unchanged |
| Logical rows and coordinates | `parent/game/PhoneDisplayLayout.hpp` | `ParentGameNode::phone_display_raster_pixels()` |
| Glyph masks | `parent/game/BitmapFont.hpp` | Native raster generator; no Godot glyph recreation |
| Display tint/brightness | `GameState::phoneDisplay` | Native raster background |
| Content revision | Native mode, selection, title, hint, labels, values, visibility | `phone_display_raster_revision()` |
| GPU presentation | Revisioned native RGBA bytes | `godot/scripts/phone_presentation_adapter.gd::NativeDisplayRaster` |
| Input hit testing | Existing native logical layout and `menu_item_at()` | Unchanged Godot inverse projection |
| Contract verification | Native bytes, revisions, actions, texture state | `godot/tests/phone_presentation.gd` |
| Framebuffer evidence | Actual Compatibility renderer | `godot/tests/phone_raster_visual_capture.gd` and `captures/` |

The mature desktop comparison remains `C:\Users\indro\Projects\data-game\desktop\DesktopRenderer.cpp::renderPhoneDisplayPixels`. V4 intentionally reuses the shared native bitmap vocabulary instead of copying the desktop-only STB TrueType renderer into the gameplay bridge.
