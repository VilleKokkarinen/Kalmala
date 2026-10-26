param(
    [string]$Project,
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe',
    [int]$TimeoutSeconds = 900,
    [switch]$ValidateOnly
)

$ErrorActionPreference = 'Stop'
$culture = [Globalization.CultureInfo]::InvariantCulture
$inputPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'BiomeFloorProfiles.csv'))
$baselinePath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'BiomeBaseline418419.csv'))
$expectedRows = 400
$expectedProfileCount = 8
$expectedSamplesPerProfile = 49
$sampleSpacingCm = 125.0
$profileLengthCm = 6000.0
$fixedRiverControlCoordinates = @(
    [pscustomobject]@{ Seed = 419; Site = 'S419_StarterMeadows_EW'; Xcm = -3781.6; Ycm = -781.6 },
    [pscustomobject]@{ Seed = 419; Site = 'S419_StarterMeadows_EW'; Xcm = -3656.6; Ycm = -781.6 },
    [pscustomobject]@{ Seed = 419; Site = 'S419_StarterMeadows_EW'; Xcm = -3531.6; Ycm = -781.6 },
    [pscustomobject]@{ Seed = 419; Site = 'S419_StarterMeadows_EW'; Xcm = -3406.6; Ycm = -781.6 },
    [pscustomobject]@{ Seed = 419; Site = 'S419_StarterMeadows_EW'; Xcm = -3281.6; Ycm = -781.6 }
)

function Test-FixedRiverControl($row) {
    foreach ($control in $fixedRiverControlCoordinates) {
        if ([int]$row.Seed -eq $control.Seed -and $row.Site -eq $control.Site -and
            [math]::Abs([double]::Parse($row.Xcm, $culture) - $control.Xcm) -le 0.01 -and
            [math]::Abs([double]::Parse($row.Ycm, $culture) - $control.Ycm) -le 0.01) {
            return $true
        }
    }
    return $false
}

foreach ($path in @($inputPath, $baselinePath)) {
    if ($path.Length -ge 260) { throw "Path exceeds MAX_PATH: $path" }
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required input not found: $path" }
}

$inputRows = @(Import-Csv -LiteralPath $inputPath)
if ($inputRows.Count -ne $expectedRows) { throw "Expected $expectedRows fixed floor samples, found $($inputRows.Count)." }
$profileRows = @($inputRows | Where-Object { $_.Site -notmatch 'Patch[XY]' })
$seamRows = @($inputRows | Where-Object { $_.Site -match 'Patch[XY]' })
$profileGroups = @($profileRows | Group-Object Site)
$seamGroups = @($seamRows | Group-Object Site)
if ($profileGroups.Count -ne $expectedProfileCount -or $seamGroups.Count -ne 8) {
    throw 'Floor fixture must contain eight profiles and eight patch-side controls.'
}
$fixtureRiverControls = @($profileRows | Where-Object { Test-FixedRiverControl $_ })
if ($fixtureRiverControls.Count -ne $fixedRiverControlCoordinates.Count) {
    throw 'Floor fixture must retain exactly the five documented seed-419 river-control coordinates.'
}

$baseline = @(Import-Csv -LiteralPath $baselinePath)
foreach ($group in $profileGroups) {
    if ($group.Name -notmatch '^S(?<seed>418|419)_(?<anchor>StarterMeadows|OuterdryMeadows)_(?<axis>EW|NS)$') {
        throw "Unknown floor profile name: $($group.Name)."
    }
    if ($group.Count -ne $expectedSamplesPerProfile) { throw "Profile $($group.Name) has $($group.Count) rows." }
    $seed = [int]$Matches.seed
    $anchorSite = [string]$Matches.anchor
    $axis = [string]$Matches.axis
    $anchor = $baseline | Where-Object { [int]$_.Seed -eq $seed -and $_.Site -eq $anchorSite } | Select-Object -First 1
    if ($null -eq $anchor) { throw "Missing fixed seed-$seed $anchorSite baseline." }
    if ([int]$anchor.BiomeIndex -ne 0 -or [int]$anchor.HasWater -ne 0 -or [double]::Parse($anchor.MeadowsWeight, $culture) -le 0.5) {
        throw "Baseline anchor for $($group.Name) is not dry Meadows ground."
    }

    $ordered = if ($axis -eq 'EW') {
        @($group.Group | Sort-Object { [double]::Parse($_.Xcm, $culture) })
    }
    else {
        @($group.Group | Sort-Object { [double]::Parse($_.Ycm, $culture) })
    }
    $axisValues = if ($axis -eq 'EW') {
        @($ordered | ForEach-Object { [double]::Parse($_.Xcm, $culture) })
    }
    else {
        @($ordered | ForEach-Object { [double]::Parse($_.Ycm, $culture) })
    }
    $fixedValues = if ($axis -eq 'EW') {
        @($ordered | ForEach-Object { [double]::Parse($_.Ycm, $culture) })
    }
    else {
        @($ordered | ForEach-Object { [double]::Parse($_.Xcm, $culture) })
    }
    $anchorAxis = if ($axis -eq 'EW') { [double]::Parse($anchor.Xcm, $culture) } else { [double]::Parse($anchor.Ycm, $culture) }
    $fixedAxis = if ($axis -eq 'EW') { [double]::Parse($anchor.Ycm, $culture) } else { [double]::Parse($anchor.Xcm, $culture) }
    if ([math]::Abs($axisValues[0] - ($anchorAxis - $profileLengthCm / 2.0)) -gt 0.01 -or
        [math]::Abs($axisValues[-1] - ($anchorAxis + $profileLengthCm / 2.0)) -gt 0.01) {
        throw "Profile $($group.Name) does not cover the fixed 60 m transect."
    }
    for ($index = 0; $index -lt $ordered.Count; $index++) {
        if ([math]::Abs($fixedValues[$index] - $fixedAxis) -gt 0.01) { throw "Profile $($group.Name) changed its fixed axis." }
        if ([int]$ordered[$index].Seed -ne $seed) { throw "Profile $($group.Name) contains a mismatched seed." }
        foreach ($column in @('Xcm', 'Ycm')) {
            $coordinate = 0.0
            if (-not [double]::TryParse([string]$ordered[$index].$column, [Globalization.NumberStyles]::Float, $culture, [ref]$coordinate) -or
                [double]::IsNaN($coordinate) -or [double]::IsInfinity($coordinate)) {
                throw "Profile $($group.Name) contains a non-finite coordinate."
            }
        }
        if ($index -gt 0 -and [math]::Abs(($axisValues[$index] - $axisValues[$index - 1]) - $sampleSpacingCm) -gt 0.01) {
            throw "Profile $($group.Name) does not sample on the 125 cm collision-cell spacing."
        }
    }
}

foreach ($seed in @(418, 419)) {
    $anchor = $baseline | Where-Object { [int]$_.Seed -eq $seed -and $_.Site -eq 'OuterdryMeadows' } | Select-Object -First 1
    if ($null -eq $anchor) { throw "Missing seed-$seed outer-Meadows baseline for seam controls." }
    $originX = [double]::Parse($anchor.PatchOriginXcm, $culture)
    $originY = [double]::Parse($anchor.PatchOriginYcm, $culture)
    $anchorX = [double]::Parse($anchor.Xcm, $culture)
    $anchorY = [double]::Parse($anchor.Ycm, $culture)
    $edgeX = $originX + ([int]$anchor.PatchX + 0.5) * 3000.0
    $edgeY = $originY + ([int]$anchor.PatchY + 0.5) * 3000.0
    $expectedSites = @{}
    $expectedSites[('S' + $seed + '_OuterPatchXWest')] = [double[]]@(($edgeX - 0.5), $anchorY)
    $expectedSites[('S' + $seed + '_OuterPatchXEast')] = [double[]]@(($edgeX + 0.5), $anchorY)
    $expectedSites[('S' + $seed + '_OuterPatchYSouth')] = [double[]]@($anchorX, ($edgeY - 0.5))
    $expectedSites[('S' + $seed + '_OuterPatchYNorth')] = [double[]]@($anchorX, ($edgeY + 0.5))
    foreach ($siteName in $expectedSites.Keys) {
        $row = $seamRows | Where-Object { $_.Site -eq $siteName } | Select-Object -First 1
        if ($null -eq $row) { throw "Missing floor patch control $siteName." }
        if ([int]$row.Seed -ne $seed -or
            [math]::Abs([double]::Parse($row.Xcm, $culture) - $expectedSites[$siteName][0]) -gt 0.01 -or
            [math]::Abs([double]::Parse($row.Ycm, $culture) - $expectedSites[$siteName][1]) -gt 0.01) {
            throw "Floor patch control $siteName no longer matches BH01 start-relative patch bounds."
        }
    }
}
if (@($seamGroups | Where-Object { $_.Count -ne 1 }).Count -ne 0) { throw 'Each floor seam control must have one fixed sample.' }

if ($ValidateOnly) {
    Write-Output "Validated $expectedRows fixed floor samples, $expectedProfileCount profiles, 125 cm spacing, and eight patch-side controls."
    return
}

if ([string]::IsNullOrWhiteSpace($Project)) { throw 'Project is required unless -ValidateOnly is used.' }
$projectPath = [IO.Path]::GetFullPath($Project)
$editorPath = [IO.Path]::GetFullPath($Editor)
$outputRoot = Join-Path $env:TEMP ('bfp' + [guid]::NewGuid().ToString('N').Substring(0, 8))
foreach ($path in @($projectPath, $editorPath, $outputRoot)) {
    if ($path.Length -ge 260) { throw "Path exceeds MAX_PATH: $path" }
}
if (-not (Test-Path -LiteralPath $projectPath -PathType Leaf)) { throw "Project file not found: $projectPath" }
if (-not (Test-Path -LiteralPath $editorPath -PathType Leaf)) { throw "Unreal Editor commandlet not found: $editorPath" }

New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
$summaryRows = [Collections.Generic.List[object]]::new()
foreach ($scaleM in @(8, 12, 20)) {
    foreach ($amplitudeM in @(0.1, 0.2, 0.3)) {
        $scaleText = $scaleM.ToString('0', $culture)
        $amplitudeText = $amplitudeM.ToString('0.0', $culture)
        $trialName = 'floor-' + $scaleText + 'm-' + $amplitudeText.Replace('.', '_') + 'm'
        $trialDirectory = Join-Path $outputRoot $trialName
        $outputPath = Join-Path $trialDirectory 'samples.csv'
        $logPath = Join-Path $trialDirectory 'editor.log'
        $userDirectory = Join-Path $trialDirectory 'u'
        foreach ($path in @($trialDirectory, $outputPath, $logPath, $userDirectory)) {
            if ($path.Length -ge 260) { throw "Path exceeds MAX_PATH: $path" }
        }
        New-Item -ItemType Directory -Path $trialDirectory -Force | Out-Null
        New-Item -ItemType Directory -Path $userDirectory -Force | Out-Null

        $quote = [string][char]34
        $arguments = @(
            ($quote + $projectPath + $quote),
            '-run=RenderWorldGenerationVisualization',
            '-nullrhi', '-unattended', '-nop4', '-nosplash', '-NoZenAutoLaunch',
            '-DDC=NoZenLocalFallback',
            ('-SampleInput=' + $quote + $inputPath + $quote),
            ('-SampleOutput=' + $quote + $outputPath + $quote),
            ('-MeadowFloorScaleM=' + $scaleText),
            ('-MeadowFloorAmplitudeM=' + $amplitudeText),
            ('-abslog=' + $quote + $logPath + $quote),
            ('-UserDir=' + $quote + $userDirectory + $quote)
        )

        $process = $null
        try {
            $process = Start-Process -FilePath $editorPath -ArgumentList ($arguments -join ' ') -PassThru -WindowStyle Hidden
            $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
            while (-not $process.HasExited -and (Get-Date) -lt $deadline) {
                Start-Sleep -Milliseconds 250
                $process.Refresh()
            }
            if (-not $process.HasExited) {
                Stop-Process -Id $process.Id -Force
                $process.WaitForExit()
                throw "Floor profile trial $trialName timed out. See $logPath"
            }
            if ($process.ExitCode -ne 0) {
                $tail = if (Test-Path -LiteralPath $logPath) { (Get-Content -LiteralPath $logPath -Tail 30) -join ([string][char]10) } else { '(no log created)' }
                throw ('Floor profile trial ' + $trialName + ' failed with exit ' + $process.ExitCode + '.' + [string][char]10 + $tail)
            }
        }
        finally {
            if ($null -ne $process -and -not $process.HasExited) {
                Stop-Process -Id $process.Id -Force
                $process.WaitForExit()
            }
        }

        if (-not (Test-Path -LiteralPath $outputPath -PathType Leaf) -or -not (Test-Path -LiteralPath $logPath -PathType Leaf)) {
            throw "Trial $trialName did not produce numeric output and a log."
        }
        $logText = Get-Content -LiteralPath $logPath -Raw
        $marker = 'Meadow floor sample trial accepted: ScaleM=' + $scaleM.ToString('0.000', $culture) + ' AmplitudeM=' + $amplitudeM.ToString('0.000', $culture)
        if ($logText.IndexOf($marker, [StringComparison]::Ordinal) -lt 0) {
            throw "Fresh floor-trial marker is missing for $trialName; the loaded binary may be stale. See $logPath"
        }
        $exportMatch = [regex]::Match($logText, 'Exported (?<count>[0-9]+) numeric world-generation samples')
        if (-not $exportMatch.Success) { throw "Trial $trialName did not report its sample count. See $logPath" }

        $rows = @(Import-Csv -LiteralPath $outputPath)
        $loggedCount = [int]$exportMatch.Groups['count'].Value
        if ($rows.Count -ne $loggedCount -or $rows.Count -ne $expectedRows) {
            throw "Trial $trialName returned $($rows.Count) rows, while the log reports $loggedCount (expected exactly $expectedRows fixed input rows)."
        }
        $forestedProfileSamples = 0
        $riverControlRows = [Collections.Generic.List[object]]::new()
        foreach ($row in $rows) {
            foreach ($column in @('FinalHeightCm', 'MeadowsWeight', 'WaterLevelCm', 'CollisionTriangleHeightCm',
                'CollisionTriangleSlopeDegrees', 'CollisionTriangleGradePercent', 'MeadowFloorNoise', 'MeadowFloorInputCm')) {
                $number = 0.0
                $parsed = [double]::TryParse([string]$row.$column, [Globalization.NumberStyles]::Float, $culture, [ref]$number)
                if (-not $parsed -or [double]::IsNaN($number) -or [double]::IsInfinity($number)) {
                    throw "Trial $trialName has a non-finite $column at $($row.Site)."
                }
            }
            $isFixedRiverControl = Test-FixedRiverControl $row
            if ($isFixedRiverControl) {
                $waterLevel = [double]::Parse($row.WaterLevelCm, $culture)
                $finalHeight = [double]::Parse($row.FinalHeightCm, $culture)
                if ([int]$row.HasWater -ne 1 -or [math]::Abs($waterLevel) -gt 0.01 -or
                    $finalHeight -ge $waterLevel -or [double]::Parse($row.MeadowsWeight, $culture) -le 0.5) {
                    throw "Trial $trialName changed fixed seed-419 river control water at $($row.Xcm),$($row.Ycm)."
                }
                $riverControlRows.Add($row)
            }
            elseif ([int]$row.HasWater -ne 0 -or [double]::Parse($row.MeadowsWeight, $culture) -le 0.0) {
                throw "Trial $trialName contains a newly wet sample or a sample with no Meadows influence at $($row.Site)."
            }
            $forestDescriptorCount = 0
            if (-not [int]::TryParse([string]$row.ForestDescriptorCount, [ref]$forestDescriptorCount) -or $forestDescriptorCount -lt 0) {
                throw "Trial $trialName has an invalid forest-descriptor count at $($row.Site)."
            }
            if ($forestDescriptorCount -gt 0) { $forestedProfileSamples++ }
            $signal = [double]::Parse($row.MeadowFloorNoise, $culture)
            $inputCm = [double]::Parse($row.MeadowFloorInputCm, $culture)
            $expectedInputCm = [double][single]($signal * $amplitudeM * 100.0)
            if ([math]::Abs($signal) -gt 1.01 -or [math]::Abs($inputCm - $expectedInputCm) -gt 0.001) {
                throw "Trial $trialName has a floor signal/input mismatch at $($row.Site)."
            }
        }
        if ($riverControlRows.Count -ne $fixedRiverControlCoordinates.Count) {
            throw "Trial $trialName did not return all five fixed river controls."
        }
        if ($forestedProfileSamples -eq 0) { throw "Trial $trialName contains no fixed sample patches with forest descriptors." }

        foreach ($group in $rows | Where-Object { $_.Site -notmatch 'Patch[XY]' } | Group-Object Site) {
            if ($group.Name -notmatch '_(?<axis>EW|NS)$') { throw "Trial $trialName has unknown profile axis in $($group.Name)." }
            $axis = [string]$Matches.axis
            $profile = if ($axis -eq 'EW') {
                @($group.Group | Sort-Object { [double]::Parse($_.Xcm, $culture) })
            }
            else {
                @($group.Group | Sort-Object { [double]::Parse($_.Ycm, $culture) })
            }
            if ($profile.Count -ne $expectedSamplesPerProfile) { throw "Trial $trialName has an unexpected profile count at $($group.Name)." }
            $profileRiverControls = @($profile | Where-Object { Test-FixedRiverControl $_ })
            $expectedControlCount = if ($group.Name -eq 'S419_StarterMeadows_EW') { $fixedRiverControlCoordinates.Count } else { 0 }
            if ($profileRiverControls.Count -ne $expectedControlCount) { throw "Trial $trialName has an unexpected fixed river-control count at $($group.Name)." }
            $dryProfile = @($profile | Where-Object { -not (Test-FixedRiverControl $_) })
            $expectedDrySamples = $expectedSamplesPerProfile - $expectedControlCount
            if ($dryProfile.Count -ne $expectedDrySamples) { throw "Trial $trialName has an unexpected dry sample count at $($group.Name)." }
            $heights = @($dryProfile | ForEach-Object { [double]::Parse($_.FinalHeightCm, $culture) })
            $signals = @($dryProfile | ForEach-Object { [double]::Parse($_.MeadowFloorNoise, $culture) })
            $inputs = @($dryProfile | ForEach-Object { [double]::Parse($_.MeadowFloorInputCm, $culture) })
            $slopes = @($dryProfile | ForEach-Object { [double]::Parse($_.CollisionTriangleSlopeDegrees, $culture) })
            $grades = @($dryProfile | ForEach-Object { [double]::Parse($_.CollisionTriangleGradePercent, $culture) })
            $controlHeights = @($profileRiverControls | ForEach-Object { [double]::Parse($_.FinalHeightCm, $culture) })
            $controlSlopes = @($profileRiverControls | ForEach-Object { [double]::Parse($_.CollisionTriangleSlopeDegrees, $culture) })
            $maxInputStep = 0.0
            $maxFinalHeightStep = 0.0
            for ($index = 1; $index -lt $profile.Count; $index++) {
                $prior = $profile[$index - 1]
                $current = $profile[$index]
                $globalCellPrior = if ($axis -eq 'EW') { [int]$prior.PatchX * 24 + [int]$prior.PatchCellX } else { [int]$prior.PatchY * 24 + [int]$prior.PatchCellY }
                $globalCellCurrent = if ($axis -eq 'EW') { [int]$current.PatchX * 24 + [int]$current.PatchCellX } else { [int]$current.PatchY * 24 + [int]$current.PatchCellY }
                if ($globalCellCurrent -ne ($globalCellPrior + 1)) { throw "Trial $trialName lost 125 cm collision-grid progression at $($group.Name)." }
                if (-not (Test-FixedRiverControl $prior) -and -not (Test-FixedRiverControl $current)) {
                    $priorFloorInput = [double]::Parse($prior.MeadowFloorInputCm, $culture)
                    $currentFloorInput = [double]::Parse($current.MeadowFloorInputCm, $culture)
                    $priorFinalHeight = [double]::Parse($prior.FinalHeightCm, $culture)
                    $currentFinalHeight = [double]::Parse($current.FinalHeightCm, $culture)
                    $maxInputStep = [math]::Max($maxInputStep, [math]::Abs($currentFloorInput - $priorFloorInput))
                    $maxFinalHeightStep = [math]::Max($maxFinalHeightStep, [math]::Abs($currentFinalHeight - $priorFinalHeight))
                }
            }
            $forestCount = ($profile | Measure-Object -Property ForestDescriptorCount -Maximum).Maximum
            $summaryRows.Add([pscustomobject]@{
                ScaleM = $scaleText
                AmplitudeM = $amplitudeText
                Site = $group.Name
                Samples = $dryProfile.Count
                WetControlSamples = $profileRiverControls.Count
                SpacingCm = $sampleSpacingCm
                FloorSignalRange = (($signals | Measure-Object -Maximum).Maximum - ($signals | Measure-Object -Minimum).Minimum)
                FloorInputRangeCm = (($inputs | Measure-Object -Maximum).Maximum - ($inputs | Measure-Object -Minimum).Minimum)
                MaxFloorInputStepCm = $maxInputStep
                FinalHeightRangeCm = (($heights | Measure-Object -Maximum).Maximum - ($heights | Measure-Object -Minimum).Minimum)
                MaxFinalHeightStepCm = $maxFinalHeightStep
                MaxTriangleSlopeDegrees = ($slopes | Measure-Object -Maximum).Maximum
                MaxTriangleGradePercent = ($grades | Measure-Object -Maximum).Maximum
                ForestDescriptorCount = $forestCount
                WetControlHeightMinCm = if ($controlHeights.Count -gt 0) { ($controlHeights | Measure-Object -Minimum).Minimum } else { '' }
                WetControlHeightMaxCm = if ($controlHeights.Count -gt 0) { ($controlHeights | Measure-Object -Maximum).Maximum } else { '' }
                WetControlSlopeMaxDegrees = if ($controlSlopes.Count -gt 0) { ($controlSlopes | Measure-Object -Maximum).Maximum } else { '' }
                SeamHeightDeltaCm = ''
                SeamCollisionDeltaCm = ''
                OutputCsv = $outputPath
            })
        }

        foreach ($seed in @(418, 419)) {
            foreach ($axis in @('X', 'Y')) {
                $pair = @($rows | Where-Object { $_.Site -match ('^S' + $seed + '_OuterPatch' + $axis) })
                if ($pair.Count -ne 2) { throw "Trial $trialName lacks seed-$seed $axis patch-side rows." }
                $negativeSide = $pair | Where-Object { $_.Site -match '(West|South)$' } | Select-Object -First 1
                $positiveSide = $pair | Where-Object { $_.Site -match '(East|North)$' } | Select-Object -First 1
                if ($null -eq $negativeSide -or $null -eq $positiveSide) { throw "Trial $trialName has malformed seed-$seed $axis seam rows." }
                if ($axis -eq 'X') {
                    if ([int]$positiveSide.PatchX -ne ([int]$negativeSide.PatchX + 1) -or [int]$positiveSide.PatchY -ne [int]$negativeSide.PatchY) {
                        throw "Trial $trialName X seam did not cross adjacent patches."
                    }
                }
                else {
                    if ([int]$positiveSide.PatchY -ne ([int]$negativeSide.PatchY + 1) -or [int]$positiveSide.PatchX -ne [int]$negativeSide.PatchX) {
                        throw "Trial $trialName Y seam did not cross adjacent patches."
                    }
                }
                $heightDelta = [math]::Abs([double]::Parse($positiveSide.FinalHeightCm, $culture) - [double]::Parse($negativeSide.FinalHeightCm, $culture))
                $collisionDelta = [math]::Abs([double]::Parse($positiveSide.CollisionTriangleHeightCm, $culture) - [double]::Parse($negativeSide.CollisionTriangleHeightCm, $culture))
                if ($heightDelta -gt 5.0 -or $collisionDelta -gt 5.0) { throw "Trial $trialName exceeded the 5 cm $seed $axis patch-seam tolerance." }
                $summaryRows.Add([pscustomobject]@{
                    ScaleM = $scaleText
                    AmplitudeM = $amplitudeText
                    Site = ('S' + $seed + '_Patch' + $axis)
                    Samples = 2
                    WetControlSamples = 0
                    SpacingCm = 1
                    FloorSignalRange = ''
                    FloorInputRangeCm = ''
                    MaxFloorInputStepCm = ''
                    FinalHeightRangeCm = ''
                    MaxFinalHeightStepCm = ''
                    MaxTriangleSlopeDegrees = ''
                    MaxTriangleGradePercent = ''
                    ForestDescriptorCount = ''
                    WetControlHeightMinCm = ''
                    WetControlHeightMaxCm = ''
                    WetControlSlopeMaxDegrees = ''
                    SeamHeightDeltaCm = $heightDelta
                    SeamCollisionDeltaCm = $collisionDelta
                    OutputCsv = $outputPath
                })
            }
        }
    }
}

$summaryPath = Join-Path $outputRoot 'summary.csv'
if ($summaryPath.Length -ge 260) { throw "Path exceeds MAX_PATH: $summaryPath" }
$summaryRows | Export-Csv -LiteralPath $summaryPath -NoTypeInformation -Encoding UTF8
Write-Output "Meadows floor profiles exported to $outputRoot"
Write-Output "Summary: $summaryPath"
