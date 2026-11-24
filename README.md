# Digital Breakdown

A native C++ action game with a desktop renderer and a separate Godot presentation integration. The native simulation is the gameplay authority; presentation adapters consume it.

## Requirements

- Git, CMake 3.22 or newer, and a supported C/C++ compiler (Visual Studio C++ tools on Windows).
- PowerShell for the development script below.
- Network access for the initial CMake dependency fetch (GLFW and miniaudio).
- On Linux, X11 development dependencies and OpenSSL development libraries.
- FFmpeg for encoded evidence video.

## Build and run (Windows)

From the repository root:

```powershell
.\Invoke-DataGame.ps1 -Action status
.\Invoke-DataGame.ps1 -Action build -Configuration Release
$revision = (git rev-parse HEAD).Substring(0,12)
& ".\artifacts\build\$revision-Release\bin\Release\DigitalBreakdown.exe"
```

The last command assumes a multi-configuration Windows build. For other generators, locate DigitalBreakdown under the corresponding build directory.

## Verify and diagnose

```powershell
.\Invoke-DataGame.ps1 -Action test -Configuration Release
.\Invoke-DataGame.ps1 -Action smoke -Configuration Release
.\Invoke-DataGame.ps1 -Action evidence -Scenario enemy-obstruction
```

The script supports status, configure, build, test, smoke and evidence. Evidence scenarios include enemy-obstruction, lighting-comparison and soul-lifecycle. Generated evidence and builds live under artifacts/. Each invocation reports the source revision and dirty state.

For developer-console builds, configure CMake with DB_ENABLE_DEVELOPER_CONSOLE=ON and DB_STOREFRONT_RELEASE=OFF. The grave accent key opens the console. See [development workflows](TIPSANDTRICKS.md) for lighting, shadows, room fixtures and deterministic captures.

## Source layout

- [game/](game/) — authoritative gameplay simulation.
- [desktop/](desktop/) — native application and rendering.
- [godot/](godot/) — Godot integration.
- [CMakeLists.txt](CMakeLists.txt) — native build and test configuration.
- [Invoke-DataGame.ps1](Invoke-DataGame.ps1) — development entry point.
- [AGENTS.md](AGENTS.md) — development execution contract.

External diagnostics are maintained in [data-game-tools](https://github.com/indrolend/data-game-tools). Historical implementation reports belong under docs/archive/. Historical captures and successful tests do not establish the behavior of a different build.
