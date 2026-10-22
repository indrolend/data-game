# Playthrough and evidence hardening

## Implemented

- Agent protocol schema increased from 2 to 3.
- `assert key=value...` validates authoritative room, deposit, objective, clear, token, upgrade, soul, grounded, camera, and vacuum state. A mismatch exits nonzero with the exact key, expected value, actual value, and native frame.
- `--authentic-playthrough` rejects soul spawning, battery refill, enemy disabling, direct capture aiming, and generated-room fixtures.
- Capture output defaults to ignored `evidence/generated/agent-playtest/`.
- A nonempty capture directory is rejected unless `--allow-existing-captures` is explicit, preventing mixed runs.
- `tests/write_evidence_manifest.ps1` records Git revision/dirty state, framebuffer renderer separately from simulation authority, scenario/state correspondence, Godot/runtime identity, command-stream hash, exit code, and selected-frame hashes. Its required provenance fields prevent a native-driven Godot frame from being mislabeled as a C++ framebuffer.
- `tests/check_parent_drift.ps1` compares authoritative `data-game/game` files with the embedded `parent/game` copies without modifying either checkout.
- `tests/prepush.ps1` combines drift, working/staged diff, generated-evidence staging, and optional full verification checks.
- The Field and all-room retained command streams now contain semantic milestone assertions.

## Verification

- Parent/native drift check: pass.
- Assertion parser contract: pass and included in `tests/verify.ps1`.
- Real RTX 3060 assertion run: exit 0 with `AGENT_ASSERT_PASS`.
- Deliberately incorrect room assertion: nonzero with `expected=99 actual=1`.
- Authentic-mode `soul_spawn`: nonzero with `authentic_mode_rejects`.
- Reusing a populated capture directory: exit 2 with `capture_directory_not_empty`.
- Manifest generation: pass with SHA-256 values for command stream and selected frame.
- Field controlled GPU loop: exit 0 with assertions for clear, transition/upgrade, and resume.
- City/Coastal/Sterile controlled GPU loop: exit 0 with assertions for each clear, transition/upgrade, and resume.
- Full `tests/verify.ps1`: exit 0.
- `tests/prepush.ps1 -SkipFullVerify`: exit 0.

Generated hardening frames and logs are intentionally retained under `evidence/generated/` and ignored by Git. No existing evidence was deleted or relocated.

## Deliberate limit

Generic stall detection was not added yet. A reliable stall signature needs native target-health/armor/ingestion progress in the consolidated playthrough snapshot; inferring a stall from player position or room economy alone would falsely fail valid stationary combat. Semantic assertions and authentic-mode enforcement are safe now, while target-aware stall detection should follow the first authentic playthrough trace.
