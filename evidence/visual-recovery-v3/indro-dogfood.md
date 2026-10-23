# INDRO / CommandHUD dogfood

## Installation inspected

- Repository: `C:/Users/indro/Projects/Indro`, HEAD `cb1a01a`, dirty with unrelated clipboard-transport work preserved.
- Runtime authority: `packages/commandhud/` per its `AGENTS.md` and `docs/ARCHITECTURE.md`.
- Supported CLI inspected directly via `packages/commandhud/cli.mjs`.

## Controlled comparison

The same read-only command, `git rev-parse HEAD`, was executed directly and through `hud run`.

| Path | Wall time | Retained result |
| --- | ---: | --- |
| Direct PowerShell/Git | 54.96 ms | Terminal output only. |
| CommandHUD CLI | 5042.06 ms wall; 222 ms recorded command duration | Immutable run `20261023214617-1c6b` with project identity, branch, dirty state, command, exit code, duration, and output headline. |

CommandHUD added roughly 5 seconds of one-time CLI/evidence overhead for this tiny command. Its benefit was durable provenance and a structured repository-state snapshot. Its `search drawDoorDataMosh parent` run (`20261023214622-e1e2`) correctly retained a zero-match result, helping establish that the migration archive had state but no copied desktop renderer implementation; direct `rg` against the mature `data-game/desktop` source was still required.

The CommandHUD Remote connector was tried twice and returned service-side HTTP 429 both times, so it provided no repository intelligence during this run. The local CLI worked. The CLI state response was excessively large in this historically dirty repository because it enumerated thousands of evidence files, which reduced usability compared with `git status --short` plus targeted `rg`.

Conclusion: CommandHUD improved auditability and handoff provenance, but not search speed or implementation quality in this slice. Direct PowerShell/Git remained substantially faster for iterative inspection. No INDRO files were modified.
