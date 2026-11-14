# Native agent playtest observation schema

This document describes the desktop-native protocol. The Godot-hosted protocol
is a separate presentation-aware interface emitted by
`godot/scripts/agent_playtest.gd`. Its schema 5 `AGENT_STATE` record includes
`state_digest`, the authoritative C++ gameplay digest at the observation tick,
alongside native/presentation transforms and `frame_file`.

The interactive `--agent-playtest` interface emits one `AGENT_STATE` line and
one `AGENT_OBJECTIVE`, `AGENT_SOUL`, and `AGENT_ENEMY` line for every
observation. All lines share `schema` and
`tick`, so a consumer can join them to the framebuffer written for that same
observation.

Schema 4 preserves earlier `AGENT_STATE` fields and adds explicit encounter
counts and nearest objective, exposed-soul, and active-enemy observations.
Consumers that ignore unknown key/value pairs remain compatible. Positions are
authoritative world-space values; selection is recomputed at each observation.

## Identity and validity

- `tick` is the playtest's fixed-step counter. One tick is 1/60 second.
- `frame` is the game's frame identity.
- `scenario=interactive` identifies the general command-driven playtest.
- `build_commit` is the commit embedded into the executable at configure time.
- `frame_file` is the tick-suffixed framebuffer capture produced by this
  observation. The path supplied to `--agent-frame` remains a latest-frame
  compatibility alias.
- `valid=0` means no subject of that record type exists; no zero-filled runtime
  fallback is presented as telemetry.
- `id` is the stable target-pool or objective index for the selected subject.
  Schema 4 selects the nearest subject of each record type at observation time.
  Consumers must join by the explicit `id` rather than assuming one subject
  persists forever.

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

The protocol intentionally exposes the nearest actionable subject rather than
dumping runtime arrays. The external policy must re-observe after every bounded
action because the selected enemy, soul, or objective can change.
