param(
    [Parameter(Mandatory)][string]$Project,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe',
    [int]$TimeoutSeconds = 900,
    [switch]$AnalyzeOnly
)

$ErrorActionPreference = 'Stop'
[Threading.Thread]::CurrentThread.CurrentCulture = [Globalization.CultureInfo]::InvariantCulture
[Threading.Thread]::CurrentThread.CurrentUICulture = [Globalization.CultureInfo]::InvariantCulture
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
$gpuFrames = 120
$temporalFrames = 8
$gpuResults = @()
$temporalResults = @()
Add-Type -AssemblyName System.Drawing
function Get-TemporalMeanAbsoluteRgbDifference([string]$BeforePath, [string]$AfterPath) {
    $before = [Drawing.Bitmap]::FromFile($BeforePath)
    $after = [Drawing.Bitmap]::FromFile($AfterPath)
    try {
        if ($before.Width -ne $after.Width -or $before.Height -ne $after.Height) { throw "Temporal frame dimensions differ: $BeforePath / $AfterPath" }
        [long]$difference = 0
        [long]$count = 0
        for ($y = 8; $y -lt $before.Height; $y += 16) {
            for ($x = 8; $x -lt $before.Width; $x += 16) {
                $a = $before.GetPixel($x, $y)
                $b = $after.GetPixel($x, $y)
                $difference += [math]::Abs($a.R - $b.R) + [math]::Abs($a.G - $b.G) + [math]::Abs($a.B - $b.B)
                $count += 3
            }
        }
        return [math]::Round($difference / [double]$count, 4)
    }
    finally { $before.Dispose(); $after.Dispose() }
}
if (-not $AnalyzeOnly) {
foreach ($view in $fixedViews) {
    $image = Join-Path $outputRoot ($view.Name + '.png')
    if ($image.Length -ge 260) { throw "Path exceeds MAX_PATH: $image" }
    $framePaths = @($image)
    for ($frame = 1; $frame -lt $temporalFrames; $frame++) {
        $framePath = Join-Path $outputRoot ("{0}_T{1:D2}.png" -f $view.Name, $frame)
        if ($framePath.Length -ge 260) { throw "Path exceeds MAX_PATH: $framePath" }
        $framePaths += $framePath
    }
    & $runner -Project $projectPath -Map '/Game/Kalmala/Tests/Lighting/L_LightingComparison' -EnableSubstrate -ComparisonFixture `
        -Seed 418 -Site $view.Name -Condition $view.Condition -Xcm $view.X -Ycm $view.Y -ViewZcm $view.Z `
        -LightingVariant Both -TemporalFrames $temporalFrames -TemporalIntervalSeconds 0.5 `
        -OutputPath $image -Editor $Editor -TimeoutSeconds $TimeoutSeconds
    $frameDeltas = @()
    for ($frame = 1; $frame -lt $framePaths.Count; $frame++) {
        $frameDeltas += Get-TemporalMeanAbsoluteRgbDifference $framePaths[$frame - 1] $framePaths[$frame]
    }
    $temporalResults += [pscustomobject]@{
        Site = $view.Name
        Condition = $view.Condition
        FrameCount = $framePaths.Count
        FrameIntervalSeconds = 0.5
        MeanAdjacentFrameAbsoluteRgbChange = [math]::Round(($frameDeltas | Measure-Object -Average).Average, 4)
        MaxAdjacentFrameAbsoluteRgbChange = [math]::Round(($frameDeltas | Measure-Object -Maximum).Maximum, 4)
    }

    foreach ($variant in @('DefaultLit', 'Toon')) {
        $costImage = Join-Path $outputRoot ($view.Name + '_' + $variant + '.png')
        if ($costImage.Length -ge 260) { throw "Path exceeds MAX_PATH: $costImage" }
        $result = & $runner -Project $projectPath -Map '/Game/Kalmala/Tests/Lighting/L_LightingComparison' -EnableSubstrate -ComparisonFixture `
            -Seed 418 -Site $view.Name -Condition $view.Condition -Xcm $view.X -Ycm $view.Y -ViewZcm $view.Z `
            -LightingVariant $variant -TemporalFrames 1 -GpuCaptureFrames $gpuFrames `
            -OutputPath $costImage -Editor $Editor -TimeoutSeconds $TimeoutSeconds
        $gpuResults += [pscustomobject]@{
            Site = $view.Name
            Condition = $view.Condition
            LightingVariant = $variant
            Frames = $result.Gpu.Frames
            MeanGpuFrameMs = $result.Gpu.MeanGpuFrameMs
            MinGpuFrameMs = $result.Gpu.MinGpuFrameMs
            MaxGpuFrameMs = $result.Gpu.MaxGpuFrameMs
            QueueUnaccountedMeanMs = $result.Gpu.QueueUnaccountedMeanMs
            Csv = $result.Gpu.Csv
        }
    }
}
}
$sampleTemporalResults = @()
$sampleDefinitions = @(
    @{ Name = 'Ground'; WidthCm = 120.0; HeightCm = 100.0; HalfHeightCm = 1.0 },
    @{ Name = 'Rock'; WidthCm = 116.0; HeightCm = 96.0; HalfHeightCm = 24.0 },
    @{ Name = 'Wood'; WidthCm = 50.0; HeightCm = 164.0; HalfHeightCm = 41.0 },
    @{ Name = 'Canopy'; WidthCm = 144.0; HeightCm = 124.0; HalfHeightCm = 31.0 },
    @{ Name = 'Creature'; WidthCm = 84.0; HeightCm = 116.0; HalfHeightCm = 29.0 },
    @{ Name = 'Construction'; WidthCm = 116.0; HeightCm = 116.0; HalfHeightCm = 29.0 }
)
function Get-ProjectedSampleRegion($View, [int]$SampleIndex, [string]$Variant) {
    $sample = $sampleDefinitions[$SampleIndex]
    $row = [int][math]::Floor($SampleIndex / 2.0)
    $column = $SampleIndex % 2
    $groupX = $View.X + $(if ($column -eq 0) { -190.0 } else { 190.0 })
    $sampleX = $groupX + $(if ($Variant -eq 'DefaultLit') { -65.0 } else { 65.0 })
    $sampleY = $View.Y + $row * 200.0
    $sampleZ = $View.Z + $sample.HalfHeightCm
    $cameraDistance = if ($View.Condition -eq 'FireDusk') { 500.0 } else { 1000.0 }
    $cameraHeight = if ($View.Condition -eq 'FireDusk') { 250.0 } else { 500.0 }
    $aimHeight = if ($View.Condition -eq 'FireDusk') { 80.0 } else { 150.0 }
    $cameraX = $View.X
    $cameraY = $View.Y - $cameraDistance
    $cameraZ = $View.Z + $cameraHeight
    $directionX = 0.0
    $directionY = $cameraDistance
    $directionZ = $aimHeight - $cameraHeight
    $directionLength = [math]::Sqrt($directionX*$directionX + $directionY*$directionY + $directionZ*$directionZ)
    $forwardX = $directionX / $directionLength
    $forwardY = $directionY / $directionLength
    $forwardZ = $directionZ / $directionLength
    $rightX = -$forwardY
    $rightY = $forwardX
    $rightLength = [math]::Sqrt($rightX*$rightX + $rightY*$rightY)
    $rightX /= $rightLength
    $rightY /= $rightLength
    $upX = $forwardY * 0.0 - $forwardZ * $rightY
    $upY = $forwardZ * $rightX - $forwardX * 0.0
    $upZ = $forwardX * $rightY - $forwardY * $rightX
    $relativeX = $sampleX - $cameraX
    $relativeY = $sampleY - $cameraY
    $relativeZ = $sampleZ - $cameraZ
    $depth = $relativeX*$forwardX + $relativeY*$forwardY + $relativeZ*$forwardZ
    if ($depth -le 1.0) { throw "Invalid projected sample depth for $($View.Name)/$($sample.Name)/${Variant}: $depth" }
    $cameraSpaceX = $relativeX*$rightX + $relativeY*$rightY
    $cameraSpaceY = $relativeX*$upX + $relativeY*$upY + $relativeZ*$upZ
    $focalPixels = 1024.0 / (2.0 * [math]::Tan(65.0 * [math]::PI / 360.0))
    $centerX = 512.0 + $focalPixels * $cameraSpaceX / $depth
    $centerY = 384.0 - $focalPixels * $cameraSpaceY / $depth
    $cropWidth = [math]::Max(12, [int][math]::Round($sample.WidthCm * $focalPixels / $depth * 0.6))
    $cropHeight = [math]::Max(12, [int][math]::Round($sample.HeightCm * $focalPixels / $depth * 0.6))
    $x0 = [math]::Max(0, [int][math]::Floor($centerX - $cropWidth / 2.0))
    $y0 = [math]::Max(0, [int][math]::Floor($centerY - $cropHeight / 2.0))
    $x1 = [math]::Min(1024, [int][math]::Ceiling($centerX + $cropWidth / 2.0))
    $y1 = [math]::Min(768, [int][math]::Ceiling($centerY + $cropHeight / 2.0))
    if ($x1 -le $x0 -or $y1 -le $y0) { throw "Projected crop falls outside image: $($View.Name)/$($sample.Name)/$Variant" }
    [pscustomobject]@{ Rectangle = [Drawing.Rectangle]::new($x0, $y0, $x1 - $x0, $y1 - $y0); CenterX = [math]::Round($centerX, 1); CenterY = [math]::Round($centerY, 1) }
}
function Get-TemporalRegionAbsoluteRgbDifference([string]$BeforePath, [string]$AfterPath, [Drawing.Rectangle]$Region) {
    $before = [Drawing.Bitmap]::FromFile($BeforePath)
    $after = [Drawing.Bitmap]::FromFile($AfterPath)
    try {
        [long]$difference = 0
        [long]$count = 0
        for ($y = $Region.Top + 2; $y -lt $Region.Bottom; $y += 4) {
            for ($x = $Region.Left + 2; $x -lt $Region.Right; $x += 4) {
                $a = $before.GetPixel($x, $y)
                $b = $after.GetPixel($x, $y)
                $difference += [math]::Abs($a.R - $b.R) + [math]::Abs($a.G - $b.G) + [math]::Abs($a.B - $b.B)
                $count += 3
            }
        }
        if ($count -eq 0) { throw "Projected crop contains no metric samples: $BeforePath / $AfterPath" }
        return [math]::Round($difference / [double]$count, 4)
    }
    finally { $before.Dispose(); $after.Dispose() }
}
function Get-RegionPixelSummary([string]$ImagePath, [Drawing.Rectangle]$Region) {
    $bitmap = [Drawing.Bitmap]::FromFile($ImagePath)
    try {
        [double]$red = 0; [double]$green = 0; [double]$blue = 0; [double]$luminance = 0; [double]$saturation = 0; [long]$count = 0
        for ($y = $Region.Top; $y -lt $Region.Bottom; $y++) {
            for ($x = $Region.Left; $x -lt $Region.Right; $x++) {
                $pixel = $bitmap.GetPixel($x, $y)
                $red += $pixel.R; $green += $pixel.G; $blue += $pixel.B
                $luminance += 0.2126 * $pixel.R + 0.7152 * $pixel.G + 0.0722 * $pixel.B
                $maximum = [math]::Max($pixel.R, [math]::Max($pixel.G, $pixel.B))
                $minimum = [math]::Min($pixel.R, [math]::Min($pixel.G, $pixel.B))
                if ($maximum -gt 0) { $saturation += ($maximum - $minimum) / $maximum }
                $count++
            }
        }
        if ($count -eq 0) { throw "Projected crop contains no image pixels: $ImagePath" }
        [pscustomobject]@{ PixelCount = $count; MeanR = [math]::Round($red / $count, 3); MeanG = [math]::Round($green / $count, 3); MeanB = [math]::Round($blue / $count, 3); MeanLuminance8Bit = [math]::Round($luminance / $count, 3); MeanPixelSaturation = [math]::Round($saturation / $count, 4) }
    }
    finally { $bitmap.Dispose() }
}
$sampleLuminanceResults = @()
foreach ($view in $fixedViews) {
    $framePaths = @((Join-Path $outputRoot ($view.Name + '.png')))
    for ($frame = 1; $frame -lt $temporalFrames; $frame++) { $framePaths += Join-Path $outputRoot ("{0}_T{1:D2}.png" -f $view.Name, $frame) }
    foreach ($sampleIndex in 0..($sampleDefinitions.Count - 1)) {
        foreach ($variant in @('DefaultLit', 'Toon')) {
            $region = Get-ProjectedSampleRegion $view $sampleIndex $variant
            $sampleImage = Join-Path $outputRoot ($view.Name + '_' + $variant + '.png')
            $pixelSummary = Get-RegionPixelSummary $sampleImage $region.Rectangle
            $sample = $sampleDefinitions[$sampleIndex]
            $row = [int][math]::Floor($sampleIndex / 2.0)
            $column = $sampleIndex % 2
            $groupX = $view.X + $(if ($column -eq 0) { -190.0 } else { 190.0 })
            $sampleX = $groupX + $(if ($variant -eq 'DefaultLit') { -65.0 } else { 65.0 })
            $sampleY = $view.Y + $row * 200.0
            $sampleLuminanceResults += [pscustomobject]@{
                Site = $view.Name; Condition = $view.Condition; Surface = $sample.Name; Variant = $variant
                DistanceFromViewAnchorCm = [math]::Round([math]::Sqrt(($sampleX - $view.X) * ($sampleX - $view.X) + ($sampleY - $view.Y) * ($sampleY - $view.Y)), 1)
                CropX = $region.Rectangle.X; CropY = $region.Rectangle.Y; CropWidth = $region.Rectangle.Width; CropHeight = $region.Rectangle.Height
                PixelCount = $pixelSummary.PixelCount; MeanR = $pixelSummary.MeanR; MeanG = $pixelSummary.MeanG; MeanB = $pixelSummary.MeanB
                MeanLuminance8Bit = $pixelSummary.MeanLuminance8Bit; MeanPixelSaturation = $pixelSummary.MeanPixelSaturation
                Method = 'Projected center crop of the selected UE BasicShapes proxy from the isolated variant capture; sRGB screenshot values, crop includes scene-edge pixels.'
            }
            $deltas = @()
            for ($frame = 1; $frame -lt $framePaths.Count; $frame++) {
                $deltas += Get-TemporalRegionAbsoluteRgbDifference $framePaths[$frame - 1] $framePaths[$frame] $region.Rectangle
            }
            $sampleTemporalResults += [pscustomobject]@{
                Site = $view.Name
                Condition = $view.Condition
                Surface = $sampleDefinitions[$sampleIndex].Name
                Variant = $variant
                FrameCount = $framePaths.Count
                FrameIntervalSeconds = 0.5
                CropX = $region.Rectangle.X
                CropY = $region.Rectangle.Y
                CropWidth = $region.Rectangle.Width
                CropHeight = $region.Rectangle.Height
                ProjectedCenterX = $region.CenterX
                ProjectedCenterY = $region.CenterY
                MeanAdjacentFrameAbsoluteRgbChange = [math]::Round(($deltas | Measure-Object -Average).Average, 4)
                MaxAdjacentFrameAbsoluteRgbChange = [math]::Round(($deltas | Measure-Object -Maximum).Maximum, 4)
                Method = 'Projected center crop of a UE BasicShapes proxy; crop includes a small amount of surrounding scene pixels.'
            }
        }
    }
}
$sampleLuminanceCsv = Join-Path $outputRoot 'LightingSampleLuminance.csv'
if ($sampleLuminanceCsv.Length -ge 260) { throw "Path exceeds MAX_PATH: $sampleLuminanceCsv" }
if ((Test-Path -LiteralPath $sampleLuminanceCsv) -and -not $AnalyzeOnly) { throw "Refusing to overwrite existing L03c luminance summary: $sampleLuminanceCsv" }
$sampleLuminanceResults | Export-Csv -LiteralPath $sampleLuminanceCsv -NoTypeInformation -Force
$sampleTemporalCsv = Join-Path $outputRoot 'TemporalSampleRegions.csv'
if ($sampleTemporalCsv.Length -ge 260) { throw "Path exceeds MAX_PATH: $sampleTemporalCsv" }
if ((Test-Path -LiteralPath $sampleTemporalCsv) -and -not $AnalyzeOnly) { throw "Refusing to overwrite existing L03c sample-region summary: $sampleTemporalCsv" }
$sampleTemporalResults | Export-Csv -LiteralPath $sampleTemporalCsv -NoTypeInformation -Force
if ($AnalyzeOnly) { return }
$temporalCsv = Join-Path $outputRoot 'TemporalStability.csv'
$gpuCsv = Join-Path $outputRoot 'PerViewGpuCost.csv'
foreach ($path in @($temporalCsv, $gpuCsv)) {
    if ($path.Length -ge 260) { throw "Path exceeds MAX_PATH: $path" }
    if (Test-Path -LiteralPath $path) { throw "Refusing to overwrite existing L03c summary: $path" }
}
$temporalResults | Export-Csv -LiteralPath $temporalCsv -NoTypeInformation
$gpuResults | Export-Csv -LiteralPath $gpuCsv -NoTypeInformation
