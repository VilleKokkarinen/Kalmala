[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
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
    @{ Label = 'player'; Path = 'Source\KalmalaGameplay\Private\KalmalaPlayerModelComponent.cpp'; Patterns = @('M_GeneratedTerrain', 'bTapered', 'CreateMeshSection_LinearColor') },
    @{ Label = 'wildlife'; Path = 'Source\KalmalaGameplay\Private\KalmalaWildlifeSpawn.cpp'; Patterns = @('BuildArchetypePresentation', 'Original low-poly silhouettes', 'CreateMeshSection_LinearColor') },
    @{ Label = 'environment'; Path = 'Source\KalmalaWorld\Private\KalmalaGeneratedTerrainPatch.cpp'; Patterns = @('AppendLowPolyRock', 'M_GeneratedTerrain', 'M_GeneratedWater', 'M_GeneratedCanopy') },
    @{ Label = 'hearth'; Path = 'Source\KalmalaGameplay\Private\KalmalaCampfire.cpp'; Patterns = @('Original low polygon stone ring', 'M_GeneratedRock', 'CreateMeshSection_LinearColor') },
    @{ Label = 'ui'; Path = 'Source\KalmalaUI\Private\KalmalaMinimapWidget.cpp'; Patterns = @('CreateTransient', 'UpdateTextureRegions') },
    @{ Label = 'feedback-status'; Path = 'Source\KalmalaUI\Private\KalmalaInventorySubsystem.cpp'; Patterns = @('Attack result:', 'Discovery:', 'Wet: inactive', 'UKalmalaSupportGlyphWidget', 'SupportGlyphRow', 'SetSupportGlyphState', 'EKalmalaSupportGlyph::DeerCall', 'HasLearnedEffect', 'GetSelectedSupportEffect') },
    @{ Label = 'feedback-crafting'; Path = 'Source\KalmalaUI\Private\KalmalaCraftingSubsystem.cpp'; Patterns = @('text does not rely on colour', 'Construction feedback: Passed=') }
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
