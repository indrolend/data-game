param(
    [string]$GodotExe = "godot_console.exe",
    [string]$Channel = "evidence\generated\agent-live\commands.txt",
    [string]$CaptureDirectory = "evidence\generated\agent-live\frames",
    [switch]$Authentic
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path "$PSScriptRoot\..").Path
$channelPath = [IO.Path]::GetFullPath((Join-Path $root $Channel))
$capturePath = [IO.Path]::GetFullPath((Join-Path $root $CaptureDirectory))
if (Test-Path -LiteralPath $channelPath) {
    throw "Command channel already exists; choose a fresh -Channel: $channelPath"
}
if (Test-Path -LiteralPath $capturePath) {
    throw "Capture directory already exists; choose a fresh -CaptureDirectory: $capturePath"
}
New-Item -ItemType Directory -Force (Split-Path -Parent $channelPath) | Out-Null
New-Item -ItemType File -Path $channelPath | Out-Null
$arguments = @(
    "--path", (Join-Path $root "godot"),
    "--script", "res://scripts/agent_playtest.gd", "--",
    "--commands=$channelPath", "--capture-dir=$capturePath", "--follow-commands"
)
if ($Authentic) { $arguments += "--authentic-playthrough" }
Write-Output "AGENT_CHANNEL=$channelPath"
Write-Output "AGENT_CAPTURES=$capturePath"
& $GodotExe @arguments
exit $LASTEXITCODE
