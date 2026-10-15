# Regression lineage and defect closure — 2026-10-15

## Historical lineage

The named repositories are not one continuous Git graph.

- `indrolend/Data-game-` is a JavaScript-era repository with two roots and May/June-dated development content. It shares no commit SHA with current `data-game`.
- `indrolend/data-game` begins its current Git history in July and contains multiple roots joined during migration. It is the canonical native product repository.
- `indrolend/Digital-breakdown-dev` and `indrolend/digital-breakdown` also share no commit SHA with the canonical repository. They are separate histories containing copied/reworked concepts, not branches that can be merged normally.
- `indrolend/digital-breakdown-apk` redirects to `indrolend/data-game` and is therefore a confirmed former GitHub name of the canonical repository, not another surviving repository.

The practical history is: JavaScript gameplay concepts in May–June; native migration and clean APK/native workflow in July; deterministic capture, build, and distribution work in August; renderer/gameplay authority convergence in September; material, lighting, asset, atmosphere, and shadow work in October. Source was copied and rewritten across some boundaries, so file behavior and visual evidence are more trustworthy than ancestry alone.

## Defect closure matrix

| Reported issue | Reproduction/evidence | Root cause | Candidate correction | Status |
|---|---|---|---|---|
| City building-to-road and general receiver shadows missing | Historical main/Mac artifact was `137d08e`; scene-wide map began at `746f27e`. Matched Windows captures at `4f5e3a5` show off/Normal/Pretty and sunset/night behavior. | The fix was formerly stranded 15 commits ahead. | PR #149 integrated the structural stack at `6fd5a23`; verify that exact-or-descendant SHA on Mac. | Source on main **PRESENT**; Windows structural **VERIFIED**; integrated Mac runtime **UNVERIFIED**. |
| Actor/tree/rock/building shadow participation | Structural renderer registers shared caster geometry and shades actual receivers; `4f5e3a5` removes legacy Field patches that ignored the toggle/doubled shadows. | Competing object-specific fallbacks plus missing general receiver pass. | Keep one mapped authority; retain bounded Low fallback only. | Windows **VERIFIED**; Mac **UNVERIFIED**. |
| Shadow direction/time mismatch | `746f27e` resolves one sun state and assigns `lightRig.sunDirection`; capture fixtures cover fixed sunset/night. | Earlier renderer mixed `lightRig.sunDirection` with independent celestial calls. | Integrate shared resolved sun state and add override assertion. | Structural Windows **VERIFIED**. |
| Authored City assets not visible | Windows structural log reports wall-window, wall-door, corner, crown modules loaded; PR #149 integrated PR #148 and the later composition. | Executable mismatch previously masked code that was not yet on main. | Require build identity, load manifest, and visual capture from current main. | Source on main **PRESENT**; Windows structural **VERIFIED**; integrated Mac runtime **UNVERIFIED**. |
| macOS assets missing | CMake installs/copies the whole model tree and macOS resolves `Contents/Resources`; run 38506296494 packaged successfully. | No evidence yet of a packaging bug; downloaded package simply predates the work. | Build structural candidate on Mac and capture module-load output. | **UNVERIFIED**; packaging-bug hypothesis not confirmed. |
| Settings changes/restarts music | Code shows in-game settings preserve stage ownership (`started && !attractMode && !dead`). Title submenus intentionally crossfade Title → Pensive. No playback-position runtime probe exists. | Possible confusion between intentional title submenu theme change and an actual restart; tests cover selection, not cursor continuity. | Add an audio ownership/cursor probe for title and in-game settings separately. | **UNVERIFIED**; source rejects a blanket in-game restart claim. |
| Room transitions alter daylight | `state.time` advances continuously while simulation runs; no room-transition assignment was found. Fixed-time capture override is renderer-side. | No confirmed reset in current source. A stale executable or presentation discontinuity may explain reports. | Add a transition test asserting monotonic time and resolved sun continuity. | **UNVERIFIED**, no code-level reset found. |
| Abrupt camera/clipping | Presentation interpolation exists; cuts occur when mode changes or position/look deltas exceed squared distance 25. Static/cinematic evidence is insufficient for traversal collisions. | Threshold-based cuts can expose abrupt room/teleport changes; no outcome-bound test exists. | Record continuous traversal with per-frame camera delta/collision telemetry. | **UNVERIFIED**. |
| Acceleration ring undesirable/broken | The visible ring located in current code is a Coastal water wake tied to immersion/speed; no independent generic acceleration-ring implementation was confirmed. | Report may refer to the water wake or an older executable/effect. | Capture the reported frame with build SHA and identify by render marker before changing it. | **UNRESOLVED**. |
| Ambient readability inconsistent | Structural evidence covers Field, City, Coastal, Sterile and multiple times. Sterile intentionally remains sealed/dingy; open worlds use resolved celestial state. | Historic independent lighting paths and old builds; Sterile darkness still needs play validation. | Same-camera luminance/readability thresholds plus human play review. | Windows **PARTIAL**; Mac **UNVERIFIED**. |

## Confirmed causes

1. Valuable work was stranded on a linear 15-commit branch and was integrated by PR #149.
2. The running Windows executable did not match either current branch HEAD or main.
3. The distributed Mac artifact was built from main before the stranded work.
4. Object-specific legacy shadows coexisted with the new mapped pass until `4f5e3a5`.
5. Several tests prove contracts or determinism but do not prove player-visible closure.

## Probable causes

- Recycled branch/PR identity obscured which commits were actually merged.
- Parallel feature branches encouraged equivalent fixes and stale experiments.
- Artifact directory labels and manually launched executables made old builds look current.

## Rejected hypotheses

- `digital-breakdown-apk` is not a separate surviving lineage; GitHub redirects it to canonical `data-game`.
- Windows and macOS do not use wholly separate desktop renderers; they share `DesktopRenderer`. Capability and packaging differences can still produce platform-specific results.
- Current source does not globally reset daylight on room transition.
- Current in-game settings code does not deliberately replace stage music with menu music.

## Unknowns

- Whether the structural shadow FBO path succeeds on supported Macs.
- Whether authored modules load from the macOS app bundle at runtime.
- Audio cursor continuity during real menu navigation.
- Worst-case camera discontinuity during ordinary unscripted traversal.
- Exact visual referred to as the acceleration ring.

## Ordered implementation plan

1. **Provenance gate.** Make launch/capture fail loudly on SHA mismatch. Acceptance: log and artifact manifest agree on SHA, config, backend, asset root, and loaded modules.
2. **Integrated baseline build.** Build current main (`24d1f4b` or a recorded descendant) without reimplementation. Acceptance: clean build and all registered tests on the exact candidate SHA.
3. **Cross-platform shadow closure.** Produce matched Windows/macOS City captures for off, Normal, Pretty, sunset, and night. Acceptance: measurable receiver darkening, correct direction, no shadows when disabled, module-load confirmation, and no obvious acne/peter-panning.
4. **Audio ownership probe.** Track selected bank, voice identity, and PCM cursor across title settings and in-game settings. Acceptance: in-game cursor continuity; title behavior explicitly approved or changed.
5. **Daylight transition outcome test.** Cross a room boundary under a fixed seed while logging simulation time and resolved sun. Acceptance: monotonic intended clock with no unrequested reset.
6. **Camera telemetry/video.** Exercise sprint, lunge, skid, ledges, room transitions, and perspective toggle. Acceptance: only declared cuts exceed the discontinuity bound; no geometry clipping in reviewed video.
7. **Archive only after integration.** Once main contains a same-SHA cross-platform verified result, archive zero-unique and superseded branches. Do not archive divergent branches until their unique commits are classified individually.

The smallest remaining closure task is a same-SHA Windows and Mac validation of the integrated shadow/authored-City stack. Rewriting shadows again would add risk without addressing the remaining provenance and platform-evidence gap.
