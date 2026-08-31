[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string]$BuildDirectory,
    [string]$OutputDirectory = 'dist/windows/DigitalBreakdown',
    [string]$ArchivePath = 'dist/DigitalBreakdown-Windows.zip',
    [string]$Configuration = 'Release',
    [string]$Version = '0.1.0-rc.1',
    [string]$Commit = '',
    [switch]$SkipExecutableTests
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$buildRoot = (Resolve-Path $BuildDirectory).Path
$stageRoot = [IO.Path]::GetFullPath((Join-Path $repositoryRoot $OutputDirectory))
$archive = [IO.Path]::GetFullPath((Join-Path $repositoryRoot $ArchivePath))

if (-not $Commit) {
    $Commit = (& git -C $repositoryRoot rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or -not $Commit) { throw 'Unable to resolve the release commit.' }
}

$executableCandidates = @(
    (Join-Path $buildRoot 'bin\Release\DigitalBreakdown.exe'),
    (Join-Path $buildRoot 'bin\DigitalBreakdown.exe')
)
$executable = $executableCandidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
if (-not $executable) { throw "DigitalBreakdown.exe was not found under $buildRoot." }

$assetSources = [ordered]@{
    audio = Join-Path $repositoryRoot 'native-desktop\audio'
    fonts = Join-Path $repositoryRoot 'native-desktop\fonts'
    models = Join-Path $repositoryRoot 'native-models'
    'tv-gifs' = Join-Path $repositoryRoot 'native-tv-gifs'
}
foreach ($entry in $assetSources.GetEnumerator()) {
    if (-not (Test-Path -LiteralPath $entry.Value -PathType Container)) {
        throw "Required release asset directory is missing: $($entry.Key)"
    }
}

$licenseSources = [ordered]@{
    'SourceSans3-OFL.txt' = Join-Path $repositoryRoot 'native-desktop\fonts\LICENSE.txt'
    'stb_truetype-MIT.txt' = Join-Path $repositoryRoot 'native-desktop\third_party\stb\LICENSE.txt'
    'GLFW-license.txt' = Join-Path $buildRoot '_deps\glfw-src\LICENSE.md'
    'miniaudio-license.txt' = Join-Path $buildRoot '_deps\miniaudio-src\LICENSE'
}
foreach ($entry in $licenseSources.GetEnumerator()) {
    if (-not (Test-Path -LiteralPath $entry.Value -PathType Leaf)) {
        throw "Required third-party license is missing: $($entry.Value)"
    }
}

if (Test-Path -LiteralPath $stageRoot) { Remove-Item -LiteralPath $stageRoot -Recurse -Force }
New-Item -ItemType Directory -Path $stageRoot | Out-Null
Copy-Item -LiteralPath $executable -Destination (Join-Path $stageRoot 'DigitalBreakdown.exe')
foreach ($entry in $assetSources.GetEnumerator()) {
    Copy-Item -LiteralPath $entry.Value -Destination (Join-Path $stageRoot $entry.Key) -Recurse
}

$licenses = Join-Path $stageRoot 'licenses'
New-Item -ItemType Directory -Path $licenses | Out-Null
foreach ($entry in $licenseSources.GetEnumerator()) {
    Copy-Item -LiteralPath $entry.Value -Destination (Join-Path $licenses $entry.Key)
}
Copy-Item -LiteralPath (Join-Path $repositoryRoot 'docs\release\WINDOWS-RC1.md') -Destination (Join-Path $stageRoot 'README.md')
Copy-Item -LiteralPath (Join-Path $repositoryRoot 'docs\release\THIRD-PARTY-NOTICES.md') -Destination (Join-Path $stageRoot 'THIRD-PARTY-NOTICES.md')

$stagedExecutable = Join-Path $stageRoot 'DigitalBreakdown.exe'
if (-not $SkipExecutableTests) {
    $smoke = & $stagedExecutable --smoke-test 2>&1
    if ($LASTEXITCODE -ne 0 -or ($smoke -join "`n") -notmatch 'SMOKE_TEST_OK') {
        throw "Packaged executable smoke test failed:`n$($smoke -join "`n")"
    }
    $models = & $stagedExecutable --model-test 2>&1
    if ($LASTEXITCODE -ne 0 -or ($models -join "`n") -notmatch 'MODEL_TEST_OK') {
        throw "Packaged model test failed:`n$($models -join "`n")"
    }
}

$files = Get-ChildItem -LiteralPath $stageRoot -File -Recurse | Sort-Object FullName | ForEach-Object {
    [pscustomobject]@{
        path = [IO.Path]::GetRelativePath($stageRoot, $_.FullName).Replace('\', '/')
        bytes = $_.Length
        sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}
$buildInfo = [ordered]@{
    schemaVersion = 1
    product = 'DATA'
    executable = 'DigitalBreakdown.exe'
    version = $Version
    commit = $Commit
    shortCommit = $Commit.Substring(0, [Math]::Min(7, $Commit.Length))
    platform = 'windows-x64'
    configuration = $Configuration
    portable = $true
    builtAt = (Get-Date).ToUniversalTime().ToString('o')
    files = @($files)
}
$buildInfo | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $stageRoot 'build-info.json') -Encoding utf8

$archiveParent = Split-Path $archive
New-Item -ItemType Directory -Force -Path $archiveParent | Out-Null
if (Test-Path -LiteralPath $archive) { Remove-Item -LiteralPath $archive -Force }
Compress-Archive -Path (Join-Path $stageRoot '*') -DestinationPath $archive -CompressionLevel Optimal
$archiveHash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()

[pscustomobject]@{
    package = $archive
    sha256 = $archiveHash
    stage = $stageRoot
    fileCount = $files.Count + 1
    bytes = (Get-Item -LiteralPath $archive).Length
    executableTests = -not $SkipExecutableTests
} | ConvertTo-Json -Depth 3
