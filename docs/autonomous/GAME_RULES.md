# Observed gameplay rules and existing pipelines

Source-backed notes from the uploaded game snapshot, not a guarantee of line numbers on current remote main.

- `Game::setTouchControls`: movement, look, vacuum, sprint, jump, melee, shoot inputs (`game/Game.cpp` around line 1493).
- Camera yaw/pitch updated from look deltas (around 1868).
- Movement is camera-relative; sprint and vacuum alter battery demand (around 2429).
- Vacuum attracts eligible exposed souls subject to geometry and charging (around 4263).
- Soul capture stores a soul and schedules enemy respawn (around 3645).
- Shooting consumes a stored soul and 7 battery; projectile originates at phone screen (around 3565).
- Projectiles must intersect an unfilled wall capture goal in current tile coordinates (around 4530).
- Room clears when filled capture slots reach required count (around 4610).
- Room transition requires clearance **and** movement into the previous tile (around 2179).
- Movement, vacuum, melee, shooting, and idle regeneration govern battery (around 1160).

## Existing infrastructure to reuse

- Authoritative simulation: `game/Game.cpp`, `game/Game.hpp`.
- Desktop agent protocol: `desktop/AgentPlaytestProtocol.hpp` and `desktop/main.cpp`.
- Existing desktop observations: `AGENT_STATE`, `AGENT_OBJECTIVE`, `AGENT_SOUL`, `AGENT_ENEMY`.
- Original framebuffer capture: `--agent-frame` and `--capture-frame`.
- Godot presentation: `godot/scripts/agent_playtest.gd`.
- Existing state identity: `ParentGameNode.authoritative_state_digest` where available.
- External orchestration `data-game-tools`: not verified in this local snapshot.

Do not duplicate game authority or conflate developer-assisted vertical-slice progression with normal gameplay.
