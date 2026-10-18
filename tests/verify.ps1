param([string]$GodotExe = "godot_console.exe")
$ErrorActionPreference = "Stop"
$root = (Resolve-Path "$PSScriptRoot/..").Path
$evidence = Join-Path $root "evidence"
New-Item -ItemType Directory -Force $evidence | Out-Null
Push-Location $root
try {
    & cmake -S . -B build-godot47 "-DGODOT_CPP_DIR=C:/Users/indro/Projects/godot-cpp" "-DGODOTCPP_DEFAULT_API_VERSION=4.7" 2>&1 | Set-Content (Join-Path $evidence "cmake-configure.log")
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed with exit code $LASTEXITCODE" }
    & cmake --build build-godot47 --config Release --parallel 2>&1 | Set-Content (Join-Path $evidence "native-build.log")
    if ($LASTEXITCODE -ne 0) { throw "Native build failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/parent_smoke.gd 2>&1 | Set-Content (Join-Path $evidence "headless-smoke.log")
    if ($LASTEXITCODE -ne 0) { throw "Headless smoke failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --quit-after 2 2>&1 | Set-Content (Join-Path $evidence "godot-startup.log")
    if ($LASTEXITCODE -ne 0) { throw "Godot startup failed with exit code $LASTEXITCODE" }
    & "$PSScriptRoot/compare_parity.ps1" -GodotExe $GodotExe 2>&1 | Set-Content (Join-Path $evidence "parity-result.log")
    if ($LASTEXITCODE -ne 0) { throw "Parity comparison failed with exit code $LASTEXITCODE" }
    Write-Output "PASS: configure, native build, headless behaviors, Godot startup, and parent parity"
} finally {
    Pop-Location
}
