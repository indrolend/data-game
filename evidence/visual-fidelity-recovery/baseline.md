# Visual fidelity recovery baseline

Date: 2026-10-20, America/Chicago.

- Native authority: `C:\Users\indro\Projects\data-game`, `main` at `18b19807333be6b1c83f0a39ae08084978285aeb`, clean and synchronized with `origin/main`.
- Mature native executable: `C:\Users\indro\Projects\data-game-tools\artifacts\build\18b19807333b-80c91579-Release\bin\Release\DigitalBreakdown.exe`, SHA-256 `56d9f603955fbf398aa5d0fb65c295a940d20552c703385f536d946d78ab410b`.
- Migration baseline: `06fc26045269dcdec8b934cd147aa0345651d8be`, branch `main` tracking `data-main/godot-migration`. The fidelity repairs are uncommitted on top of this revision.
- Embedded parent metadata revision: `dcf49a2045ecd32445a2f77eb515a21b57a61f5c`. Inspected rendering-contract files match that revision exactly; all but a one-line `RoomLighting.hpp` change also match native HEAD.
- Rebuilt GDExtension: `godot/bin/digital_breakdown_parent.dll`, SHA-256 `4e46864f768a839683ab8b3aebd46dfca9557903ba440e41a101e954b565ccc9`.
- Rebuilt retained reference: `build-godot47/Release/parent_reference.exe`, SHA-256 `49f9d6b14d3b25d0d75c4400eacc9a122eabc260603ac216f4da40c19448c7ed`.
- Godot: `4.7.2.stable.official.ed1daf0bf`; graphical evidence used OpenGL 3.3 Compatibility on NVIDIA GeForce RTX 3060, driver string 610.88.
- Existing dirty menu-navigation work and evidence were preserved. No resets, cleans, rebases, pushes, or branch switches were performed.

The retained gauntlet establishes zero cross-runtime captures with identical serialized state, tick, camera, and environment. That limitation remains; new screenshots are controlled Godot before/after experiments, not native pixel-parity claims.
