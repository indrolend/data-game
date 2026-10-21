param(
    [string]$NativeRoot = "C:\Users\indro\Projects\data-game"
)

$ErrorActionPreference = "Stop"
$migrationRoot = (Resolve-Path "$PSScriptRoot\..").Path
$nativeRootResolved = (Resolve-Path $NativeRoot).Path
$pairs = @(
    @{ Native = "game\Game.cpp"; Embedded = "parent\game\Game.cpp" },
    @{ Native = "game\Game.hpp"; Embedded = "parent\game\Game.hpp" },
    @{ Native = "game\FieldIsland.hpp"; Embedded = "parent\game\FieldIsland.hpp" }
)

$mismatches = @()
foreach ($pair in $pairs) {
    $native = Join-Path $nativeRootResolved $pair.Native
    $embedded = Join-Path $migrationRoot $pair.Embedded
    if (-not (Test-Path -LiteralPath $native) -or -not (Test-Path -LiteralPath $embedded)) {
        $mismatches += "$($pair.Native): missing native or embedded file"
        continue
    }
    $nativeHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $native).Hash
    $embeddedHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $embedded).Hash
    if ($nativeHash -ne $embeddedHash) {
        $mismatches += "$($pair.Native): native=$nativeHash embedded=$embeddedHash"
    }
}

if ($mismatches.Count -gt 0) {
    $mismatches | ForEach-Object { Write-Error "PARENT_DRIFT $_" }
    exit 2
}

Write-Output "PASS: authoritative native files match the embedded parent copy"
exit 0
