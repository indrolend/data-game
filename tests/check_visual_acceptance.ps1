param(
    [string]$LedgerPath = "$PSScriptRoot\..\evidence\visual-acceptance\ledger.json"
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path "$PSScriptRoot\..").Path
$ledgerFile = (Resolve-Path $LedgerPath).Path
$ledger = Get-Content -Raw -LiteralPath $ledgerFile | ConvertFrom-Json
$errors = [System.Collections.Generic.List[string]]::new()
$allowedAxis = @("verified", "partial", "unverified", "rejected", "not_applicable")
$allowedVisual = @("accepted", "rejected", "unverified")
$allowedReference = @("native_renderer", "approved_design", "intentional_improvement", "none")
$allowedMatch = @("exact", "approximate", "not_applicable")
$completionWords = '(?i)\b(fixed|restored|final|visually verified|visual parity|pixel parity)\b'

if ($ledger.schema -ne 1) { $errors.Add("ledger schema must be 1") }
if (-not $ledger.cases -or $ledger.cases.Count -eq 0) { $errors.Add("ledger must contain cases") }

$ids = @{}
foreach ($case in $ledger.cases) {
    $prefix = "case '$($case.id)'"
    if ([string]::IsNullOrWhiteSpace($case.id)) {
        $errors.Add("case has no id")
        continue
    }
    if ($ids.ContainsKey($case.id)) { $errors.Add("duplicate case id '$($case.id)'") }
    $ids[$case.id] = $true

    foreach ($axisName in @("behavior", "presentation_contract")) {
        $axis = $case.$axisName
        if ($null -eq $axis -or $allowedAxis -notcontains $axis.status) {
            $errors.Add("$prefix has invalid $axisName status")
        }
        if ($axis.status -in @("verified", "partial") -and
            ($null -eq $axis.evidence -or $axis.evidence.Count -eq 0)) {
            $errors.Add("$prefix $axisName status '$($axis.status)' requires evidence")
        }
    }

    if ($null -eq $case.visual -or $allowedVisual -notcontains $case.visual.status) {
        $errors.Add("$prefix has invalid visual status")
        continue
    }
    if ($allowedReference -notcontains $case.visual.reference_kind) {
        $errors.Add("$prefix has invalid reference_kind '$($case.visual.reference_kind)'")
    }
    if ($allowedMatch -notcontains $case.visual.state_correspondence) {
        $errors.Add("$prefix has invalid state_correspondence '$($case.visual.state_correspondence)'")
    }

    $allEvidence = @($case.behavior.evidence) + @($case.presentation_contract.evidence) + @($case.visual.evidence)
    foreach ($path in $allEvidence | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique) {
        $absolute = Join-Path $root $path
        if (-not (Test-Path -LiteralPath $absolute)) {
            $errors.Add("$prefix references missing evidence '$path'")
        }
    }

    $blockers = @($case.visual.review.blockers | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($case.visual.status -eq "accepted") {
        if ($case.visual.reference_kind -eq "none") { $errors.Add("$prefix accepted without a reference") }
        if ($case.visual.evidence.Count -eq 0) { $errors.Add("$prefix accepted without framebuffer evidence") }
        if (-not $case.visual.review.whole_frame) { $errors.Add("$prefix accepted without whole-frame review") }
        if ([string]::IsNullOrWhiteSpace($case.visual.review.reviewer)) { $errors.Add("$prefix accepted without reviewer") }
        if ([string]::IsNullOrWhiteSpace($case.visual.review.reviewed_utc)) { $errors.Add("$prefix accepted without review time") }
        if ($blockers.Count -gt 0) { $errors.Add("$prefix accepted with blocking defects") }
    } elseif ($case.claim -match $completionWords) {
        $errors.Add("$prefix uses completion language while visual status is '$($case.visual.status)'")
    }

    if ($case.visual.status -eq "rejected" -and $blockers.Count -eq 0) {
        $errors.Add("$prefix is rejected but records no blocking defects")
    }
}

if ($errors.Count -gt 0) {
    $errors | ForEach-Object { Write-Error $_ }
    exit 1
}

foreach ($case in $ledger.cases) {
    Write-Output ("VISUAL_STATUS id={0} behavior={1} presentation={2} visual={3} claim=`"{4}`"" -f
        $case.id, $case.behavior.status, $case.presentation_contract.status, $case.visual.status, $case.claim)
}
Write-Output "PASS: visual acceptance claims are evidence-bounded"
