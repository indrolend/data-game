param(
    [Parameter(Position = 0)]
    [ValidateSet(
        'help',
        'ui',
        'status',
        'doctor',
        'repair',
        'sync',
        'desktop-build',
        'desktop-run',
        'desktop-test',
        'desktop-smoke',
        'evidence',
        'playtest',
        'room-smoke',
        'release-windows',
        'diagnostics'
    )]
    [string]$Command = 'status',
    [ValidateSet('game','rally','traversal','rooms','tv-room','tv-enter')]
    [string]$Mode = 'game',
    [ValidateSet('enemy-obstruction')]
    [string]$Scenario = 'enemy-obstruction',
    [string]$Output,
    [ValidateSet('Debug','Release')]
    [string]$Configuration = 'Release',
    [switch]$Automation,
    [switch]$Reconfigure
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$DesktopScript = Join-Path $RepoRoot 'tools\desktop\run-desktop.ps1'
$ReleaseScript = Join-Path $RepoRoot 'tools\release\get-latest-native.ps1'
$DevUiBuildScript = Join-Path $RepoRoot 'tools\dev-ui\build-dev-ui.ps1'
$EnvironmentResolver = Join-Path $RepoRoot 'tools\environment\resolve-dev-environment.ps1'
$EnvironmentRepair = Join-Path $RepoRoot 'tools\environment\repair-dev-environment.ps1'
$GameplayVerifier = Join-Path $RepoRoot 'scripts\verify-gameplay.ps1'

function Invoke-Checked {
    param(
        [Parameter(Mandatory)] [string]$FilePath,
        [string[]]$ArgumentList = @()
    )

    & $FilePath @ArgumentList
    if ($LASTEXITCODE -ne 0) {
        throw "$FilePath exited with code $LASTEXITCODE."
    }
}

function Get-GitState {
    $commit = (& git -C $RepoRoot rev-parse --short HEAD).Trim()
    $branch = (& git -C $RepoRoot branch --show-current).Trim()
    $dirtyLines = @(& git -C $RepoRoot status --porcelain)
    [pscustomobject]@{
        Branch = $branch
        Commit = $commit
        Dirty = $dirtyLines.Count -gt 0
        DirtyCount = $dirtyLines.Count
    }
}

function Get-DesktopExe {
    param([ValidateSet('Debug','Release')][string]$RequestedConfiguration = 'Release')
    $buildName = $RequestedConfiguration.ToLowerInvariant()
    $candidates = @(
        (Join-Path $RepoRoot "build\desktop-$buildName\bin\$RequestedConfiguration\DigitalBreakdown.exe"),
        (Join-Path $RepoRoot "build\desktop-$buildName\bin\DigitalBreakdown.exe")
    )
    $matches = @($candidates | Where-Object { Test-Path $_ } | Select-Object -First 1)
    if ($matches.Count -gt 0) { return $matches[0] }
    return $null
}

function Invoke-DesktopBuild {
    if (-not (Test-Path $DesktopScript)) { throw "Missing $DesktopScript" }
    if ($Reconfigure) { & $DesktopScript -Configuration $Configuration -BuildOnly -Reconfigure }
    else { & $DesktopScript -Configuration $Configuration -BuildOnly }
    if ($LASTEXITCODE -ne 0) { throw 'Desktop build failed.' }
}

function Start-DesktopGame {
    param([string[]]$GameArguments = @())
    $exe = Get-DesktopExe $Configuration
    if (-not $exe) { throw "Desktop $Configuration executable was not produced." }
    $existing = @(Get-Process -Name DigitalBreakdown -ErrorAction SilentlyContinue)
    if ($existing.Count -gt 0) { throw 'DESKTOP_ALREADY_RUNNING: Close the existing game before starting a playtest.' }
    # Windows PowerShell 5.1 can concatenate a string array passed through a
    # function boundary. These are closed-set, repo-owned flags, so construct
    # one explicit command line for Start-Process.
    if ($GameArguments.Count -gt 0) {
        Start-Process -FilePath $exe -WorkingDirectory $RepoRoot -ArgumentList ($GameArguments -join ' ')
    } else {
        Start-Process -FilePath $exe -WorkingDirectory $RepoRoot
    }
    Write-Host "PLAYTEST_READY mode=$Mode automation=$($Automation.IsPresent) executable=$exe" -ForegroundColor Green
    if ($Automation) {
        $capture = Join-Path $env:LOCALAPPDATA 'DigitalBreakdownDev\captures\automation-latest.ppm'
        Write-Host "PLAYTEST_CAPTURE path=$capture" -ForegroundColor Green
    }
}

function Show-Help {
    Write-Host 'DIGITAL BREAKDOWN DEVELOPMENT COMMANDS' -ForegroundColor Cyan
    Write-Host '  status | doctor | repair | sync'
    Write-Host '  desktop-build [-Configuration Debug|Release] [-Reconfigure]'
    Write-Host '  desktop-run   [-Configuration Debug|Release] [-Reconfigure]'
    Write-Host '  desktop-test | desktop-smoke | room-smoke'
    Write-Host '  evidence -Scenario enemy-obstruction [-Output PATH]'
    Write-Host '  playtest -Mode game|rally|traversal|rooms|tv-room|tv-enter [-Automation]'
    Write-Host '  ui | release-windows | diagnostics'
}

function Get-Environment {
    param([switch]$ProvisionCMake)
    if (-not (Test-Path $EnvironmentResolver)) { throw "Missing $EnvironmentResolver" }
    if ($ProvisionCMake) { return & $EnvironmentResolver -ProvisionCMake }
    return & $EnvironmentResolver
}

function Show-Status {
    $git = Get-GitState
    Write-Host 'DIGITAL BREAKDOWN DEV' -ForegroundColor Green
    Write-Host "Repository : $RepoRoot"
    Write-Host "Branch     : $($git.Branch)"
    Write-Host "Commit     : $($git.Commit)$(if ($git.Dirty) { '-dirty' })"
    Write-Host "Local edits: $($git.DirtyCount)"

    $desktopExe = Get-DesktopExe 'Release'
    Write-Host "Desktop    : $(if ($desktopExe) { $desktopExe } else { 'not built' })"

}

function Show-Doctor {
    $environment = Get-Environment
    $gitState = Get-GitState
    $desktopExe = Get-DesktopExe 'Release'

    Write-Host 'DIGITAL BREAKDOWN DOCTOR' -ForegroundColor Cyan
    Write-Host ("{0,-18} {1}" -f 'Repository', $(if ($environment.repository.available) { 'ready' } else { 'missing' }))
    Write-Host ("{0,-18} {1}" -f 'Git', $(if ($environment.git.available) { 'ready' } else { 'missing' }))
    Write-Host ("{0,-18} {1}" -f 'CMake', $(if ($environment.cmake.available) { "$($environment.cmake.version) [$($environment.cmake.source)]" } else { 'missing; repairable' }))
    Write-Host ("{0,-18} {1}" -f 'C++ compiler', $(if ($environment.compiler.available) { $environment.compiler.generator } else { 'missing; manual install required' }))
    Write-Host ("{0,-18} {1}" -f 'Desktop build', $(if ($desktopExe) { 'ready' } else { 'not built' }))
    Write-Host ("{0,-18} {1}" -f 'Working tree', $(if ($gitState.Dirty) { "$($gitState.DirtyCount) local changes" } else { 'clean' }))
}

switch ($Command) {
    'help' {
        Show-Help
    }
    'ui' {
        if (-not (Test-Path $DevUiBuildScript)) { throw "Missing $DevUiBuildScript" }
        & $DevUiBuildScript -Launch
        if ($LASTEXITCODE -ne 0) { throw 'Developer UI build failed.' }
    }
    'status' { Show-Status }
    'doctor' { Show-Doctor }
    'repair' {
        if (-not (Test-Path $EnvironmentRepair)) { throw "Missing $EnvironmentRepair" }
        & $EnvironmentRepair -ClearDesktopCache
        if ($LASTEXITCODE -ne 0) { throw 'Safe environment repair failed.' }
        Show-Doctor
    }
    'sync' {
        $git = Get-GitState
        if ($git.Dirty) { throw 'Local edits are present. Sync stopped without overwriting them.' }
        Invoke-Checked git @('-C', $RepoRoot, 'fetch', 'origin', 'main', '--prune')
        Invoke-Checked git @('-C', $RepoRoot, 'checkout', 'main')
        Invoke-Checked git @('-C', $RepoRoot, 'pull', '--ff-only', 'origin', 'main')
        Show-Status
    }
    'desktop-build' {
        Invoke-DesktopBuild
    }
    'desktop-run' {
        Invoke-DesktopBuild
        Start-DesktopGame
    }
    'desktop-test' {
        if (-not (Test-Path $GameplayVerifier)) { throw "Missing $GameplayVerifier" }
        & $GameplayVerifier
        if ($LASTEXITCODE -ne 0) { throw 'Desktop gameplay verification failed.' }
    }
    'desktop-smoke' {
        $exe = Get-DesktopExe $Configuration
        if (-not $exe) {
            Invoke-DesktopBuild
            $exe = Get-DesktopExe $Configuration
        }
        if (-not $exe) { throw 'Desktop executable was not produced.' }
        Invoke-Checked $exe @('--smoke-test')
    }
    'evidence' {
        Invoke-DesktopBuild
        $exe = Get-DesktopExe $Configuration
        if (-not $exe) { throw 'Desktop executable was not produced.' }
        $git = Get-GitState
        if (-not $Output) {
            $runId = '{0}-{1}{2}' -f (Get-Date -Format 'yyyyMMdd-HHmmss'), $git.Commit, $(if ($git.Dirty) { '-dirty' } else { '' })
            $Output = Join-Path $RepoRoot "artifacts\evidence\$Scenario\$runId"
        } elseif (-not [System.IO.Path]::IsPathRooted($Output)) {
            $Output = Join-Path $RepoRoot $Output
        }
        $outputPath = [System.IO.Path]::GetFullPath($Output)
        if (Test-Path -LiteralPath (Join-Path $outputPath 'manifest.json')) {
            throw "EVIDENCE_OUTPUT_EXISTS: $outputPath"
        }
        New-Item -ItemType Directory -Force -Path $outputPath | Out-Null
        $stdoutPath = Join-Path $outputPath 'stdout.log'
        $stderrPath = Join-Path $outputPath 'stderr.log'
        & $exe --evidence-scenario $Scenario --evidence-output $outputPath --capture-width 1280 --capture-height 720 1> $stdoutPath 2> $stderrPath
        $nativeExit = $LASTEXITCODE
        if (Test-Path $stdoutPath) { Get-Content $stdoutPath | Write-Host }
        if ((Test-Path $stderrPath) -and (Get-Item $stderrPath).Length -gt 0) { Get-Content $stderrPath | Write-Warning }

        $encoding = 'not_available'
        $videoPath = Join-Path $outputPath 'evidence.mp4'
        $ffmpeg = Get-Command ffmpeg -ErrorAction SilentlyContinue
        if ($ffmpeg) {
            $cleanFrames = @(Get-ChildItem (Join-Path $outputPath 'frames\clean') -Filter '*.ppm' | Sort-Object Name)
            $allFrames = @(Get-ChildItem (Join-Path $outputPath 'frames') -Recurse -Filter '*.ppm' | Sort-Object FullName)
            foreach ($frame in $allFrames) {
                $png = [System.IO.Path]::ChangeExtension($frame.FullName, '.png')
                & $ffmpeg.Source -y -loglevel error -i $frame.FullName $png
                if ($LASTEXITCODE -ne 0) { $encoding = 'failed'; break }
            }
            if ($encoding -ne 'failed' -and $cleanFrames.Count -gt 0) {
                $concatPath = Join-Path $outputPath 'video-frames.txt'
                $concatLines = foreach ($frame in $cleanFrames) {
                    "file '$($frame.FullName.Replace("'", "''"))'"
                    'duration 1.0'
                }
                $concatLines += "file '$($cleanFrames[-1].FullName.Replace("'", "''"))'"
                $concatLines | Set-Content -LiteralPath $concatPath -Encoding ascii
                & $ffmpeg.Source -y -loglevel error -f concat -safe 0 -i $concatPath -vf 'format=yuv420p' -movflags '+faststart' $videoPath
                $encoding = if ($LASTEXITCODE -eq 0) { 'pass' } else { 'failed' }
            }
        }
        $manifestPath = Join-Path $outputPath 'manifest.json'
        if (Test-Path $manifestPath) {
            $manifest = Get-Content -Raw $manifestPath | ConvertFrom-Json
            $manifest | Add-Member -NotePropertyName stdout -NotePropertyValue 'stdout.log'
            $manifest | Add-Member -NotePropertyName stderr -NotePropertyValue 'stderr.log'
            $manifest | Add-Member -NotePropertyName encoding -NotePropertyValue $encoding
            if ($encoding -eq 'pass') { $manifest | Add-Member -NotePropertyName video -NotePropertyValue 'evidence.mp4' }
            $manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $manifestPath -Encoding utf8
        }
        Write-Host "EVIDENCE_BUNDLE path=$outputPath manifest=$manifestPath video=$(if ($encoding -eq 'pass') { $videoPath } else { 'none' }) encoding=$encoding" -ForegroundColor Cyan
        if ($nativeExit -ne 0) { throw "Evidence scenario exited with code $nativeExit. Evidence was preserved at $outputPath" }
        if ($encoding -eq 'failed') { throw "Evidence encoding failed. Native evidence remains at $outputPath" }
    }
    'playtest' {
        Invoke-DesktopBuild
        [string[]]$arguments = @(switch ($Mode) {
            'rally' { @('--rally-lab') }
            'traversal' { @('--traversal-lab') }
            'rooms' { @('--room-inspector') }
            'tv-room' { @('--tv-room-test') }
            'tv-enter' { @('--tv-room-enter') }
            default { @() }
        })
        if ($Automation) { $arguments += '--automation-playtest' }
        Start-DesktopGame $arguments
    }
    'room-smoke' {
        Invoke-DesktopBuild
        $exe = Get-DesktopExe $Configuration
        Invoke-Checked $exe @('--room-inspector-smoke')
    }
    'release-windows' { & $ReleaseScript -Platform Windows -Launch }
    'diagnostics' {
        Show-Status
        Write-Host ''
        Show-Doctor
    }
}
