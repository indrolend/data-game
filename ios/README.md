# DATA for iPhone (native iOS bootstrap)

Native iPhone target (`DataIOS`, bundle `com.indrolend.data.ios`) that runs the
real `Game` simulation and the real `DesktopRenderer` scene.

## Architecture

```
touches -> ios_touch::TouchControls -> Game::setTouchControls()   (same call the desktop host makes each frame)
Game::update()  (authoritative, unchanged)
GameState -> DesktopRenderer::draw()  (desktop/DesktopRenderer.cpp, compiled with -DDB_USE_GL_SHIM)
          -> GLShim.cpp: fixed-function GL recorder (matrices, per-vertex lighting, fog, display lists, textures)
          -> glshim::Frame triangle stream -> Metal (ios/main.mm) -> drawable
```

iOS has no desktop OpenGL, so the narrowest boundary is the GL entry points the
renderer calls. `GLShim.cpp` implements the subset it uses; gameplay and scene
code are not forked. `TouchOverlay.cpp` draws the on-screen controls through the
same shim.

## Controls

| Control | Action | Behaviour |
| --- | --- | --- |
| Lower-left drag (floating stick) | Move | held; pushing to the rim sprints |
| Anywhere else drag | Look | accumulated deltas |
| VAC | Vacuum / capture | held while the finger is down |
| RUN | Sprint | held |
| JUMP, HIT, FIRE, CAM | Jump, melee, shoot, camera toggle | one press per tap |

Fingers are tracked independently, so move + look + VAC + JUMP work together.
When dead or on the start screen, tap to restart; on the upgrade menu, tap the
left/middle/right third of the screen to choose track 0/1/2. Controls respect
the safe area.

## Build and run (macOS with Xcode)

```sh
cmake -S ios -B ios/build-sim -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator \
  -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build ios/build-sim --config Debug --target DataIOS -- -sdk iphonesimulator CODE_SIGNING_ALLOWED=NO

xcrun simctl boot "iPhone 15" || true
open -a Simulator
APP=$(find ios/build-sim -name DataIOS.app -path '*iphonesimulator*' | head -1)
xcrun simctl install booted "$APP"
xcrun simctl launch booted com.indrolend.data.ios
xcrun simctl io booted screenshot ios/evidence/runtime-gameplay.png
```

`ios/build*` is git-ignored.

## Host-side checks (any OS)

`IosGlShimTest` renders the real game through the real renderer + shim and
rasterises the triangle stream on the CPU; `IosTouchControlsTest` checks held,
simultaneous and edge input against a real `Game`. Both run under `ctest`.
`DB_IOS_SHIM_PPM=out.ppm` dumps the CPU raster; `evidence/shim-cpu-raster-not-a-simulator-screenshot.png`
is such a dump (not a simulator capture).

## Known limitations

- Not yet verified on a simulator in this change (see the PR description).
- Stencil-based directional shadows are skipped (default graphics preset uses cheap blob shadows).
- Door data-mosh effect (framebuffer copy) is not rendered.
- The HUD is drawn at framebuffer edges and is not inset for the notch.
- Audio, multiplayer and persistence are not wired on iOS.
- Menus beyond restart/upgrade-pick have no touch UI.
