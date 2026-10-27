param(
    [Parameter(Mandatory)][string]$Project,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe',
    [int]$TimeoutSeconds = 900
)

$ErrorActionPreference = 'Stop'
$projectPath = [IO.Path]::GetFullPath($Project)
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
$runner = Join-Path $PSScriptRoot '..\World\BiomeBaselineCapture.ps1'
$fixedViews = @(
    @{ Name = 'MeadowsDaylight'; Condition = 'Default'; X = -781.6; Y = -781.6; Z = 528.94458 },
    @{ Name = 'ForestShade'; Condition = 'Default'; X = 207132.4; Y = -325940.4; Z = 15.1806488 },
    @{ Name = 'OpenWater'; Condition = 'Default'; X = 970004.9; Y = -243087.4; Z = 0.0 },
    @{ Name = 'OvercastRain'; Condition = 'OvercastRain'; X = -970004.9; Y = -243087.4; Z = 768.734985 },
    @{ Name = 'FireDusk'; Condition = 'FireDusk'; X = -781.6; Y = -781.6; Z = 528.94458 }
)
foreach ($path in @($projectPath, $outputRoot, $Editor, $runner)) {
    if ($path.Length -ge 260) { throw "Path exceeds MAX_PATH: $path" }
}
if (-not (Test-Path -LiteralPath $projectPath -PathType Leaf)) { throw "Project file not found: $projectPath" }
if (-not (Test-Path -LiteralPath $runner -PathType Leaf)) { throw "Capture runner not found: $runner" }
New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
foreach ($view in $fixedViews) {
    $image = Join-Path $outputRoot ($view.Name + '.png')
    if ($image.Length -ge 260) { throw "Path exceeds MAX_PATH: $image" }
    & $runner -Project $projectPath -Map '/Game/Kalmala/Tests/Lighting/L_LightingComparison' -EnableSubstrate `
        -Seed 418 -Site $view.Name -Condition $view.Condition -Xcm $view.X -Ycm $view.Y -ViewZcm $view.Z `
        -OutputPath $image -Editor $Editor -TimeoutSeconds $TimeoutSeconds
}
