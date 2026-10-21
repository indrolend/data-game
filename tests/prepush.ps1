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
    & git diff --cached --check -- . ':!evidence/generated/*'
    if ($LASTEXITCODE -ne 0) { throw "Staged diff check failed" }

    $generatedStaged = @(& git diff --cached --name-only -- "evidence/generated")
    if ($generatedStaged.Count -gt 0) {
        throw "Generated evidence is staged: $($generatedStaged -join ', ')"
    }

    if (-not $SkipFullVerify) {
        & "$PSScriptRoot\verify.ps1" -GodotExe $GodotExe
        if ($LASTEXITCODE -ne 0) { throw "Full verification failed" }
    }
    Write-Output "PASS: pre-push integrity checks"
} finally {
    Pop-Location
}
