param(
    [string]$NativeRoot = "C:\Users\indro\Projects\data-game"
)

$ErrorActionPreference = "Stop"
$migrationRoot = (Resolve-Path "$PSScriptRoot\..").Path
$nativeRootResolved = (Resolve-Path $NativeRoot).Path
$mismatches = @()
$nativeGame = Join-Path $nativeRootResolved "game"
$embeddedGame = Join-Path $migrationRoot "parent\game"
$nativeFiles = @{}
$embeddedFiles = @{}
foreach ($file in Get-ChildItem -LiteralPath $nativeGame -Recurse -File) {
    $relative = [IO.Path]::GetRelativePath($nativeGame, $file.FullName).Replace('\', '/')
    $nativeFiles[$relative] = $file.FullName
}
foreach ($file in Get-ChildItem -LiteralPath $embeddedGame -Recurse -File) {
    $relative = [IO.Path]::GetRelativePath($embeddedGame, $file.FullName).Replace('\', '/')
    $embeddedFiles[$relative] = $file.FullName
}
$allPaths = @($nativeFiles.Keys) + @($embeddedFiles.Keys) | Sort-Object -Unique
foreach ($relative in $allPaths) {
    if (-not $nativeFiles.ContainsKey($relative) -or -not $embeddedFiles.ContainsKey($relative)) {
        $mismatches += "game/$relative`: missing from native or embedded tree"
        continue
    }
    $native = $nativeFiles[$relative]
    $embedded = $embeddedFiles[$relative]
    $nativeHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $native).Hash
    $embeddedHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $embedded).Hash
    if ($nativeHash -ne $embeddedHash) {
        $mismatches += "game/$relative`: native=$nativeHash embedded=$embeddedHash"
    }
}

if ($mismatches.Count -gt 0) {
    $mismatches | ForEach-Object { Write-Error "PARENT_DRIFT $_" }
    exit 2
}

Write-Output "PASS: authoritative native tree matches the embedded parent copy ($($allPaths.Count) files)"
exit 0
