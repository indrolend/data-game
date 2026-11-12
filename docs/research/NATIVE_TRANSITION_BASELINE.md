# Native transition baseline

## Execution identity

- Product base: `indrolend/data-game` PR #157, commit
  `0f32796a0aa209eb2196472c108cef8ad5620919` (not merged at measurement time).
- Measurement diff identity: Git object
  `4a1ec01de6d676fe1c2142fd393a867b1e7cf3ce`.
- Compiler: MSVC 19.51.36248, x86-64, Release `/O2`, static release runtime.
- Host: AMD Ryzen 9 5900X, Windows 11 Pro 10.0.26200, one benchmark
  thread pinned to logical processor 0.
- Benchmark executable SHA-256:
  `D46D68A0C8FC2017D2D08ADB94F3EFEBAF8DB439677609F7B4AD92E317414E18`.
- Workload: 240 warm-up ticks followed by 12,000 measured 60 Hz
  transitions, seed 424242, room 3. Timing uses `steady_clock`; cycle
  measurement uses fenced `RDTSCP` in ordinary user mode.

Retired-instruction, branch-miss, and cache-miss counters were unavailable
without adding a privileged counter provider, so no values are inferred for
them.

## Source to instruction to output

For one public transition, `DataEngine::step(Command const&)`:

1. Loads the four command floats from command offsets `0x00..0x0c` and six
   booleans from `0x10..0x15` into registers/stack argument slots.
2. Calls `Game::setTouchControls` on the embedded authoritative `Game`.
3. Loads fixed `dt=1/60` and calls `Game::update`.
4. Calls `DataEngine::captureEvents`.
5. Reads frame, room, soul, goal, started/dead/clear/upgrade fields at stable
   object-relative offsets, including the filled-goal loop over capture state.
6. Calls the existing `dbnet::authoritativeStateDigestHex(GameState const&)`.
7. Returns the 56-byte `Observation` containing the observable fields and
   digest.

The optimized `DataEngine::step` body spans offsets `0x000..0x117` before its
callees. The generated code directly shows the three authoritative calls and
the final digest call. `Game::update` is the physical fixed-tick implementation;
its prologue reserves `0xB18` bytes of stack, retains floating-point temporaries
in XMM registers, reads/writes `GameState` by object-relative offsets, and calls
the active subsystem updates. Absolute virtual addresses are intentionally not
used because ASLR makes them execution-specific; object-relative locations and
the executable hash identify the measured implementation.

The command is 24 bytes, `GameState` is 72,272 bytes, and `Observation` is 56
bytes in this build. This is not a RAM-free computation: instructions, the
authoritative object, stack frames, and returned state all depend on memory.
After warm-up, the raw transition caused no heap allocations and only a 4 KiB
working-set increase in the representative retained run.

## Hardware baseline

Seven repetitions were run under comparable conditions; values below are the
median, with the observed nanosecond range in parentheses.

| Policy | ns/tick | cycles/tick | heap allocations / 12,000 | working-set delta |
| --- | ---: | ---: | ---: | ---: |
| Final digest only | 4,513 (4,455–4,522) | 16,669 | 0 | 4 KiB |
| Digest every 60 ticks | 4,654 (4,579–4,735) | 17,189 | 0 | 0 |
| Selected fields every tick | 4,494 (4,429–4,514) | 16,596 | 0 | 0 |
| Numeric digest every tick | 13,173 (13,050–13,317) | 48,650 | 0 | 88 KiB |
| Digest plus retained trace every tick | 13,146 (13,079–13,315) | 48,551 | 0 | 552 KiB |
| Public `DataEngine::step` | 13,309 (13,248–13,614) | 49,152 | 12,001 | 0 |

Every policy produced final digest `1e2d223257e4e8e3`. The raw kernel therefore
achieved about 221,600 transitions/second. The complete public step achieved
about 75,100 transitions/second. The existing reference executable, including
process overhead and periodic JSON serialization, had a median 12,000-tick
duration of 276.24 ms; subtracting its 11.54 ms zero-frame startup/initialization
median gives approximately 22.06 microseconds per tick. Startup had one large
114.54 ms outlier, while its other measured minimum was 11.25 ms.

## Observation cost

Periodic digesting added about 3.1% to the median raw transition. Selected
field reads were within timing noise. Digesting every tick added about 192%.
The retained full trace cost little more CPU than the digest itself but retained
552 KiB for 12,000 records. The public step allocated once per tick because a
16-character hexadecimal digest does not fit MSVC's 15-character small-string
buffer.

The numeric-digest policy is an experiment, not a replacement authority. It
preserves the same digest value and removes per-tick string allocation, but it
does not satisfy the current public `Observation::stateDigest` string contract.

## Decision

**Investigate a specific remaining bottleneck with the cheapest next
experiment.** Retain the gameplay transition: the measured raw tick is
allocation-free and no CPU-kernel limitation was established. Next, isolate
digest traversal from hexadecimal formatting and allocation, then test whether
an additive numeric digest accessor can preserve the existing string contract
while reducing high-frequency observation cost. Do not change gameplay layout,
floating-point order, or the authoritative hash.

## Evidence

- INDRO run: `20261112004401-1527`.
- Benchmark and identity:
  `data-game-tools/artifacts/evidence/native-transition-20261112/benchmark.txt`.
- Disassembly excerpt:
  `data-game-tools/artifacts/evidence/native-transition-20261112/data-engine-step-disassembly.txt`.
- Executable identity:
  `data-game-tools/artifacts/evidence/native-transition-20261112/executable-sha256.txt`.
