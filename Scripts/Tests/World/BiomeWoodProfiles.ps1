param(
    [string]$Project,
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe',
    [int]$TimeoutSeconds = 900,
    [switch]$ValidateOnly
)

$ErrorActionPreference = 'Stop'
$culture = [Globalization.CultureInfo]::InvariantCulture
$inputPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'BiomeWoodProfiles.csv'))
$expectedProfiles = @(
    [pscustomobject]@{ Seed = 419; Site = 'ElderwoodInterior419'; X = -205569.1; Y = 91450.9; Axis = 'X'; Group = 'Elderwood' },
    [pscustomobject]@{ Seed = 418; Site = 'WoodedElderwood418'; X = 207132.4; Y = -325940.4; Axis = 'Y'; Group = 'Elderwood' },
    [pscustomobject]@{ Seed = 419; Site = 'WoodedElderwood419'; X = 294675.1; Y = 171177.3; Axis = 'X'; Group = 'Elderwood' },
    [pscustomobject]@{ Seed = 418; Site = 'ElderwoodTundra418'; X = -191499.8; Y = -773033.7; Axis = 'Y'; Group = 'Transition' },
    [pscustomobject]@{ Seed = 419; Site = 'ElderwoodTundra419'; X = 314997.6; Y = 735515.4; Axis = 'X'; Group = 'Transition' },
    [pscustomobject]@{ Seed = 419; Site = 'TundraInterior419'; X = -243087.4; Y = 970004.9; Axis = 'Y'; Group = 'Tundra' }
)
$expectedSamples = $expectedProfiles.Count * 11
foreach ($path in @($inputPath)) {
    if ($path.Length -ge 260) { throw "Path exceeds MAX_PATH: $path" }
}
if (-not (Test-Path -LiteralPath $inputPath -PathType Leaf)) { throw "Profile fixture not found: $inputPath" }

$inputRows = @(Import-Csv -LiteralPath $inputPath)
if ($inputRows.Count -ne $expectedSamples) { throw "Expected $expectedSamples fixed profile rows, found $($inputRows.Count)." }
$expectedNames = @($expectedProfiles | ForEach-Object { $_.Site })
$siteGroups = @($inputRows | Group-Object Site)
if ($siteGroups.Count -ne $expectedProfiles.Count -or @($siteGroups | Where-Object { $_.Count -ne 11 }).Count -ne 0) {
    throw 'Fixture must contain six fixed 11-sample transects.'
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
        throw "Profile $($profile.Site) center differs from its seed-fixed baseline coordinate."
    }
}
if (@($siteGroups | Where-Object { $_.Name -notin $expectedNames }).Count -ne 0) { throw 'Fixture contains an unexpected site.' }

if ($ValidateOnly) {
    Write-Output "Validated $expectedSamples fixed Elderwood/Tundra samples across six 100 m transects."
    return
}
if ([string]::IsNullOrWhiteSpace($Project)) { throw 'Project is required unless -ValidateOnly is specified.' }

$projectPath = [IO.Path]::GetFullPath($Project)
$editorPath = [IO.Path]::GetFullPath($Editor)
$outputRoot = Join-Path $env:TEMP ('bew' + [guid]::NewGuid().ToString('N').Substring(0, 8))
foreach ($path in @($projectPath, $editorPath, $outputRoot)) {
    if ($path.Length -ge 260) { throw "Path exceeds MAX_PATH: $path" }
}
if (-not (Test-Path -LiteralPath $projectPath -PathType Leaf)) { throw "Project file not found: $projectPath" }
if (-not (Test-Path -LiteralPath $editorPath -PathType Leaf)) { throw "Unreal Editor commandlet not found: $editorPath" }

$trials = @(
    [pscustomobject]@{ Layer = 'Broad'; Parameter = 'ElderwoodBroadAmplitudeM'; Values = @(4.0, 9.0, 16.0) },
    [pscustomobject]@{ Layer = 'Middle'; Parameter = 'ElderwoodMiddleAmplitudeM'; Values = @(0.6, 2.4, 4.0) },
    [pscustomobject]@{ Layer = 'Local'; Parameter = 'ElderwoodLocalAmplitudeM'; Values = @(0.1, 0.4, 1.0) }
)
$summaryRows = [Collections.Generic.List[object]]::new()

foreach ($trial in $trials) {
    foreach ($amplitudeM in $trial.Values) {
        $trialTag = $trial.Layer.ToLowerInvariant() + '-' + $amplitudeM.ToString('0.###', $culture) + 'm'
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
            ('-' + $trial.Parameter + '=' + $amplitudeM.ToString('0.###', $culture)),
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
                throw "Elderwood profile trial $trialTag timed out. See $logPath"
            }
            if ($process.ExitCode -ne 0) {
                $tail = if (Test-Path -LiteralPath $logPath) { (Get-Content -LiteralPath $logPath -Tail 30) -join "`n" } else { '(no log created)' }
                throw "Elderwood profile trial $trialTag failed with exit $($process.ExitCode).`n$tail"
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
        $effective = @{ Broad = 9.0; Middle = 2.4; Local = 0.4 }
        $effective[$trial.Layer] = [double]$amplitudeM
        $marker = ('Elderwood profile trial accepted: BroadAmplitudeM=' + $effective.Broad.ToString('0.000', $culture) +
            ' MiddleAmplitudeM=' + $effective.Middle.ToString('0.000', $culture) +
            ' LocalAmplitudeM=' + $effective.Local.ToString('0.000', $culture))
        $logText = Get-Content -LiteralPath $logPath -Raw
        if ($logText.IndexOf($marker, [StringComparison]::Ordinal) -lt 0) {
            throw "Fresh Elderwood-trial marker not found for $trialTag; loaded binary may be stale. See $logPath"
        }
        if ($logText -notmatch ('Exported ' + $expectedSamples + ' numeric world-generation samples')) {
            throw "Trial $trialTag did not export the expected sample count. See $logPath"
        }

        $rows = @(Import-Csv -LiteralPath $outputPath)
        if ($rows.Count -ne $expectedSamples) { throw "Trial $trialTag returned $($rows.Count) rows, expected $expectedSamples." }
        foreach ($row in $rows) {
            foreach ($column in @('FinalHeightCm', 'ElderwoodWeight', 'FreezingTundraWeight', 'WaterLevelCm', 'CollisionTriangleHeightCm', 'CollisionTriangleSlopeDegrees', 'ElderwoodCandidateCm', 'TundraCandidateCm', 'ReliefBroad', 'ReliefMiddle', 'ReliefLocal', 'ElderwoodBroadInputCm', 'ElderwoodMiddleInputCm', 'ElderwoodLocalInputCm')) {
                $number = 0.0
                $parsed = [double]::TryParse([string]$row.$column, [Globalization.NumberStyles]::Float, $culture, [ref]$number)
                if (-not $parsed -or [double]::IsNaN($number) -or [double]::IsInfinity($number)) {
                    throw "Trial $trialTag has a non-finite $column for $($row.Site)."
                }
            }
            if ([math]::Abs([double]$row.ElderwoodCandidateCm - [double]$row.TundraCandidateCm) -gt 0.0001) {
                throw "Trial $trialTag has divergent Elderwood/Tundra candidates for $($row.Site)."
            }
            $weightTotal = 0.0
            foreach ($column in @('MeadowsWeight', 'ShimmeringLakesWeight', 'ElderwoodWeight', 'MossyMireWeight', 'FreezingTundraWeight', 'ThunderMountainsWeight', 'OceanWeight')) {
                $weightTotal += [double]$row.$column
            }
            if ([math]::Abs($weightTotal - 1.0) -gt 0.0001) { throw "Trial $trialTag has unnormalized weights for $($row.Site)." }
        }
        foreach ($profile in $expectedProfiles) {
            $center = @($rows | Where-Object { $_.Site -ceq $profile.Site })[5]
            if ($center.HasWater -ne '0') { throw "Fixed control $($profile.Site) is wet in trial $trialTag." }
            if ($profile.Group -eq 'Elderwood' -and [double]$center.ElderwoodWeight -lt 0.45) { throw "Fixed Elderwood control lost its biome weight in $trialTag." }
            if ($profile.Group -eq 'Tundra' -and [double]$center.FreezingTundraWeight -lt 0.5) { throw "Fixed Tundra control lost its biome weight in $trialTag." }
            if ($profile.Group -eq 'Transition' -and ([double]$center.ElderwoodWeight -lt 0.35 -or [double]$center.FreezingTundraWeight -lt 0.35)) {
                throw "Fixed Elderwood/Tundra blend lost one candidate weight in $trialTag."
            }
        }

        foreach ($group in $rows | Group-Object Site) {
            $candidate = @($group.Group | ForEach-Object { [double]::Parse($_.ElderwoodCandidateCm, $culture) })
            $final = @($group.Group | ForEach-Object { [double]::Parse($_.FinalHeightCm, $culture) })
            $slopes = @($group.Group | ForEach-Object { [double]::Parse($_.CollisionTriangleSlopeDegrees, $culture) })
            $summaryRows.Add([pscustomobject]@{
                Layer = $trial.Layer
                AmplitudeM = $amplitudeM
                Site = $group.Name
                Samples = $group.Count
                CandidateRangeCm = (($candidate | Measure-Object -Maximum).Maximum - ($candidate | Measure-Object -Minimum).Minimum)
                FinalHeightRangeCm = (($final | Measure-Object -Maximum).Maximum - ($final | Measure-Object -Minimum).Minimum)
                MaxTriangleSlopeDegrees = ($slopes | Measure-Object -Maximum).Maximum
                OutputCsv = $outputPath
            })
        }
    }
}

$summaryPath = Join-Path $outputRoot 'summary.csv'
if ($summaryPath.Length -ge 260) { throw "Path exceeds MAX_PATH: $summaryPath" }
$summaryRows | Export-Csv -LiteralPath $summaryPath -NoTypeInformation -Encoding UTF8
Write-Output "Elderwood/Tundra profiles exported to $outputRoot"
Write-Output "Summary: $summaryPath"
