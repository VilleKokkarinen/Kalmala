param(
    [Parameter(Mandatory)][string]$Project,
    [string]$Map = '/Game/Kalmala/Maps/Prototype/L_Prototype',
    [switch]$EnableSubstrate,
    [switch]$ComparisonFixture,
    [switch]$SkyOnlyFill,
    [Parameter(Mandatory)][ValidateSet(418, 419)][int]$Seed,
    [Parameter(Mandatory)][string]$Site,
    [ValidateSet('Both', 'DefaultLit', 'Toon')][string]$LightingVariant = 'Both',
    [ValidateRange(1, 12)][int]$TemporalFrames = 1,
    [ValidateRange(0.1, 2.0)][double]$TemporalIntervalSeconds = 0.5,
    [ValidateRange(0, 1200)][int]$GpuCaptureFrames = 0,
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
    if ($LightingVariant -ne 'Both' -or $TemporalFrames -ne 1 -or $GpuCaptureFrames -gt 0 -or $ComparisonFixture) {
        throw 'WeatherTransition requires one ordinary capture, without a comparison fixture or GPU profile.'
    }
    $captureBasePath = Join-Path $outputDirectory ([IO.Path]::GetFileNameWithoutExtension($imagePath))
    $capturePaths = @(
        ($captureBasePath + '_OnsetStart.png'),
        ($captureBasePath + '_OnsetEnd.png'),
        ($captureBasePath + '_ReleaseStart.png'),
        ($captureBasePath + '_ReleaseEnd.png')
    )
}
else { $capturePaths = @($imagePath) }
$imagePaths = @($capturePaths)
if ($Condition -ne 'WeatherTransition') {
    for ($frame = 1; $frame -lt $TemporalFrames; $frame++) {
        $imagePaths += Join-Path $outputDirectory ("{0}_T{1:D2}{2}" -f [IO.Path]::GetFileNameWithoutExtension($imagePath), $frame, [IO.Path]::GetExtension($imagePath))
    }
}
$csvPath = Join-Path $outputDirectory ("L03_GPU_{0}_{1}_{2}.csv" -f $Site, $Condition, $LightingVariant)
$paths = @($projectPath, $Editor, $imagePath, $logPath, $userDirectory) + $imagePaths
if ($GpuCaptureFrames -gt 0) { $paths += $csvPath }
foreach ($path in $paths) {
    if ($path.Length -ge 260) { throw "Path exceeds MAX_PATH: $path" }
}
if ($SkyOnlyFill -and (-not $ComparisonFixture -or $Site -ne 'ForestShade' -or $Condition -ne 'Default' -or $LightingVariant -eq 'Both' -or $TemporalFrames -ne 1 -or $GpuCaptureFrames -gt 0)) { throw 'SkyOnlyFill requires one isolated ForestShade comparison-fixture capture with default conditions and no GPU profile.' }
if ($Map -notmatch '^/Game/[A-Za-z0-9_/]+$') { throw "Invalid project map package: $Map" }
if (-not (Test-Path -LiteralPath $projectPath -PathType Leaf)) { throw "Project file not found: $projectPath" }
if (-not (Test-Path -LiteralPath $Editor -PathType Leaf)) { throw "Unreal Editor not found: $Editor" }
foreach ($path in $imagePaths) {
    if (Test-Path -LiteralPath $path) { throw "Refusing to overwrite existing capture: $path" }
}
if ($Condition -eq 'WeatherTransition' -and (Test-Path -LiteralPath $imagePath)) { throw "Refusing to overwrite existing capture: $imagePath" }
if ($GpuCaptureFrames -gt 0 -and (Test-Path -LiteralPath $csvPath)) { throw "Refusing to overwrite existing GPU CSV capture: $csvPath" }
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
New-Item -ItemType Directory -Path $userDirectory -Force | Out-Null

$x = $Xcm.ToString('R', $culture)
$y = $Ycm.ToString('R', $culture)
$z = $ViewZcm.ToString('R', $culture)
$arguments = @(
    "`"$projectPath`"",
    $Map,
    '-game', '-windowed', '-RenderOffscreen', '-ForceRes', '-ResX=1024', '-ResY=768',
    '-nosound', '-unattended', '-nosplash', '-NoZenAutoLaunch', '-DDC=NoZenLocalFallback',
    "-WorldSeed=$Seed", '-KalmalaBiomeBaselineCapture', "-KalmalaBiomeBaselineSite=$Site",
    "-KalmalaBiomeBaselineXcm=$x", "-KalmalaBiomeBaselineYcm=$y", "-KalmalaBiomeBaselineViewZcm=$z",
    "-KalmalaBiomeBaselineCondition=$Condition", "-KalmalaBiomeBaselineLightingVariant=$LightingVariant",
    "-KalmalaBiomeBaselineTemporalFrames=$TemporalFrames",
    "-KalmalaBiomeBaselineTemporalIntervalSeconds=$($TemporalIntervalSeconds.ToString('R', $culture))",
    "-KalmalaBiomeBaselineOutput=`"$imagePath`"", "-abslog=`"$logPath`"", "-UserDir=`"$userDirectory`""
)
if ($EnableSubstrate) { $arguments += '-ini:Engine:[/Script/Engine.RendererSettings]:bEnableSubstrate=True' }
if ($ComparisonFixture) { $arguments += '-KalmalaBiomeBaselineComparisonFixture' }
if ($SkyOnlyFill) { $arguments += '-KalmalaBiomeBaselineSkyOnlyFill' }
if ($GpuCaptureFrames -gt 0) {
    $arguments += '-csvGpuStats'
    $arguments += "-KalmalaBiomeBaselineGpuFrames=$GpuCaptureFrames"
}

$process = $null
try {
    $process = Start-Process -FilePath $Editor -ArgumentList ($arguments -join ' ') -PassThru -WindowStyle Hidden
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $lastLength = -1L
    $stableChecks = @{}
    $lastLengths = @{}
    foreach ($path in $imagePaths) { $stableChecks[$path] = 0; $lastLengths[$path] = -1L }
    $requiredPaths = @($imagePaths)
    if ($GpuCaptureFrames -gt 0) {
        $stableChecks[$csvPath] = 0
        $lastLengths[$csvPath] = -1L
        $requiredPaths += $csvPath
    }
    while ((Get-Date) -lt $deadline) {
        if ($process.HasExited) {
            $tail = if (Test-Path -LiteralPath $logPath) { (Get-Content -LiteralPath $logPath -Tail 30) -join "`n" } else { '(no log created)' }
            throw "Game exited before producing a rendered capture (exit $($process.ExitCode)).`n$tail"
        }
        foreach ($path in $requiredPaths) {
            if (Test-Path -LiteralPath $path) {
                $length = (Get-Item -LiteralPath $path).Length
                $minimumBytes = if ($path -eq $csvPath) { 100 } else { 1000 }
                if ($length -gt $minimumBytes -and $length -eq $lastLengths[$path]) { $stableChecks[$path]++ } else { $stableChecks[$path] = 0 }
                $lastLengths[$path] = $length
            }
        }
        if (($stableChecks.Values | Where-Object { $_ -lt 2 }).Count -eq 0) { break }
        Start-Sleep -Milliseconds 500
    }
    if (($stableChecks.Values | Where-Object { $_ -lt 2 }).Count -gt 0) {
        $tail = if (Test-Path -LiteralPath $logPath) { (Get-Content -LiteralPath $logPath -Tail 40) -join "`n" } else { '(no log created)' }
        throw "Timed out waiting for stable rendered frames or GPU CSV data.`n$tail"
    }
    Add-Type -AssemblyName System.Drawing
    $captureSizes = @()
    foreach ($path in $imagePaths) {
        $bytes = [IO.File]::ReadAllBytes($path)
        if ($bytes.Length -lt 8 -or [BitConverter]::ToString($bytes[0..7]) -ne '89-50-4E-47-0D-0A-1A-0A') {
            throw "Capture is not a PNG: $path"
        }
        $bitmap = [Drawing.Bitmap]::FromFile($path)
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
            $minimumVisibleSamples = if ($SkyOnlyFill) { 5 } else { 40 }
            if ($colors.Count -lt 8 -or $visibleSamples -lt $minimumVisibleSamples) { throw "Rendered capture appears blank or uniform: Colors=$($colors.Count) NonBlackSamples=$visibleSamples Minimum=$minimumVisibleSamples Path=$path" }
        }
        finally { $bitmap.Dispose() }
        $captureSizes += $bytes.Length
    }
    $logText = Get-Content -LiteralPath $logPath -Raw
    if ($logText -notmatch 'Biome baseline rendered capture requested: Seed=' + $Seed + ' Site=' + [regex]::Escape($Site) + ' Condition=' + [regex]::Escape($Condition) + '\b') {
        throw "Runtime did not log the requested seed/site/condition capture: $logPath"
    }
    if ($SkyOnlyFill -and $logText -notmatch 'L03 fixture neutral-cubemap fill capture: SunIntensity=0\.000 SkyIntensity=.* Source=Engine GrayTextureCube') {
        throw "Runtime did not confirm the isolated zero-sun/skylight capture: $logPath"
    }
    $gpuSummary = $null
    if ($GpuCaptureFrames -gt 0) {
        $rows = @(Import-Csv -LiteralPath $csvPath)
        if ($rows.Count -eq 0) { throw "GPU CSV has no data rows: $csvPath" }
        $gpuFrameColumns = @($rows[0].PSObject.Properties.Name | Where-Object { $_ -eq 'GPUTime' })
        if ($gpuFrameColumns.Count -ne 1) { throw "GPU CSV lacks UE's total frame GPUTime metric: $csvPath" }
        $gpuFrameValues = @()
        $queueUnaccountedColumns = @($rows[0].PSObject.Properties.Name | Where-Object { $_ -match 'Unaccounted' })
        $queueUnaccountedValues = @()
        foreach ($row in $rows) {
            foreach ($column in $gpuFrameColumns) {
                $value = 0.0
                if ([double]::TryParse([string]$row.$column, [Globalization.NumberStyles]::Float, $culture, [ref]$value) -and $value -gt 0.0) { $gpuFrameValues += $value }
            }
            foreach ($column in $queueUnaccountedColumns) {
                $value = 0.0
                if ([double]::TryParse([string]$row.$column, [Globalization.NumberStyles]::Float, $culture, [ref]$value) -and $value -gt 0.0) { $queueUnaccountedValues += $value }
            }
        }
        if ($gpuFrameValues.Count -eq 0) { throw "GPU CSV has no positive total GPUTime samples: $csvPath" }
        $gpuStats = $gpuFrameValues | Measure-Object -Average -Minimum -Maximum
        $queueStats = if ($queueUnaccountedValues.Count -gt 0) { $queueUnaccountedValues | Measure-Object -Average -Minimum -Maximum } else { $null }
        $gpuSummary = [pscustomobject]@{
            Csv = $csvPath
            Frames = $gpuFrameValues.Count
            MeanGpuFrameMs = [math]::Round($gpuStats.Average, 3)
            MinGpuFrameMs = [math]::Round($gpuStats.Minimum, 3)
            MaxGpuFrameMs = [math]::Round($gpuStats.Maximum, 3)
            QueueUnaccountedMeanMs = if ($null -ne $queueStats) { [math]::Round($queueStats.Average, 3) } else { $null }
        }
    }
    [pscustomobject]@{ Seed = $Seed; Site = $Site; Condition = $Condition; LightingVariant = $LightingVariant; Map = $Map; Outputs = $imagePaths; CaptureBytes = $captureSizes; Gpu = $gpuSummary; Log = $logPath }
}
finally {
    if ($null -ne $process -and -not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
        $process.WaitForExit()
    }
}
