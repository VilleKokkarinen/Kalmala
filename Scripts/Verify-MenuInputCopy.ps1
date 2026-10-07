[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$inventoryMenuPath = Join-Path $projectRoot 'Source\KalmalaUI\Private\KalmalaInventoryMenuWidget.cpp'
$inventoryInspectPath = Join-Path $projectRoot 'Source\KalmalaUI\Private\KalmalaInventoryInspectWidget.cpp'
$craftingPath = Join-Path $projectRoot 'Source\KalmalaUI\Private\KalmalaCraftingSubsystem.cpp'
$inventoryMenu = Get-Content -LiteralPath $inventoryMenuPath -Raw
$inventoryInspect = Get-Content -LiteralPath $inventoryInspectPath -Raw
$crafting = Get-Content -LiteralPath $craftingPath -Raw

# Ignore development-only review assertions; this audit covers player-facing
# implementation copy, not test descriptions of the strings being rejected.
$playerFacingCrafting = ($crafting -split '#if !UE_BUILD_SHIPPING', 2)[0]
$playerFacingCopy = @($inventoryMenu, $inventoryInspect, $playerFacingCrafting) -join "`n"
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
    '(activate to cycle)'
)
foreach ($copy in $forbiddenCopy) {
    if ($playerFacingCopy.IndexOf($copy, [StringComparison]::OrdinalIgnoreCase) -ge 0) {
        throw "Player-facing menu copy still contains an input/help legend: $copy"
    }
}

if ($crafting -match 'GetDefault\s*<\s*UInputSettings\s*>') {
    throw 'The crafting view still reads an input binding for display.'
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

Write-Output 'PASS: inventory and build/crafting/repair/storage copy omits input legends while retaining action labels and focus navigation.'
