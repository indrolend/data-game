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
    & $GodotExe --headless --path godot --script res://tests/phone_presentation.gd 2>&1 | Set-Content (Join-Path $evidence "phone-presentation.log")
    if ($LASTEXITCODE -ne 0) { throw "Phone presentation contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/menu_navigation.gd 2>&1 | Set-Content (Join-Path $evidence "menu-navigation.log")
    if ($LASTEXITCODE -ne 0) { throw "Actual-scene menu navigation contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/room_presentation.gd 2>&1 | Set-Content (Join-Path $evidence "room-presentation.log")
    if ($LASTEXITCODE -ne 0) { throw "Room presentation contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/field_island_traversal.gd 2>&1 | Set-Content (Join-Path $evidence "field-island-traversal.log")
    if ($LASTEXITCODE -ne 0) { throw "Field island traversal contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/gameplay_loop.gd 2>&1 | Set-Content (Join-Path $evidence "gameplay-loop.log")
    if ($LASTEXITCODE -ne 0) { throw "Native Field gameplay-loop contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/all_room_gameplay_loops.gd 2>&1 | Set-Content (Join-Path $evidence "all-room-gameplay-loops.log")
    if ($LASTEXITCODE -ne 0) { throw "Native all-room gameplay-loop contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/human_model_presentation.gd 2>&1 | Set-Content (Join-Path $evidence "human-model-presentation.log")
    if ($LASTEXITCODE -ne 0) { throw "Human model presentation contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/hud_presentation.gd 2>&1 | Set-Content (Join-Path $evidence "hud-presentation.log")
    if ($LASTEXITCODE -ne 0) { throw "HUD presentation contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/lighting_presentation.gd 2>&1 | Set-Content (Join-Path $evidence "lighting-presentation.log")
    if ($LASTEXITCODE -ne 0) { throw "Lighting presentation contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/upgrade_presentation.gd 2>&1 | Set-Content (Join-Path $evidence "upgrade-presentation.log")
    if ($LASTEXITCODE -ne 0) { throw "Upgrade presentation contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/room_transition.gd 2>&1 | Set-Content (Join-Path $evidence "room-transition.log")
    if ($LASTEXITCODE -ne 0) { throw "Native room transition contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/playable_encounter.gd 2>&1 | Set-Content (Join-Path $evidence "playable-encounter.log")
    if ($LASTEXITCODE -ne 0) { throw "Godot playable encounter test failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --quit-after 2 2>&1 | Set-Content (Join-Path $evidence "godot-startup.log")
    if ($LASTEXITCODE -ne 0) { throw "Godot startup failed with exit code $LASTEXITCODE" }
    & "$PSScriptRoot/compare_parity.ps1" -GodotExe $GodotExe 2>&1 | Set-Content (Join-Path $evidence "parity-result.log")
    if ($LASTEXITCODE -ne 0) { throw "Parity comparison failed with exit code $LASTEXITCODE" }
    Write-Output "PASS: configure, native build, headless behaviors, Godot startup, and parent parity"
} finally {
    Pop-Location
}
