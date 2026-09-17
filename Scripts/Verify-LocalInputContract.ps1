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
$requiredMappings = @{
    MoveForward = @('W', 'S', 'Up', 'Down')
    MoveRight = @('D', 'A', 'Right', 'Left')
    Turn = @('MouseX', 'Gamepad_RightX')
    LookUp = @('MouseY', 'Gamepad_RightY')
    MinimapZoom = @('MouseWheelAxis')
}
$requiredActions = @{
    Interact = @('E', 'Gamepad_FaceButton_Bottom')
    Attack = @('LeftMouseButton', 'Gamepad_RightShoulder')
    Jump = @('SpaceBar')
    Sprint = @('LeftShift', 'RightShift')
    SettingsMenu = @('Escape', 'O')
    WorldMap = @('M')
    WorldMapRecenter = @('R')
    CraftMenu = @('B', 'Gamepad_Special_Left')
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

Write-Output "PASS: local input baseline contains $($requiredMappings.Count) axes and $($requiredActions.Count) actions with their documented keyboard/controller bindings."
