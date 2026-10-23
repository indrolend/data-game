# Visual recovery V3 source map

## Authority

- Native gameplay state: `parent/game/Game.hpp`, `parent/game/Game.cpp`.
- Mature desktop renderer reference: `C:/Users/indro/Projects/data-game/desktop/DesktopRenderer.cpp` at local native revision `381cd04d84fe10e050d11c24d4f7f68f7509dc59`.
- Godot bridge: `native/parent_bridge.hpp`, `native/parent_game_node.hpp`, `native/parent_game_node.cpp`.
- Default presentation: `godot/scenes/main.tscn`, `godot/scripts/main.gd`.
- Unified Evidence V2: `UNIFIED_EVIDENCE.md`, `godot/scripts/agent_playtest.gd`, `tools/invoke-unified-evidence-v1.ps1`.
- Visual acceptance policy: `VISUAL_ACCEPTANCE.md`, `evidence/visual-acceptance/ledger.json`.

## Selected slice lineage

- Native transition state: `Game.hpp:564` (`DoorTransitionState`).
- Native transition activation and decay: `Game.cpp:2179-2180`, `Game.cpp:2277-2285`.
- Mature desktop effect: `DesktopRenderer.cpp:2154-2201`. It uses transition progress, frame motion, five displaced passes, and a deterministic 48×27 cell peel.
- Baseline Godot behavior contract: `godot/tests/room_transition.gd` established progression and room adapter sync but had no transition-presentation assertion.
- V3 bridge projection: `ParentGameNode::door_transition_presentation_snapshot()`.
- V3 Godot adapter and shader: `godot/scripts/door_transition_presentation.gd`, `godot/shaders/door_datamosh.gdshader`.
- V3 schema-4 observation: `agent_playtest.gd` emits stable ID, active state, progress, frame motion, presentation visibility, and progress error. Unified packaging emits a `door-transition-effect` object record.
- V3 tests/capture: `godot/tests/room_transition.gd`, `godot/tests/door_transition_visual_capture.gd`.

## Relevant history

- `2eb2b56` — bridge native presentation into Godot.
- `09d60ea` — verify native room transition through Godot.
- `842ba69` — checkpoint gameplay and presentation recovery.
- `378d330` — unified evidence diagnostics.
- `6775630` / `de26438` — object-aware schema-4 evidence and camera obstruction ablation.

No gameplay constants, room progression mechanics, or protected V2 controller bytes were changed.
