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
| City building-to-road and general receiver shadows missing | Main/Mac artifact is `137d08e`; scene-wide map begins at `746f27e`. Matched Windows captures at `4f5e3a5` show off/Normal/Pretty and sunset/night behavior. | Main still uses selective planar/patch techniques; the fix is stranded 15 commits ahead. | Integrate structural stack and verify same SHA on Mac. | Windows structural **VERIFIED**; main/Mac **NOT FIXED**. |
| Actor/tree/rock/building shadow participation | Structural renderer registers shared caster geometry and shades actual receivers; `4f5e3a5` removes legacy Field patches that ignored the toggle/doubled shadows. | Competing object-specific fallbacks plus missing general receiver pass. | Keep one mapped authority; retain bounded Low fallback only. | Windows **VERIFIED**; Mac **UNVERIFIED**. |
| Shadow direction/time mismatch | `746f27e` resolves one sun state and assigns `lightRig.sunDirection`; capture fixtures cover fixed sunset/night. | Earlier renderer mixed `lightRig.sunDirection` with independent celestial calls. | Integrate shared resolved sun state and add override assertion. | Structural Windows **VERIFIED**. |
| Authored City assets not visible | Windows structural log reports wall-window, wall-door, corner, crown modules loaded; PR #148 added initial bays and `746f27e` adds full-wall/corner/crown meshes. | Later assets and runtime composition are absent from main; executable mismatch masks them. | Integrate stack; require load manifest plus visual capture. | Windows structural **VERIFIED**; main/Mac **NOT FIXED/UNVERIFIED**. |
| macOS assets missing | CMake installs/copies the whole model tree and macOS resolves `Contents/Resources`; run 38506296494 packaged successfully. | No evidence yet of a packaging bug; downloaded package simply predates the work. | Build structural candidate on Mac and capture module-load output. | **UNVERIFIED**; packaging-bug hypothesis not confirmed. |
| Settings changes/restarts music | Code shows in-game settings preserve stage ownership (`started && !attractMode && !dead`). Title submenus intentionally crossfade Title → Pensive. No playback-position runtime probe exists. | Possible confusion between intentional title submenu theme change and an actual restart; tests cover selection, not cursor continuity. | Add an audio ownership/cursor probe for title and in-game settings separately. | **UNVERIFIED**; source rejects a blanket in-game restart claim. |
| Room transitions alter daylight | `state.time` advances continuously while simulation runs; no room-transition assignment was found. Fixed-time capture override is renderer-side. | No confirmed reset in current source. A stale executable or presentation discontinuity may explain reports. | Add a transition test asserting monotonic time and resolved sun continuity. | **UNVERIFIED**, no code-level reset found. |
| Abrupt camera/clipping | Presentation interpolation exists; cuts occur when mode changes or position/look deltas exceed squared distance 25. Static/cinematic evidence is insufficient for traversal collisions. | Threshold-based cuts can expose abrupt room/teleport changes; no outcome-bound test exists. | Record continuous traversal with per-frame camera delta/collision telemetry. | **UNVERIFIED**. |
| Acceleration ring undesirable/broken | The visible ring located in current code is a Coastal water wake tied to immersion/speed; no independent generic acceleration-ring implementation was confirmed. | Report may refer to the water wake or an older executable/effect. | Capture the reported frame with build SHA and identify by render marker before changing it. | **UNRESOLVED**. |
| Ambient readability inconsistent | Structural evidence covers Field, City, Coastal, Sterile and multiple times. Sterile intentionally remains sealed/dingy; open worlds use resolved celestial state. | Historic independent lighting paths and old builds; Sterile darkness still needs play validation. | Same-camera luminance/readability thresholds plus human play review. | Windows **PARTIAL**; Mac **UNVERIFIED**. |

## Confirmed causes

1. Valuable work is stranded on a linear 15-commit branch.
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
2. **Candidate integration build.** Create a review candidate from the existing 15-commit linear stack without reimplementation. Acceptance: clean build and 54/54 tests on the candidate SHA.
3. **Cross-platform shadow closure.** Produce matched Windows/macOS City captures for off, Normal, Pretty, sunset, and night. Acceptance: measurable receiver darkening, correct direction, no shadows when disabled, module-load confirmation, and no obvious acne/peter-panning.
4. **Audio ownership probe.** Track selected bank, voice identity, and PCM cursor across title settings and in-game settings. Acceptance: in-game cursor continuity; title behavior explicitly approved or changed.
5. **Daylight transition outcome test.** Cross a room boundary under a fixed seed while logging simulation time and resolved sun. Acceptance: monotonic intended clock with no unrequested reset.
6. **Camera telemetry/video.** Exercise sprint, lunge, skid, ledges, room transitions, and perspective toggle. Acceptance: only declared cuts exceed the discontinuity bound; no geometry clipping in reviewed video.
7. **Archive only after integration.** Once main contains a same-SHA cross-platform verified result, archive zero-unique and superseded branches. Do not archive divergent branches until their unique commits are classified individually.

The smallest next task that removes a real player-visible defect is task 3 after tasks 1–2: ship the already implemented shadow/authored-City stack into a same-SHA Windows and Mac candidate and validate the actual frames. Rewriting shadows again would add risk without addressing the proven failure.
