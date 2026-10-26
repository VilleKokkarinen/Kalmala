param(
    [string]$Project,
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe',
    [int]$TimeoutSeconds = 900,
    [switch]$ValidateOnly
)

$ErrorActionPreference = 'Stop'
$culture = [Globalization.CultureInfo]::InvariantCulture
$inputPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'BiomeMireProfiles.csv'))
$expectedProfiles = @(
    [pscustomobject]@{ Seed = 418; Site = 'MireInterior418_X'; X = -669858.3; Y = -601074.7; Axis = 'X'; Kind = 'Mire' },
    [pscustomobject]@{ Seed = 418; Site = 'MireInterior418_Y'; X = -669858.3; Y = -601074.7; Axis = 'Y'; Kind = 'Mire' },
    [pscustomobject]@{ Seed = 419; Site = 'MireInterior419_X'; X = -78944.8; Y = 896531.5; Axis = 'X'; Kind = 'Mire' },
    [pscustomobject]@{ Seed = 419; Site = 'MireInterior419_Y'; X = -78944.8; Y = 896531.5; Axis = 'Y'; Kind = 'Mire' },
    [pscustomobject]@{ Seed = 418; Site = 'SourceCoast418_X'; X = 691744.0; Y = -107083.5; Axis = 'X'; Kind = 'CoastLand' },
    [pscustomobject]@{ Seed = 418; Site = 'SourceCoast418_Y'; X = 691744.0; Y = -107083.5; Axis = 'Y'; Kind = 'CoastLand' },
    [pscustomobject]@{ Seed = 419; Site = 'SourceCoast419_X'; X = -71128.5; Y = -696433.8; Axis = 'X'; Kind = 'DryCoast' },
    [pscustomobject]@{ Seed = 419; Site = 'SourceCoast419_Y'; X = -71128.5; Y = -696433.8; Axis = 'Y'; Kind = 'DryCoast' },
    [pscustomobject]@{ Seed = 418; Site = 'HighlandRiver418_X'; X = 679237.9; Y = -16414.3; Axis = 'X'; Kind = 'River' },
    [pscustomobject]@{ Seed = 418; Site = 'HighlandRiver418_Y'; X = 679237.9; Y = -16414.3; Axis = 'Y'; Kind = 'River' },
    [pscustomobject]@{ Seed = 419; Site = 'HighlandRiver419_X'; X = 1145090.4; Y = -66438.7; Axis = 'X'; Kind = 'River' },
    [pscustomobject]@{ Seed = 419; Site = 'HighlandRiver419_Y'; X = 1145090.4; Y = -66438.7; Axis = 'Y'; Kind = 'River' }
)
$samplesPerProfile = 21
$stepCm = 500.0
$expectedSamples = $expectedProfiles.Count * $samplesPerProfile
if ($inputPath.Length -ge 260) { throw "Path exceeds MAX_PATH: $inputPath" }
if (-not (Test-Path -LiteralPath $inputPath -PathType Leaf)) { throw "Profile fixture not found: $inputPath" }

$inputRows = @(Import-Csv -LiteralPath $inputPath)
if ($inputRows.Count -ne $expectedSamples) { throw "Expected $expectedSamples fixed profile rows, found $($inputRows.Count)." }
$expectedNames = @($expectedProfiles | ForEach-Object { $_.Site })
$siteGroups = @($inputRows | Group-Object Site)
if ($siteGroups.Count -ne $expectedProfiles.Count -or @($siteGroups | Where-Object { $_.Count -ne $samplesPerProfile }).Count -ne 0) {
    throw 'Fixture must contain twelve fixed 100 m transects with 21 samples each.'
}

foreach ($profile in $expectedProfiles) {
    $group = @($inputRows | Where-Object { $_.Site -ceq $profile.Site })
    if ($group.Count -ne $samplesPerProfile) { throw "Missing fixed profile $($profile.Site)." }
    $actualSeeds = @($group | ForEach-Object { [uint64]::Parse($_.Seed, $culture) } | Select-Object -Unique)
    if ($actualSeeds.Count -ne 1 -or $actualSeeds[0] -ne $profile.Seed) { throw "Profile $($profile.Site) has the wrong seed." }

    for ($index = 0; $index -lt $samplesPerProfile; $index++) {
        $offset = ($index - [int](($samplesPerProfile - 1) / 2)) * $stepCm
        $x = [double]::Parse($group[$index].Xcm, $culture)
        $y = [double]::Parse($group[$index].Ycm, $culture)
        if ([double]::IsNaN($x) -or [double]::IsInfinity($x) -or [double]::IsNaN($y) -or [double]::IsInfinity($y)) {
            throw "Profile $($profile.Site) contains a non-finite coordinate."
        }
        $expectedX = [double]$profile.X + $(if ($profile.Axis -eq 'X') { $offset } else { 0.0 })
        $expectedY = [double]$profile.Y + $(if ($profile.Axis -eq 'Y') { $offset } else { 0.0 })
        if ([math]::Abs($x - $expectedX) -gt 0.01 -or [math]::Abs($y - $expectedY) -gt 0.01) {
            throw "Profile $($profile.Site) does not match its 500 cm fixed transect."
        }
    }
    $centerIndex = [int](($samplesPerProfile - 1) / 2)
    $centerX = [double]::Parse($group[$centerIndex].Xcm, $culture)
    $centerY = [double]::Parse($group[$centerIndex].Ycm, $culture)
    if ([math]::Abs($centerX - $profile.X) -gt 0.01 -or [math]::Abs($centerY - $profile.Y) -gt 0.01) {
        throw "Profile $($profile.Site) center differs from its BH01 baseline coordinate."
    }
}
if (@($siteGroups | Where-Object { $_.Name -notin $expectedNames }).Count -ne 0) { throw 'Fixture contains an unexpected site.' }
if (($inputRows[0].PSObject.Properties.Name -join ',') -cne 'Seed,Site,Xcm,Ycm') {
    throw 'Fixture columns must remain Seed,Site,Xcm,Ycm for the commandlet.'
}

if ($ValidateOnly) {
    Write-Output "Validated $expectedSamples fixed Mire samples across twelve coast, interior and river transects."
    return
}
if ([string]::IsNullOrWhiteSpace($Project)) { throw 'Project is required unless -ValidateOnly is specified.' }

$projectPath = [IO.Path]::GetFullPath($Project)
$editorPath = [IO.Path]::GetFullPath($Editor)
$outputRoot = Join-Path $env:TEMP ('bm' + [guid]::NewGuid().ToString('N').Substring(0, 8))
foreach ($path in @($projectPath, $editorPath, $outputRoot)) {
    if ($path.Length -ge 260) { throw "Path exceeds MAX_PATH: $path" }
}
if (-not (Test-Path -LiteralPath $projectPath -PathType Leaf)) { throw "Project file not found: $projectPath" }
if (-not (Test-Path -LiteralPath $editorPath -PathType Leaf)) { throw "Unreal Editor commandlet not found: $editorPath" }

$trials = @(
    [pscustomobject]@{ Control = 'TargetHeightCm'; Parameter = 'MireTargetHeightCm'; Values = @(200.0, 300.0, 500.0) },
    [pscustomobject]@{ Control = 'CompressionScaleCm'; Parameter = 'MireCompressionScaleCm'; Values = @(300.0, 700.0, 1200.0) },
    [pscustomobject]@{ Control = 'ResidualDetailAmplitudeCm'; Parameter = 'MireResidualDetailAmplitudeCm'; Values = @(10.0, 25.0, 60.0) }
)
$summaryRows = [Collections.Generic.List[object]]::new()
$trialRowsByTag = @{}

foreach ($trial in $trials) {
    foreach ($value in $trial.Values) {
        $effective = @{ TargetHeightCm = 300.0; CompressionScaleCm = 700.0; ResidualDetailAmplitudeCm = 25.0 }
        $effective[$trial.Control] = [double]$value
        $clearanceAtZero = $effective.TargetHeightCm * $effective.CompressionScaleCm /
            ($effective.TargetHeightCm + $effective.CompressionScaleCm) - $effective.ResidualDetailAmplitudeCm
        if ($clearanceAtZero -le 0.0) { throw "Trial $($trial.Control)=$value violates positive source-land clearance." }

        $trialValue = ([double]$value).ToString('0.###', $culture)
        $trialTag = $trial.Control.ToLowerInvariant() + '-' + $trialValue
        $trialDirectory = Join-Path $outputRoot $trialTag
        $outputPath = Join-Path $trialDirectory 's.csv'
        $logPath = Join-Path $trialDirectory 'e.log'
        $userDirectory = Join-Path $trialDirectory 'u'
        foreach ($path in @($trialDirectory, $outputPath, $logPath, $userDirectory)) {
            if ($path.Length -ge 260) { throw "Path exceeds MAX_PATH: $path" }
        }
        New-Item -ItemType Directory -Path $trialDirectory -Force | Out-Null
        New-Item -ItemType Directory -Path $userDirectory -Force | Out-Null

        $arguments = @(
            "`"$projectPath`"",
            '-run=RenderWorldGenerationVisualization',
            '-nullrhi', '-unattended', '-nop4', '-nosplash', '-NoZenAutoLaunch',
            '-DDC=NoZenLocalFallback',
            "-SampleInput=`"$inputPath`"",
            "-SampleOutput=`"$outputPath`"",
            ('-' + $trial.Parameter + '=' + $trialValue),
            "-abslog=`"$logPath`"",
            "-UserDir=`"$userDirectory`""
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
                throw "Mossy Mire trial $trialTag timed out. See $logPath"
            }
            if ($process.ExitCode -ne 0) {
                $tail = if (Test-Path -LiteralPath $logPath) { (Get-Content -LiteralPath $logPath -Tail 30) -join "`n" } else { '(no log created)' }
                throw "Mossy Mire trial $trialTag failed with exit $($process.ExitCode).`n$tail"
            }
        }
        finally {
            if ($null -ne $process -and -not $process.HasExited) {
                Stop-Process -Id $process.Id -Force
                $process.WaitForExit()
            }
        }

        if (-not (Test-Path -LiteralPath $outputPath -PathType Leaf) -or -not (Test-Path -LiteralPath $logPath -PathType Leaf)) {
            throw "Trial $trialTag did not produce both numeric output and a log."
        }
        $marker = ('Mossy Mire profile trial accepted: TargetHeightCm=' + $effective.TargetHeightCm.ToString('0.0', $culture) +
            ' CompressionScaleCm=' + $effective.CompressionScaleCm.ToString('0.0', $culture) +
            ' ResidualDetailAmplitudeCm=' + $effective.ResidualDetailAmplitudeCm.ToString('0.0', $culture))
        $logText = Get-Content -LiteralPath $logPath -Raw
        if ($logText.IndexOf($marker, [StringComparison]::Ordinal) -lt 0) {
            throw "Fresh Mossy Mire trial marker not found for $trialTag; loaded binary may be stale. See $logPath"
        }
        if ($logText -notmatch ('Exported ' + $expectedSamples + ' numeric world-generation samples')) {
            throw "Trial $trialTag did not export the expected sample count. See $logPath"
        }

        $rows = @(Import-Csv -LiteralPath $outputPath)
        if ($rows.Count -ne $expectedSamples) { throw "Trial $trialTag returned $($rows.Count) rows, expected $expectedSamples." }
        for ($index = 0; $index -lt $expectedSamples; $index++) {
            $expectedRow = $inputRows[$index]
            $actualRow = $rows[$index]
            if ($actualRow.Site -cne $expectedRow.Site -or $actualRow.Seed -cne $expectedRow.Seed -or
                [math]::Abs([double]::Parse($actualRow.Xcm, $culture) - [double]::Parse($expectedRow.Xcm, $culture)) -gt 0.01 -or
                [math]::Abs([double]::Parse($actualRow.Ycm, $culture) - [double]::Parse($expectedRow.Ycm, $culture)) -gt 0.01) {
                throw "Trial $trialTag output row $($index + 1) differs from the fixed fixture."
            }
        }
        $numericColumns = @(
            'FinalHeightCm', 'MeadowsWeight', 'ShimmeringLakesWeight', 'ElderwoodWeight', 'MossyMireWeight',
            'FreezingTundraWeight', 'ThunderMountainsWeight', 'OceanWeight', 'WaterLevelCm',
            'CollisionTriangleHeightCm', 'CollisionTriangleSlopeDegrees', 'CollisionTriangleGradePercent',
            'MireCandidateCm', 'SourceHeightCm', 'MireDetail', 'MireTargetHeightCm',
            'MireCompressionScaleCm', 'MireResidualDetailAmplitudeCm', 'MireWorstCaseClearanceCm',
            'BasinWeight', 'RiverWeight', 'StreamWeight'
        )
        foreach ($row in $rows) {
            $numbers = @{}
            foreach ($column in $numericColumns) {
                $number = 0.0
                $parsed = [double]::TryParse([string]$row.$column, [Globalization.NumberStyles]::Float, $culture, [ref]$number)
                if (-not $parsed -or [double]::IsNaN($number) -or [double]::IsInfinity($number)) {
                    throw "Trial $trialTag has a non-finite $column for $($row.Site)."
                }
                $numbers[$column] = $number
            }
            $weightTotal = 0.0
            foreach ($column in @('MeadowsWeight', 'ShimmeringLakesWeight', 'ElderwoodWeight', 'MossyMireWeight', 'FreezingTundraWeight', 'ThunderMountainsWeight', 'OceanWeight')) {
                $weightTotal += $numbers[$column]
            }
            if ([math]::Abs($weightTotal - 1.0) -gt 0.0001) { throw "Trial $trialTag has unnormalized weights for $($row.Site)." }
            if ([math]::Abs($numbers.BasinWeight) -gt 0.000001) { throw "Trial $trialTag enabled analytic basin support for $($row.Site)." }
            if ($row.HasWater -eq '1' -and ($numbers.RiverWeight + $numbers.StreamWeight + $numbers.OceanWeight) -le 0.000001) {
                throw "Trial $trialTag has water without ocean or a river/stream course for $($row.Site)."
            }
            if ([math]::Abs($numbers.MireTargetHeightCm - $effective.TargetHeightCm) -gt 0.000001 -or
                [math]::Abs($numbers.MireCompressionScaleCm - $effective.CompressionScaleCm) -gt 0.000001 -or
                [math]::Abs($numbers.MireResidualDetailAmplitudeCm - $effective.ResidualDetailAmplitudeCm) -gt 0.000001) {
                throw "Trial $trialTag reports stale or unexpected effective Mire controls for $($row.Site)."
            }
            if ([math]::Abs($numbers.MireDetail) -gt 1.000001) { throw "Trial $trialTag has out-of-range Mire detail for $($row.Site)." }

            $source = $numbers.SourceHeightCm
            $detail = $numbers.MireDetail
            $target = $numbers.MireTargetHeightCm
            $scale = $numbers.MireCompressionScaleCm
            $residual = $numbers.MireResidualDetailAmplitudeCm
            $curve = $target + ($source - $target) / (1.0 + [math]::Abs($source - $target) / $scale)
            $expectedCandidate = $curve + [math]::Max(-1.0, [math]::Min(1.0, $detail)) * $residual
            if ([math]::Abs($numbers.MireCandidateCm - $expectedCandidate) -gt 0.02) {
                throw "Trial $trialTag Mire candidate violates its bounded compression formula at $($row.Site)."
            }
            if ([math]::Abs($numbers.MireWorstCaseClearanceCm - ($curve - $residual)) -gt 0.02) {
                throw "Trial $trialTag worst-case source-land clearance is inconsistent at $($row.Site)."
            }
            if ($source -ge 0.0 -and $numbers.MossyMireWeight -gt 0.000001 -and $numbers.MireWorstCaseClearanceCm -le 0.0) {
                throw "Trial $trialTag loses positive source-land clearance at $($row.Site)."
            }
        }

        foreach ($profile in $expectedProfiles) {
            $group = @($rows | Where-Object { $_.Site -ceq $profile.Site })
            $center = $group[[int](($samplesPerProfile - 1) / 2)]
            if ($profile.Kind -eq 'Mire' -and ([double]$center.MossyMireWeight -le 0.01 -or $center.HasWater -ne '0')) {
                throw "Fixed Mossy Mire control $($profile.Site) lost its dry Mire center in trial $trialTag."
            }
            if ($profile.Kind -eq 'CoastLand') {
                # BH25b2 restored this source-land point above Z=0; nearby negative-source ocean samples remain submerged.
                $centerSource = [double]::Parse($center.SourceHeightCm, $culture)
                $centerHeight = [double]::Parse($center.FinalHeightCm, $culture)
                $centerWaterLevel = [double]::Parse($center.WaterLevelCm, $culture)
                $wetOceanNeighbors = @($group | Where-Object {
                    [double]::Parse($_.OceanWeight, $culture) -ge 0.99 -and
                    $_.HasWater -eq '1' -and
                    [double]::Parse($_.WaterLevelCm, $culture) -gt [double]::Parse($_.FinalHeightCm, $culture)
                })
                if ([double]$center.OceanWeight -lt 0.99 -or $centerSource -le 0.0 -or
                    $centerHeight -le $centerWaterLevel -or $center.HasWater -ne '0' -or
                    $wetOceanNeighbors.Count -eq 0) {
                    throw "Fixed source-land coast control $($profile.Site) lost its dry positive shoreline or adjacent submerged ocean samples in trial $trialTag."
                }
            }
            if ($profile.Kind -eq 'DryCoast' -and ($center.HasWater -ne '0' -or [double]$center.FinalHeightCm -le 0.0)) {
                throw "Fixed dry coast control $($profile.Site) lost positive surface clearance in trial $trialTag."
            }
            if ($profile.Kind -eq 'River' -and ($center.HasWater -ne '1' -or [double]$center.RiverWeight -le 0.0 -or
                [double]$center.WaterLevelCm -ne 0.0 -or [double]$center.FinalHeightCm -gt -79.0)) {
                throw "Fixed river corridor control $($profile.Site) lost its sea-level carved channel in trial $trialTag."
            }
        }

        foreach ($group in $rows | Group-Object Site) {
            $source = @($group.Group | ForEach-Object { [double]::Parse($_.SourceHeightCm, $culture) })
            $candidate = @($group.Group | ForEach-Object { [double]::Parse($_.MireCandidateCm, $culture) })
            $final = @($group.Group | ForEach-Object { [double]::Parse($_.FinalHeightCm, $culture) })
            $collision = @($group.Group | ForEach-Object { [double]::Parse($_.CollisionTriangleHeightCm, $culture) })
            $slopes = @($group.Group | ForEach-Object { [double]::Parse($_.CollisionTriangleSlopeDegrees, $culture) })
            $grade = @($group.Group | ForEach-Object { [double]::Parse($_.CollisionTriangleGradePercent, $culture) })
            $clearance = @($group.Group | Where-Object { [double]::Parse($_.SourceHeightCm, $culture) -ge 0.0 } |
                ForEach-Object { [double]::Parse($_.MireWorstCaseClearanceCm, $culture) })
            $dryFinal = @($group.Group | Where-Object { $_.HasWater -eq '0' -and
                [double]::Parse($_.RiverWeight, $culture) -le 0.000001 -and
                [double]::Parse($_.StreamWeight, $culture) -le 0.000001 -and
                [double]::Parse($_.OceanWeight, $culture) -lt 0.5 } |
                ForEach-Object { [double]::Parse($_.FinalHeightCm, $culture) })
            $center = @($group.Group)[[int](($samplesPerProfile - 1) / 2)]
            $summaryRows.Add([pscustomobject]@{
                Control = $trial.Control
                ValueCm = [double]$value
                Site = $group.Name
                Samples = $group.Count
                SourceHeightMinCm = ($source | Measure-Object -Minimum).Minimum
                SourceHeightMaxCm = ($source | Measure-Object -Maximum).Maximum
                MireCandidateMinCm = ($candidate | Measure-Object -Minimum).Minimum
                MireCandidateMaxCm = ($candidate | Measure-Object -Maximum).Maximum
                MireCandidateRangeCm = (($candidate | Measure-Object -Maximum).Maximum - ($candidate | Measure-Object -Minimum).Minimum)
                FinalHeightMinCm = ($final | Measure-Object -Minimum).Minimum
                FinalHeightMaxCm = ($final | Measure-Object -Maximum).Maximum
                FinalHeightRangeCm = (($final | Measure-Object -Maximum).Maximum - ($final | Measure-Object -Minimum).Minimum)
                CollisionHeightMinCm = ($collision | Measure-Object -Minimum).Minimum
                CollisionHeightMaxCm = ($collision | Measure-Object -Maximum).Maximum
                CollisionHeightRangeCm = (($collision | Measure-Object -Maximum).Maximum - ($collision | Measure-Object -Minimum).Minimum)
                MinWorstCaseClearanceCm = if ($clearance.Count) { ($clearance | Measure-Object -Minimum).Minimum } else { $null }
                MinDryFinalHeightCm = if ($dryFinal.Count) { ($dryFinal | Measure-Object -Minimum).Minimum } else { $null }
                MaxTriangleSlopeDegrees = ($slopes | Measure-Object -Maximum).Maximum
                MaxTriangleGradePercent = ($grade | Measure-Object -Maximum).Maximum
                WaterSamples = @($group.Group | Where-Object { $_.HasWater -eq '1' }).Count
                CenterMireWeight = [double]::Parse($center.MossyMireWeight, $culture)
                CenterRiverWeight = [double]::Parse($center.RiverWeight, $culture)
                OutputCsv = $outputPath
            })
        }
        $trialRowsByTag[$trialTag] = $rows
    }
}

$nominalTag = 'targetheightcm-300'
if (-not $trialRowsByTag.ContainsKey($nominalTag)) { throw 'The combined nominal Mire trial is missing.' }
$nominalRows = $trialRowsByTag[$nominalTag]
foreach ($trialTag in $trialRowsByTag.Keys) {
    $trialRows = $trialRowsByTag[$trialTag]
    if ($trialRows.Count -ne $nominalRows.Count) { throw "Trial $trialTag row count differs from the nominal Mire trial." }
    for ($index = 0; $index -lt $nominalRows.Count; $index++) {
        $baseline = $nominalRows[$index]
        $actual = $trialRows[$index]
        if ($actual.Site -cne $baseline.Site -or $actual.Seed -cne $baseline.Seed -or
            [math]::Abs([double]::Parse($actual.Xcm, $culture) - [double]::Parse($baseline.Xcm, $culture)) -gt 0.01 -or
            [math]::Abs([double]::Parse($actual.Ycm, $culture) - [double]::Parse($baseline.Ycm, $culture)) -gt 0.01) {
            throw "Trial $trialTag row $($index + 1) does not align with the nominal sample."
        }
        if ($baseline.HasWater -eq '0' -and $actual.HasWater -eq '1') {
            throw "Trial $trialTag introduced new wet terrain at $($actual.Site) ($($actual.Xcm), $($actual.Ycm))."
        }
        if ($actual.HasWater -cne $baseline.HasWater) {
            throw "Trial $trialTag changed existing water coverage at $($actual.Site) ($($actual.Xcm), $($actual.Ycm))."
        }
        foreach ($column in @('BasinWeight', 'RiverWeight', 'StreamWeight')) {
            $baseValue = [double]::Parse($baseline.$column, $culture)
            $actualValue = [double]::Parse($actual.$column, $culture)
            if ([math]::Abs($actualValue - $baseValue) -gt 0.000001) {
                throw "Trial $trialTag changed $column at $($actual.Site) ($($actual.Xcm), $($actual.Ycm))."
            }
        }
        # Dry land stores a below-ground WaterLevel sentinel derived from its height;
        # compare levels only where a surface or supported water course is active.
        $hasWaterCourse = $baseline.HasWater -eq '1' -or $actual.HasWater -eq '1' -or
            [double]::Parse($baseline.BasinWeight, $culture) -gt 0.000001 -or
            [double]::Parse($actual.BasinWeight, $culture) -gt 0.000001 -or
            [double]::Parse($baseline.RiverWeight, $culture) -gt 0.000001 -or
            [double]::Parse($actual.RiverWeight, $culture) -gt 0.000001 -or
            [double]::Parse($baseline.StreamWeight, $culture) -gt 0.000001 -or
            [double]::Parse($actual.StreamWeight, $culture) -gt 0.000001
        if ($hasWaterCourse -and [math]::Abs(
            [double]::Parse($actual.WaterLevelCm, $culture) - [double]::Parse($baseline.WaterLevelCm, $culture)) -gt 0.000001) {
            throw "Trial $trialTag changed an active water level at $($actual.Site) ($($actual.Xcm), $($actual.Ycm))."
        }
    }
}

$summaryPath = Join-Path $outputRoot 'summary.csv'
if ($summaryPath.Length -ge 260) { throw "Path exceeds MAX_PATH: $summaryPath" }
$summaryRows | Export-Csv -LiteralPath $summaryPath -NoTypeInformation -Encoding UTF8
Write-Output "Mossy Mire profiles exported to $outputRoot"
Write-Output "Summary: $summaryPath"
