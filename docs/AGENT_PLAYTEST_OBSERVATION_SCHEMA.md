# Agent playtest observation schema

The interactive `--agent-playtest` interface emits one `AGENT_STATE` line and
one `AGENT_ENEMY` line for every observation. Both lines share `schema` and
`tick`, so a consumer can join them to the framebuffer written for that same
observation.

Schema 2 preserves every schema 1 `AGENT_STATE` field and appends build,
scenario, and frame-file provenance. Consumers that ignore unknown key/value
pairs remain compatible. Schema 2 adds `AGENT_ENEMY`.

## Identity and validity

- `tick` is the playtest's fixed-step counter. One tick is 1/60 second.
- `frame` is the game's frame identity.
- `scenario=interactive` identifies the general command-driven playtest.
- `build_commit` is the commit embedded into the executable at configure time.
- `frame_file` is the tick-suffixed framebuffer capture produced by this
  observation. The path supplied to `--agent-frame` remains a latest-frame
  compatibility alias.
- `valid=0` means no active enemy exists; no zero-filled runtime fallback is
  presented as enemy telemetry.
- `id` is the stable target-pool index for the selected active enemy. Schema 2
  selects the active enemy nearest the player at observation time, making the
  record more likely to correspond to visible gameplay. Consumers must join by
  the explicit `id` rather than assuming one subject persists forever.

## Units and semantics

- Fields ending in `_m` are world-space metres.
- Fields ending in `_mps` are metres per second.
- Fields ending in `_s` are seconds.
- Confidence, uncertainty, contact, load, recovery, disruption, support,
  arousal, caution, and fixation are normalized `[0, 1]` runtime signals.
- `steering` is the motor's unitless world-space intent vector.
- `speed_scale`, `brace`, and `pace_expression` are bounded motor expressions,
  not direct world velocity.
- `behavior` is one of `rest`, `orient`, `investigate`, `alert`, `engage`, or
  `search`.
- `fallen`, `perception_confirmed`, and `locomotion_initialized` are booleans
  encoded as `0` or `1`.

The protocol intentionally exposes one active enemy rather than dumping every
runtime array. Multi-enemy scenarios should select subjects explicitly in a
future schema revision.
