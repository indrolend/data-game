# Endorphin-style enemy physics experiment

Date: 2026-11-02  
Branch: `experiment/endorphin-enemy-physics`  
Control revision: `ff2e683` (`Promote persistent enemy presentation buffers`)

## Result

The useful direction is to extend the existing native embodied controller, not
replace it with a Godot ragdoll. The current enemy is already closer to an
active character controller than its final pose initially suggests: native
code owns perception, intention, support, stepping, body attitude, impacts,
falling, recovery, combat, and the final skinned pose. This experiment adds a
deterministic one-enemy impulse fixture and closes one specific gap: a strong
angular disturbance can now overwhelm balance and cause a recoverable fall.

This is an experiment, not a production promotion. Ordinary melee-scale
impacts remain standing responses. The packed renderer and its control fallback
are unchanged.

## Research boundary

Public descriptions support the general architecture of a physics model driven
by controllers with goals such as balance and stepping. They do not disclose
NaturalMotion's proprietary Endorphin implementation. This work therefore uses
documented active-character principles rather than attempting to reproduce a
proprietary algorithm.

Relevant public references:

- [Active Game Characters: Beyond Ragdoll Puppets](https://alogicalmind.com/res/active_game_characters/paper.pdf)
  describes controller-driven physical characters, including balance and
  stepping goals.
- [Godot ragdoll system](https://docs.godotengine.org/en/latest/tutorials/physics/ragdoll_system.html)
  documents `PhysicalBoneSimulator3D`, selected-bone simulation, and influence
  blending. It does not provide active locomotion, balance, or self-preservation
  automatically.
- [Kickback](https://github.com/blugart-dev/kickback) is a Godot 4.7/Jolt active
  ragdoll reference. Its published roadmap still lists stumble stepping,
  arm/wall bracing, grabbing, and procedural poses as future self-preservation
  work, which is important when estimating integration scope.

## Existing system map

```text
EnemyPerception
  -> EnemyBehaviorState
  -> EnemyMotor (desired velocity, facing, brace)
  -> EnemyLocomotion (support region, foot phases, corrective step)
  -> PhysicalEnemyBody (traction, attitude, impact, fall, recovery)
  -> HumanModelExpressiveness / HumanModelData::skin()
  -> native packed position/normal buffers
  -> persistent Godot ArrayMesh surfaces
```

Native C++ remains the sole gameplay authority. Godot consumes snapshots and
packed final geometry; it does not decide combat outcomes or enemy motion.

| Capability | Current authority/mechanism | Final pose visibility |
|---|---|---|
| Perception, memory, target selection | Native behavioral state | Head/facing/behavior response |
| Locomotion intent | Native motor | Root velocity, facing, gait amount |
| Support and stepping | Native support region and foot state machine | Foot targets/contact and gait pose |
| Balance | Reduced native pitch/roll spring-damper | Root/torso attitude |
| Impact response | Native angular impulse and instability | Tilt, disruption, hit expression |
| Fall/recovery | Deterministic reduced body state | Fallen attitude and recovery blend |
| Attacks | Native timing and hit authority | Procedural attack pose |
| Shell crack/soul collapse | Native lifecycle and deformation | Preserved by final CPU-skinned mesh |
| Joint torque, mass, constraints | Not modeled | Not available |
| Arm bracing/contact | Not modeled | Not available |

The model is physically motivated but reduced. Root translation is kinematic,
body attitude has two controlled angular degrees of freedom, and limbs are
procedurally posed rather than forward-dynamics rigid bodies.

## Candidate A: native articulated-control extension

### Implemented experiment

- Added an experiment-only fixture containing one normal enemy and the normal
  player/combat loop.
- Added a world-space impulse hook with impact height. It converts the impulse
  to the enemy's local frame, applies an angular lever, and transfers linear
  momentum to the existing root body.
- Added an overload condition to `updatePhysicalEnemyBody`: a near-saturated
  impact combined with angular speed above 2.8 rad/s causes a fall. Existing
  tilt-based fall thresholds remain intact.
- Added deterministic native coverage for identical strong impulses, fall, and
  recovery.
- Added an interactive and automated Godot harness. Controls are `1` for a
  small absorbed impact, `2` for a corrective-step disturbance, `3` for a fall,
  `R` to reset, and `Esc` to quit.

The overload condition was necessary because the control controller damped a
saturated angular impulse at roughly 0.30 rad of maximum tilt, while fall was
only possible beyond 0.76-0.82 rad. In practice the impact-driven fall path was
unreachable even though an explicit fallen state and recovery already existed.

### Automated evidence

Command:

```powershell
godot_console.exe --path godot --resolution 1280x720 `
  --rendering-method gl_compatibility --disable-vsync `
  --script res://experiments/endorphin_enemy_experiment.gd -- --automated
```

Observed result:

```text
RESULT: PASS standing, impact absorption, corrective step, fall, recovery, tracking, combat
```

The run writes `evidence/generated/endorphin-enemy/summary.json` and five PNGs:
standing, small impact, corrective step, fallen, and recovered. The summary
records `corrective_step=true`, `fallen=true`, `attack_seen=true`, and no
failures. The same initial state and impulse produce bit-identical tested body
pitch and roll values in the native deterministic test.

### Performance

`PhysicalEnemyBodyTest` simulates 32 bodies for 3,600 frames. Five fresh runs
were compared with the exact `ff2e683` control in a detached worktree:

| Version | Median | Range |
|---|---:|---:|
| `ff2e683` control | 0.16586 us/enemy-frame | 0.16522-0.16595 |
| Candidate A | 0.16681 us/enemy-frame | 0.16644-0.16931 |

The median difference is 0.00095 us/enemy-frame (about 0.57%), below a
meaningful playable cost and small enough to be benchmark noise. The experiment
does not change packed mesh generation, bridge conversion, surface upload, draw
calls, or GPU work. It therefore inherits the verified `ff2e683` presentation
performance rather than claiming a new whole-scene rendering improvement.

## Candidate B: Godot active ragdoll

Candidate B was bounded at design/feasibility level and was not installed into
the normal scene. A useful prototype would require:

1. A persistent `Skeleton3D` presentation path rather than final packed
   vertices alone.
2. Authored collision shapes, masses, joints, limits, and drive gains for a
   reduced bone set.
3. A controller for pose targets, center-of-mass balance, support estimation,
   stepping, and recovery; `PhysicalBone3D` does not supply these behaviors.
4. A single explicit authority boundary for root motion, contacts, combat hit
   volumes, and deterministic replay.
5. A mapping back to shell cracking, foot correction, damage deformation, soul
   collapse, and the optimized render path.

Starting Candidate B as a replacement would introduce a second simulation and
would discard verified visible functions before demonstrating balance. It also
has a determinism and synchronization risk: Godot/Jolt contact resolution would
need to feed or replace native state without allowing two bodies to move the
same enemy.

Candidate B remains useful later as an isolated upper-body/contact laboratory:
one reduced skeleton, no combat authority, scripted impulses, and measurements
of joint control, wall bracing, and arm protection. It should only advance if
it demonstrates a behavior the native controller cannot express economically.

## Validation

- Godot 4.7.2 editor/headless parse: pass.
- Automated rendered experiment on RTX 3060/OpenGL Compatibility: pass.
- Native suite: 57/57 pass.
- Godot suites: `parent_smoke`, `parent_parity`, `human_model_presentation`,
  `playable_encounter`, `packed_enemy_buffer`, and
  `packed_enemy_all_targets` pass.
- Visual review: packed enemy topology/materials remain intact; standing,
  disturbance, collapse, and recovery frames were captured. The renderer
  fallback was not removed.

## Limitations and recommendation

The prototype demonstrates an active character intent but not a full active
ragdoll. It has no per-limb momentum, collision-driven joint motion, contact
location beyond a scalar height, arm bracing, wall support, or physically
planned get-up. Falling and recovery are controlled state transitions, not a
multi-body solve. The strongest impulse is intentionally a laboratory input and
needs gameplay calibration before any production promotion.

Recommendation: **combine and extend Candidate A**. Preserve native perception,
combat, deterministic support/stepping, expressive deformation, and packed
rendering. The next smallest high-value experiment is a reduced articulated
upper body (pelvis/torso/head plus two arm chains) inside native authority, with
contact-location torque and protective arm targets. Keep Candidate B as an A/B
research fixture, not the normal gameplay path. Do not replace the existing
enemy until a reduced articulated prototype proves superior visible behavior,
determinism, and whole-scene cost.
