param(
    [Parameter(Mandatory = $true)][string]$OutputPath,
    [Parameter(Mandatory = $true)][string]$CommandStream,
    [int]$ExitCode = 0,
    [string[]]$SelectedFrames = @(),
    [string]$GodotExe = "godot_console.exe"
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path "$PSScriptRoot\..").Path
$commandPath = (Resolve-Path $CommandStream).Path
$outputParent = Split-Path -Parent $OutputPath
if ($outputParent) { New-Item -ItemType Directory -Force $outputParent | Out-Null }

$resolvedFrames = @()
foreach ($frame in $SelectedFrames) {
    $resolved = (Resolve-Path $frame).Path
    $resolvedFrames += [ordered]@{
        path = [IO.Path]::GetRelativePath($root, $resolved).Replace('\', '/')
        sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $resolved).Hash
    }
}

$status = @(& git -C $root status --porcelain=v1)
$godotVersion = (& $GodotExe --version 2>$null | Select-Object -First 1)
$manifest = [ordered]@{
    schema = 1
    created_utc = [DateTime]::UtcNow.ToString('o')
    repository = $root
    git_sha = (& git -C $root rev-parse HEAD).Trim()
    git_dirty = $status.Count -gt 0
    godot_version = "$godotVersion".Trim()
    command_stream = [IO.Path]::GetRelativePath($root, $commandPath).Replace('\', '/')
    command_stream_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $commandPath).Hash
    exit_code = $ExitCode
    selected_frames = $resolvedFrames
}
$manifest | ConvertTo-Json -Depth 5 | Set-Content -Encoding utf8 $OutputPath
Write-Output "WROTE: $OutputPath"
