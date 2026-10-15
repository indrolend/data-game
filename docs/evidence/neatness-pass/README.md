# Neatness and accomplishment pass

Native Release captures from revision `6b8ae98` plus the current dirty progression/neatness changes.

- `pause-final.png`: the solo pause menu remains a centered physical phone in the live world, using the established menu font and paint-selection treatment.
- `phone-final.png`: normal gameplay keeps the battery glance bar at top center while the physical phone carries detailed run information and the quiet `MEMORY n / 3` accomplishment count.
- `wireframe-locked.png`: seed 1 / room 44 before earning a material memory; ordinary City materials remain active.
- `wireframe-unlocked.png`: the matched room after earning the wireframe memory; collision and room composition are unchanged.
- `gummy-unlocked.png`: seed 1 / room 9 after earning the gummy memory.
- `phone-idle-night-final.png`: a fixed-time night capture showing the phone screen remaining visibly emissive while idle; the same emission now drives its local world light.
- `flower-readability-final.png`: the deterministic flower fixture with coincident soul matter, showing the flower's elevated translucent shell, dark core, and foreground glyph remaining separately readable.
- `menu-select.png`, `menu-confirm.png`, `menu-left.png`, and `menu-right.png`: matched native captures of the simplified two-row audio page. Selection paint is generated per letter from the same Source Sans Semibold glyphs used by the UI; horizontal input displaces its restrained splatter in the input direction.

Material memories unlock through either sustained play or a strong single run: gummy at 12 lifetime rooms or best room 8, wireframe at 35 or 18, and festive at 75 or 30. Unlock state is persisted in save format 5; formats 1–4 continue to load with zero accomplishment progress.

The shadow pass now samples the same collider, slope, and faceted-rock support geometry used by gameplay for contact-shadow receiver height. Directional silhouettes retain the existing broad projection while a restrained contact layer prevents actors from floating on elevated or transitional surfaces. Ordinary photographed obstacles also receive deterministic warm/cool material drift instead of repeating one exact tint.

The solo pause menu is reduced to Resume / Settings / Exit Run. Audio is reduced to two self-unmuting sliders, and graphics to Quality / Frame Rate. Landing slide is now an explicit action: holding run at impact preserves momentum and enters the face-down skid, while releasing run applies stronger drag without the slide pose or scrape effect.

Validation: native Release executable built; save roundtrip passed; the 256-room progression probe passed; CTest passed 51/51. The aggregate dependency build still reports missing Visual Studio bookkeeping directories for two unused miniaudio example-node targets; the game executable and all registered game tests compile and run successfully.
