[CmdletBinding()]
param(
    [string]$InputFile = ''
)
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot


$ErrorActionPreference = 'Stop'
$projectRoot = $repoPaths.ProjectRoot
if ([string]::IsNullOrWhiteSpace($InputFile)) {
    $InputFile = Join-Path $projectRoot 'Config\DefaultInput.ini'
}

if (-not (Test-Path -LiteralPath $InputFile -PathType Leaf)) {
    throw "Input configuration was not found: $InputFile"
}

$text = Get-Content -LiteralPath $InputFile -Raw
if ($text -match 'ActionName="TutorialPrompt(Dismiss|Revisit)"') {
    throw 'Retired tutorial card dismiss/revisit bindings remain in the default input configuration.'
}
foreach ($retiredSupportAction in @(
    'SupportSelectMending', 'SupportSelectHearthShield', 'SupportSelectBearsVigor',
    'SupportSelectDeerCall', 'SupportActivate'
)) {
    if ($text -match ('(?:AxisName|ActionName)="' + [regex]::Escape($retiredSupportAction) + '"')) {
        throw "Retired support selection input remains in the default input configuration: $retiredSupportAction"
    }
}
$requiredMappings = @{
    MoveForward = @('W', 'S', 'Gamepad_LeftY', 'Up', 'Down')
    MoveRight = @('D', 'A', 'Gamepad_LeftX', 'Right', 'Left')
    Turn = @('MouseX', 'Gamepad_RightX')
    LookUp = @('MouseY', 'Gamepad_RightY')
    MinimapZoom = @('MouseWheelAxis')
}
$requiredActions = @{
    Interact = @('E', 'Gamepad_FaceButton_Bottom')
    Attack = @('LeftMouseButton', 'Gamepad_RightShoulder')
    Jump = @('SpaceBar', 'Gamepad_FaceButton_Left')
    Sprint = @('LeftShift', 'RightShift', 'Gamepad_LeftThumbstick')
    SettingsMenu = @('Escape', 'O')
    WorldMap = @('M')
    WorldMapRecenter = @('R')
    InventoryMenu = @('Tab', 'I')
    CraftMenu = @('B', 'Gamepad_Special_Left')
    Hotbar1 = @('One')
    Hotbar2 = @('Two')
    Hotbar3 = @('Three')
    Hotbar4 = @('Four')
    Hotbar5 = @('Five')
    Hotbar6 = @('Six')
    Hotbar7 = @('Seven')
    Hotbar8 = @('Eight')
    Hotbar9 = @('Nine')
    Hotbar0 = @('Zero')
}

foreach ($mapping in $requiredMappings.GetEnumerator()) {
    foreach ($key in $mapping.Value) {
        $pattern = 'AxisName="' + [regex]::Escape($mapping.Key) + '",Key=' + [regex]::Escape($key)
        if ($text -notmatch $pattern) {
            throw "Missing axis baseline: $($mapping.Key) -> $key"
        }
    }
}
foreach ($action in $requiredActions.GetEnumerator()) {
    foreach ($key in $action.Value) {
        $pattern = 'ActionName="' + [regex]::Escape($action.Key) + '"[^\r\n]*Key=' + [regex]::Escape($key)
        if ($text -notmatch $pattern) {
            throw "Missing action baseline: $($action.Key) -> $key"
        }
    }
}

if ($text -notmatch 'DefaultInputComponentClass=/Script/EnhancedInput.EnhancedInputComponent') {
    throw 'Enhanced Input component baseline is missing'
}

$characterSourcePath = Join-Path $projectRoot 'Source\KalmalaGameplay\Private\KalmalaCharacter.cpp'
$characterSource = Get-Content -LiteralPath $characterSourcePath -Raw
if ($characterSource -match 'BindAction\(TEXT\("Support(?:SelectMending|SelectHearthShield|SelectBearsVigor|SelectDeerCall|Activate)"\)') {
    throw 'Retired support selection action is still bound by the character.'
}

Write-Output "PASS: local input baseline contains $($requiredMappings.Count) axes and $($requiredActions.Count) actions; retired support selection bindings are absent."
