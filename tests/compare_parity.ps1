param([string]$ReferenceExe = "$PSScriptRoot/../build-godot47/Release/parent_reference.exe", [string]$GodotExe = "godot_console.exe", [double]$Tolerance = 0.00001)
$ErrorActionPreference = "Stop"
$root = (Resolve-Path "$PSScriptRoot/..").Path
$evidence = Join-Path $root "evidence"
New-Item -ItemType Directory -Force $evidence | Out-Null
$referenceLog = Join-Path $evidence "parent-reference.csv"
$godotLog = Join-Path $evidence "godot-parity.log"
& $ReferenceExe | Set-Content $referenceLog
if ($LASTEXITCODE -ne 0) { throw "Reference harness failed with exit code $LASTEXITCODE" }
& $GodotExe --headless --path (Join-Path $root "godot") --script "res://tests/parent_parity.gd" 2>&1 | Set-Content $godotLog
if ($LASTEXITCODE -ne 0) { throw "Godot parity harness failed with exit code $LASTEXITCODE" }
$reference = @(Get-Content $referenceLog | Where-Object { $_ -like "PARITY,*" -or $_ -like "PHONE,*" -or $_ -like "MENULAYOUT,*" -or $_ -like "MENUROW,*" -or $_ -like "ACTION,*" -or $_ -like "TARGET,*" -or $_ -like "INTERACT,*" -or $_ -like "TARGETPOSE,*" -or $_ -like "TETHER,*" -or $_ -like "HIT,*" -or $_ -like "MELEE,*" -or $_ -like "SIGNAL,*" -or $_ -like "PAUSE,*" -or $_ -like "ROOM,*" -or $_ -like "ROOMSTRUCT,*" -or $_ -like "ROOMOBJ,*" -or $_ -like "ROCKVERTEX,*" -or $_ -like "HOUSEPART,*" -or $_ -like "MARKERPART,*" -or $_ -like "RUINPART,*" -or $_ -like "UPGRADE,*" -or $_ -like "ROOMCROSS,*" -or $_ -like "LIGHTING,*" })
$migrated = @(Get-Content $godotLog | Where-Object { $_ -like "PARITY,*" -or $_ -like "PHONE,*" -or $_ -like "MENULAYOUT,*" -or $_ -like "MENUROW,*" -or $_ -like "ACTION,*" -or $_ -like "TARGET,*" -or $_ -like "INTERACT,*" -or $_ -like "TARGETPOSE,*" -or $_ -like "TETHER,*" -or $_ -like "HIT,*" -or $_ -like "MELEE,*" -or $_ -like "SIGNAL,*" -or $_ -like "PAUSE,*" -or $_ -like "ROOM,*" -or $_ -like "ROOMSTRUCT,*" -or $_ -like "ROOMOBJ,*" -or $_ -like "ROCKVERTEX,*" -or $_ -like "HOUSEPART,*" -or $_ -like "MARKERPART,*" -or $_ -like "RUINPART,*" -or $_ -like "UPGRADE,*" -or $_ -like "ROOMCROSS,*" -or $_ -like "LIGHTING,*" })
if ($reference.Count -ne $migrated.Count) { throw "Snapshot count mismatch: parent=$($reference.Count), Godot=$($migrated.Count)" }
for ($row = 0; $row -lt $reference.Count; ++$row) {
    $left = $reference[$row].Split(',')
    $right = $migrated[$row].Split(',')
    if ($left.Count -ne $right.Count) { throw "Column count mismatch at row $row" }
    $firstNumericColumn = if ($left[0] -eq "ACTION") { 2 } else { 1 }
    for ($column = $firstNumericColumn; $column -lt $left.Count; ++$column) {
        $leftNumber = 0.0
        $rightNumber = 0.0
        $leftNumeric = [double]::TryParse($left[$column], [Globalization.NumberStyles]::Float, [Globalization.CultureInfo]::InvariantCulture, [ref]$leftNumber)
        $rightNumeric = [double]::TryParse($right[$column], [Globalization.NumberStyles]::Float, [Globalization.CultureInfo]::InvariantCulture, [ref]$rightNumber)
        if ($leftNumeric -and $rightNumeric) {
            $delta = [Math]::Abs($leftNumber - $rightNumber)
            if ($delta -gt $Tolerance) { throw "Parity mismatch row=$row column=$column parent=$($left[$column]) Godot=$($right[$column]) delta=$delta tolerance=$Tolerance" }
        } elseif ($left[$column] -cne $right[$column]) {
            throw "Parity mismatch row=$row column=$column parent=$($left[$column]) Godot=$($right[$column])"
        }
    }
}
Write-Output "PASS: $($reference.Count) parent-vs-Godot snapshots match within tolerance $Tolerance"
