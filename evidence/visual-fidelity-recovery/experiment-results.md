# Experiment results

## Directional light convention

Controlled Field fixture, same camera/player fixture but startup ticks 6, 10, and 11:

| Frame | State | Lower-half Y' average |
|---|---|---:|
| `../visual-parity-gauntlet/godot-environments-ok/frame-001-tick-000006.png` | original adapter | 34.4237 |
| `experiments/directional-sign/frame-001-tick-000010.png` | direction sign only | 48.8010 |
| `after-initial-milestone/frame-001-tick-000011.png` | direction + normals + lawn | 48.8026 |

The direction-only change produced +14.3773 Y' (+41.77%) in the lower half. The final material correction did not raise global exposure. Frames were measured with FFmpeg `crop=iw:ih/2:0:ih/2,signalstats`.

The sign experiment also yielded Coastal lower-half Y' 121.157 and Sterile 91.4176. These are adjacent-environment regression observations, not native parity targets.

## Human normals

`experiments/directional-sign-combat/` is the direction-only sequence. `experiments/directional-sign-flat-normals/` adds flat triangle normals. Both are real GPU captures of the same deterministic interaction command sequence, but their capture ticks differ slightly and are therefore approximate visual comparisons. The contract test verifies normal count and direction directly against the first native triangle.

## Lawn material

The desktop draw call was recovered directly from `desktop/DesktopRenderer.cpp`: FieldGrass texture, tint `(0.72, 0.84, 0.70)`, scale `1.45`, phase `fmod(variant * 0.219, 1)`. The bridge now exposes those values and the existing adapter applies the existing native texture with triplanar projection. Geometry, ID, transform, and no-collision ownership are unchanged.
