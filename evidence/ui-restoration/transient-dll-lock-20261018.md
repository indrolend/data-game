# Transient verification retry — 2026-10-18

The first final `tests/verify.ps1` retry exited 1 during the native build with
MSVC linker error `LNK1104`: unable to open
`godot/bin/digital_breakdown_parent.dll`. Read-only process inspection showed
two leftover Godot processes running the just-completed
`res://tests/hud_presentation.gd` command; their output logs already reported
the HUD contract as PASS. After stopping only those two processes (the console
process and its Godot child), rerunning `tests/verify.ps1` printed:

```text
PASS: configure, native build, headless behaviors, Godot startup, and parent parity
```

The passing rerun regenerated the normal current verification logs. The prior
logs were copied before either attempt to `pre-final-verify-20261018/`.
