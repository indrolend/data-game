# INDRO dogfood during Visual Recovery V4

## Controlled observations

The first optimized CommandHUD search (`run 20261023220350-891c`) took 2,536.66 ms wall time and retained a useful failure: the requested `desktop/` scope did not exist in the migration checkout. The command itself took 58 ms. This showed that cold-cache CommandHUD remains inappropriate for routine source searches.

The complete verification suite was then run through CommandHUD as `20261023220725-1e06`:

- CommandHUD wall time: 21,475.17 ms.
- Recorded child duration: 20,223 ms.
- Direct repeated verification wall time: 19,881.26 ms.
- CommandHUD immutable evidence: 3 files, 90,339 bytes.
- Both paths returned exit 0 and the same verification headline.

The observed wrapper wall difference was 1,593.91 ms, about 8.0% of the direct run. Build caching and normal run-to-run variation prevent treating the entire difference as causal overhead. No recovery effort was needed for either successful run. The failed search was easier to diagnose later because its exact argv, stderr, exit code, repository currency, and run ID were retained.

## Judgment

After optimization, INDRO is useful for milestone operations where durable provenance is worth roughly one second of steady-state overhead. Direct PowerShell/`rg` remains materially better for trivial reads and rapid archaeology. V4 used that split deliberately rather than routing every operation through CommandHUD.
