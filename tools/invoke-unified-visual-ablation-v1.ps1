param(
    [Parameter(Mandatory=$true)][string]$Source,
    [ValidateSet('black-frame','frozen-frame','missing-frame','all')][string]$Mode = 'all',
    [string]$OutputRoot = 'evidence\generated\visual-ablations'
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..").Path
$sourceFull = (Resolve-Path (Join-Path $root $Source)).Path
$outputRootFull = [IO.Path]::GetFullPath((Join-Path $root $OutputRoot))
if (-not $outputRootFull.StartsWith($root,[StringComparison]::OrdinalIgnoreCase)) { throw 'Output escapes repository' }
New-Item -ItemType Directory -Force $outputRootFull | Out-Null

function Write-Manifest([string]$Package, [string]$Ablation) {
    $path = Join-Path $Package 'manifest.json'
    $manifest = Get-Content -Raw $path | ConvertFrom-Json
    $manifest | Add-Member -Force NoteProperty ablation ([ordered]@{ synthetic=$true; mode=$Ablation; source_package=$sourceFull; purpose='diagnostic rule validation only; not gameplay evidence' })
    $files = @()
    foreach($file in Get-ChildItem $Package -Recurse -File | Where-Object { $_.Name -ne 'manifest.json' }) {
        $files += [ordered]@{ path=[IO.Path]::GetRelativePath($Package,$file.FullName).Replace('\','/'); bytes=$file.Length; sha256=(Get-FileHash $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
    }
    $manifest.files = $files
    [IO.File]::WriteAllText($path,(($manifest|ConvertTo-Json -Depth 16)+"`n"),[Text.UTF8Encoding]::new($false))
}
function New-Ablation([string]$Name) {
    $destination = Join-Path $outputRootFull ("{0}-{1}" -f $Name,(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
    Copy-Item -LiteralPath $sourceFull -Destination $destination -Recurse
    $destination
}
function Run-Diagnosis([string]$Package,[string]$Name) {
    Write-Manifest $Package $Name
    & (Join-Path $root 'tools\invoke-unified-evidence-v1.ps1') -Operation diagnose -Source ([IO.Path]::GetRelativePath($root,$Package)) | Out-Host
    if($LASTEXITCODE -ne 0){ throw "Diagnosis failed for $Name" }
    $result=Get-Content -Raw (Join-Path $Package 'diagnostics.json')|ConvertFrom-Json
    [ordered]@{ mode=$Name; package=$Package; finding_count=$result.finding_count; findings=@($result.findings|ForEach-Object{$_.id}) }
}

$results=@()
if($Mode -in @('black-frame','all')) {
    $package=New-Ablation 'black-frame'
    $target=Get-ChildItem (Join-Path $package 'frames') -Filter '*tick-000600.png'|Select-Object -First 1
    if(-not $target){throw 'Frame 600 not found'}
    & ffmpeg -hide_banner -loglevel error -f lavfi -i 'color=black:s=1280x720' -frames:v 1 -y $target.FullName
    if($LASTEXITCODE -ne 0){throw 'Black-frame synthesis failed'}
    $results+=Run-Diagnosis $package 'black-frame'
}
if($Mode -in @('frozen-frame','all')) {
    $package=New-Ablation 'frozen-frame'
    $prior=Get-ChildItem (Join-Path $package 'frames') -Filter '*tick-000872.png'|Select-Object -First 1
    $target=Get-ChildItem (Join-Path $package 'frames') -Filter '*tick-000873.png'|Select-Object -First 1
    if(-not $prior -or -not $target){throw 'Frames 872/873 not found'}
    Copy-Item -LiteralPath $prior.FullName -Destination $target.FullName -Force
    $results+=Run-Diagnosis $package 'frozen-frame'
}
if($Mode -in @('missing-frame','all')) {
    $package=New-Ablation 'missing-frame'
    $target=Get-ChildItem (Join-Path $package 'frames') -Filter '*tick-000810.png'|Select-Object -First 1
    if(-not $target){throw 'Frame 810 not found'}
    Remove-Item -LiteralPath $target.FullName
    $results+=Run-Diagnosis $package 'missing-frame'
}
$report=[ordered]@{ schema_version='digital-breakdown.visual-ablation/v1'; created_utc=[DateTime]::UtcNow.ToString('o'); source=$sourceFull; results=$results }
$reportPath=Join-Path $outputRootFull ("ablation-report-{0}.json" -f (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
[IO.File]::WriteAllText($reportPath,(($report|ConvertTo-Json -Depth 10)+"`n"),[Text.UTF8Encoding]::new($false))
Write-Output "VISUAL_ABLATION PASS report=$reportPath"
$results|ForEach-Object{Write-Output ("ABLATION mode={0} findings={1} package={2}" -f $_.mode,$_.finding_count,$_.package)}
