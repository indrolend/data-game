param([string]$GodotExe = "godot_console.exe")
$ErrorActionPreference = "Stop"
$root = (Resolve-Path "$PSScriptRoot/..").Path
$evidence = Join-Path $root "evidence"
New-Item -ItemType Directory -Force $evidence | Out-Null
Push-Location $root
try {
    $oldEap = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & cmake -S . -B build-godot47 "-DGODOT_CPP_DIR=C:/Users/indro/Projects/godot-cpp" "-DGODOTCPP_DEFAULT_API_VERSION=4.7" 2>&1 | Set-Content (Join-Path $evidence "cmake-configure.log")
    $cmakeConfigureExit = $LASTEXITCODE
    $ErrorActionPreference = $oldEap
    if ($cmakeConfigureExit -ne 0) { throw "CMake configure failed with exit code $cmakeConfigureExit" }

    & cmake --build build-godot47 --config Release --parallel 2>&1 | Set-Content (Join-Path $evidence "native-build.log")
    if ($LASTEXITCODE -ne 0) { throw "Native build failed with exit code $LASTEXITCODE" }
    & (Join-Path $root "build-godot47/Release/phone_pointer_contract.exe") 2>&1 | Set-Content (Join-Path $evidence "phone-pointer-native.log")
    if ($LASTEXITCODE -ne 0) { throw "Native phone pointer contract failed with exit code $LASTEXITCODE" }
    & (Join-Path $root "build-godot47/Release/restart_menu_contract.exe") 2>&1 | Set-Content (Join-Path $evidence "restart-menu-native.log")
    if ($LASTEXITCODE -ne 0) { throw "Native restart menu contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/parent_smoke.gd 2>&1 | Set-Content (Join-Path $evidence "headless-smoke.log")
    if ($LASTEXITCODE -ne 0) { throw "Headless smoke failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/phone_presentation.gd 2>&1 | Set-Content (Join-Path $evidence "phone-presentation.log")
    if ($LASTEXITCODE -ne 0) { throw "Phone presentation contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/menu_navigation.gd 2>&1 | Set-Content (Join-Path $evidence "menu-navigation.log")
    if ($LASTEXITCODE -ne 0) { throw "Actual-scene menu navigation contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/restart_menu.gd 2>&1 | Set-Content (Join-Path $evidence "restart-menu.log")
    if ($LASTEXITCODE -ne 0) { throw "Restart menu contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/room_presentation.gd 2>&1 | Set-Content (Join-Path $evidence "room-presentation.log")
    if ($LASTEXITCODE -ne 0) { throw "Room presentation contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/field_island_traversal.gd 2>&1 | Set-Content (Join-Path $evidence "field-island-traversal.log")
    if ($LASTEXITCODE -ne 0) { throw "Field island traversal contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/gameplay_loop.gd 2>&1 | Set-Content (Join-Path $evidence "gameplay-loop.log")
    if ($LASTEXITCODE -ne 0) { throw "Native Field gameplay-loop contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/all_room_gameplay_loops.gd 2>&1 | Set-Content (Join-Path $evidence "all-room-gameplay-loops.log")
    if ($LASTEXITCODE -ne 0) { throw "Native all-room gameplay-loop contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/agent_playtest_hardening.gd 2>&1 | Set-Content (Join-Path $evidence "agent-playtest-hardening-headless.log")
    if ($LASTEXITCODE -ne 0) { throw "Agent playtest hardening contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/damage_probe.gd 2>&1 | Set-Content (Join-Path $evidence "native-damage-phone-light.log")
    if ($LASTEXITCODE -ne 0) { throw "Native enemy damage and phone-light response contract failed with exit code $LASTEXITCODE" }
    & $GodotExe --headless --path godot --script res://tests/diagnostic_utilities.gd 2>&1 | Set-Content (Join-Path $evidence "diagnostic-utilities.log")
    if ($LASTEXITCODE -ne 0) { throw "Native diagnostic utilities contract failed with exit code $LASTEXITCODE" }
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
