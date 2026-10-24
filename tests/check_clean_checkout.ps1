param([switch]$KeepMaterializedCheckout)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..").Path
$tempRoot = Join-Path ([IO.Path]::GetTempPath()) ("digital-breakdown-index-{0}" -f [Guid]::NewGuid().ToString('N'))
$prefix = $tempRoot.TrimEnd('\') + '\'

try {
    New-Item -ItemType Directory -Path $tempRoot | Out-Null
    & git -C $root checkout-index --all --force --prefix=$prefix
    if ($LASTEXITCODE -ne 0) { throw "Unable to materialize the Git index (exit $LASTEXITCODE)" }

    $required = @(
        'CMakeLists.txt',
        'godot/project.godot',
        'godot/scripts/main.gd',
        'godot/scripts/music_presentation_adapter.gd',
        'godot/fonts/SourceSans3-Regular.ttf',
        'godot/fonts/SourceSans3-Semibold.ttf',
        'godot/audio/game_music.mp3',
        'native/parent_game_node.cpp',
        'parent/game/Game.cpp',
        'tools/invoke-godot-agent-capture-shoot-v2.ps1',
        'tests/verify.ps1'
    )
    $missing = @($required | Where-Object { -not (Test-Path -LiteralPath (Join-Path $tempRoot $_) -PathType Leaf) })
    if ($missing.Count -gt 0) { throw "Git index is missing required runtime inputs: $($missing -join ', ')" }

    $expectedControllerHash = '14184B9059AD071FC69FDA52FD28B3D897999204ACF742E3C47ADCAC61F6CFFD'
    $controller = Join-Path $tempRoot 'tools\invoke-godot-agent-capture-shoot-v2.ps1'
    if ((Get-FileHash -LiteralPath $controller -Algorithm SHA256).Hash -ne $expectedControllerHash) {
        throw 'Materialized protected V2 controller hash mismatch'
    }

    $tracked = @(& git -C $root ls-files)
    if ($LASTEXITCODE -ne 0) { throw 'Unable to enumerate tracked files' }
    $forbidden = @($tracked | Where-Object {
        $_ -like 'build/*' -or $_ -like 'build-godot47/*' -or $_ -like 'godot/.godot/*' -or
        $_ -like 'evidence/generated/*' -or $_ -match '(?i)(\.bak|\.orig|\.rej|\.before-[^/]*)$'
    })
    if ($forbidden.Count -gt 0) { throw "Generated or disposable files are tracked: $($forbidden -join ', ')" }

    Write-Output "PASS: Git index materializes a source-complete checkout ($($tracked.Count) tracked files)"
    Write-Output "MATERIALIZED_CHECKOUT=$tempRoot"
} finally {
    if (-not $KeepMaterializedCheckout -and (Test-Path -LiteralPath $tempRoot)) {
        Remove-Item -LiteralPath $tempRoot -Recurse -Force
    }
}
