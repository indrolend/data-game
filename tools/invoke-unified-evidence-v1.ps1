param(
    [ValidateSet('run','ingest','verify','pack','diagnose')][string]$Operation = 'run',
    [ValidateSet('capture-shoot-v2')][string]$Scenario = 'capture-shoot-v2',
    [string]$Source,
    [string]$Output,
    [string]$GodotExe = 'godot_console.exe',
    [ValidateRange(1,100)][int]$MaximumDecisions = 40
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..").Path
$schema = 'digital-breakdown.unified-evidence/v2'
$supportedSchemas = @('digital-breakdown.unified-evidence/v1', $schema)

function Resolve-InRoot([string]$Path) {
    $full = if ([IO.Path]::IsPathRooted($Path)) {
        [IO.Path]::GetFullPath($Path)
    } else {
        [IO.Path]::GetFullPath((Join-Path $root $Path))
    }
    if (-not $full.StartsWith($root, [StringComparison]::OrdinalIgnoreCase)) { throw "Path escapes repository: $Path" }
    return $full
}
function Write-Ndjson([string]$Path, [object[]]$Rows) {
    $text = (($Rows | ForEach-Object { $_ | ConvertTo-Json -Compress -Depth 12 }) -join "`n")
    if ($text) { $text += "`n" }
    [IO.File]::WriteAllText($Path, $text, [Text.UTF8Encoding]::new($false))
}
function Write-Json([string]$Path, [object]$Value) {
    [IO.File]::WriteAllText($Path, (($Value | ConvertTo-Json -Depth 16) + "`n"), [Text.UTF8Encoding]::new($false))
}
function Read-AgentState([string]$Line) {
    $state = [ordered]@{}
    foreach ($match in [regex]::Matches($Line, '(?<key>[a-z_]+)=(?<value>"[^"]*"|[^ ]+)')) {
        $value = $match.Groups['value'].Value.Trim('"')
        if ($value -match '^-?\d+$') { $value = [long]$value }
        elseif ($value -match '^-?(?:\d+\.\d+|\d+)(?:[eE][+-]?\d+)?$') { $value = [double]$value }
        $state[$match.Groups['key'].Value] = $value
    }
    return $state
}
function Get-Relative([string]$Base, [string]$Path) { [IO.Path]::GetRelativePath($Base, $Path).Replace('\','/') }

function Test-Package([string]$PackagePath) {
    $package = Resolve-Path $PackagePath
    $manifest = Get-Content -Raw (Join-Path $package 'manifest.json') | ConvertFrom-Json
    if ($manifest.schema_version -notin $supportedSchemas) { throw "Unsupported schema: $($manifest.schema_version)" }
    $states = @(Get-Content (Join-Path $package 'states.ndjson') | Where-Object { $_ } | ConvertFrom-Json)
    $lastFrame = -1
    $ids = @{}
    foreach ($state in $states) {
        if ($state.simulation_frame -lt $lastFrame) { throw 'State frames are not monotonic' }
        $lastFrame = $state.simulation_frame
        if ($state.run_id -ne $manifest.run_id) { throw 'State run identity mismatch' }
    }
    $objectsPath = Join-Path $package 'objects\objects.ndjson'
    if (Test-Path $objectsPath) {
        foreach ($object in @(Get-Content $objectsPath | Where-Object { $_ } | ConvertFrom-Json)) {
            if (-not $object.stable_object_id) { throw 'Object lacks stable identity' }
            if ($ids.ContainsKey($object.stable_object_id) -and $ids[$object.stable_object_id] -ne $object.object_type) { throw 'Object identity changed type' }
            $ids[$object.stable_object_id] = $object.object_type
        }
    }
    foreach ($file in $manifest.files) {
        $path = Join-Path $package $file.path
        if (-not (Test-Path -LiteralPath $path)) { throw "Missing evidence file: $($file.path)" }
        $actual = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($actual -ne $file.sha256) { throw "Hash mismatch: $($file.path)" }
    }
    $assertions = Get-Content -Raw (Join-Path $package 'assertions.json') | ConvertFrom-Json
    if (@($assertions.assertions | Where-Object { $_.status -eq 'fail' }).Count -gt 0) { throw 'One or more evidence assertions failed' }
    Write-Output "UNIFIED_VERIFY PASS package=$package states=$($states.Count) files=$(@($manifest.files).Count)"
}

function Update-ManifestFiles([string]$PackagePath) {
    $manifestPath = Join-Path $PackagePath 'manifest.json'
    $manifest = Get-Content -Raw $manifestPath | ConvertFrom-Json
    $files = @()
    foreach ($file in Get-ChildItem $PackagePath -Recurse -File | Where-Object { $_.Name -ne 'manifest.json' }) {
        $files += [ordered]@{ path=Get-Relative $PackagePath $file.FullName; bytes=$file.Length; sha256=(Get-FileHash $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
    }
    $manifest.files = $files
    Write-Json $manifestPath $manifest
}

function Invoke-PackageDiagnosis([string]$PackagePath) {
    $package = (Resolve-Path $PackagePath).Path
    Test-Package $package
    $states = @(Get-Content (Join-Path $package 'states.ndjson') | Where-Object { $_ } | ConvertFrom-Json)
    $findings = [Collections.Generic.List[object]]::new()
    $metrics = [Collections.Generic.List[object]]::new()
    $previousHash = $null
    $previousFrame = -1
    foreach ($state in $states) {
        $relative = [string]$state.godot_presentation.framebuffer_capture
        if (-not $relative) {
            $findings.Add([ordered]@{ id="missing-frame-$($state.observation_id)"; severity='P1'; classification='observed'; category='capture'; simulation_frame=$state.simulation_frame; summary='Authoritative observation has no Godot framebuffer capture'; evidence=$state.observation_id })
            continue
        }
        $framePath = Join-Path $package $relative
        if (-not (Test-Path $framePath)) {
            $findings.Add([ordered]@{ id="missing-frame-file-$($state.observation_id)"; severity='P1'; classification='observed'; category='capture'; simulation_frame=$state.simulation_frame; summary='Godot framebuffer path is recorded but the PNG file is missing'; evidence=$relative })
            continue
        }
        $hash = (Get-FileHash $framePath -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($null -ne $previousHash -and $hash -eq $previousHash -and $state.simulation_frame -gt $previousFrame) {
            $findings.Add([ordered]@{ id="frozen-frame-$($state.observation_id)"; severity='P1'; classification='automated_suspicion'; category='presentation'; simulation_frame=$state.simulation_frame; summary='Framebuffer is byte-identical while native simulation advanced'; evidence=$relative })
        }
        $previousHash = $hash
        $previousFrame = [long]$state.simulation_frame
        $signal = & ffmpeg -hide_banner -nostats -i $framePath -vf 'signalstats,metadata=print' -frames:v 1 -f null - 2>&1 | Out-String
        $yavgMatch = [regex]::Match($signal, 'lavfi.signalstats.YAVG=(?<v>[0-9.]+)')
        $yminMatch = [regex]::Match($signal, 'lavfi.signalstats.YMIN=(?<v>[0-9.]+)')
        $ymaxMatch = [regex]::Match($signal, 'lavfi.signalstats.YMAX=(?<v>[0-9.]+)')
        if ($yavgMatch.Success) {
            $avg=[double]$yavgMatch.Groups['v'].Value; $min=[double]$yminMatch.Groups['v'].Value; $max=[double]$ymaxMatch.Groups['v'].Value
            $metrics.Add([ordered]@{ observation_id=$state.observation_id; simulation_frame=$state.simulation_frame; frame=$relative; luma_average=$avg; luma_min=$min; luma_max=$max; sha256=$hash })
            if ($avg -lt 18) { $findings.Add([ordered]@{ id="dark-$($state.observation_id)"; severity='P1'; classification='automated_suspicion'; category='exposure'; simulation_frame=$state.simulation_frame; summary='Frame is nearly black'; evidence=$relative }) }
            elseif ($avg -gt 235) { $findings.Add([ordered]@{ id="bright-$($state.observation_id)"; severity='P1'; classification='automated_suspicion'; category='exposure'; simulation_frame=$state.simulation_frame; summary='Frame is nearly overexposed'; evidence=$relative }) }
            if (($max-$min) -lt 12) { $findings.Add([ordered]@{ id="uniform-$($state.observation_id)"; severity='P1'; classification='automated_suspicion'; category='rendering'; simulation_frame=$state.simulation_frame; summary='Frame has extremely low luminance range'; evidence=$relative }) }
        }
        if ([double]$state.native_state.nearest_distance -lt 0.75) {
            $findings.Add([ordered]@{ id="camera-crowding-$($state.observation_id)"; severity='P2'; classification='automated_suspicion'; category='camera'; simulation_frame=$state.simulation_frame; summary='Native target is within 0.75 units of player; inspect for camera occlusion/clipping'; evidence=$relative })
        }
        if ([int]$state.native_state.projectiles -gt 0 -and (-not $state.native_state.projectile_id -or $state.native_state.projectile_id -eq 'unavailable')) {
            $findings.Add([ordered]@{ id="projectile-correlation-$($state.observation_id)"; severity='P1'; classification='unsupported_correlation'; category='missing-observation'; simulation_frame=$state.simulation_frame; summary='Native projectile exists, but protocol lacks projectile transform/velocity and Godot presentation identity'; evidence=$relative })
        }
    }
    if (@($states | Where-Object { $_.native_state.camera_yaw -ne $null -and -not $_.native_state.camera_pos }).Count -gt 0) {
        $findings.Add([ordered]@{ id='camera-transform-gap'; severity='P2'; classification='unavailable'; category='missing-observation'; summary='Protocol records camera yaw/pitch but not synchronized camera position, target, or projection result'; evidence='states.ndjson' })
    }
    $diagnostics = [ordered]@{ schema_version='digital-breakdown.visual-diagnostics/v1'; run_id=$states[0].run_id; created_utc=[DateTime]::UtcNow.ToString('o'); review_status='automated_only'; rules=[ordered]@{ exposure='signalstats thresholds'; frozen='byte-identical adjacent PNG with advancing native frame'; crowding='nearest native target distance < 0.75'; correlation='required presentation identity unavailable' }; finding_count=$findings.Count; findings=$findings; frame_metrics=$metrics }
    Write-Json (Join-Path $package 'diagnostics.json') $diagnostics
    $lines = @('# Automated visual diagnostics','',"Run: ``$($states[0].run_id)``",'',"Findings: $($findings.Count)",'','Automated findings are triage signals, not visual acceptance. Unsupported correlation is reported as a data gap, not proof that an object is absent.','')
    foreach ($finding in $findings) { $lines += "- [$($finding.severity)] $($finding.classification): $($finding.summary) (frame $($finding.simulation_frame))" }
    [IO.File]::WriteAllText((Join-Path $package 'diagnostics.md'), ($lines -join "`n") + "`n", [Text.UTF8Encoding]::new($false))
    Update-ManifestFiles $package
    Test-Package $package
    $zip = "$package.zip"
    Compress-Archive -Path (Join-Path $package '*') -DestinationPath $zip -CompressionLevel Optimal -Force
    Write-Output "UNIFIED_DIAGNOSE PASS package=$package findings=$($findings.Count) archive=$zip"
}

function Build-Package([string]$SourcePath, [string]$OutputPath, [string]$RunId) {
    $sourceFull = (Resolve-Path $SourcePath).Path
    $outFull = Resolve-InRoot $OutputPath
    if (Test-Path -LiteralPath $outFull) { throw "Output already exists: $outFull" }
    New-Item -ItemType Directory -Path $outFull | Out-Null
    foreach ($name in 'frames','objects','replay','original') { New-Item -ItemType Directory -Path (Join-Path $outFull $name) | Out-Null }
    $finalized = $false
    try {
        $stdout = Join-Path $sourceFull 'stdout.log'
        $commandsFile = Join-Path $sourceFull 'commands.txt'
        if (-not (Test-Path $stdout) -or -not (Test-Path $commandsFile)) { throw 'Source must contain stdout.log and commands.txt' }
        Copy-Item $stdout (Join-Path $outFull 'original\stdout.log')
        foreach ($name in 'stderr.log','manifest.json','result.json','timeline.ndjson') {
            $candidate = Join-Path $sourceFull $name
            if (Test-Path $candidate) { Copy-Item $candidate (Join-Path $outFull "original\$name") }
        }
        Copy-Item $commandsFile (Join-Path $outFull 'replay\commands.txt')
        $commands = [Collections.Generic.List[object]]::new()
        $commandIndex = 0
        foreach ($line in Get-Content $commandsFile) {
            if ([string]::IsNullOrWhiteSpace($line)) { continue }
            $commands.Add([ordered]@{ schema_version=$schema; run_id=$RunId; command_id="cmd-{0:d4}" -f $commandIndex; sequence=$commandIndex; source_subsystem='powershell-controller'; command=$line; status='submitted'; correlation='ordered-best-effort' })
            $commandIndex++
        }
        Write-Ndjson (Join-Path $outFull 'commands.ndjson') $commands
        $states = [Collections.Generic.List[object]]::new()
        $events = [Collections.Generic.List[object]]::new()
        $objects = [Collections.Generic.List[object]]::new()
        $previous = $null
        $observation = 0
        foreach ($line in Get-Content $stdout) {
            if ($line -notlike 'AGENT_STATE *') { continue }
            $s = Read-AgentState $line
            $frameName = Split-Path ([string]$s.frame_file) -Leaf
            $sourceFrame = Join-Path $sourceFull "frames\$frameName"
            $frameRel = $null
            if (Test-Path $sourceFrame) { Copy-Item $sourceFrame (Join-Path $outFull "frames\$frameName"); $frameRel = "frames/$frameName" }
            $commandId = if ($s.command_id -and $s.command_id -ne 'uncommanded') { [string]$s.command_id } elseif ($observation -eq 0 -or $commands.Count -eq 0) { $null } else { $commands[[Math]::Min($observation - 1, $commands.Count - 1)].command_id }
            $stateRow = [ordered]@{
                schema_version=$schema; run_id=$RunId; observation_id="obs-{0:d4}" -f $observation
                simulation_frame=[long]$s.frame; simulation_time_seconds=([double]$s.frame / 60.0)
                source_subsystem='native-cpp-via-godot-bridge'; evidence_class='observed'; command_id=$commandId
                native_state=$s
                godot_presentation=[ordered]@{ evidence_class='observed'; framebuffer_capture=$frameRel; capture_succeeded=([int]$s.frame_capture -eq 1); adapter_sync='same-agent-observation'; camera=[ordered]@{position=$s.camera_pos;target=$s.camera_target;fov=$s.camera_fov;obstruction_fade_enabled=$s.camera_obstruction_fade}; visual_verification='not_performed' }
                source_schema=[int]$s.schema
            }
            $states.Add($stateRow)
            if ([int]$s.nearest_target -ge 0) {
                $objects.Add([ordered]@{ schema_version=$schema; run_id=$RunId; observation_id=$stateRow.observation_id; simulation_frame=[long]$s.frame; stable_object_id="native-target-$($s.nearest_target)"; object_type='enemy'; source_subsystem='native-cpp+godot'; evidence_class='observed'; native_transform=[ordered]@{ position="$($s.target_pos)" }; godot_transform=[ordered]@{ status=if($s.nearest_node -and $s.nearest_node -ne 'unavailable'){'observed'}else{'unavailable'}; node=$s.nearest_node; position=$s.nearest_godot_pos; error=$s.nearest_transform_error }; presentation=[ordered]@{camera_alpha=$s.nearest_camera_alpha}; geometry_export=[ordered]@{ status='unsupported'; reason='no runtime mesh export contract' }; state=[ordered]@{ alive=$s.target_alive; slurpable=$s.target_slurpable; armor=$s.target_armor; health=$s.target_health } })
            }
            if ($s.projectile_id -and $s.projectile_id -ne 'unavailable') {
                $objects.Add([ordered]@{ schema_version=$schema; run_id=$RunId; observation_id=$stateRow.observation_id; simulation_frame=[long]$s.frame; stable_object_id=$s.projectile_id; object_type='soul-projectile'; source_subsystem='native-cpp+godot'; evidence_class='observed'; native_transform=[ordered]@{position=$s.projectile_pos;velocity=$s.projectile_vel}; godot_transform=[ordered]@{status=if([int]$s.projectile_visible -eq 1){'observed'}else{'unavailable'};node=$s.godot_projectile_node;position=$s.godot_projectile_pos;error=$s.projectile_transform_error}; presentation=[ordered]@{visible=([int]$s.projectile_visible -eq 1)}; geometry_export=[ordered]@{status='unsupported';reason='runtime presentation uses a generated SphereMesh; no GLB exporter contract'} })
            }
            if ([int]$s.objective -ge 0) {
                $objects.Add([ordered]@{ schema_version=$schema; run_id=$RunId; observation_id=$stateRow.observation_id; simulation_frame=[long]$s.frame; stable_object_id="capture-goal-$($s.objective)"; object_type='capture-goal'; source_subsystem='native-cpp'; evidence_class='observed'; native_transform=[ordered]@{ position="$($s.objective_pos)" }; godot_transform=[ordered]@{ status='unavailable'; reason='agent protocol v3 does not emit adapter global transform' }; geometry_export=[ordered]@{ status='unsupported' }; state=[ordered]@{ deposited=$s.deposited; required=$s.required } })
            }
            if ($null -ne $previous) {
                foreach ($field in 'souls','projectiles','deposited','tokens','living','dead') {
                    if ($s[$field] -ne $previous[$field]) { $events.Add([ordered]@{ schema_version=$schema; run_id=$RunId; event_id="event-{0:d4}" -f $events.Count; simulation_frame=[long]$s.frame; simulation_time_seconds=([double]$s.frame/60.0); source_subsystem='native-cpp'; evidence_class='observed'; event_type="state-transition:$field"; command_id=$commandId; before=$previous[$field]; after=$s[$field] }) }
                }
            }
            $previous = $s
            $observation++
        }
        if ($states.Count -eq 0) { throw 'No authoritative AGENT_STATE observations found' }
        Write-Ndjson (Join-Path $outFull 'states.ndjson') $states
        Write-Ndjson (Join-Path $outFull 'events.ndjson') $events
        Write-Ndjson (Join-Path $outFull 'objects\objects.ndjson') $objects
        $capture = @($states | Where-Object { [int]$_.native_state.souls -gt 0 }).Count -gt 0
        $projectile = @($states | Where-Object { [int]$_.native_state.projectiles -gt 0 }).Count -gt 0
        $deposit = ([int]$states[-1].native_state.deposited -gt [int]$states[0].native_state.deposited) -and ([int]$states[-1].native_state.tokens -gt [int]$states[0].native_state.tokens)
        $assertionRows = @(
            [ordered]@{ id='authoritative-state-present'; status=if($states.Count -gt 0){'pass'}else{'fail'}; evidence='states.ndjson' },
            [ordered]@{ id='native-soul-captured'; status=if($capture){'pass'}else{'fail'}; evidence='observed souls counter > 0' },
            [ordered]@{ id='native-projectile-created'; status=if($projectile){'pass'}else{'fail'}; evidence='observed projectiles counter > 0' },
            [ordered]@{ id='deposit-objective-achieved'; status=if($deposit){'pass'}else{'not_achieved'}; evidence='requires deposited and token counters both increase' },
            [ordered]@{ id='godot-frames-captured'; status=if(@(Get-ChildItem (Join-Path $outFull 'frames') -File).Count -gt 0){'pass'}else{'unavailable'}; evidence='frames/' },
            [ordered]@{ id='visual-quality-verified'; status='not_performed'; evidence='capture is not visual acceptance' }
        )
        $assertions = [ordered]@{ schema_version=$schema; run_id=$RunId; objective='capture-and-shoot'; objective_achieved=($capture -and $projectile); deposit_objective_achieved=$deposit; assertions=$assertionRows }
        Write-Json (Join-Path $outFull 'assertions.json') $assertions
        $status = @(& git -C $root status --porcelain=v1)
        $godotVersion = (& $GodotExe --version 2>$null | Select-Object -First 1)
        $report = @("# Unified evidence report",'',"- Run: ``$RunId``","- Scenario: ``$Scenario``","- Authoritative observations: $($states.Count)","- Commands retained: $($commands.Count)","- Native soul captured: $capture","- Native projectile observed: $projectile","- Deposit/token transition: $deposit","- Godot frames captured: $(@(Get-ChildItem (Join-Path $outFull 'frames') -File).Count)",'- Visual quality verified: no (capture only)','','## Compatibility and limitations','','Agent protocol schema 4 supplies explicit command IDs, synchronized camera state, native projectile vectors, and matching Godot presentation transforms. Schema 3 inputs remain readable with unavailable fields preserved. Runtime mesh GLB export and deterministic replay verification remain unsupported and are not fabricated.') -join "`n"
        [IO.File]::WriteAllText((Join-Path $outFull 'report.md'), $report + "`n", [Text.UTF8Encoding]::new($false))
        $files = [Collections.Generic.List[object]]::new()
        foreach ($file in Get-ChildItem $outFull -Recurse -File | Where-Object { $_.Name -ne 'manifest.json' }) {
            $files.Add([ordered]@{ path=Get-Relative $outFull $file.FullName; bytes=$file.Length; sha256=(Get-FileHash $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant() })
        }
        $manifest = [ordered]@{ schema_version=$schema; compatibility=[ordered]@{ reads=@('agent-playtest/schema-3','capture-shoot-v2/schema-1'); missing_fields='explicitly unavailable/unsupported'; forward_compatible=$false }; run_id=$RunId; created_utc=[DateTime]::UtcNow.ToString('o'); scenario=$Scenario; outcome=if($capture -and $projectile){'objective_achieved'}else{'objective_not_achieved'}; capture_status='captured'; visual_verification='not_performed'; repository=$root; source_revision=[ordered]@{ git_head=(& git -C $root rev-parse HEAD).Trim(); dirty=($status.Count -gt 0); status=$status }; runtime=[ordered]@{ godot_version="$godotVersion".Trim(); fixed_dt=1.0/60.0 }; replay=[ordered]@{ inputs_retained=$true; deterministic_replay_verified=$false; reason='historical controller uses a live policy and protocol lacks a full initial-state snapshot' }; source_evidence=Get-Relative $root $sourceFull; files=$files }
        Write-Json (Join-Path $outFull 'manifest.json') $manifest
        $finalized = $true
    } finally {
        if (-not $finalized) {
            Write-Json (Join-Path $outFull 'assertions.json') ([ordered]@{ schema_version=$schema; run_id=$RunId; objective_achieved=$false; assertions=@([ordered]@{id='package-generation';status='fail';evidence="$($Error[0])"}) })
            [IO.File]::WriteAllText((Join-Path $outFull 'report.md'), "# Unified evidence report`n`nPackage generation failed; retained files remain inspectable.`n", [Text.UTF8Encoding]::new($false))
        }
    }
    if (-not $finalized) { throw 'Package generation failed; partial evidence was retained' }
    Test-Package $outFull
    $zip = "$outFull.zip"
    Compress-Archive -Path (Join-Path $outFull '*') -DestinationPath $zip -CompressionLevel Optimal
    if ($IsWindows) {
        try {
            Add-Type -AssemblyName System.Windows.Forms
            $drop = [Collections.Specialized.StringCollection]::new()
            [void]$drop.Add($zip)
            [Windows.Forms.Clipboard]::SetFileDropList($drop)
            Write-Output "UNIFIED_CLIPBOARD file_drop=$zip"
        } catch {
            Write-Warning "Evidence archive was created but the Windows file-drop clipboard was unavailable: $($_.Exception.Message)"
        }
    }
    Write-Output "UNIFIED_RESULT PASS package=$outFull archive=$zip"
    return $outFull
}

if ($Operation -eq 'verify') { if (-not $Source) { throw '-Source package is required' }; Test-Package (Resolve-InRoot $Source); exit 0 }
if ($Operation -eq 'diagnose') { if (-not $Source) { throw '-Source package is required' }; Invoke-PackageDiagnosis (Resolve-InRoot $Source); exit 0 }
if ($Operation -eq 'pack') { if (-not $Source) { throw '-Source package is required' }; $p=(Resolve-Path (Resolve-InRoot $Source)).Path; Test-Package $p; Compress-Archive -Path (Join-Path $p '*') -DestinationPath "$p.zip" -Force; exit 0 }
$runId = "{0}-{1}" -f $Scenario,(Get-Date -Format 'yyyyMMdd-HHmmss-fff')
if ($Operation -eq 'run') {
    $sourceRelative = "evidence\generated\$runId-original"
    & (Join-Path $root 'tools\invoke-godot-agent-capture-shoot-v2.ps1') -GodotExe $GodotExe -Session $sourceRelative -MaximumDecisions $MaximumDecisions
    if ($LASTEXITCODE -ne 0) { throw "Scenario controller exited $LASTEXITCODE" }
    $Source = $sourceRelative
}
if (-not $Source) { throw '-Source is required for ingest' }
if (-not $Output) { $Output = "evidence\generated\unified-$runId" }
Build-Package (Resolve-InRoot $Source) $Output $runId

