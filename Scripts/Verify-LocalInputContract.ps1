[CmdletBinding()]
param(
    [string]$InputFile = ''
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
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
    SupportSelectMending = @('One', 'Gamepad_DPad_Up')
    SupportSelectHearthShield = @('Two', 'Gamepad_DPad_Right')
    SupportSelectBearsVigor = @('Three', 'Gamepad_DPad_Down')
    SupportSelectDeerCall = @('Four', 'Gamepad_DPad_Left')
    SupportActivate = @('Q', 'Gamepad_FaceButton_Top')
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
foreach ($binding in @(
    'SupportSelectMending', 'SupportSelectHearthShield', 'SupportSelectBearsVigor',
    'SupportSelectDeerCall', 'SupportActivate'
)) {
    if ($characterSource -notmatch ('BindAction\(TEXT\("' + [regex]::Escape($binding) + '"\)')) {
        throw "Support input action is configured but not bound by the character: $binding"
    }
}

Write-Output "PASS: local input baseline contains $($requiredMappings.Count) axes and $($requiredActions.Count) actions with their documented keyboard/controller bindings."
