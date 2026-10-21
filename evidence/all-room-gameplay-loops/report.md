# All-room gameplay-loop verification

## Result

Field, City, Coastal, and Sterile now share one verified native-authoritative progression contract. The non-Field fixtures each completed five native projectile deposits, awarded five native permanent-memory tokens, opened the native path, crossed the doorway into room 13, displayed the original native upgrade state through Godot, accepted a native temporary upgrade, and resumed.

No room-specific soul, battery, capture, room-clear, token, doorway, or upgrade rules were added to Godot.

## Deterministic proof

`godot/tests/all_room_gameplay_loops.gd` independently instantiates City, Coastal, and Sterile. For each setting it verifies:

- the requested native room setting and a nonzero native objective count;
- firing and depositing every required soul through the original C++ input/projectile path;
- one native token per deposit;
- native room-clear state;
- doorway progression from fixture room 12 to generated room 13;
- native room-recompile pause, upgrade mutation, and resume.

The test is part of `tests/verify.ps1`. The complete suite returned exit code 0 in `evidence/full-verify-all-room-loops-20261021.log`.

## GPU proof

Command stream: `evidence/all-room-gameplay-loops.commands`

Runtime log: `evidence/all-room-gameplay-loops-gpu.log` (retained locally; the concise committed frames and command stream are the portable proof set).

Captured on Godot 4.7.2 using OpenGL 3.3 on an NVIDIA RTX 3060. The process returned exit code 0.

| Setting | Room clear / path open | Room 13 upgrade |
|---|---|---|
| City | `frame-027-tick-000410.png` | `frame-028-tick-000525.png` |
| Coastal | `frame-058-tick-000410.png` | `frame-059-tick-000525.png` |
| Sterile | `frame-089-tick-000394.png` | `frame-090-tick-000509.png` |

The log is the authoritative state evidence: each clear frame records `deposited=5 required=5 clear=1`; each transition records `room=13 upgrade=1`. Tokens accumulated 5, 10, and 15 because the ordered graphical run intentionally used one process and native permanent progression survives fixture resets.

## Observed presentation differences

- City is visually recognizable and traversable, but remains a bounded street set rather than the proposed ocean-bounded parkour city.
- Coastal visibly distinguishes sand and analytical water, but its current water is a bounded shoreline patch rather than a surrounding ocean/island contract.
- Sterile remains an enclosed, fog-heavy chamber as intended; the fifth-goal camera faces the bright doorway panel, reducing environmental readability in that particular proof frame.
- Every setting transitions to the independently generated room 13. The subsequent setting is native seed/rule output, not forced to match the room being cleared.

## Remaining work before the unassisted playthrough

The progression loop itself is no longer the blocker. The remaining gameplay proof is to earn souls through live enemy damage, cracking, defeat, vacuum, ingestion, and capture rather than the native `soul spawn` proof command. Coastal island/ocean boundaries and the larger City traversal redesign are separate environment work and should not be represented as already recovered native functionality.
