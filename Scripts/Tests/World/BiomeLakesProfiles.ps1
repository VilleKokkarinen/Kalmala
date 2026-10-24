param(
    [string]$Project,
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe',
    [int]$TimeoutSeconds = 900,
    [switch]$ValidateOnly
)

$ErrorActionPreference = 'Stop'
$culture = [Globalization.CultureInfo]::InvariantCulture
$inputPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'BiomeLakesProfiles.csv'))
$expectedProfiles = @(
    [pscustomobject]@{ Seed = 418; Site = 'LakesFormerBasin418_X'; X = 100830.5; Y = -143038.6; Axis = 'X'; Kind = 'Composite' },
    [pscustomobject]@{ Seed = 418; Site = 'LakesFormerBasin418_Y'; X = 100830.5; Y = -143038.6; Axis = 'Y'; Kind = 'Composite' },
    [pscustomobject]@{ Seed = 419; Site = 'LakesInterior419_X'; X = -166487.5; Y = 53932.6; Axis = 'X'; Kind = 'Interior' },
    [pscustomobject]@{ Seed = 419; Site = 'LakesInterior419_Y'; X = -166487.5; Y = 53932.6; Axis = 'Y'; Kind = 'Interior' }
)
$expectedSamples = $expectedProfiles.Count * 11
if ($inputPath.Length -ge 260) { throw "Path exceeds MAX_PATH: $inputPath" }
if (-not (Test-Path -LiteralPath $inputPath -PathType Leaf)) { throw "Profile fixture not found: $inputPath" }

$inputRows = @(Import-Csv -LiteralPath $inputPath)
if ($inputRows.Count -ne $expectedSamples) { throw "Expected $expectedSamples fixed profile rows, found $($inputRows.Count)." }
$expectedNames = @($expectedProfiles | ForEach-Object { $_.Site })
$siteGroups = @($inputRows | Group-Object Site)
if ($siteGroups.Count -ne $expectedProfiles.Count -or @($siteGroups | Where-Object { $_.Count -ne 11 }).Count -ne 0) {
    throw 'Fixture must contain four fixed 11-sample transects.'
}

foreach ($profile in $expectedProfiles) {
    $group = @($inputRows | Where-Object { $_.Site -ceq $profile.Site })
    if ($group.Count -ne 11) { throw "Missing fixed profile $($profile.Site)." }
    $actualSeeds = @($group | ForEach-Object { [uint64]::Parse($_.Seed, $culture) } | Select-Object -Unique)
    if ($actualSeeds.Count -ne 1 -or $actualSeeds[0] -ne $profile.Seed) { throw "Profile $($profile.Site) has the wrong seed." }

    for ($index = 0; $index -lt 11; $index++) {
        $offset = ($index - 5) * 1000.0
        $x = [double]::Parse($group[$index].Xcm, $culture)
        $y = [double]::Parse($group[$index].Ycm, $culture)
        if ([double]::IsNaN($x) -or [double]::IsInfinity($x) -or [double]::IsNaN($y) -or [double]::IsInfinity($y)) {
            throw "Profile $($profile.Site) contains a non-finite coordinate."
        }
        $expectedX = [double]$profile.X + $(if ($profile.Axis -eq 'X') { $offset } else { 0.0 })
        $expectedY = [double]$profile.Y + $(if ($profile.Axis -eq 'Y') { $offset } else { 0.0 })
        if ([math]::Abs($x - $expectedX) -gt 0.01 -or [math]::Abs($y - $expectedY) -gt 0.01) {
            throw "Profile $($profile.Site) does not match its 1,000 cm fixed transect."
        }
    }
    $centerX = [double]::Parse($group[5].Xcm, $culture)
    $centerY = [double]::Parse($group[5].Ycm, $culture)
    if ([math]::Abs($centerX - $profile.X) -gt 0.01 -or [math]::Abs($centerY - $profile.Y) -gt 0.01) {
        throw "Profile $($profile.Site) center differs from its BH01 baseline coordinate."
    }
}
if (@($siteGroups | Where-Object { $_.Name -notin $expectedNames }).Count -ne 0) { throw 'Fixture contains an unexpected site.' }

if ($ValidateOnly) {
    Write-Output "Validated $expectedSamples fixed Lakes samples across four 100 m transects."
    return
}
if ([string]::IsNullOrWhiteSpace($Project)) { throw 'Project is required unless -ValidateOnly is specified.' }

$projectPath = [IO.Path]::GetFullPath($Project)
$editorPath = [IO.Path]::GetFullPath($Editor)
$outputRoot = Join-Path $env:TEMP ('bl' + [guid]::NewGuid().ToString('N').Substring(0, 8))
foreach ($path in @($projectPath, $editorPath, $outputRoot)) {
    if ($path.Length -ge 260) { throw "Path exceeds MAX_PATH: $path" }
}
if (-not (Test-Path -LiteralPath $projectPath -PathType Leaf)) { throw "Project file not found: $projectPath" }
if (-not (Test-Path -LiteralPath $editorPath -PathType Leaf)) { throw "Unreal Editor commandlet not found: $editorPath" }

$trials = @(
    [pscustomobject]@{ Control = 'MeadowShare'; Parameter = 'ShimmeringLakesMeadowShare'; Values = @(0.35, 0.50, 0.65) },
    [pscustomobject]@{ Control = 'LowlandFloorCm'; Parameter = 'ShimmeringLakesLowlandFloorCm'; Values = @(100.0, 200.0, 500.0) },
    [pscustomobject]@{ Control = 'LowlandCeilingCm'; Parameter = 'ShimmeringLakesLowlandCeilingCm'; Values = @(500.0, 1200.0, 2000.0) },
    [pscustomobject]@{ Control = 'LowlandCompression'; Parameter = 'ShimmeringLakesLowlandCompression'; Values = @(0.05, 0.20, 0.35) }
)
$summaryRows = [Collections.Generic.List[object]]::new()
$smoothStep = {
    param([double]$A, [double]$B, [double]$X)
    $value = [math]::Max(0.0, [math]::Min(1.0, ($X - $A) / ($B - $A)))
    return $value * $value * (3.0 - 2.0 * $value)
}

foreach ($trial in $trials) {
    foreach ($value in $trial.Values) {
        $effective = @{ MeadowShare = 0.50; LowlandFloorCm = 200.0; LowlandCeilingCm = 1200.0; LowlandCompression = 0.20 }
        $effective[$trial.Control] = [double]$value
        if ($effective.LowlandCeilingCm -le $effective.LowlandFloorCm) {
            throw "Trial $($trial.Control)=$value violates ceiling > floor."
        }
        $trialValue = ([double]$value).ToString('0.###', $culture)
        $trialTag = $trial.Control.ToLowerInvariant() + '-' + $trialValue
        $trialDirectory = Join-Path $outputRoot $trialTag
        $outputPath = Join-Path $trialDirectory 'samples.csv'
        $logPath = Join-Path $trialDirectory 'editor.log'
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
                throw "Shimmering Lakes trial $trialTag timed out. See $logPath"
            }
            if ($process.ExitCode -ne 0) {
                $tail = if (Test-Path -LiteralPath $logPath) { (Get-Content -LiteralPath $logPath -Tail 30) -join "`n" } else { '(no log created)' }
                throw "Shimmering Lakes trial $trialTag failed with exit $($process.ExitCode).`n$tail"
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
        $marker = ('Shimmering Lakes profile trial accepted: MeadowShare=' + $effective.MeadowShare.ToString('0.000', $culture) +
            ' LowlandFloorCm=' + $effective.LowlandFloorCm.ToString('0.0', $culture) +
            ' LowlandCeilingCm=' + $effective.LowlandCeilingCm.ToString('0.0', $culture) +
            ' LowlandCompression=' + $effective.LowlandCompression.ToString('0.000', $culture))
        $logText = Get-Content -LiteralPath $logPath -Raw
        if ($logText.IndexOf($marker, [StringComparison]::Ordinal) -lt 0) {
            throw "Fresh Shimmering Lakes trial marker not found for $trialTag; loaded binary may be stale. See $logPath"
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
            'CollisionTriangleHeightCm', 'CollisionTriangleSlopeDegrees', 'ShimmeringLakesCandidateCm',
            'MeadowCandidateCm', 'ElderwoodCandidateCm', 'LowlandCandidateCm', 'SourceHeightCm', 'Humidity',
            'WetlandHumidityMinimum', 'WetlandHumidityFull', 'MeadowShare', 'LowlandFloorCm',
            'LowlandCeilingCm', 'LowlandCompression', 'BasinWeight', 'RiverWeight', 'StreamWeight'
        )
        foreach ($row in $rows) {
            $valuesByColumn = @{}
            foreach ($column in $numericColumns) {
                $number = 0.0
                $parsed = [double]::TryParse([string]$row.$column, [Globalization.NumberStyles]::Float, $culture, [ref]$number)
                if (-not $parsed -or [double]::IsNaN($number) -or [double]::IsInfinity($number)) {
                    throw "Trial $trialTag has a non-finite $column for $($row.Site)."
                }
                $valuesByColumn[$column] = $number
            }
            $weightTotal = 0.0
            foreach ($column in @('MeadowsWeight', 'ShimmeringLakesWeight', 'ElderwoodWeight', 'MossyMireWeight', 'FreezingTundraWeight', 'ThunderMountainsWeight', 'OceanWeight')) {
                $weightTotal += $valuesByColumn[$column]
            }
            if ([math]::Abs($weightTotal - 1.0) -gt 0.0001) { throw "Trial $trialTag has unnormalized weights for $($row.Site)." }
            if ([math]::Abs($valuesByColumn.BasinWeight) -gt 0.000001) { throw "Trial $trialTag enabled analytic basin support for $($row.Site)." }
            if ($row.HasWater -eq '1' -and ($valuesByColumn.RiverWeight + $valuesByColumn.StreamWeight + $valuesByColumn.OceanWeight) -le 0.000001) {
                throw "Trial $trialTag has water without ocean or a river/stream course for $($row.Site)."
            }
            foreach ($field in $effective.Keys) {
                if ([math]::Abs($valuesByColumn[$field] - $effective[$field]) -gt 0.000001) {
                    throw "Trial $trialTag reports stale or unexpected $field for $($row.Site)."
                }
            }

            $source = $valuesByColumn.SourceHeightCm
            $floor = $valuesByColumn.LowlandFloorCm
            $ceiling = $valuesByColumn.LowlandCeilingCm
            $compression = $valuesByColumn.LowlandCompression
            $humidityWeight = & $smoothStep $valuesByColumn.WetlandHumidityMinimum $valuesByColumn.WetlandHumidityFull $valuesByColumn.Humidity
            $bandWidth = ($ceiling - $floor) * 0.25
            $band = (& $smoothStep $floor ($floor + $bandWidth) $source) * (1.0 - (& $smoothStep ($ceiling - $bandWidth) $ceiling $source))
            $expectedLowland = $source - [math]::Max(0.0, $source - $floor) * $compression * $humidityWeight * $band
            if ([math]::Abs($valuesByColumn.LowlandCandidateCm - $expectedLowland) -gt 0.02) {
                throw "Trial $trialTag lowland candidate violates its bounded humidity/height formula at $($row.Site)."
            }
            if ($source -gt $floor -and ($valuesByColumn.LowlandCandidateCm -lt ($floor - 0.02) -or $valuesByColumn.LowlandCandidateCm -gt ($source + 0.02))) {
                throw "Trial $trialTag lowland candidate escaped its source/floor bounds at $($row.Site)."
            }
            $shoulderBlend = $valuesByColumn.ElderwoodCandidateCm +
                ($valuesByColumn.MeadowCandidateCm - $valuesByColumn.ElderwoodCandidateCm) * $valuesByColumn.MeadowShare
            $expectedLakes = $shoulderBlend + $valuesByColumn.LowlandCandidateCm - $source
            if ([math]::Abs($valuesByColumn.ShimmeringLakesCandidateCm - $expectedLakes) -gt 0.02) {
                throw "Trial $trialTag Lakes candidate does not match its blended lowland composition at $($row.Site)."
            }
        }

        foreach ($profile in $expectedProfiles) {
            $center = @($rows | Where-Object { $_.Site -ceq $profile.Site })[5]
            if ($center.HasWater -ne '0') { throw "Fixed Lakes control $($profile.Site) is wet in trial $trialTag." }
            $minimumLakeWeight = if ($profile.Kind -eq 'Composite') { 0.40 } else { 0.55 }
            if ([double]$center.ShimmeringLakesWeight -lt $minimumLakeWeight) {
                throw "Fixed Lakes control $($profile.Site) lost its expected biome weight in $trialTag."
            }
        }

        foreach ($group in $rows | Group-Object Site) {
            $lakeCandidate = @($group.Group | ForEach-Object { [double]::Parse($_.ShimmeringLakesCandidateCm, $culture) })
            $lowland = @($group.Group | ForEach-Object { [double]::Parse($_.LowlandCandidateCm, $culture) })
            $final = @($group.Group | ForEach-Object { [double]::Parse($_.FinalHeightCm, $culture) })
            $slopes = @($group.Group | ForEach-Object { [double]::Parse($_.CollisionTriangleSlopeDegrees, $culture) })
            $lakeWeights = @($group.Group | ForEach-Object { [double]::Parse($_.ShimmeringLakesWeight, $culture) })
            $summaryRows.Add([pscustomobject]@{
                Control = $trial.Control
                Value = [double]$value
                Site = $group.Name
                Samples = $group.Count
                LakesCandidateRangeCm = (($lakeCandidate | Measure-Object -Maximum).Maximum - ($lakeCandidate | Measure-Object -Minimum).Minimum)
                LowlandCandidateRangeCm = (($lowland | Measure-Object -Maximum).Maximum - ($lowland | Measure-Object -Minimum).Minimum)
                FinalHeightRangeCm = (($final | Measure-Object -Maximum).Maximum - ($final | Measure-Object -Minimum).Minimum)
                MinLakesWeight = ($lakeWeights | Measure-Object -Minimum).Minimum
                MaxLakesWeight = ($lakeWeights | Measure-Object -Maximum).Maximum
                MaxTriangleSlopeDegrees = ($slopes | Measure-Object -Maximum).Maximum
                OutputCsv = $outputPath
            })
        }
    }
}

$summaryPath = Join-Path $outputRoot 'summary.csv'
if ($summaryPath.Length -ge 260) { throw "Path exceeds MAX_PATH: $summaryPath" }
$summaryRows | Export-Csv -LiteralPath $summaryPath -NoTypeInformation -Encoding UTF8
Write-Output "Shimmering Lakes profiles exported to $outputRoot"
Write-Output "Summary: $summaryPath"
