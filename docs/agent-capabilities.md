# Agent capability index

Use the narrowest existing interface that observes the behavior under test.
The C++ simulation remains authoritative; Godot presents and observes it.

| Capability | Owner | Entry point | Evidence |
| --- | --- | --- | --- |
| Native fixed-step control | data-game | `desktop/main.cpp --agent-playtest --agent-frame=...` | Native schema 2 state, enemy telemetry, framebuffer |
| Presentation-aware control | data-game | `godot/scripts/agent_playtest.gd` | Godot schema 5 state digest, native/presentation transforms, PNG per observation |
| Authentic soul capture | data-game | `godot/tests/fixtures/agent_authentic_soul_capture.commands` with `--authentic-playthrough` | Exact assertion for one stored soul; debug commands are rejected |
| Authoritative state identity | data-game | `ParentGameNode.authoritative_state_digest()` | Stable digest changes with gameplay state and ignores presentation-only state |
| Deterministic reference loop | data-game | `data_engine_reference` | Renderer-free command/observation example |
| Build, test, and evidence orchestration | data-game-tools | `Invoke-DataGame.ps1` | External artifact directories and revision-linked evidence bundles |

## Regression workflow

1. Record the source revision and build the existing native/Godot target.
2. Reproduce with a checked-in command stream; add `--authentic-playthrough`
   when the claim concerns normal player controls.
3. Compare `state_digest` sequences to find the first authoritative divergence.
4. Inspect transform errors and captured frames at that tick for presentation
   divergence.
5. Change the smallest causal owner and replay the identical command stream.

Do not recover PR #152's separate snapshot and hash authority wholesale. Its
useful first-divergence behavior is available by comparing the current
authoritative digest sequence emitted by schema 5.
