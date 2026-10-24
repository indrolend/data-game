# Visual Recovery V5 verification

## Results

- `human_model_presentation.gd`: PASS. A real native interaction produced
  airborne combat particles, capture at simulation frame 1045, and settled
  signal residue after vacuum release.
- `fundamental_loop_visual_capture.gd`: PASS. Captured and visually inspected
  the airborne digital boxes and settled surface stain at 1280x720.
- `tests/verify.ps1 -GodotExe godot_console.exe`: PASS, including native build,
  headless behavior, Godot startup, and 454 native/Godot parity snapshots.

## Assertions

- Airborne geometry is a `BoxMesh`, not the prior shared `SphereMesh`.
- Every native particle maps to either one airborne box or one settled residue.
- Every settled residue maps to exactly four lobes and three glints.
- Native material color, alpha, position, surface normal, and lifetime remain
  the source of presentation state.

## Capture hashes

- `00-airborne-digital-boxes.png` — `ACD19DD3D00658EB2965E63562813B860F5BF2D433CD6FDA3C54A5DE2E25A414`
- `01-settled-signal-residue.png` — `CD54FD588FEA38A081C00556FEF422B087DFCDF52A76D26A39A7A8C6C788CDFF`

The frames were inspected for this recovery slice. Exact pixel parity with a
matched native framebuffer is still unsupported and is not claimed.
