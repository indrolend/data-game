# GGG-PERF-007 — Static room payload suppression

## Decision: rejected and reverted

The experiment added an optional partial room snapshot. On ordinary frames it
retained live room metadata, capture points, grass/water state, and presentation
time while omitting structures, props, backdrop trees, and City façade records
already cached by the Godot adapter. A full snapshot was requested when room
seed/index/tile/setting/form identity changed.

## Result

Godot 4.7.2, OpenGL Compatibility, 1280×720, VSync disabled, RTX 3060. Three
fresh processes per retained control/candidate; 180 warmup plus 600 measured
playable frames.

| Condition | Median frame | P95 frame | Median sync | Room bridge | Room adapter |
|---|---:|---:|---:|---:|---:|
| Retained soul-gated control | 4.15 ms | 4.70 ms | 2.62 ms | 0.12 ms | 0.48 ms |
| Partial room snapshot | 4.18 ms | 4.72 ms | 2.63 ms | 0.06 ms | 0.48 ms |

The intended bridge conversion was halved, but complete playable performance
did not improve. The candidate required a second snapshot schema, room identity
tracking, transition refetch, and adapter support. That is disproportionate for
an isolated 0.06 ms reduction, so all experimental source was reverted.

Before rejection, editor parse and these contracts passed: room presentation,
room transition, Field traversal, Field gameplay loop, all-room gameplay loops,
lighting presentation, parent smoke, and performance-profile parity.
