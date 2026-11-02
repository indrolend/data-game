# PC gameplay baseline — 2026-11-02

## Runtime identity

- Source: `indrolend/data-game`
- Revision under test: `1ae1f9a36c4e5a8d94647da60c949d65d2b37031`
- Godot: `4.7.2.stable.official.ed1daf0bf`
- Native extension: locally rebuilt Release `digital_breakdown_parent.dll`
- Renderer: Godot OpenGL Compatibility
- Authority: `game/Game.cpp` through `ParentGameNode`; Godot owns presentation only

Generated captures, performance logs, and Codex session files in the working
tree were not treated as source and were not committed with this baseline.

## Automated feature matrix

| Game area | Evidence | Result |
|---|---|---|
| Continuous movement, sprint, jump, look, camera | `parent_smoke.gd`, `playable_encounter.gd`, native deterministic input soak | PASS |
| Melee action and enemy damage/defeat | `parent_smoke.gd`, `playable_encounter.gd` | PASS |
| Melee arc/streak/ring presentation | `melee_arc_presentation.gd` | PASS |
| Vacuum, soul exposure, ingestion, capture | `parent_smoke.gd`, `playable_encounter.gd` | PASS |
| Stored-soul projectile and synchronized observation | `projectile_observation.gd`, native soul projectile lifecycle | PASS |
| Field room objective/economy/progression | `gameplay_loop.gd` | PASS |
| Coastal, City, Sterile objective loops | `all_room_gameplay_loops.gd` | PASS |
| Door transition and upgrade pause/resume | `room_transition.gd`, `upgrade_presentation.gd` | PASS |
| Phone menus, settings, first-person model | `phone_presentation.gd`, `menu_navigation.gd` | PASS |
| HUD and reticle synchronization | `hud_presentation.gd` | PASS |
| Death, restart, quit-to-start input routes | `restart_menu.gd` | PASS |
| Field boundary fall and recovery | `field_island_traversal.gd` | PASS |
| Enemy model/skin and vacuum presentation | `human_model_presentation.gd` | PASS |
| Lighting and day/night presentation | `lighting_presentation.gd` | PASS |
| Music and sound-effect presentation | `music_presentation.gd`, `sfx_presentation.gd` | PASS |
| Save recovery | native `DesktopSaveRecoveryTest` | PASS |

The native Windows suite passed 57/57 tests. Fourteen broad Godot gameplay and
presentation suites also passed headlessly. This establishes functional PC
coverage, not final visual acceptance: headless tests cannot certify framing,
legibility, animation quality, audio balance, or frame pacing.

## Remaining PC work, in order

1. Run a rendered end-to-end encounter and inspect the actual visual and audio
   result without developer intervention.
2. Re-run the existing representative performance profile at the current
   revision and verify median, p95, and worst frame time against 16.67 ms.
3. Address only measured presentation defects or bottlenecks, preserving native
   gameplay authority.
4. Produce a packaged Windows candidate from the accepted revision and repeat
   smoke, asset, restart, and full-loop checks outside the source checkout.

Hardware- and operating-system-specific optimization is intentionally deferred
until these platform-neutral game and presentation checks are complete.

## Current rendered and performance observations

The graphical `fundamental_loop_visual_capture.gd` sequence passed on an NVIDIA
GeForce RTX 3060 using OpenGL Compatibility at 1280×720. The captured sequence
visibly retained the gameplay phone, enemy-hit response, cyan targeting and
vacuum cues, magenta melee swing, chartreuse impact ring, shell cracking,
capture response, settled signal residue, and pause-phone menu. No missing core
gameplay cue was observed in this representative Field encounter.

Three current-revision performance trials used 180 warm-up frames and 600
sampled frames each with VSync disabled. The middle trial by median frame time
reported:

| Measurement | Result |
|---|---:|
| Median frame | 9.914 ms |
| p95 frame | 10.630 ms |
| Worst sampled frame | 15.270 ms |
| FPS from median frame | 100.87 |
| Godot scene synchronization median | 8.335 ms |
| Enemy native bridge median | 3.088 ms |
| Enemy Godot adapter median | 3.868 ms |
| GPU render median | 2.349 ms |
| Draw calls median | 271 |

All three trials remained below 16.67 ms at p95. One non-middle trial recorded
a 21.790 ms worst-frame outlier, so isolated frame spikes remain an observation
target. The enemy presentation path still consumes about 6.96 ms per median
frame, but it is not yet a release blocker on this PC and should not be changed
before visual parity review.
