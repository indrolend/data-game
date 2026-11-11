# Structural Polish: surface-aware shadows and city construction

## Authority and baseline

- Working branch: `codex/structural-polish-shadow-geometry`
- Starting commit: `315a2676ab45b79f249ce0c3f0566e39cb891b33`
- Preferred `DigitalBreakdown-structural-polish-cycle2.zip`: not present locally.
- The starting commit is the newest coherent development state found in the available worktrees and includes the earlier visual-soul and living-world work.
- Verified host: Windows x64, Release, OpenGL 2.1 compatibility context.

The former renderer used cheap floor patches and a Pretty-only planar stencil projection for selected actors. Buildings, trees, rocks, walls, roofs, and elevated surfaces did not share one casting/receiving capability. The planar pass could not cast onto walls, rocks, actors, stairs, or uneven surfaces.

## Rendering decision

The desktop renderer now owns one directional depth map. Relevant scene geometry is submitted through the same room/actor drawing paths for a light-space depth pass, then re-submitted as a receiver mask against the already-rendered scene depth. The fixed-function material pass remains authoritative; the shadow result is a bounded darkening overlay rather than a second renderer.

- Normal: 1024 px map, 24-world-unit half coverage, current and adjacent room tiles.
- Pretty: 2048 px map, 34-world-unit half coverage, two neighboring tiles each way.
- Low: retained inexpensive nearby contact/ground shadows.
- Disabled: no shadow depth or overlay work; scene lighting remains active.
- Compatibility fallback: if framebuffer/depth comparison entry points are unavailable, Normal/Pretty deliberately fall back to the cheap path.

The depth texture uses hardware linear depth comparison, clamped lit borders, polygon offset, and a camera-centered texel-snapped projection. Static world geometry and dynamic phone/human actors are handled by the same caster registration path. High-density facade details are visible geometry only; shadow passes use the gameplay-aligned building mass because mullions do not materially change a building-scale silhouette and tripled submission cost in the first measured implementation.

## One lighting authority

`resolveSunState` receives the already-resolved effective time once per frame. Atmosphere, directional light, shadow direction, daylight strength, and fixed-time overrides consume that result. Open-sky scenes stop generating directional maps after sunset, while their ambient and emissive identity remain readable. Sterile rooms retain their sealed-environment light direction and do not manufacture outdoor celestial shadows.

## Authored city geometry

The runtime now uses more of Poly Haven's **Modular Urban Apartments Facade** by James Ray Cock (CC0): complete wall-window and wall-door courses, an L-shaped corner termination, and crown pieces in addition to the existing windows, doors, door-windows, and cornice. Exact source nodes, source bounds, normalized bounds, and runtime vertex counts are recorded in `models/environment/structure/`.

The visible facade is hybrid:

- Existing deterministic colliders remain the structural and traversal authority.
- Complete authored wall courses establish plaster joints, opening reveals, lintels, corners, and roof termination.
- Existing separate frame/glass modules retain window and door intricacy.
- Low keeps the same building families and street composition with the procedural facade fallback.
- Normal uses authored construction on a bounded cadence and two eye-level stories.
- Pretty uses a denser cadence, up to three authored stories, structural corner modules, and crown modules.

This avoids both an asset-pack collage and arbitrary stacks of decorative boxes. The imported geometry follows the existing facade grammar, scale, palette, collision, and room generation.

## Native evidence

Matched City fixture: seed 2, room 2, player `(0, 4)`, 1280x720, HUD hidden.

- `artifacts/structural-polish/city-shadows-off.png`
- `artifacts/structural-polish/city-normal.png`
- `artifacts/structural-polish/city-pretty-final.png`
- `artifacts/structural-polish/city-sunset.png`
- `artifacts/structural-polish/city-night.png`
- `artifacts/structural-polish/field-pretty.png`
- `artifacts/structural-polish/validation-contact-sheet.png`

The Pretty frame shows a building shadow crossing the road and road markings; humans inside that footprint are darkened by the same receiver pass. The no-shadow frame removes the footprint without changing illumination. Sunset rotates/lengthens the result. Night produces no implausible daytime map and retains ambient navigation visibility and occupied-window light.

## Measured cost

The deterministic CPU-demo capture synchronously reads 640x360 frames and is not representative GPU profiling, but it provides matched relative evidence for the same seed/room/Pretty sequence:

| Build | Mean renderer samples | Approximate p95 |
| --- | ---: | ---: |
| Starting commit `315a267` | 6.6-8.1 ms | 11.5-17.2 ms |
| Initial unbounded detailed shadow submission | 25.1-27.4 ms | 29.9-35.2 ms |
| Final bounded structural shadow submission | 12.1-12.5 ms | 13.8-13.9 ms |

The optimized result removes roughly half of the initial shadow implementation's render cost and remained above 68 captured updates/second in this readback-heavy harness. Trace CSVs are under `artifacts/structural-polish/`. A trustworthy GPU-time counter is not available in the legacy harness, so no GPU-time claim is made.

## Verification

- Release executable builds and launches.
- Native framebuffer capture succeeds and reports directional shadow-map support.
- All 54 registered tests pass.
- Model validation now loads the four new architectural derivatives, checks normalized vertical bounds, minimum retained detail, and finite face normals.
- Existing gameplay, progression, multiplayer, traversal, environment, water, soul, save, and soak tests remain green.

## Remaining limitations

- This OpenGL 2.1 path uses one directional map, not cascades. Very distant shadows intentionally fall outside bounded coverage.
- Hardware comparison is linearly filtered, but the stencil receiver composition is effectively a narrow edge filter rather than a wide shader PCF kernel. Long diagonal building edges can still look firm.
- Thin facade relief receives the building-mass shadow approximation rather than being re-submitted in the receiver pass. Major walls, terrain, rocks, platforms, actors, and props participate.
- The current deterministic camera is excellent for matched composition but does not yet expose a dedicated elevated-angle capture switch.

The next highest-leverage rendering experiment is a small shader-assisted receiver path for 3x3 PCF on capable hardware, retaining the current structural depth pass and compatibility fallback. It should be attempted only after GPU timing is added to the native harness.
