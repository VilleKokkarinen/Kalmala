$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot

$retiredPaths = @(
    'Source\KalmalaUI\Private\KalmalaInventorySubsystem.cpp',
    'Source\KalmalaUI\Public\KalmalaInventorySubsystem.h',
    'Source\KalmalaUI\Private\Tests\KalmalaInventoryWidgetTest.cpp',
    'Scripts\Read-InventoryCapture.ps1'
)
foreach ($relativePath in $retiredPaths) {
    if (Test-Path -LiteralPath (Join-Path $projectRoot $relativePath)) {
        throw "Retired persistent inventory HUD source remains: $relativePath"
    }
}

$uiRoot = Join-Path $projectRoot 'Source\KalmalaUI'
$uiSources = Get-ChildItem -LiteralPath $uiRoot -Recurse -File |
    Where-Object { $_.Extension -in @('.h', '.cpp') }
$retiredSourceTerms = @(
    'UKalmalaInventorySubsystem',
    'UKalmalaInventoryWidget',
    'SetCraftingMenuSuppressed',
    'IsCraftingMenuSuppressed',
    'Pack | Build/craft:',
    'Inventory grid fixture:',
    'Inventory presentation: Owner=1',
    'Crafting HUD overlap:',
    'BuildPreparedFoodDetails'
)
foreach ($sourceFile in $uiSources) {
    $source = Get-Content -LiteralPath $sourceFile.FullName -Raw
    foreach ($term in $retiredSourceTerms) {
        if ($source.Contains($term)) {
            throw "Retired panel source remains in $($sourceFile.FullName): $term"
        }
    }
}

$inventoryScript = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'Verify-Inventory.ps1') -Raw
foreach ($term in @('Inventory presentation: Owner=1', 'Inventory grid fixture:', 'Read-InventoryCapture.ps1', '-EquipmentView')) {
    if ($inventoryScript.Contains($term)) {
        throw "Inventory verification still depends on retired HUD output: $term"
    }
}

$menuSubsystem = Get-Content -LiteralPath (Join-Path $uiRoot 'Private\KalmalaInventoryMenuSubsystem.cpp') -Raw
$menuWidget = Get-Content -LiteralPath (Join-Path $uiRoot 'Private\KalmalaInventoryMenuWidget.cpp') -Raw
$menuTest = Get-Content -LiteralPath (Join-Path $uiRoot 'Private\Tests\KalmalaInventoryMenuSelectionTest.cpp') -Raw
if (-not $menuSubsystem.Contains('CreateWidget<UKalmalaInventoryMenuWidget>') -or
    -not $menuSubsystem.Contains('InventoryWidget->Open()') -or
    -not $menuWidget.Contains('SetVisibility(ESlateVisibility::Collapsed)')) {
    throw 'The owner-local Inventory menu must remain on-demand and collapsed before opening.'
}
foreach ($emptyPath in @(
    'Normal gameplay keeps Inventory collapsed until opened',
    'Empty owner pack clears selection',
    'Empty Inventory renders sixteen empty pack slots',
    'Empty pack hides the stale detail panel'
)) {
    if (-not $menuTest.Contains($emptyPath)) {
        throw "Inventory empty-state automation is missing: $emptyPath"
    }
}

$architecture = Get-Content -LiteralPath (Join-Path $projectRoot 'docs\02-technical-architecture.md') -Raw
if ($architecture.Contains('UKalmalaInventorySubsystem') -or $architecture.Contains('hit-test-invisible pack panel')) {
    throw 'Technical architecture still describes the retired persistent inventory panel.'
}

Write-Output 'PASS: legacy inventory HUD files, runtime hooks, help text and capture expectations are absent; on-demand Inventory and empty-state coverage remain.'
