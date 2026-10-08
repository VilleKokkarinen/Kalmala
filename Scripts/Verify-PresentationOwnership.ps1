[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
& (Join-Path $PSScriptRoot 'Verify-InventoryPanelRemoval.ps1')
$documentPath = Join-Path $projectRoot 'docs\15-presentation-ownership.md'
if (-not (Test-Path -LiteralPath $documentPath -PathType Leaf)) {
    throw "Presentation ownership document was not found: $documentPath"
}

$document = Get-Content -LiteralPath $documentPath -Raw
$requiredSections = @(
    '## Ownership ledger',
    '## Allowed and forbidden sources',
    '## Static audit and runtime limits',
    '## Multiplayer and persistence boundary'
)
foreach ($section in $requiredSections) {
    if ($document -notmatch [regex]::Escape($section)) {
        throw "Presentation ownership document is missing section: $section"
    }
}

$requiredAssets = @(
    'Content\Kalmala\World\Materials\M_GeneratedBark.uasset',
    'Content\Kalmala\World\Materials\M_GeneratedCanopy.uasset',
    'Content\Kalmala\World\Materials\M_GeneratedLakeShore.uasset',
    'Content\Kalmala\World\Materials\M_GeneratedRock.uasset',
    'Content\Kalmala\World\Materials\M_GeneratedTerrain.uasset',
    'Content\Kalmala\World\Materials\M_GeneratedTerrainBiomeDebug.uasset',
    'Content\Kalmala\World\Materials\M_GeneratedWater.uasset'
)
foreach ($relativePath in $requiredAssets) {
    $assetPath = Join-Path $projectRoot $relativePath
    if (-not (Test-Path -LiteralPath $assetPath -PathType Leaf)) {
        throw "Required project-owned presentation asset is missing: $relativePath"
    }
}

$sourceContracts = @(
    @{ Label = 'accessibility-feedback'; Path = 'Source\KalmalaUI\Private\KalmalaAccessibilityFeedbackSubsystem.cpp'; Patterns = @('MarkerLine', 'COLOUR-INDEPENDENT FEEDBACK', 'GetLocalPlayer()', 'HitTestInvisible') },
    @{ Label = 'support-selection'; Path = 'Source\KalmalaUI\Private\KalmalaSupportSelectionSubsystem.cpp'; Patterns = @('GetLocalPlayer()', 'GetSelectedSupportEffect()', 'HasLearnedEffect', 'SetIsFocusable(false)', 'HitTestInvisible') },
    @{ Label = 'action-notifications'; Path = 'Source\KalmalaUI\Private\KalmalaNotificationSubsystem.cpp'; Patterns = @('GetLocalPlayer()', 'ObserveCombat', 'ObserveSupport', 'ObserveDiscovery', 'HitTestInvisible') },
    @{ Label = 'status-hotbar'; Path = 'Source\KalmalaUI\Private\KalmalaStatusHotbarWidget.cpp'; Patterns = @('HitTestInvisible', 'SetIsFocusable(false)', 'SetExplicitWrapSize', 'ongoing') },
    @{ Label = 'catalogue-icons'; Path = 'Source\KalmalaUI\Private\KalmalaIconWidget.cpp'; Patterns = @('FindCatalogueIcon', 'ConstructionHammer', 'MakeLines') },
    @{ Label = 'player'; Path = 'Source\KalmalaGameplay\Private\KalmalaPlayerModelComponent.cpp'; Patterns = @('M_GeneratedTerrain', 'bTapered', 'CreateMeshSection_LinearColor') },
    @{ Label = 'wildlife'; Path = 'Source\KalmalaGameplay\Private\KalmalaWildlifeSpawn.cpp'; Patterns = @('BuildArchetypePresentation', 'Original low-poly silhouettes', 'CreateMeshSection_LinearColor') },
    @{ Label = 'environment'; Path = 'Source\KalmalaWorld\Private\KalmalaGeneratedTerrainPatch.cpp'; Patterns = @('AppendLowPolyRock', 'M_GeneratedTerrain', 'M_GeneratedWater', 'M_GeneratedCanopy') },
    @{ Label = 'hearth'; Path = 'Source\KalmalaGameplay\Private\KalmalaCampfire.cpp'; Patterns = @('Original low polygon stone ring', 'M_GeneratedRock', 'CreateMeshSection_LinearColor') },
    @{ Label = 'ui'; Path = 'Source\KalmalaUI\Private\KalmalaMinimapWidget.cpp'; Patterns = @('CreateTransient', 'UpdateTextureRegions') },
    @{ Label = 'survival-status'; Path = 'Source\KalmalaUI\Private\KalmalaSurvivalStatusWidget.cpp'; Patterns = @('BuildStatusText', 'Source: exposed rain or water', 'Source: prepared food', 'Recovery: shelter or a lit hearth restores warmth', 'SetIsFocusable(false)') },
    @{ Label = 'survival-status-owner'; Path = 'Source\KalmalaUI\Private\KalmalaSurvivalStatusSubsystem.cpp'; Patterns = @('GetLocalPlayer()', 'GetServerWorldTimeSeconds()', 'SetSnapshot', 'AddToPlayerScreen(54)', 'HotbarWidget->SetSnapshot') },
    @{ Label = 'inventory-menu'; Path = 'Source\KalmalaUI\Private\KalmalaInventoryMenuWidget.cpp'; Patterns = @('GetOwningPlayer()', 'FindComponentByClass<UKalmalaInventoryComponent>()', 'GetCarriedToolInventory()', 'Waiting for your pack.') },
    @{ Label = 'feedback-crafting'; Path = 'Source\KalmalaUI\Private\KalmalaCraftingSubsystem.cpp'; Patterns = @('text does not rely on colour', 'Construction feedback: Passed=') },
    @{ Label = 'cooking-recipe-requirements'; Path = 'Source\KalmalaUI\Private\KalmalaRecipeRequirements.cpp'; Patterns = @('Stations[Index] == TEXT("CookingRack")', 'Stations[Index] == TEXT("Cauldron")', 'Stations[Index] == TEXT("FryingPan")', 'Cooking heat: usable lit hearth') },
    @{ Label = 'workbench-craft-context'; Path = 'Source\KalmalaUI\Private\KalmalaCraftingSubsystem.cpp'; Patterns = @('IsStationContextShellKit', 'WorkbenchToolRackKit', 'GetEffectiveStationLevel(Station)', 'Tool Rack', 'Workbench Craft scope:') },
    @{ Label = 'forge-craft-context'; Path = 'Source\KalmalaUI\Private\KalmalaCraftingSubsystem.cpp'; Patterns = @('IsStationContextSectionSupported', 'GetInitialStationContextSection', 'bForgeCraftContext', 'FryingPanKit', 'ForgeAnvilKit', 'GetEffectiveStationLevel(Station)', 'Forge Craft scope:') },
    @{ Label = 'cooking-rack-context'; Path = 'Source\KalmalaUI\Private\KalmalaCraftingSubsystem.cpp'; Patterns = @('CookedBoarMeatRecipe', 'CookedDeerMeatRecipe', 'RequiredStation.Contains(TEXT("CookingRack"))', 'bCookingRackContext', 'GetRecipeAvailability(Recipes[VisibleIndices[Selected]].RecipeId)', 'Hearth heat: %s', 'ServerCraft(Recipe.RecipeId, 1)', 'Cooking Rack scope:') },
    @{ Label = 'forge-repair-context'; Path = 'Source\KalmalaUI\Private\KalmalaCraftingSubsystem.cpp'; Patterns = @('bForgeRepairContext', 'SelectForgeRepairSection', 'WorkbenchRepairInspector->GetSelectedItem()', 'IsStationContextValid()', 'ServerRepairTool(SelectedToolId)', 'Forge Repair scope:') },
    @{ Label = 'workbench-repair-context'; Path = 'Source\KalmalaUI\Private\KalmalaCraftingSubsystem.cpp'; Patterns = @('GetOwningPlayerPawn()', 'GetCarriedToolInventory()', 'WorkbenchRepairInspector->GetSelectedItem()', 'IsStationContextValid()', 'ServerRepairTool(SelectedToolId)', 'Workbench Repair scope:') },
    @{ Label = 'owner-tool-repair'; Path = 'Source\KalmalaGameplay\Private\KalmalaCraftingComponent.cpp'; Patterns = @('LastResult, COND_OwnerOnly', 'ServerRepairTool_Implementation', 'RepairToolFromServer(ToolId, Reason)', 'Need a visible same-world Workbench or Forge within 2.5 m') },
    @{ Label = 'owner-carried-tool-state'; Path = 'Source\KalmalaGameplay\Private\KalmalaCharacter.cpp'; Patterns = @('CarriedTools, COND_OwnerOnly') }
)
$forbiddenPatterns = @('BasicShape', '/Engine/BasicShapes', 'StarterContent', 'Marketplace', 'Quixel', 'ThirdParty')
foreach ($contract in $sourceContracts) {
    $sourcePath = Join-Path $projectRoot $contract.Path
    if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
        throw "Presentation source is missing for $($contract.Label): $($contract.Path)"
    }
    $source = Get-Content -LiteralPath $sourcePath -Raw
    foreach ($pattern in $contract.Patterns) {
        if ($source -notmatch [regex]::Escape($pattern)) {
            throw "Presentation source '$($contract.Label)' is missing the ownership anchor '$pattern'"
        }
    }
    foreach ($forbiddenPattern in $forbiddenPatterns) {
        if ($source -match [regex]::Escape($forbiddenPattern)) {
            throw "Presentation source '$($contract.Label)' contains forbidden asset path/token '$forbiddenPattern'"
        }
    }
    if ($contract.Label -eq 'inventory-menu') {
        foreach ($movedFeedback in @('Attack result:', 'Discovery:', 'SetSupportGlyphState', 'GetSelectedSupportEffect', 'GetFeedbackSerial')) {
            if ($source.Contains($movedFeedback)) { throw "Inventory menu still owns separate HUD feedback: $movedFeedback" }
        }
    }
    if ($contract.Label -in @('feedback-status','accessibility-feedback')) {
        foreach ($duplicateStatusQuery in @('GetRemainingSeconds(', 'GetActiveEffect()', 'Wet: inactive', 'ACTIVE SUPPORT')) {
            if ($source.Contains($duplicateStatusQuery)) { throw "Duplicate active-status presentation in $($contract.Label): $duplicateStatusQuery" }
        }
    }
}

$requiredBoundaryTerms = @(
    'project-owned',
    'presentation-only',
    'server-owned replicated state',
    'not replicated',
    'saved-data schemas'
)
foreach ($term in $requiredBoundaryTerms) {
    if ($document -notmatch [regex]::Escape($term)) {
        throw "Presentation ownership document is missing boundary term: $term"
    }
}

Write-Output "PASS: presentation ownership covers $($requiredAssets.Count) project assets and $($sourceContracts.Count) procedural/UI source seams without external asset paths."
