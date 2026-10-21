# Capture matrix

Strictly matched scenarios: **0**. State-aligned approximate scenarios: **8 of 10**. Unsupported/unverified in the current cross-runtime interfaces: Field nighttime and a current-native/Godot room-transition frame at the same serialized state.

| Requested case | Native evidence | Godot evidence | Classification | State notes |
|---|---|---|---|---|
| Field daylight | Current native functionality established by integrated source/tests; no current-main product scenario emits this exact room | `godot-environments-ok/frame-001-tick-000006.png` | Approximate | Godot fixture tick 6, player `(0,0.08,15.5)`, third-person. No same-state native framebuffer. |
| Field nighttime | Existing repository lunar/open-sky captures only | Existing `evidence/native-lunar-*` history | Unsupported cross-runtime match | No bounded native/Godot command sets the same time-of-day and room state. |
| City | Native City capability is integrated on `main`; historical renderer evidence exists | Existing `evidence/native-city-street-side-valid/` | Approximate/historical | Godot fixture has deterministic room selection, but native current evidence scenario does not expose it. |
| Coastal/water | Native source/test contract integrated | `godot-environments-ok/frame-004-tick-000006.png` and first-person frame 006 | Approximate | Same conceptual analytical water contract; not the same serialized state. |
| Third-person phone | Native soul/lighting captures show phone composition | `godot-phone-menu-ok/frame-001-tick-000015.png` | Approximate | Godot native snapshot and camera are authoritative within migration only. |
| First-person phone | Native soul lifecycle frames | `godot-phone-menu-ok/frame-002-tick-000022.png` | Approximate | Camera mode agrees; tick/fixture do not. |
| Enemy animation/damage | `native-combat/frame-001.png` through `frame-021.png` | `godot-combat-ok/frame-014-tick-000925.png` through frame 018 | Approximate | Both are fixed 60 Hz native simulations, but scenario timelines differ. |
| Vacuum/ingestion/particles | Native `soul-lifecycle`, 20 samples, ticks 0–383 | `godot-combat-ok/` and existing `evidence/native-supplemental-effects-verified/` | Approximate | Native scenario explicitly reports soul states and particle counts; Godot driver reports player/nearest target only. |
| HUD/pause/navigation | Native renderer and menu contracts tested | `godot-phone-menu-ok/` plus current dirty menu test/evidence | Approximate | Godot menu state is native-owned; no shared screenshot tick. |
| Room transition/upgrade | Native progression tests; no paired current framebuffer | `godot-upgrade-ok/frame-001-tick-000005.png` through frame 007; `godot/tests/room_transition.gd` | Partial | Upgrade graphics captured; transition is headless/structured proof only.

Native lighting telemetry records camera `(0,5.8,14)`, look target `(0,1.2,-2)`, FOV 52°, room seed 424242/index 3/Sterile, graphics preset 1, shadows on, and samples at ticks 0/300/600. See `provenance/native-lighting-*` after the provenance files are disambiguated; the authoritative source bundle remains under `data-game-tools/artifacts/evidence/lighting-comparison/20261020-074437-18b19807333b/`.

Godot frame names contain authoritative fixed tick values. The successful graphical invocations returned zero and printed player position, battery, souls, grounded/camera/vacuum flags, nearest target, and capture path. The four successful sets are `godot-environments-ok`, `godot-combat-ok`, `godot-phone-menu-ok`, and `godot-upgrade-ok`. The similarly named directories without `-ok` are retained failed invocations: they rendered frame zero but exited 1 because the command-file path was relative.
