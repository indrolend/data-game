# Digital Breakdown visual soul pass — planning handoff

## What we are trying to make

Digital Breakdown is an endless action game about an unnatural luminous phone
surviving a half-physical, half-computed post-human world. Its signature contrast
is ordinary organic or architectural reality versus synthetic soul extraction.
The tone should be pleasurable, funny, physical, uncanny, and subtly sinister.

The player character is the central visual joke: a smartphone runs, lunges,
jumps, crashes, skids, and vacuums human remnants with absurd athletic commitment.
The world should support that joke without becoming visual noise. Storytelling
should happen through behavior, material transitions, recurring spaces, color,
and consequences rather than tutorials or explanatory HUD symbols.

## Current design commitments

- Keep the existing native game, gameplay timing, controls, collision, soul
  economy, battery economy, multiplayer, and endless room progression.
- Treat the phone as both character and diegetic interface. Looking at its screen
  should carry some danger because gameplay does not pause.
- Use the established UI font consistently. Menu feedback should resemble flat,
  directional paint applied per letter, not decorative cartoon splatter.
- Natural spaces retain day/night cycles. Sterile interiors retain controlled,
  dingy artificial lighting.
- The phone and supernatural events may violate physical lighting; ordinary
  surfaces should otherwise feel grounded and coherent.
- Use a constrained semantic palette: organic matter, wounds/digital blood,
  exposed soul matter, ingestible energy, attacks, and phone emission should
  look causally related without excessive labels.
- Soul capture should be the signature continuous event: control, resistance,
  transfer, phone response, nearby illumination, completion, and recovery.
- The TV room should become a recurring run-defining special room and upgrade
  economy, not a disposable random Easter egg.
- Visual humor should be behavioral: squash, stretch, momentum, awkward enemy
  downtime, birds with simple readable behavior, face-first slides, and physical
  recovery. Avoid constant shake, explosions, or verbose jokes.
- Texture/material mutations such as wireframe, gummy, and festive presentations
  are accomplishment unlocks rather than random visual clutter from the start.

## What the latest branch contains

The branch combines the recent cohesion, UI, traversal, environment, and asset
work into the real desktop renderer and gameplay source:

- Simplified phone menus and restored top-center battery presentation.
- Consistent Source Sans 3 menu typography and directional per-letter paint
  feedback.
- Sprint-gated landing slides, face-first high-momentum presentation, and cleaner
  movement feedback without changing ordinary landings into constant spectacle.
- Phone idle emission, screen/capture response, environmental action lighting,
  and cleaner soul presentation.
- Enemy locomotion/embodiment work and multiplayer convergence checks.
- Bird behavior and public-domain sprite presentation.
- Material progression unlocks and deterministic evidence fixtures.
- City facade grammar based on real architectural modules: stories, bays,
  entrances, recesses, courses, cornices, and crowns face the playable route.
- Four CC0 authored tree families converted to the game's compact DBM1 format.
  Their silhouettes, branching, and taper are preserved while their materials
  are remapped into Digital Breakdown's palette.
- Field composition now uses asymmetric edge groups, recognizable species,
  deadwood, contact shadows, shorter grass, and photographic ground material.
- A reusable `tools/convert-glb-to-dbmesh.mjs` ingestion path for studying public
  GLB assets and producing small deterministic runtime derivatives.

Public asset provenance and the distinction between source reference material
and runtime derivatives are recorded in `docs/assets/CC0_REFERENCE_VOCABULARY.md`.

## Runtime evidence and honest diagnosis

Release builds succeed and all 51 current tests pass. Native matched captures
are stored in the evidence directories. Field is materially stronger than the
earlier empty plane: it has foreground/midground/far silhouettes and coherent
ground texture. City facades now read as buildings rather than decorated blocks.

This is progress, not completion. The largest visible weaknesses are:

1. City streets still read as empty grey runways. They need restrained
   street-scale structure, intersections, drainage, service details, and a real
   terminating landmark—not random clutter.
2. Field's distant terrain is still made from simple mound geometry. The tree
   line helps, but terrain silhouettes and value grouping remain primitive.
3. Characters and the phone can look too small or visually quiet in wide room
   captures. Encounter composition needs to protect gameplay readability.
4. Static environment shadows remain approximate. Contact is improved, but
   surfaces do not yet participate in one fully convincing shadow language.
5. Coastal and Sterile have not received the same authored structural depth as
   Field and City.
6. The game still risks becoming piecemeal whenever a new effect introduces its
   own shape, color, timing, or UI grammar. Every new feature should reuse a
   small shared visual vocabulary or justify a deliberate violation.

## Recommended planning method

Plan future work from matched native frames, not feature inventories.

For each environment or signature interaction:

1. State the intended emotion and gameplay read in one sentence.
2. Identify foreground, playable midground, landmark, and termination.
3. Identify which materials are ordinary and which are digitally corrupted.
4. Define what moves, what reacts to the player, and what changes over time.
5. Choose at most three high-leverage changes for the iteration.
6. Capture the same deterministic camera state before and after.
7. Reject additions that improve the source tree but do not improve the image.

The next planning conversation should probably decide between:

- a City street-composition pass;
- the definitive TV-room/run-upgrade loop;
- a complete soul-capture choreography pass; or
- a character-scale and encounter-staging pass across all environments.

The strongest immediate visual return is City composition. The strongest
identity/gameplay return is the TV room plus soul economy. The strongest
signature-moment return is soul capture. These should be planned separately and
then checked against the same visual and behavioral language.

## Useful opening prompt for a normal ChatGPT planning chat

> Help me methodologize the next phase of Digital Breakdown. Read this handoff
> as design context. I want a cohesive endless action game where a physically
> expressive smartphone captures post-human souls in ordinary environments that
> become digitally corrupted. Avoid feature sprawl, explanatory HUD clutter,
> generic particle spectacle, and unrelated asset-store aesthetics. Ask me
> deductive questions, then help me choose between City composition, the TV-room
> upgrade economy, soul-capture choreography, and cross-environment encounter
> staging. Convert the chosen direction into a small number of visually
> testable outcomes with matched native evidence criteria.
