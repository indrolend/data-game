param([string]$Fixture = 'evidence\generated\capture-shoot-v2-20261022-205627')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..").Path
$fixturePath = [IO.Path]::GetFullPath((Join-Path $root $Fixture))
if (-not (Test-Path -LiteralPath $fixturePath)) {
    Write-Output "UNIFIED_EVIDENCE_TEST SKIP fixture_not_found=$fixturePath"
    exit 0
}
$out = "evidence\generated\unified-test-$([Guid]::NewGuid().ToString('N'))"
& "$root\tools\invoke-unified-evidence-v1.ps1" -Operation ingest -Source $Fixture -Output $out
if ($LASTEXITCODE -ne 0) { throw "ingest exited $LASTEXITCODE" }
& "$root\tools\invoke-unified-evidence-v1.ps1" -Operation verify -Source $out
if ($LASTEXITCODE -ne 0) { throw "verify exited $LASTEXITCODE" }
& "$root\tools\invoke-unified-evidence-v1.ps1" -Operation diagnose -Source $out
if ($LASTEXITCODE -ne 0) { throw "diagnose exited $LASTEXITCODE" }
$package = Join-Path $root $out
$assertions = Get-Content -Raw (Join-Path $package 'assertions.json') | ConvertFrom-Json
if (-not $assertions.objective_achieved) { throw 'V2 objective was not independently classified as achieved' }
if ($assertions.deposit_objective_achieved) { throw 'Capture-and-shoot was incorrectly classified as a successful deposit' }
$states = @(Get-Content (Join-Path $package 'states.ndjson') | ConvertFrom-Json)
if ($states[-1].native_state.projectiles -lt 1 -or $states[-1].native_state.souls -ne 0) { throw 'Final authoritative projectile state is wrong' }
$diagnostics = Get-Content -Raw (Join-Path $package 'diagnostics.json') | ConvertFrom-Json
if (@($diagnostics.frame_metrics).Count -ne @($states).Count) { throw 'Visual metrics do not cover every captured state' }
if (-not @($diagnostics.findings | Where-Object { $_.id -like 'projectile-correlation-*' })) { throw 'Missing projectile presentation-correlation diagnostic' }
Write-Output "UNIFIED_EVIDENCE_TEST PASS package=$package"
