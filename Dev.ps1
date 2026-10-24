[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet('help', 'status', 'verify', 'prepush', 'clean-checkout', 'evidence')]
    [string]$Action = 'help',
    [string]$GodotExe = 'godot_console.exe',
    [string]$NativeRoot,
    [string]$GodotCppDirectory,
    [ValidateSet('ingest', 'run', 'verify', 'diagnose', 'pack')]
    [string]$EvidenceOperation = 'verify',
    [string]$Source,
    [string]$Output
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
if (-not $NativeRoot) { $NativeRoot = Join-Path (Split-Path $root -Parent) 'data-game' }
if (-not $GodotCppDirectory) { $GodotCppDirectory = Join-Path (Split-Path $root -Parent) 'godot-cpp' }

switch ($Action) {
    'help' {
        @'
.\Dev.ps1 status          concise branch/revision/dirty state
.\Dev.ps1 clean-checkout verify that the Git index contains required runtime inputs
.\Dev.ps1 verify          run the full build, behavioral, and parity suite
.\Dev.ps1 prepush         run drift, hygiene, visual-claim, and full verification gates
.\Dev.ps1 evidence        invoke unified evidence (use -EvidenceOperation and -Source)
'@
    }
    'status' { & git -C $root status --short --branch; if ($LASTEXITCODE -ne 0) { throw 'git status failed' } }
    'clean-checkout' { & "$root\tests\check_clean_checkout.ps1" }
    'verify' { & "$root\tests\verify.ps1" -GodotExe $GodotExe -GodotCppDirectory $GodotCppDirectory }
    'prepush' { & "$root\tests\prepush.ps1" -GodotExe $GodotExe -NativeRoot $NativeRoot -GodotCppDirectory $GodotCppDirectory }
    'evidence' {
        $arguments = @{ Operation = $EvidenceOperation; GodotExe = $GodotExe }
        if ($Source) { $arguments.Source = $Source }
        if ($Output) { $arguments.Output = $Output }
        & "$root\tools\invoke-unified-evidence-v1.ps1" @arguments
    }
}
