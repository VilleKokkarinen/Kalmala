[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$sourcePath = Join-Path $projectRoot 'Source\KalmalaUI\Private\KalmalaTutorialSubsystem.cpp'
$headerPath = Join-Path $projectRoot 'Source\KalmalaUI\Public\KalmalaTutorialSubsystem.h'
foreach ($path in @($sourcePath, $headerPath)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Tutorial presenter source was not found: $path"
    }
}

$source = Get-Content -LiteralPath $sourcePath -Raw
$header = Get-Content -LiteralPath $headerPath -Raw

function Get-FunctionBody {
    param(
        [Parameter(Mandatory)] [string] $Text,
        [Parameter(Mandatory)] [string] $Name
    )

    $marker = "UKalmalaTutorialSubsystem::$Name("
    $start = $Text.IndexOf($marker, [StringComparison]::Ordinal)
    if ($start -lt 0) { throw "Tutorial presenter function was not found: $Name" }
    $open = $Text.IndexOf('{', $start)
    if ($open -lt 0) { throw "Tutorial presenter function has no body: $Name" }

    $depth = 0
    for ($index = $open; $index -lt $Text.Length; $index++) {
        if ($Text[$index] -eq '{') { $depth++ }
        elseif ($Text[$index] -eq '}') {
            $depth--
            if ($depth -eq 0) { return $Text.Substring($open, $index - $open + 1) }
        }
    }
    throw "Tutorial presenter function body is unbalanced: $Name"
}

function Assert-Contains {
    param([string] $Text, [string] $Pattern, [string] $Requirement)
    if ($Text -notmatch $Pattern) { throw "Route-free tutorial check failed: $Requirement" }
}

function Assert-NotContains {
    param([string] $Text, [string] $Pattern, [string] $Requirement)
    if ($Text -match $Pattern) { throw "Route-free tutorial check failed: $Requirement" }
}

$tick = Get-FunctionBody $source 'Tick'
$focus = Get-FunctionBody $source 'UpdateVisibleFocus'
$choose = Get-FunctionBody $source 'FindAvailableBeat'
$context = Get-FunctionBody $source 'IsBeatContextActive'
$body = Get-FunctionBody $source 'BuildBody'

Assert-Contains $header 'class KALMALAUI_API UKalmalaTutorialSubsystem\s*:\s*public ULocalPlayerSubsystem' 'prompts must remain local-player UI state'
Assert-Contains $tick 'World->IsGameWorld\(\)' 'presenter must run during ordinary gameplay'
Assert-Contains $tick 'Controller->IsLocalController\(\)' 'presenter must bind only to the owning local controller'
Assert-Contains $tick 'FindAvailableBeat\(Pawn\)' 'normal possession must choose prompts without a tutorial-enable command'
Assert-Contains $choose 'ChooseUnseen\(EKalmalaTutorialBeat::Arrive\)' 'a fresh possessed pawn must receive arrival guidance before other context'
if ($choose.IndexOf('ChooseUnseen(EKalmalaTutorialBeat::Arrive)', [StringComparison]::Ordinal) -gt $choose.IndexOf('const AActor* FocusActor', [StringComparison]::Ordinal)) {
    throw 'Route-free tutorial check failed: arrival guidance is ordered behind a contextual prerequisite'
}

Assert-Contains $focus 'GetPlayerViewPoint' 'world actors may only be considered through the local visible view'
Assert-Contains $focus 'LineTraceSingleByChannel' 'contextual actors must come from a local visibility trace'
Assert-Contains $focus 'VisibleFocusActor = Actor' 'the visibility trace must be the source of contextual actors'
Assert-NotContains ($focus + $choose) 'TActorIterator|GetAllActorsOfClass|GetActorsOfClass' 'presenter must not enumerate hidden world content'

foreach ($requirement in @(
    @{ Pattern = 'AKalmalaDiscoveryActor'; Name = 'visible discovery context' },
    @{ Pattern = 'AKalmalaHarvestNode'; Name = 'visible gathering context' },
    @{ Pattern = 'AKalmalaWildlifeSpawn'; Name = 'visible optional encounter context' },
    @{ Pattern = 'UKalmalaCraftingSubsystem'; Name = 'player-opened preparation context' },
    @{ Pattern = 'WetStatusId'; Name = 'existing readable weather-status context' },
    @{ Pattern = 'SupportMagicComponent'; Name = 'already learned support context' }
)) {
    Assert-Contains $choose $requirement.Pattern $requirement.Name
}

Assert-Contains $choose 'Cast<AKalmalaCampfire>\(FocusActor\)' 'a return prompt may appear only for a currently visible campfire'
if ([regex]::Matches($choose, 'AKalmalaCampfire').Count -ne 1) {
    throw 'Route-free tutorial check failed: campfire context must not gate other prompts'
}
Assert-Contains $choose 'InitialPawnLocation' 'exploration guidance must use local movement from the fresh pawn position'
Assert-Contains $choose 'ExplorationDistance' 'exploration guidance must have a local movement threshold'
Assert-Contains $context 'FVector::DistSquared2D\(Pawn->GetActorLocation\(\), InitialPawnLocation\)' 'exploration context must not select a fixed direction or destination'
Assert-Contains $choose 'return ChooseUnseen\(EKalmalaTutorialBeat::Explore\)' 'movement may prompt exploration without a camp or quest completion'
Assert-Contains $body 'Pick a heading' 'exploration text must leave direction to the player'
Assert-Contains $body 'not a required route' 'exploration text must explicitly reject a prescribed route'
Assert-Contains $body 'You can engage or move on' 'encounters must remain optional'
Assert-Contains $body 'keep exploring' 'a visible camp prompt must preserve the option to continue exploring'
Assert-NotContains ($choose + $context) '\bQuest\b|\bMission\b|\bObjective\b|OpenLevel|ClientTravel|ServerTravel' 'prompt selection must not depend on quest progression or select a destination'
Assert-NotContains ($source + $header) 'FParse::|FCommandLine::|GetCommandLine|KalmalaTutorial\w*Test|bEnableTutorial|UFUNCTION\s*\(\s*Server|SaveGame|Serialize\(' 'tutorial prompts must not require a developer flag, RPC, or gameplay save'

Write-Output 'PASS: tutorial beats are local and opportunistic; visible context and player movement drive them, with no route, quest, camp prerequisite, hidden-content scan, or tutorial launch flag.'
