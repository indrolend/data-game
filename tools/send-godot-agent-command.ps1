param(
    [Parameter(Mandatory = $true)][string]$Channel,
    [Parameter(Mandatory = $true)][string]$Command
)

$ErrorActionPreference = "Stop"
$channelPath = (Resolve-Path $Channel).Path
if ($Command.Contains("`r") -or $Command.Contains("`n") -or $Command.Length -gt 1024) {
    throw "Command must be one line of at most 1024 characters"
}
$allowed = '^(observe|reset|quit|pause|menu_confirm|menu_back|upgrade_confirm|field_island_fixture|soul_spawn|battery_full|enemies_off|capture_shot_fixture|city_fixture|coastal_fixture|sterile_fixture|ruin_fixture|upgrade_fixture|room_inspector|room_next|room_reroll|room_enemies|menu_move direction=-?1|menu_adjust direction=-?1|upgrade_select index=[0-5]|upgrade_move horizontal=-?[01] vertical=-?[01]|assert( [a-z_]+=-?[0-9]+)+|step( [A-Za-z]+=[^ ]+)+)$'
if ($Command -notmatch $allowed) {
    throw "Command does not match the bounded agent protocol: $Command"
}
Add-Content -LiteralPath $channelPath -Value $Command -Encoding utf8
Write-Output "AGENT_COMMAND_SENT $Command"
