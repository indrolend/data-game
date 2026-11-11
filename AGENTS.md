# Agent Instructions

- Inspect current repository, branch, working tree, and relevant runtime before changing anything.
- Preserve existing user changes and known-good behavior.
- Use the smallest existing command or test that can establish the next decision.
- Keep native C++ simulation authoritative; Godot is a presentation and integration consumer where applicable.
- Reuse established implementations and verification funnels before introducing new systems.
- Verify externally observable behavior, not merely successful builds.
- Record exact commands, results, failures, and remaining uncertainty.
- Do not commit, reset, clean, or delete unrelated work without authorization.
- Keep documentation minimal. Put verified operational knowledge in TIPSANDTRICKS.md.
