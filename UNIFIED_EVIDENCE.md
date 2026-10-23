# Unified evidence V2

The unified pipeline keeps four claims separate:

1. `commands.ndjson` records what the controller submitted.
2. `states.ndjson` records authoritative native state observed through `ParentGameNode`, plus the Godot framebuffer captured at that observation.
3. `diagnostics.json` records deterministic triage rules over the state/frame pair.
4. `visual-review.json` records human inspection. A saved frame or automated diagnostic is not visual acceptance.

## Wiring

```text
PowerShell controller
  -> commands.txt
  -> agent_playtest.gd
  -> ParentGameNode / ParentBridge
  -> native Game fixed ticks
  -> AGENT_STATE authoritative snapshot
  -> main.gd presentation sync
  -> Godot viewport PNG
  -> unified states/events/objects
  -> diagnose rules
  -> automated findings
  -> optional human visual review
```

Protocol schema 4 emits command IDs, authoritative camera and projectile vectors, and matching Godot node identities/transforms. A zero transform error demonstrates correlation, not visual quality. Historical schema 3 remains ingestible; its absent fields stay explicitly unavailable.

Third-person enemy presentation also has a camera-ray obstruction correction. It changes only material alpha and can be disabled for a controlled ablation with `DB_DISABLE_CAMERA_OBSTRUCTION_FADE=1`; native simulation state is unchanged.

## Commands

Run the untouched V2 controller and package its evidence:

```powershell
.\tools\invoke-unified-evidence-v1.ps1 -Operation run -Scenario capture-shoot-v2
```

Diagnose an existing package:

```powershell
.\tools\invoke-unified-evidence-v1.ps1 -Operation diagnose -Source <package>
```

Run disposable causal ablations against a valid package:

```powershell
.\tools\invoke-unified-visual-ablation-v1.ps1 -Source <package> -Mode all
```

Supported ablations are:

- `black-frame`: replaces the frame at simulation tick 600 with black pixels. This should add both dark-frame and low-range findings.
- `frozen-frame`: replaces tick 873 with the tick-872 PNG while retaining tick-873 native state. This should add a frozen-presentation finding.
- `missing-frame`: removes the tick-810 PNG from the disposable copy. This should add a missing-frame-file finding.

Each ablated package is explicitly marked synthetic in its manifest and must not be used as gameplay evidence.

## Dogfood result

Against `capture-shoot-v2-20261023-004949-598`, the unmodified evidence had no blank, dark, overexposed, uniform, frozen, or missing frames. The causal ablations produced only their expected new signals:

| Ablation | Added diagnostic |
| --- | --- |
| Black tick 600 | `dark-obs-0012`, `uniform-obs-0012` |
| Freeze tick 873 from tick 872 | `frozen-frame-obs-0020` |
| Remove tick 810 | `missing-frame-file-obs-0017` |

The real run still has presentation risks that pixel statistics alone cannot decide. Human review rejected the placeholder-like white goal marker, unclear projectile presentation, crowded enemy silhouettes, and weak action-banner legibility. Native state independently verifies soul capture and projectile creation; it does not verify those visuals.

## V2 controlled comparison

The same protected V2 controller was run with obstruction fading disabled and enabled. Both runs reached soul capture at tick 870 and firing at tick 873 with matching authoritative state. At tick 600 the nearest enemy's presentation alpha changed from `1.0` to `0.18`, clearing an opaque foreground obstruction. At tick 873 `native-projectile-0` was observed in both subsystems at `1.165676,0.926169,-11.699404` with transform error `0`; the rendered frame visibly contains the generated projectile.

The implementation remains deliberately narrow: generated projectile geometry is identified as a `SphereMesh`, but no GLB is claimed; deterministic replay is not claimed; automated diagnostics remain triage rather than visual acceptance.
