[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$inventoryMenuPath = Join-Path $projectRoot 'Source\KalmalaUI\Private\KalmalaInventoryMenuWidget.cpp'
$inventoryInspectPath = Join-Path $projectRoot 'Source\KalmalaUI\Private\KalmalaInventoryInspectWidget.cpp'
$craftingPath = Join-Path $projectRoot 'Source\KalmalaUI\Private\KalmalaCraftingSubsystem.cpp'
$worldMapPath = Join-Path $projectRoot 'Source\KalmalaUI\Private\KalmalaWorldMapWidget.cpp'
$settingsPath = Join-Path $projectRoot 'Source\KalmalaUI\Private\KalmalaSettingsWidget.cpp'
$survivalStatusPath = Join-Path $projectRoot 'Source\KalmalaUI\Private\KalmalaSurvivalStatusWidget.cpp'
$statusHotbarPath = Join-Path $projectRoot 'Source\KalmalaUI\Private\KalmalaStatusHotbarWidget.cpp'
$weatherStatusPath = Join-Path $projectRoot 'Source\KalmalaUI\Private\KalmalaWeatherActivityWidget.cpp'
$itemDetailPath = Join-Path $projectRoot 'Source\KalmalaUI\Private\KalmalaItemDetailWidget.cpp'
$inventoryMenu = Get-Content -LiteralPath $inventoryMenuPath -Raw
$inventoryInspect = Get-Content -LiteralPath $inventoryInspectPath -Raw
$crafting = Get-Content -LiteralPath $craftingPath -Raw
$worldMap = Get-Content -LiteralPath $worldMapPath -Raw
$settings = Get-Content -LiteralPath $settingsPath -Raw
$survivalStatus = Get-Content -LiteralPath $survivalStatusPath -Raw
$statusHotbar = Get-Content -LiteralPath $statusHotbarPath -Raw
$weatherStatus = Get-Content -LiteralPath $weatherStatusPath -Raw
$itemDetail = Get-Content -LiteralPath $itemDetailPath -Raw

# Ignore development-only review assertions; this audit covers player-facing
# implementation copy, not test descriptions of the strings being rejected.
$playerFacingCrafting = ($crafting -split '#if !UE_BUILD_SHIPPING', 2)[0]
$playerFacingCopy = @($inventoryMenu, $inventoryInspect, $playerFacingCrafting, $worldMap, $settings,
    $survivalStatus, $statusHotbar, $weatherStatus, $itemDetail) -join "`n"
$forbiddenCopy = @(
    'Construction hammer menu input:',
    'Controller View / special-left',
    'Up/Down or D-pad:',
    'Enter / A:',
    'Escape / B:',
    'Controller Y:',
    'Mouse buttons and focused keyboard/controller',
    'Recipe browsing: Tab reaches',
    'Page Up/Down cycles',
    'Arrows or D-pad select an item',
    'Tab continues to other menu controls',
    'Tab to search/controls',
    'Page Up / left shoulder cycles',
    'Page Down / right shoulder cycles',
    'use arrows or D-pad to change',
    '(activate to cycle)',
    'C / pad Menu: share',
    'Middle-click: ping',
    'Q / right-stick click: ping centre',
    'FILTER FOCUS: arrows / D-pad',
    'Enter / A toggles visibility',
    'Esc / B leaves filter focus',
    'Click row toggles',
    'F / L3 focus',
    'Up/Down or D-pad: select',
    'Enter / A toggle',
    'Drag/Arrows pan',
    'Wheel/PgUp zoom',
    'R recenter',
    'M / Esc close',
    'Enter complete',
    'H show/hide',
    'Delete remove',
    '1 Cairn',
    '2 Lantern',
    '3 Thread',
    'Enter save',
    'Esc cancel',
    'Press Esc to return to the game'
)
foreach ($copy in $forbiddenCopy) {
    if ($playerFacingCopy.IndexOf($copy, [StringComparison]::OrdinalIgnoreCase) -ge 0) {
        throw "Player-facing menu copy still contains an input/help legend: $copy"
    }
}

if ($crafting -match 'GetDefault\s*<\s*UInputSettings\s*>') {
    throw 'The crafting view still reads an input binding for display.'
}

$otherUiSources = Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Source\KalmalaUI\Private') -Filter '*.cpp' -Recurse |
    Where-Object { $_.FullName -notmatch '\\Tests\\' -and $_.Name -ne 'KalmalaSettingsWidget.cpp' }
foreach ($source in $otherUiSources) {
    $contents = Get-Content -LiteralPath $source.FullName -Raw
    if ($contents -match 'GetDefault\s*<\s*UInputSettings\s*>|\.GetDisplayName\s*\(') {
        throw "A player-facing UI source outside Options resolves a key label: $($source.Name)"
    }
    # SettingsSubsystem reads labels only for its developer accessibility log.
    if ($source.Name -notin @('KalmalaSettingsSubsystem.cpp') -and $contents -match 'GetLocalInputBindingLabel\s*\(') {
        throw "A UI view outside Options reads a display binding label: $($source.Name)"
    }
}
if ($settings -notmatch '(?s)SetText\s*\(.*?GetLocalInputBindingLabel' -or
    $settings -notmatch 'ControlButtons\[0\]->SetKeyboardFocus') {
    throw 'Options > Controls no longer owns visible binding labels and focus.'
}

foreach ($mapLabel in @('MAP SYMBOLS', 'Personal pins', 'Co-op players', 'Map pings', 'Share map · Ping · Ping centre')) {
    if ($worldMap.IndexOf($mapLabel, [StringComparison]::Ordinal) -lt 0) {
        throw "Expected readable map data/action label is missing: $mapLabel"
    }
}

$requiredActionLabels = @(
    @{ Source = $inventoryMenu; Label = 'Previous item' },
    @{ Source = $inventoryMenu; Label = 'Next item' },
    @{ Source = $inventoryMenu; Label = 'Repair selected tool' },
    @{ Source = $inventoryMenu; Label = 'Eat one serving' },
    @{ Source = $playerFacingCrafting; Label = 'Inspect inventory' },
    @{ Source = $playerFacingCrafting; Label = 'Craft one' },
    @{ Source = $playerFacingCrafting; Label = 'Build / place selected' },
    @{ Source = $playerFacingCrafting; Label = 'Repair Reed Knife' },
    @{ Source = $playerFacingCrafting; Label = 'Inspect nearby chest' },
    @{ Source = $playerFacingCrafting; Label = 'Store one' },
    @{ Source = $playerFacingCrafting; Label = 'Take one' }
)
foreach ($entry in $requiredActionLabels) {
    $pattern = [regex]::Escape('TEXT("' + $entry.Label + '")')
    if ($entry.Source -notmatch $pattern) {
        throw "Expected player-facing action label is missing: $($entry.Label)"
    }
}

if ($inventoryMenu -notmatch 'NavigateInventoryMenu' -or
    $inventoryMenu -notmatch 'EKeys::Gamepad_DPad' -or
    $inventoryInspect -notmatch 'NativeOnPreviewKeyDown' -or
    $inventoryInspect -notmatch 'EKeys::Gamepad_DPad' -or
    $crafting -notmatch 'NativeOnPreviewKeyDown' -or
    $crafting -notmatch 'EKeys::Gamepad_DPad') {
    throw 'Keyboard/controller focus navigation seams are missing from a menu view.'
}

Write-Output 'PASS: map, status/detail and menu copy omit binding legends; Options retains current labels, actions and navigation.'
