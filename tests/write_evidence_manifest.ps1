param(
    [Parameter(Mandatory = $true)][string]$OutputPath,
    [Parameter(Mandatory = $true)][string]$CommandStream,
    [Parameter(Mandatory = $true)][ValidateSet("godot", "cpp-desktop")][string]$Renderer,
    [Parameter(Mandatory = $true)][ValidateSet("native-cpp", "godot")][string]$SimulationAuthority,
    [Parameter(Mandatory = $true)][string]$Scenario,
    [Parameter(Mandatory = $true)][string]$StateId,
    [ValidateSet("exact", "approximate", "not_applicable")][string]$StateCorrespondence = "approximate",
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
$runtimeCommand = Get-Command $GodotExe -ErrorAction SilentlyContinue
$runtimePath = if ($null -ne $runtimeCommand) { $runtimeCommand.Source } else { $null }
$runtimeHash = if ($runtimePath -and (Test-Path -LiteralPath $runtimePath)) {
    (Get-FileHash -Algorithm SHA256 -LiteralPath $runtimePath).Hash
} else { $null }
$manifest = [ordered]@{
    schema = 2
    created_utc = [DateTime]::UtcNow.ToString('o')
    repository = $root
    git_sha = (& git -C $root rev-parse HEAD).Trim()
    git_dirty = $status.Count -gt 0
    framebuffer_renderer = $Renderer
    simulation_authority = $SimulationAuthority
    scenario = $Scenario
    state_id = $StateId
    state_correspondence = $StateCorrespondence
    godot_version = "$godotVersion".Trim()
    runtime_executable = $runtimePath
    runtime_executable_sha256 = $runtimeHash
    command_stream = [IO.Path]::GetRelativePath($root, $commandPath).Replace('\', '/')
    command_stream_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $commandPath).Hash
    exit_code = $ExitCode
    selected_frames = $resolvedFrames
}
$json = (($manifest | ConvertTo-Json -Depth 5) -replace "`r`n", "`n") + "`n"
[IO.File]::WriteAllText((Join-Path (Get-Location) $OutputPath), $json,
    [Text.UTF8Encoding]::new($false))
Write-Output "WROTE: $OutputPath"
