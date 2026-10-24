param(
    [string]$GodotExe = "godot_console.exe",
    [string]$Session = "evidence\generated\agent-policy",
    [ValidateRange(1, 100)][int]$MaximumDecisions = 40
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path "$PSScriptRoot\..").Path
$sessionPath = [IO.Path]::GetFullPath((Join-Path $root $Session))
if (Test-Path -LiteralPath $sessionPath) {
    throw "Agent policy session already exists; choose a fresh -Session: $sessionPath"
}
$channelPath = Join-Path $sessionPath "commands.txt"
$capturePath = Join-Path $sessionPath "frames"
$stdoutPath = Join-Path $sessionPath "stdout.log"
$stderrPath = Join-Path $sessionPath "stderr.log"
New-Item -ItemType Directory -Path $sessionPath | Out-Null
New-Item -ItemType File -Path $channelPath | Out-Null
$timeline = [Collections.Generic.List[object]]::new()
$commandTranscript = [Collections.Generic.List[string]]::new()

function Read-AgentState([string]$Line) {
    $state = @{}
    foreach ($match in [regex]::Matches($Line, '(?<key>[a-z_]+)=(?<value>"[^"]*"|[^ ]+)')) {
        $state[$match.Groups['key'].Value] = $match.Groups['value'].Value.Trim('"')
    }
    return $state
}

function Send-AgentCommand([string]$Command) {
    if ($Command.Contains("`r") -or $Command.Contains("`n") -or $Command.Length -gt 1024) {
        throw "Policy produced an invalid command"
    }
    Add-Content -LiteralPath $channelPath -Value $Command -Encoding utf8
    $commandTranscript.Add($Command)
    Write-Output "POLICY_COMMAND $Command"
}

$arguments = @(
    "--path", (Join-Path $root "godot"),
    "--script", "res://scripts/agent_playtest.gd", "--",
    "--commands=$channelPath", "--capture-dir=$capturePath", "--follow-commands"
)
$process = Start-Process -FilePath $GodotExe -ArgumentList $arguments -PassThru -NoNewWindow `
    -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath
$seenWaiting = 0
$decision = 0
$resetSent = $false
$vacuumAimed = $false
$succeeded = $false
$shotSent = $false
$projectileObserved = $false
$shootPending = $false
try {
    while (-not $process.HasExited -and $decision -lt $MaximumDecisions) {
        Start-Sleep -Milliseconds 100
        $lines = if (Test-Path -LiteralPath $stdoutPath) { @(Get-Content -LiteralPath $stdoutPath) } else { @() }
        $waiting = @($lines | Where-Object { $_ -like 'AGENT_WAITING *' }).Count
        if ($waiting -le $seenWaiting) { continue }
        $seenWaiting = $waiting
        $stateLine = $lines | Where-Object { $_ -like 'AGENT_STATE *' } | Select-Object -Last 1
        if (-not $stateLine) { continue }
        $state = Read-AgentState $stateLine
        $timeline.Add([ordered]@{
            decision = $decision
            frame = [int]$state.frame
            room = [int]$state.room
            seed = [int]$state.seed
            environment = $state.environment
            souls = [int]$state.souls
            living = [int]$state.living
            nearest_target = [int]$state.nearest_target
            nearest_distance = [double]$state.nearest_distance
            target_armor = [double]$state.target_armor
            target_health = [double]$state.target_health
            target_slurpable = [int]$state.target_slurpable
            battery = [double]$state.battery
            projectiles = [int]$state.projectiles
            frame_file = $state.frame_file
        })
        Write-Output ("POLICY_OBSERVE decision={0} frame={1} souls={2} living={3} target={4} distance={5} armor={6} slurpable={7} battery={8}" -f `
            $decision, $state.frame, $state.souls, $state.living, $state.nearest_target,
            $state.nearest_distance, $state.target_armor, $state.target_slurpable, $state.battery)
        if (-not $resetSent) {
            Send-AgentCommand "reset"
            $resetSent = $true
        } elseif ([int]$state.dead -ne 0 -or [int]$state.downed -ne 0) {
            Send-AgentCommand "quit"
            break
        } elseif ($shootPending) {
            Send-AgentCommand "step frames=1 shoot=1"
            $shootPending = $false
        } elseif ($shotSent) {
            if ([int]$state.projectiles -gt 0 -and [int]$state.souls -eq 0) {
                $projectileObserved = $true
                $succeeded = $true
                Send-AgentCommand "quit"
                break
            }
            Send-AgentCommand "step frames=1"
        } elseif ([int]$state.souls -gt 0) {
            Send-AgentCommand "step frames=2 vacuum=0"
            $shotSent = $true
            $shootPending = $true
        } elseif ([double]$state.battery -lt 30.0) {
            Send-AgentCommand "battery_full"
        } elseif ([int]$state.target_slurpable -ne 0) {
            $look = if ($vacuumAimed) { "" } else { " lookX=120" }
            Send-AgentCommand "step frames=60 vacuum=1$look"
            $vacuumAimed = $true
        } elseif ([double]$state.nearest_distance -gt 3.2) {
            Send-AgentCommand "step frames=60 moveZ=1 sprint=1"
        } else {
            Send-AgentCommand "step frames=50 melee=1"
        }
        $decision++
    }
    if (-not $process.HasExited -and $decision -ge $MaximumDecisions) {
        Send-AgentCommand "quit"
    }
    if (-not $process.WaitForExit(15000)) {
        $process.Kill($true)
        throw "Agent runtime did not exit after the policy stop command"
    }
    if ($process.ExitCode -ne 0) {
        throw "Agent runtime failed with exit code $($process.ExitCode); see $stderrPath"
    }
    if (-not $succeeded) {
        throw "Policy did not capture and shoot a native soul within $MaximumDecisions decisions"
    }
    $timelinePath = Join-Path $sessionPath "timeline.ndjson"
    $timeline | ForEach-Object { $_ | ConvertTo-Json -Compress } | Set-Content -LiteralPath $timelinePath -Encoding utf8NoBOM
    $commit = (& git -C $root rev-parse HEAD).Trim()
    $dirty = -not [string]::IsNullOrWhiteSpace((& git -C $root status --short | Out-String).Trim())
    $godotCommand = Get-Command $GodotExe -ErrorAction Stop
    $godotPath = $godotCommand.Source
    $extensionPath = Join-Path $root "godot\bin\digital_breakdown_parent.dll"
    $frames = @(Get-ChildItem -LiteralPath $capturePath -Filter "*.png" -File | Sort-Object Name)
    $manifest = [ordered]@{
        schema_version = 1
        scenario = "fixture-combat-ingestion-shoot-policy"
        classification = "deterministic diagnostic; not an authentic unaided playthrough"
        repository = $root
        git_commit = $commit
        git_dirty = $dirty
        godot_version = (& $GodotExe --version | Select-Object -First 1)
        godot_executable = $godotPath
        godot_sha256 = (Get-FileHash -LiteralPath $godotPath -Algorithm SHA256).Hash.ToLowerInvariant()
        extension = $extensionPath
        extension_sha256 = (Get-FileHash -LiteralPath $extensionPath -Algorithm SHA256).Hash.ToLowerInvariant()
        fixed_dt = 1.0 / 60.0
        maximum_decisions = $MaximumDecisions
        decisions = $decision
        final_frame = [int]$timeline[-1].frame
        command_count = $commandTranscript.Count
        observation_count = $timeline.Count
        frame_count = $frames.Count
        command_transcript = "commands.txt"
        timeline = "timeline.ndjson"
        stdout = "stdout.log"
        stderr = "stderr.log"
        frames = "frames"
    }
    $manifestPath = Join-Path $sessionPath "manifest.json"
    $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestPath -Encoding utf8NoBOM
    $result = [ordered]@{
        outcome = "PASS"
        captured_native_soul = 1
        projectile_observed = $projectileObserved
        exit_code = $process.ExitCode
        final_frame = [int]$timeline[-1].frame
        manifest_sha256 = (Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash.ToLowerInvariant()
        timeline_sha256 = (Get-FileHash -LiteralPath $timelinePath -Algorithm SHA256).Hash.ToLowerInvariant()
        commands_sha256 = (Get-FileHash -LiteralPath $channelPath -Algorithm SHA256).Hash.ToLowerInvariant()
        stdout_sha256 = (Get-FileHash -LiteralPath $stdoutPath -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    $result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $sessionPath "result.json") -Encoding utf8NoBOM
    Write-Output "POLICY_RESULT PASS projectile_observed=1 decisions=$decision evidence=$sessionPath"
} finally {
    if (-not $process.HasExited) { $process.Kill($true) }
}
