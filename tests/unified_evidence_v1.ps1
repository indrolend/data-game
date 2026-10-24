param([string]$Fixture = 'evidence\generated\capture-shoot-v2-20261022-205627')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..").Path
$fixturePath = [IO.Path]::GetFullPath((Join-Path $root $Fixture))
if (-not (Test-Path -LiteralPath $fixturePath)) {
    Write-Output "UNIFIED_EVIDENCE_TEST SKIP fixture_not_found=$fixturePath"
    exit 0
}
$out = "evidence\generated\unified test $([Guid]::NewGuid().ToString('N'))"
$absolutePackage = [IO.Path]::GetFullPath((Join-Path $root $out))
$generatedRoot = [IO.Path]::GetFullPath((Join-Path $root 'evidence\generated'))
try {
    & "$root\tools\invoke-unified-evidence-v1.ps1" -Operation ingest -Source $Fixture -Output $out -SkipClipboard
    if ($LASTEXITCODE -ne 0) { throw "ingest exited $LASTEXITCODE" }
    & "$root\tools\invoke-unified-evidence-v1.ps1" -Operation verify -Source $out
    if ($LASTEXITCODE -ne 0) { throw "verify exited $LASTEXITCODE" }
    & "$root\tools\invoke-unified-evidence-v1.ps1" -Operation verify -Source $absolutePackage
    if ($LASTEXITCODE -ne 0) { throw "absolute verify exited $LASTEXITCODE" }
    & "$root\tools\invoke-unified-evidence-v1.ps1" -Operation diagnose -Source $out
    if ($LASTEXITCODE -ne 0) { throw "diagnose exited $LASTEXITCODE" }
    & "$root\tools\invoke-unified-evidence-v1.ps1" -Operation pack -Source $absolutePackage
    if ($LASTEXITCODE -ne 0) { throw "absolute pack exited $LASTEXITCODE" }
    $assertions = Get-Content -Raw (Join-Path $absolutePackage 'assertions.json') | ConvertFrom-Json
    if (-not $assertions.objective_achieved) { throw 'V2 objective was not independently classified as achieved' }
    if ($assertions.deposit_objective_achieved) { throw 'Capture-and-shoot was incorrectly classified as a successful deposit' }
    $states = @(Get-Content (Join-Path $absolutePackage 'states.ndjson') | ConvertFrom-Json)
    if ($states[-1].native_state.projectiles -lt 1 -or $states[-1].native_state.souls -ne 0) { throw 'Final authoritative projectile state is wrong' }
    $diagnostics = Get-Content -Raw (Join-Path $absolutePackage 'diagnostics.json') | ConvertFrom-Json
    if (@($diagnostics.frame_metrics).Count -ne @($states).Count) { throw 'Visual metrics do not cover every captured state' }
    if (-not @($diagnostics.findings | Where-Object { $_.id -like 'projectile-correlation-*' })) { throw 'Missing projectile presentation-correlation diagnostic' }
    Write-Output "UNIFIED_EVIDENCE_TEST PASS package=$absolutePackage"
} finally {
    $testRelative = [IO.Path]::GetRelativePath($generatedRoot, $absolutePackage)
    if (-not [IO.Path]::IsPathRooted($testRelative) -and $testRelative -notmatch '^\.\.(?:[\\/]|$)') {
        if (Test-Path -LiteralPath $absolutePackage) { Remove-Item -LiteralPath $absolutePackage -Recurse -Force }
        if (Test-Path -LiteralPath "$absolutePackage.zip") { Remove-Item -LiteralPath "$absolutePackage.zip" -Force }
    }
}
