# V3 verification

## Focused results

- Native extension build: exit `0`.
- `godot_console.exe --headless --path godot --script res://tests/agent_playtest_hardening.gd`: exit `0`.
- `godot_console.exe --headless --path godot --script res://tests/room_transition.gd`: exit `0`; crossing at 14 frames, room 2, upgrade active.
- `godot_console.exe --path godot --script res://tests/door_transition_visual_capture.gd`: exit `0`; three RTX 3060 framebuffers captured.

## Evidence identity

| Capture | Bytes | SHA-256 | Review |
| --- | ---: | --- | --- |
| `00-before-door-transition.png` | 173811 | `acdea4da936ed3b85fe9748026d3b20b71c0abfb3a58c62bf863fb8b1d364ae3` | Inspected; clean baseline. |
| `01-native-transition-strength-1.png` | 142494 | `dda1cc5fd9ce992fb22955fc41d728994ba58a57184e21f1a65a4f57a39dc96c` | Inspected; active and dark at maximum strength. |
| `02-native-transition-peel.png` | 339533 | `e960710c9a8e6aa42118b9093da55aebd31cdec5e6ceddc9b4338d3dd5e207b4` | Inspected; visible 48×27 peel and motion smear. |

Gameplay equivalence and presentation behavior are separately asserted. Captures are not labeled accepted visual parity. Deterministic replay and GLB export remain unsupported.

## Full regression

- `tests/verify.ps1 -GodotExe godot_console.exe`: exit `0`.
- Summary: `PASS: configure, native build, headless behaviors, Godot startup, and parent parity`.
- Parent/Godot parity: 454 snapshots matched within tolerance `1e-5`.
- Log: `full-verify.log`; stderr log retained separately.
- `tests/unified_evidence_v1.ps1 -GodotExe godot_console.exe`: exit `0`; legacy capture-shoot ingestion, absolute-path/space handling, diagnostics, manifest verification, ZIP, and clipboard regressions pass.

## Unified package

- `unified-door-transition/`: five schema-4 observations and three active transition object records.
- Native active, Godot visible, and progress correlation assertions all pass; maximum observed progress error is `0.0`.
- `unified-door-transition.zip` passed manifest/SHA-256 verification and was placed on the Windows file-drop clipboard.
