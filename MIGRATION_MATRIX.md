# Digital Breakdown Godot migration matrix

The default Godot scene runs the original C++ `Game` through `ParentBridge` and
`ParentGameNode`; that simulation owns gameplay. Godot owns platform input,
scene objects, and presentation. `playable_runtime.tscn` is an isolated
Godot-owned experiment, not the default game and not evidence of a faithful
mechanic port. Do not advance both runtimes for the same gameplay event.

Status values: **verified**, **partial**, **contract next**, **not exposed**.

| Domain | Native producer / source of truth | Desktop consumer / evidence | Godot responsibility | Verification gate | Status |
|---|---|---|---|---|---|
| Fixed simulation clock | `Game::update`, `ParentBridge::tick` | desktop main loop / Godot scene | Godot submits input then advances the original simulation by `1/60` per tick | direct-parent parity plus behavior checks | verified |
| Platform input | `Game::setTouchControls`; Godot `main.gd` input events | desktop `main.cpp` / Godot input events | Convert platform events to semantic input and forward to the original `Game` | headless actions and parity | verified; control coverage partial |
| Player locomotion | `updateInputActions`, `updatePlayer` | native player/phone/camera draws / Godot phone adapter | Present original player position and camera state; do not run parallel Godot movement | deterministic movement and parity checks | verified headlessly; visual parity partial |
| Traversal and support | `TraversalCapabilities`, `TraversalGraph`, room/slope/rock support | room geometry and debug rendering | Render the same physical surfaces and observe traversal state | traversal fixtures plus Godot scene checks | partial |
| Camera | `updateCamera`, native camera state | desktop view setup | Apply native position, target, vertical FOV, and mode; transitions remain | camera vectors/mode parity and native FOV projection test | partial |
| Pause | `Game::setUiPaused`, `Game::update` pause branch | cursor-capture/menu layer | Couple cursor state to native pause; render native pause state | clock and movement freeze/resume parity | verified |
| Communication signals | `setCommSignal`, player signal timer | player/HUD feedback | Forward selection and render resulting state | activation/persistence/expiry parity | verified |
| Melee | Default path: `triggerMelee`, `applyMeleeHits`; exploratory path: `player_controller.gd` and `Area3D` | human/phone/effect draws / Godot overlap response | Default forwards semantic action to parent; HUD reads native melee sequence/visual-hit state; exploratory overlap combat is not a parity claim | parent contract/parity and separately scoped prototype checks | native path verified; cue is headless-contract only |
| Soul shooting | `shootStoredSoul`, `processPendingShots` | projectile/effect/audio draws | Render pending action and native bullets | inventory and delayed projectile parity | partial |
| Vacuum and ingestion | `updateVacuum`, `processQueuedSoulCaptures`, `captureSoul`, target tether fields | phone, soul, field, particles, audio | Render the native target-to-phone tether and native ingestion percentage; Godot never selects/pulls/captures targets | controlled lifecycle parity, tether transform/width contract, inspected 1,050-tick GPU evidence | partial; one lifecycle/tether scenario captured, original visual effect parity remains |
| Soul economy | `SoulEconomy`, progression state | HUD, upgrade menu | Present capacity, records, deposits, tokens, and upgrades | economic transition fixtures | not exposed |
| Enemy cognition | Default path: original behavior modules; exploratory path: `enemy_controller.gd` | human model and reaction overlays / Godot humanoid scene | Present parent target state in default scene; exploratory state machine is not a port | native interaction fixtures and scoped prototype checks | native scenarios partial; prototype exploratory |
| Enemy embodiment | `PhysicalEnemyBody`, `HumanVisual`, `HumanModelData`, `parent/models/human.dbhuman` | `drawHumanModel` | Render original triangle model skinned natively; apply native hit amount as a reversible render-only damage tint and native root/reaction state; Godot owns no enemy response | target-pose parity, native-model/deformation/no-collision tests, and inspected combat GPU sample; all variants remain | partial |
| Target lifecycle | `TargetLifecycle`, target state, respawn queue | humans, souls, particles | Pool scene objects by stable target slot/lifecycle | pursuit through capture parity | partial |
| Room identity | `roomPlan(seed,index)` and run rules | `drawRoomTile` | Read setting/form/scale/condition/traversal intent | direct plan metadata parity | verified |
| Room topology | `RoomTopologyState`, `world/RoomGeometry` | tile horizon, doors, camera/lighting origin | Maintain visible tile nodes keyed by native tile index | stable structural IDs/transforms verified and initial tile rendered; transitions remain | partial |
| Endless progression | `updateRoomTopology`, `advanceRunRulesForRoom`, `activeHumanTarget`, `resetRoom` | cleared-room transition, scaled next-room encounter, persistent run feedback | Keep room generation, seed progression, difficulty scaling, and resets native; present native room index and upgrade choices | direct-parent transition/scaling source tests; Godot upgrade snapshot/action roundtrip; actual multi-room GPU interaction | native simulation authoritative; Godot upgrade input/UI now integrated; full multi-room presentation remains partial |
| Computed scene objects | `ComputedSceneObjects::environmentObjects`; `HouseGeometry::parts`; `MarkerPillarGeometry::parts`; `RuinGeometry::parts`; `FacetedRock::makeVisualMesh` | room prop drawing | Materialize render-only lawn, house, marker-pillar, ruin, and faceted-rock records; stable IDs/transforms, no collision; retain missing texture identity/scale without inventing pixels | object IDs/transforms/material tests, direct-parent geometry/color parity, no-collision test, and GPU inspection | partial: lawn, houses, markers, ruins, and faceted rock geometry; source texture pixels absent |
| Collision geometry | Default path: native room supports/collision; exploratory path: authored Godot `StaticBody3D` arena and actor shapes | native debug draw / Godot scene meshes | Default gameplay remains parent-owned; exploratory Godot physics applies only to its isolated prototype | parent behavior/parity plus scoped prototype floor/wall checks | partial |
| Houses | `HouseGeometry::parts`, native `EnvironmentPropSpec`, and `DesktopRenderer` house surface-color mapping | room prop drawing | Render each native house part as a box mesh using snapshot color and stable transforms; preserve physical flag as metadata only; retain stone/metal texture IDs without substituting textures | CityCourtyard fixture, part/material direct-parent parity, no-collision contract, GPU capture | partial: native geometry/color mapping; textures and renderer decorations/models absent |
| Ruins | `RuinGeometry`, `DesktopRenderer` weather/scar pass | room prop drawing | Render native ruin boxes, weather colors, stone identity/phase, and source-sized scar accents; no gameplay authority | Field fixture, scar/material adapter contract, 4-part direct-parent parity, inspected GPU capture | partial: geometry/color/scars integrated; stone texture pixels and full renderer lighting absent |
| Trees and foliage | `TreeGeometry`, foliage clusters | tree models/cards/shadows | Construct trunk/crown presentation and occlusion policy | geometry snapshot and camera captures | not exposed |
| Rocks | `FacetedRock::makeVisualMesh` and native room prop seed/index | faceted mesh/support drawing | Consume native-generated vertices/normals and source-derived substrate color in a render-only mesh; native remains gameplay/support authority | 90-vertex/normal direct-parent comparison plus adapter test; inspect GPU capture, without claiming texture parity | partial: faceted mesh/color integrated; stone texture and clear in-scene visibility remain |
| Marker pillars | `MarkerPillarGeometry`, `DesktopRenderer` marker surface mapping | room prop drawing | Render native two-part pillar records with source-mapped body/cap colors and metal texture identity; no collision or gameplay authority | SterileChamber fixture, adapter contract, 8-part direct-parent parity, inspected GPU capture | partial: native geometry/color mapping integrated; metal texture pixels and exact renderer shading absent |
| City composition | `CityStreetGrammar`, `CityFacadeGrammar` | city ground/street/façade passes | Build native street and façade records | seeded-room structural comparison | not exposed |
| Coastal water | `CoastalWater`, `CoastalWaterSimulation` | water update/draw pass | Render native height field and disturbances | sampled heights plus GPU capture | not exposed |
| Grass | room grass generation/reaction, `FieldGrassTexture` | field grass pass | Generate/render native blades or equivalent batched geometry | seeded blade samples and reaction capture | not exposed |
| Surface geometry | `SurfaceGeometry`, `TriangleNormal`, slopes | structural/slope drawing | Create meshes from native surfaces and normals | vertex/normal snapshot | not exposed |
| Visual identity | `VisualIdentity`, `MaterialResponse` | all desktop material choices | Map semantic identities to Godot materials; avoid ad-hoc colors | structural material fields/parity and capture pass; prop families remain | partial |
| Generated textures | `CitySurfaceTexture`, `FieldGrassTexture` | textured room passes | Deferred until models, hit geometry, rigid-body presentation, traversal, and animation/behavior coupling are verified | texture identity/world scale retained as metadata only | deferred |
| Lighting | `RoomLighting`, `SceneLightingResponse`, render contracts | desktop lighting/shadow passes | Create lights/environment/shadows from native lighting snapshot | numeric snapshot and matched capture | not exposed |
| Phone body | phone transform/visual state, `PhoneBody` | phone model/body pass | Present bounded native body transform and dimensions | phone snapshot and GPU capture | partial |
| Phone screen | `PhoneDisplay`, `PhoneMenuModel`, `PhoneDisplayLayout`, stencil reveal, gameplay phone model | phone render target and `makePhoneDisplayMenuLayout()` | Render native menu rows on the phone and send semantic commands; native menu/settings state stays authoritative | direct-parent layout parity, settings-transition test, third-/first-person captures | partial; basic native menu visible, readability and full phone effects remain |
| Menus/settings | `PhoneMenuModel`, layout, native settings and pause state | `desktop/main.cpp` navigation and `DesktopRenderer.cpp` layout | Translate keyboard/mouse events to native move/adjust/confirm/back; Godot has no duplicate page or settings state | layout parity and deterministic menu/settings transition tests | partial; offline navigation/settings and resume supported; rebind capture, exit, online host/join absent |
| Room recompile/upgrades | `Game::chooseTemporaryUpgrade`, `Game::purchasePermanentUpgrade`, run/permanent progression state | native six-choice Shot/Lunge/Attack overlay and desktop menu navigation | Read native levels/tokens/availability; render six choices and forward grid movement/activation; no local progression state | native snapshot contract, original grid-navigation behavior, token spend/run-level increment/resume, direct-parent parity and GPU capture | partial: default Godot upgrade loop now playable; controller/mouse affordances and exact animation/effects remain |
| HUD/crosshair | `GameState` HUD/progression/combat state; native camera and target snapshots | `DesktopRenderer::drawHud()` | One `HudPresentationAdapter` draws the native-driven four-arm reticle, top battery strip, conditional room objective, projected target/communication markers, ticker, and critical edge pulse. No local gameplay timers or duplicate diagnostic HUD. | `hud_presentation.gd` validates authoritative values, camera FOV, and single-adapter integration; inspect `evidence/ui-restoration/` captures | partial: mapped subset; original font, full menu/death/context UI, exact animation/layout parity not established |
| Bullets, flowers, captures | corresponding `GameState` arrays | world object passes | Pool nodes from native lifecycle records | count/state/transform parity | not exposed |
| Particles and residue | particle array, `SignalResidue` | particle/data-mosh passes | Render native events/state; presentation interpolation allowed | lifecycle samples and captures | not exposed |
| Cinematics/transitions | cinematic, door transition, intro/death state | camera, overlay, door passes | Present native phases and curves | phase snapshots and video comparison | not exposed |
| Audio | native audio/music state, `MusicDirector` | `DesktopAudio` and existing assets | Play/import assets from native event/state contract | event trace and audible capture | not exposed |
| Controller feedback | gameplay/audio events and local settings | desktop rumble adapters | Platform vibration adapter only | event-to-feedback tests | not exposed |
| Persistence | progression/settings state and desktop persistence policy | desktop storage | Serialize explicit native DTO/version; no scene-owned progress | round-trip and compatibility tests | not exposed |
| Multiplayer | native command/snapshot state and protocol | `DesktopMultiplayer` | Transport/session adapter; native host remains authority | protocol/determinism plus multi-instance Godot test | not exposed |
| Debug/playtest | native fixtures and agent protocol | desktop diagnostics | Keep deterministic commands, state observations, and captures | nonzero failures and retained evidence | partial |

## Dependency order

1. Complete player/enemy presentation snapshots: hit volumes, attack windows,
   displacement/repositioning, grabs, body response, traversal, and recovery.
2. Replace target capsules with native human model/rig parts driven exclusively
   by the native pose and reaction contract; verify enemy variants independently.
3. Bind native room collision/support/traversal surfaces to matching render models
   without granting Godot physics gameplay authority.
4. Expose room transitions and complete structural/prop model families: slopes,
   houses, ruins, trees, pillars, rocks, city composition, water, and grass.
5. Add bullets, flowers, capture points, particles, residue, and combat effects.
6. Complete the phone screen, HUD, menus, cinematics, transitions, and audio.
7. Only then implement generated texture pixels and broader cosmetic polish.
8. Bind controller feedback, persistence, and multiplayer adapters.
9. Verify seeded rooms and complete run sequences against desktop captures.

## Contract rules

- Every scene object must trace to a native stable ID, state slot, or event.
- A renderer may interpolate observed values but may not advance mechanic time.
- Physics nodes may support picking or presentation, never gameplay collision.
- Seeded generation stays in C++; Godot receives generated records.
- Material names describe native semantic roles, not arbitrary Godot styling.
- A subsystem is not “ported” until direct-state tests and an appropriate visual,
  audio, or multi-instance check both pass.
