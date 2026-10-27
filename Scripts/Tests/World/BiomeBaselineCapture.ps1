param(
    [Parameter(Mandatory)][string]$Project,
    [Parameter(Mandatory)][ValidateSet(418, 419)][int]$Seed,
    [Parameter(Mandatory)][string]$Site,
    [ValidateSet('Default', 'Dry', 'RainOnly', 'OvercastOnly', 'OvercastRain', 'WeatherTransition', 'FireDusk')][string]$Condition = 'Default',
    [Parameter(Mandatory)][double]$Xcm,
    [Parameter(Mandatory)][double]$Ycm,
    [Parameter(Mandatory)][double]$ViewZcm,
    [Parameter(Mandatory)][string]$OutputPath,
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [int]$TimeoutSeconds = 900
)

$ErrorActionPreference = 'Stop'
$culture = [Globalization.CultureInfo]::InvariantCulture
$projectPath = [IO.Path]::GetFullPath($Project)
$imagePath = [IO.Path]::GetFullPath($OutputPath)
$outputDirectory = Split-Path -Parent $imagePath
$logPath = [IO.Path]::ChangeExtension($imagePath, '.log')
$userDirectory = Join-Path $outputDirectory 'u'
if ($Condition -eq 'WeatherTransition') {
    $captureBasePath = Join-Path $outputDirectory ([IO.Path]::GetFileNameWithoutExtension($imagePath))
    $capturePaths = @(
        "$($captureBasePath)_OnsetStart.png",
        "$($captureBasePath)_OnsetEnd.png",
        "$($captureBasePath)_ReleaseStart.png",
        "$($captureBasePath)_ReleaseEnd.png"
    )
}
else {
    $capturePaths = @($imagePath)
}
$paths = @($projectPath, $Editor, $imagePath, $logPath, $userDirectory) + $capturePaths
foreach ($path in $paths) {
    if ($path.Length -ge 260) { throw "Path exceeds MAX_PATH: $path" }
}
if (-not (Test-Path -LiteralPath $projectPath -PathType Leaf)) { throw "Project file not found: $projectPath" }
if (-not (Test-Path -LiteralPath $Editor -PathType Leaf)) { throw "Unreal Editor not found: $Editor" }
foreach ($capturePath in $capturePaths) {
    if (Test-Path -LiteralPath $capturePath) { throw "Refusing to overwrite existing capture: $capturePath" }
}
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
New-Item -ItemType Directory -Path $userDirectory -Force | Out-Null

$x = $Xcm.ToString('R', $culture)
$y = $Ycm.ToString('R', $culture)
$z = $ViewZcm.ToString('R', $culture)
$arguments = @(
    "`"$projectPath`"",
    '/Game/Kalmala/Maps/Prototype/L_Prototype',
    '-game', '-windowed', '-RenderOffscreen', '-ForceRes', '-ResX=1024', '-ResY=768',
    '-nosound', '-unattended', '-nosplash', '-NoZenAutoLaunch', '-DDC=NoZenLocalFallback',
    "-WorldSeed=$Seed", '-KalmalaBiomeBaselineCapture', "-KalmalaBiomeBaselineSite=$Site",
    "-KalmalaBiomeBaselineXcm=$x", "-KalmalaBiomeBaselineYcm=$y", "-KalmalaBiomeBaselineViewZcm=$z",
    "-KalmalaBiomeBaselineCondition=$Condition",
    "-KalmalaBiomeBaselineOutput=`"$imagePath`"", "-abslog=`"$logPath`"", "-UserDir=`"$userDirectory`""
)
$process = $null
try {
    $process = Start-Process -FilePath $Editor -ArgumentList ($arguments -join ' ') -PassThru -WindowStyle Hidden
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    foreach ($capturePath in $capturePaths) {
        $lastLength = -1L
        $stableChecks = 0
        while ((Get-Date) -lt $deadline) {
            if (Test-Path -LiteralPath $capturePath) {
                $length = (Get-Item -LiteralPath $capturePath).Length
                if ($length -gt 1000 -and $length -eq $lastLength) { $stableChecks++ } else { $stableChecks = 0 }
                if ($stableChecks -ge 2) { break }
                $lastLength = $length
            }
            elseif ($process.HasExited) {
                $tail = if (Test-Path -LiteralPath $logPath) { (Get-Content -LiteralPath $logPath -Tail 30) -join [Environment]::NewLine } else { '(no log created)' }
                throw "Game exited before producing rendered capture $capturePath (exit $($process.ExitCode)). $tail"
            }
            Start-Sleep -Milliseconds 500
        }
        if (-not (Test-Path -LiteralPath $capturePath) -or $stableChecks -lt 2) {
            $tail = if (Test-Path -LiteralPath $logPath) { (Get-Content -LiteralPath $logPath -Tail 40) -join [Environment]::NewLine } else { '(no log created)' }
            throw "Timed out waiting for stable rendered capture $capturePath. $tail"
        }
    }
    Add-Type -AssemblyName System.Drawing
    $captureSizes = @()
    foreach ($capturePath in $capturePaths) {
        $bytes = [IO.File]::ReadAllBytes($capturePath)
        if ($bytes.Length -lt 8 -or [BitConverter]::ToString($bytes[0..7]) -ne '89-50-4E-47-0D-0A-1A-0A') {
            throw "Capture is not a PNG: $capturePath"
        }
        $bitmap = [Drawing.Bitmap]::FromFile($capturePath)
        try {
            if ($bitmap.Width -ne 1024 -or $bitmap.Height -ne 768) { throw "Unexpected capture dimensions: $($bitmap.Width)x$($bitmap.Height)" }
            $colors = [Collections.Generic.HashSet[int]]::new()
            $visibleSamples = 0
            for ($y = 8; $y -lt $bitmap.Height; $y += 32) {
                for ($x = 8; $x -lt $bitmap.Width; $x += 32) {
                    $pixel = $bitmap.GetPixel($x, $y)
                    [void]$colors.Add($pixel.ToArgb())
                    if ($pixel.R -gt 8 -or $pixel.G -gt 8 -or $pixel.B -gt 8) { $visibleSamples++ }
                }
            }
            if ($colors.Count -lt 8 -or $visibleSamples -lt 40) { throw "Rendered capture appears blank or uniform: $capturePath Colors=$($colors.Count) NonBlackSamples=$visibleSamples" }
        }
        finally { $bitmap.Dispose() }
        $captureSizes += $bytes.Length
    }
    $logText = Get-Content -LiteralPath $logPath -Raw
    if ($logText -notmatch 'Biome baseline rendered capture requested: Seed=' + $Seed + ' Site=' + [regex]::Escape($Site) + ' Condition=' + [regex]::Escape($Condition) + '\b') {
        throw "Runtime did not log the requested seed/site/condition capture: $logPath"
    }
    [pscustomobject]@{ Seed = $Seed; Site = $Site; Output = ($capturePaths -join ';'); Bytes = ($captureSizes -join ','); Log = $logPath }
}
finally {
    if ($null -ne $process -and -not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
        $process.WaitForExit()
    }
}
