# Masterpiece vertical slice evidence

## Runtime authority

- Source: `indrolend/data-game` at `52b6c7136274990fc3628ca0d6a91ed84f4e0b8a` (`main`), with the documented local changes in this worktree.
- Configuration: Visual Studio 18 2026, Windows SDK 10.0.26100.0, `Release`.
- Executable: `C:\Users\indro\Projects\data-game-tools\artifacts\build\52b6c7136274-Release\bin\Release\DigitalBreakdown.exe`.
- Active renderer: `desktop/DesktopRenderer.cpp`, OpenGL fixed-function lighting with the `ProgressiveCandidate` atmosphere, semantic materials, directional/cheap shadow selection, and environment light rigs.
- Workflow: `C:\Users\indro\Projects\data-game-tools\Invoke-DataGame.ps1`.

Recent visual work in PRs 135, 136, 137, 138, 140, and 143 is already in `main`. The recent City, Sterile, physical-embodiment, material-response, normal-integrity, and rendering-authority branches are ancestors or partial predecessors of the current runtime; wholesale cherry-picks would regress the diagnostics workflow. The only uncommitted authority-checkout change found was a Sterile fixture radius experiment, which was preserved and not copied.

## Baseline diagnosis (ranked)

1. Soul capture had weak visual causality. The target changed shape, but there was little readable energy transfer, almost no nearby light response, and the phone did not become the clear event destination.
2. Environment hierarchy was too flat. Large surfaces read as color slabs, shadow detail disappeared, and actors competed poorly with walls and ground.
3. The four environment identities were mainly small color multipliers. Fog and ambient response did not sufficiently distinguish Field, City, Sterile, and Coastal.
4. Ground contact existed but was visually understated. Grass, targets, and props often appeared detached at gameplay distance.
5. The upper field image fell to near-black while the play surface remained murky green, reducing silhouette readability without creating useful cinematic contrast.

## Implemented pass

- Added environment-specific ambient and fog shaping while preserving the existing lighting architecture and runtime controls.
- Restored a visible, continuous sun and day/night cycle to Field, City, and Coastal while keeping Sterile on its authored interior lighting track. Grass and other organic materials now inherit the ground response instead of becoming self-lit at night.
- Converted active soul extraction into a local cyan capture light, centered on the physical target and coupled to phone-light strength.
- Reworked the existing tether presentation into a restrained three-strand synthetic transfer filament using the existing lattice, pull, latch, and ingest states.
- Added a bounded additive phone-aperture response driven by capture contact, ingest spring, and capture activity.
- Added deterministic render-only mid-scale composition without altering collision: Field boundary markers and ground bands, City curbs/windows/center seam, Sterile wall panels and guidance rails, and a Coastal animated waterline with restrained foam.
- Broke up uniform obstacle and prop materials with deterministic substrate tinting, grounded plinths, worn top caps, environment-specific face bands, varied house panels, and weathered ruin growth. These details stay within existing silhouettes and collider footprints.
- Added bounded third-person camera response during capture: a smooth target bias, slight dolly, and narrow FOV breath focus the extraction, while critical impacts receive a very small deterministic recoil. First-person, spectator, intro, death, menus, simulation state, and input remain untouched.
- Extended successful ingestion into a 1.15-second recovery beat derived directly from fresh signal residue: a local cyan light, restrained cyan/magenta geometric afterimage, and phone-screen decay now bridge extraction into the persistent residue left in the world.
- Synchronized the existing slurp-loop stop, quiet end-call release, received-message confirmation, and reward cue on the exact capture commit tick. No new audio asset or reward timing was introduced.
- Replaced the synthetic three-strand lasso with one soft additive light cone. Its apex is the authoritative phone vacuum point and its mouth is centered on the soul lattice anchor, so it remains visibly attached to both world objects without extra simulation state.
- Unified the phone display presentation around one resolved screen rectangle: the emissive glass and textured UI share the same inset, the texture has deliberate depth separation instead of near-coplanar layering, the capture aura follows that aperture, and the menu camera targets the screen center rather than the model origin.
- Replaced the selected-row rectangle with a sampled four-cell wet-paint atlas whose meaning follows the input: compact selection impact, denser confirmation, leftward swipe, and rightward swipe. The existing Source Sans 3 semibold/stencil text path is unchanged and is rendered after the paint, so the effect supports rather than replaces the established typography.
- Added sparse camera-facing bird flocks to Field and Coastal settings using Rafael Mazanek's public-domain bird-behavior sheet. The flock alternates flying, ground-foraging, and alert cycles on deterministic world-space paths; nearest-neighbor sampling keeps the photographic sprite visibly, intentionally artificial while depth testing and physical placement keep it part of the scene.
- Connected the enemy's existing locomotion and cognition signals to the actual skinned rig. Pelvis counter-rotation, alternating thigh drive, swing-knee flex, foot roll, arm counter-swing, pursuit lean, search/head scanning, disruption, impact recoil, and vacuum arm drag now layer over the authored clip and planted-foot correction. Navigation, attacks, hit windows, damage, and simulation ownership are unchanged.
- Recommitted the semantic palette as a small gameplay vocabulary: cyan marks capturable signal and phone ingestion, magenta marks hostile corruption/damage, warm amber marks physical soul matter, and environment colors remain subordinate. Enemy shells receive bounded variation from behavior and power rather than unrelated state colors; emitted residue stays visibly related to its source.
- Returned the battery readout to the top-center HUD and removed the decorative full-palette strip from the upgrade screen, reducing non-diegetic clutter while keeping the phone itself the detailed status surface.
- Converged authoritative multiplayer enemies onto the same perception, behavior, motor, and physical-body locomotion path used by single-player. Host simulation remains authoritative, guests retain prediction/interpolation, and snapshots continue carrying the established combat and animation contract without a service protocol migration.
- Preserved capture timing, hit detection, soul accounting, movement, authoritative camera state, and controls.

## Matched evidence

- `soul-before-attracted.png` / `soul-after-attracted.png`: same deterministic lifecycle phase and camera.
- `soul-before-ingest.png` / `soul-after-ingest.png`: same deterministic ingestion phase and camera.
- `lighting-before-sterile.png` / `lighting-after-sterile.png`: same lighting scenario, tick 300, progressive profile.
- `composition-before-sterile.png` / `composition-after-sterile.png`: same camera and lighting state before and after the mid-scale composition pass.
- `materials-before-sterile.png` / `materials-after-sterile.png`: same camera and lighting state before and after obstacle material/contact treatment.
- `camera-before-attracted.png` / `camera-after-attracted.png`: matched extraction phase before and after the presentation-only target bias and dolly.
- `camera-before-ingest.png` / `camera-after-ingest.png`: matched ingestion phase, confirming the phone and target remain framed through recovery.
- `completion-before.png` / `completion-after.png`: exact `08_captured` lifecycle checkpoint before and after the completion-light, phone-decay, and geometric afterimage pass.
- `completion-before.mp4` / `completion-after.mp4`: matched complete lifecycle runs for judging the temporal release rather than a still alone.
- `cohesion-menu-before.png` / `cohesion-menu-after.png`: deterministic main-menu framing before and after screen inset, depth, aperture, and camera alignment.
- `cohesion-cone-before.png` / `cohesion-cone-after.png`: matched latched-capture presentation showing the lasso replaced by a single phone-origin light volume.
- `menu-paint/graphics-select.png`, `graphics-confirm.png`, `graphics-left.png`, and `graphics-right.png`: one deterministic Graphics-menu camera state with each paint gesture fixed independently, demonstrating atlas direction, screen alignment, and unchanged lettering.
- `birds/field-billboard-flock.png`: deterministic Field inspector capture showing two animated billboard flocks in world-space depth and mirrored travel directions.
- `enemy-motion-after.mp4`: complete 1,200-tick obstruction traversal after the rig-expression pass; the scenario retains its planted-foot, collision, detour, and progress assertions.
- `enemy-motion-baseline-contact.png` / `enemy-motion-after-contact.png`: matched wide-camera temporal contact sheets used to check path stability and gross pose oscillation before and after the rig overlay.
- `enemy-motion-close-reel.mp4`: 18-second, 540-frame native close-range diagnostic reel covering acceleration, turn/search, all four attack variants, recoil, fall/recovery, and vacuum resistance through the live skinned renderer. The reel uses a fixed readable three-point diagnostic light state so joint motion can be judged without temporal lighting changes.
- `enemy-motion-close-contact.png`: representative temporal sheet for the close reel.
- `soul-before.mp4` / `soul-after.mp4`: two complete deterministic capture cycles each.
- `enemy-obstruction-after.mp4`: 1,200 ticks / 301 frames at 15 fps, verifying continuous enemy locomotion and obstruction behavior in the modified renderer.

## Validation

- `lighting-comparison`: pass, 600 ticks, renderer-observed light state, 9 frames.
- `soul-lifecycle`: pass (`20261012-225434-52b6c7136274-dirty`), 383 ticks, 20 frames, two complete cycles, monotonic shell opacity, repeating/settling phone spring, aligned cone, and visible completion recovery.
- `enemy-obstruction`: pass (`20261012-192809-52b6c7136274-dirty`), 1,200 ticks, 301 continuous frames.
- Native CTest suite: 51/51 passed.
- Multiplayer coverage: protocol, deterministic impaired-network profiles, authoritative host/remote-peer isolation, lifecycle/revive/restart, room transition, stale-world rejection, and connection-state tests passed as part of the 51-test suite.
- Multiplayer server: lockfile-clean install followed by `wrangler types --check`, TypeScript, and Vitest validation passed.
- Enemy-motion obstruction rerun: pass, 1,200 ticks / 301 frames, finite values, no collider overlap, meaningful detour/progress, and maximum pre-goal stall of 9 ticks.
- Close enemy-motion reel: pass, 540/540 native frames at 30 fps; HUD and persistence isolated; deterministic phase/signal timeline recorded.

The close reel makes the next weakness concrete: the enemy now reads as articulated and reactive, but some attack silhouettes are intentionally extreme and their transitions still snap harder than the grounded locomotion. The next highest-leverage change is a small transition/weight pass—short anticipation and recovery envelopes, plus cleaner fall-to-stand foot replanting—without changing combat timing or introducing a new animation system.
