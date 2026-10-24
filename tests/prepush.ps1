param(
    [string]$GodotExe = "godot_console.exe",
    [string]$NativeRoot = "C:\Users\indro\Projects\data-game",
    [switch]$SkipFullVerify
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path "$PSScriptRoot\..").Path
Push-Location $root
try {
    & "$PSScriptRoot\check_parent_drift.ps1" -NativeRoot $NativeRoot
    if ($LASTEXITCODE -ne 0) { throw "Native/embedded-parent drift check failed" }

    & git diff --check -- . ':!evidence/*'
    if ($LASTEXITCODE -ne 0) { throw "Working-tree diff check failed" }
    $protectedController = Join-Path $root "tools\invoke-godot-agent-capture-shoot-v2.ps1"
    $expectedControllerHash = "14184B9059AD071FC69FDA52FD28B3D897999204ACF742E3C47ADCAC61F6CFFD"
    if (-not (Test-Path -LiteralPath $protectedController) -or
        (Get-FileHash -LiteralPath $protectedController -Algorithm SHA256).Hash -ne $expectedControllerHash) {
        throw "Protected V2 controller hash mismatch"
    }
    & git diff --cached --check -- . ':!evidence/generated/*' ':!tools/invoke-godot-agent-capture-shoot-v2.ps1'
    if ($LASTEXITCODE -ne 0) { throw "Staged diff check failed" }

    $generatedStaged = @(& git diff --cached --name-only -- "evidence/generated")
    if ($generatedStaged.Count -gt 0) {
        throw "Generated evidence is staged: $($generatedStaged -join ', ')"
    }

    & "$PSScriptRoot\check_visual_acceptance.ps1"
    if ($LASTEXITCODE -ne 0) { throw "Visual acceptance claim validation failed" }

    if (-not $SkipFullVerify) {
        & "$PSScriptRoot\verify.ps1" -GodotExe $GodotExe
        if ($LASTEXITCODE -ne 0) { throw "Full verification failed" }
    }
    Write-Output "PASS: pre-push integrity checks"
} finally {
    Pop-Location
}
