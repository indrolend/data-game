# Visual humor pass — native evidence

Captured from source revision `1b3d4ad14a1047edb8b8cbae145d8fbd0f09c0bd` plus the uncommitted visual-humor changes, Release configuration, using the native `DigitalBreakdown.exe` produced by `Invoke-DataGame.ps1`.

## What changed

- Phone locomotion now uses presentation-only squash, stretch, pitch, and gait roll driven by actual running, jumping, falling, hanging, and mantling state.
- High-speed landings and air-lunge landings now enter a momentum skid whose duration scales with fall impact and approach speed. Held direction bends the skid gradually; opposite input produces a harder Wario-like carve instead of an instant velocity reversal. The phone drops into a travel-aligned superhero lean, compresses wider and lower, and sheds two short directional ground-scrape trails.
- Soul ingestion drives a deliberately excessive, asymmetrical stuffed-phone bulge while preserving collision and gameplay timing.
- Human enemies have stronger impact and locomotion deformation plus interruptible social idle poses: seated rest, hand inspection, and a rhythmic hand-game gesture.
- Damage and vacuuming produce a camera-facing, cadence-driven reaction placard. Its final utterance bridges the human-to-soul transition instead of disappearing on the kill tick.
- The previous realistic bird atlas was replaced by an eight-frame childlike V-mark sprite sheet. Existing grounded, flocking, and disturbance behavior still drives it.

The reaction text intentionally uses only short project-safe phrases (`RUDE`, `MY KNEES`, `WAIT`, `LETS GO`, `NOT THE PHONE`, `BEEP`). No personal archives or unrelated private chat history were mined.

## Evidence

- `cinematic.mp4`: continuous native lunge, fracture, soul transfer, phone ingestion, reaction placard, and bird behavior.
- `cinematic-contact.png`: matched timeline contact sheet for the cinematic capture.
- `enemy-motion.mp4`: continuous native enemy locomotion and idle-behavior reel.
- `enemy-motion-contact.png`: broad motion contact sheet.
- `enemy-action-contact.png`: selected enemy action poses.
- `momentum-skid.mp4`: continuous post-change native cinematic capture including the lunge landing and recovery window.
- `momentum-skid-contact.png`: timeline contact sheet for that capture.

The bird source image was generated specifically for this pass with OpenAI image generation: eight equal horizontal transparent cells, a single black childlike V/unibrow bird mark per cell, and no bodies, scenery, shading, or text. `models/visual-humor-v-birds.rgba` is the runtime conversion; nontransparent marks are recolored off-white for legibility in the game world.

## Verification

- Release native build: passed.
- Full native test suite: 51/51 passed after the final capture. The run also exposed and fixed an intermittent Windows stack overflow in the multiplayer protocol integration-test executable by giving that fixture-heavy test an explicit 8 MiB stack.
- Final native cinematic capture: passed (`CAPTURE_CPU_DEMO_OK`, one soul stored).

