param(
    [Parameter(Mandatory)][string]$Project,
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe',
    [int]$TimeoutSeconds = 900,
    [switch]$ValidateOnly
)

$ErrorActionPreference = 'Stop'
$culture = [Globalization.CultureInfo]::InvariantCulture
$projectPath = [IO.Path]::GetFullPath($Project)
$inputPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'BiomeRollingTransects.csv'))
$baselinePath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'BiomeBaseline418419.csv'))
$editorPath = [IO.Path]::GetFullPath($Editor)
$outputRoot = Join-Path $env:TEMP ('brp' + [guid]::NewGuid().ToString('N').Substring(0, 8))
$expectedRows = 264
$expectedProfiles = 24
$expectedSeams = 8
$profileCounts = @{ '30m' = 7; '45m' = 10; '70m' = 15 }

foreach ($path in @($projectPath, $inputPath, $baselinePath, $editorPath, $outputRoot)) {
    if ($path.Length -ge 260) { throw "Path exceeds MAX_PATH: $path" }
}
foreach ($path in @($projectPath, $inputPath, $baselinePath)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required input not found: $path" }
}

$inputRows = @(Import-Csv -LiteralPath $inputPath)
if ($inputRows.Count -ne $expectedRows) { throw "Expected $expectedRows fixed samples, found $($inputRows.Count)." }
$profileRows = @($inputRows | Where-Object { $_.Site -notmatch 'Patch[XY]' })
$seamRows = @($inputRows | Where-Object { $_.Site -match 'Patch[XY]' })
$profileGroups = @($profileRows | Group-Object Site)
$seamGroups = @($seamRows | Group-Object Site)
if ($profileGroups.Count -ne $expectedProfiles -or $seamGroups.Count -ne $expectedSeams) {
    throw "Expected $expectedProfiles profiles and $expectedSeams patch-side samples."
}
foreach ($group in $profileGroups) {
    $expectedCount = $null
    foreach ($suffix in $profileCounts.Keys) {
        if ($group.Name.EndsWith($suffix, [StringComparison]::Ordinal)) { $expectedCount = $profileCounts[$suffix] }
    }
    if ($null -eq $expectedCount -or $group.Count -ne $expectedCount) {
        throw "Profile $($group.Name) has an unexpected sample count."
    }
}
if (@($seamGroups | Where-Object { $_.Count -ne 1 }).Count -ne 0) {
    throw 'Patch seam controls must contain one fixed sample on each side.'
}

$baseline = @(Import-Csv -LiteralPath $baselinePath)
foreach ($group in $profileGroups) {
    if ($group.Name -notmatch '^S(?<seed>418|419)_(?<anchor>StarterMeadows|OuterdryMeadows)_(?<axis>EW|NS)_(?<length>30|45|70)m$') {
        throw "Unknown rolling profile name: $($group.Name)."
    }
    $seed = [int]$Matches.seed
    $anchorSite = [string]$Matches.anchor
    $axis = [string]$Matches.axis
    $lengthCm = [double]::Parse($Matches.length, $culture) * 100.0
    $anchor = $baseline | Where-Object { [int]$_.Seed -eq $seed -and $_.Site -eq $anchorSite } | Select-Object -First 1
    if ($null -eq $anchor) { throw "Missing fixed seed-$seed $anchorSite baseline." }
    $anchorX = [double]::Parse($anchor.Xcm, $culture)
    $anchorY = [double]::Parse($anchor.Ycm, $culture)
    $values = if ($axis -eq 'EW') {
        @($group.Group | ForEach-Object { [double]::Parse($_.Xcm, $culture) } | Sort-Object)
    }
    else {
        @($group.Group | ForEach-Object { [double]::Parse($_.Ycm, $culture) } | Sort-Object)
    }
    $fixedValues = if ($axis -eq 'EW') {
        @($group.Group | ForEach-Object { [double]::Parse($_.Ycm, $culture) })
    }
    else {
        @($group.Group | ForEach-Object { [double]::Parse($_.Xcm, $culture) })
    }
    $anchorAxis = if ($axis -eq 'EW') { $anchorX } else { $anchorY }
    if ([math]::Abs($values[0] - ($anchorAxis - $lengthCm / 2.0)) -gt 0.01 -or
        [math]::Abs($values[-1] - ($anchorAxis + $lengthCm / 2.0)) -gt 0.01) {
        throw "Rolling profile $($group.Name) does not cover its documented length."
    }
    $fixedAxisValue = if ($axis -eq 'EW') { $anchorY } else { $anchorX }
    if (@($fixedValues | Where-Object { [math]::Abs($_ - $fixedAxisValue) -gt 0.01 }).Count -ne 0) {
        throw "Rolling profile $($group.Name) has a changed fixed axis."
    }
    for ($index = 1; $index -lt $values.Count; $index++) {
        if ([math]::Abs(($values[$index] - $values[$index - 1]) - 500.0) -gt 0.01) {
            throw "Rolling profile $($group.Name) does not use 5 m spacing."
        }
    }
}
foreach ($seed in @(418, 419)) {
    $anchor = $baseline | Where-Object { [int]$_.Seed -eq $seed -and $_.Site -eq 'OuterdryMeadows' } | Select-Object -First 1
    if ($null -eq $anchor) { throw "Missing fixed seed-$seed outer-Meadows baseline." }
    $originX = [double]::Parse($anchor.PatchOriginXcm, $culture)
    $originY = [double]::Parse($anchor.PatchOriginYcm, $culture)
    $edgeX = $originX + ([int]$anchor.PatchX + 0.5) * 3000
    $edgeY = $originY + ([int]$anchor.PatchY + 0.5) * 3000
    $siteRows = @($seamRows | Where-Object { $_.Site.StartsWith(('S' + $seed + '_'), [StringComparison]::Ordinal) })
    if ($siteRows.Count -ne 4) { throw "Seed $seed must have X and Y seam pairs." }
    $expectedSites = @{}
    $anchorXValue = [double]::Parse($anchor.Xcm, $culture)
    $anchorYValue = [double]::Parse($anchor.Ycm, $culture)
    $expectedSites[('S' + $seed + '_OuterPatchXWest')] = [double[]]@(($edgeX - 0.5), $anchorYValue)
    $expectedSites[('S' + $seed + '_OuterPatchXEast')] = [double[]]@(($edgeX + 0.5), $anchorYValue)
    $expectedSites[('S' + $seed + '_OuterPatchYSouth')] = [double[]]@($anchorXValue, ($edgeY - 0.5))
    $expectedSites[('S' + $seed + '_OuterPatchYNorth')] = [double[]]@($anchorXValue, ($edgeY + 0.5))
    foreach ($siteName in $expectedSites.Keys) {
        $row = $seamRows | Where-Object { $_.Site -eq $siteName } | Select-Object -First 1
        if ($null -eq $row) { throw "Missing patch seam sample $siteName." }
        $xDelta = [math]::Abs([double]::Parse($row.Xcm, $culture) - $expectedSites[$siteName][0])
        $yDelta = [math]::Abs([double]::Parse($row.Ycm, $culture) - $expectedSites[$siteName][1])
        if ($xDelta -gt 0.01 -or $yDelta -gt 0.01) {
            throw "Patch seam sample $siteName no longer matches BH01 start-relative patch bounds."
        }
    }
}

if ($ValidateOnly) {
    Write-Output "Validated $expectedRows fixed rolling samples, $expectedProfiles profiles, and $expectedSeams patch-side points."
    return
}
if (-not (Test-Path -LiteralPath $editorPath -PathType Leaf)) { throw "Unreal Editor commandlet not found: $editorPath" }

New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
$summaryRows = [Collections.Generic.List[object]]::new()
foreach ($amplitudeM in @(0.6, 1.2, 1.8)) {
    $amplitudeText = $amplitudeM.ToString('0.0', $culture)
    $trialDirectory = Join-Path $outputRoot ('roll-' + $amplitudeText.Replace('.', '_') + 'm')
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
        ('-MeadowRollingAmplitudeM=' + $amplitudeText),
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
            throw "Rolling profile trial $amplitudeText m timed out. See $logPath"
        }
        if ($process.ExitCode -ne 0) {
            $tail = if (Test-Path -LiteralPath $logPath) { (Get-Content -LiteralPath $logPath -Tail 30) -join ([string][char]10) } else { '(no log created)' }
            throw ("Rolling profile trial " + $amplitudeText + " m failed with exit " + $process.ExitCode + "." + [string][char]10 + $tail)
        }
    }
    finally {
        if ($null -ne $process -and -not $process.HasExited) {
            Stop-Process -Id $process.Id -Force
            $process.WaitForExit()
        }
    }

    if (-not (Test-Path -LiteralPath $outputPath -PathType Leaf) -or -not (Test-Path -LiteralPath $logPath -PathType Leaf)) {
        throw "Trial $amplitudeText m did not produce both numeric output and a log."
    }
    $logText = Get-Content -LiteralPath $logPath -Raw
    $marker = 'Meadow rolling sample trial accepted: AmplitudeM=' + $amplitudeM.ToString('0.000', $culture)
    if ($logText.IndexOf($marker, [StringComparison]::Ordinal) -lt 0) {
        throw "Fresh rolling-trial commandlet marker was not found for $amplitudeText m; the loaded binary may be stale. See $logPath"
    }
    if ($logText -notmatch ('Exported ' + $expectedRows + ' numeric world-generation samples')) {
        throw "Trial $amplitudeText m did not export the expected sample count. See $logPath"
    }

    $rows = @(Import-Csv -LiteralPath $outputPath)
    if ($rows.Count -ne $expectedRows) { throw "Trial $amplitudeText m returned $($rows.Count) rows, expected $expectedRows." }
    foreach ($row in $rows) {
        foreach ($column in @('FinalHeightCm', 'MeadowsWeight', 'WaterLevelCm', 'CollisionTriangleHeightCm', 'CollisionTriangleSlopeDegrees', 'CollisionTriangleGradePercent')) {
            $number = 0.0
            $parsed = [double]::TryParse([string]$row.$column, [Globalization.NumberStyles]::Float, $culture, [ref]$number)
            if (-not $parsed -or [double]::IsNaN($number) -or [double]::IsInfinity($number)) {
                throw "Trial $amplitudeText m has a non-finite $column for $($row.Site)."
            }
        }
        if ([int]$row.HasWater -ne 0 -or [double]::Parse($row.MeadowsWeight, $culture) -le 0.5) {
            throw "Trial $amplitudeText m left fixed dry-Meadows profile $($row.Site)."
        }
    }

    foreach ($group in $rows | Where-Object { $_.Site -notmatch 'Patch[XY]' } | Group-Object Site) {
        $heights = @($group.Group | ForEach-Object { [double]::Parse($_.FinalHeightCm, $culture) })
        $slopes = @($group.Group | ForEach-Object { [double]::Parse($_.CollisionTriangleSlopeDegrees, $culture) })
        $grades = @($group.Group | ForEach-Object { [double]::Parse($_.CollisionTriangleGradePercent, $culture) })
        $summaryRows.Add([pscustomobject]@{
            AmplitudeM = $amplitudeText
            Site = $group.Name
            Samples = $group.Count
            ProfileHeightRangeCm = (($heights | Measure-Object -Maximum).Maximum - ($heights | Measure-Object -Minimum).Minimum)
            MaxTriangleSlopeDegrees = ($slopes | Measure-Object -Maximum).Maximum
            MaxTriangleGradePercent = ($grades | Measure-Object -Maximum).Maximum
            SeamHeightDeltaCm = ''
            SeamCollisionDeltaCm = ''
            OutputCsv = $outputPath
        })
    }

    foreach ($seed in @(418, 419)) {
        foreach ($axis in @('X', 'Y')) {
            $pair = @($rows | Where-Object { $_.Site -match ("^S" + $seed + "_OuterPatch" + $axis) })
            if ($pair.Count -ne 2) { throw "Trial $amplitudeText m is missing seed-$seed $axis seam pair." }
            $westOrSouth = $pair | Where-Object { $_.Site -match '(West|South)$' } | Select-Object -First 1
            $eastOrNorth = $pair | Where-Object { $_.Site -match '(East|North)$' } | Select-Object -First 1
            if ($null -eq $westOrSouth -or $null -eq $eastOrNorth) { throw "Trial $amplitudeText m has a malformed seed-$seed $axis seam pair." }
            if ($axis -eq 'X') {
                if ([int]$eastOrNorth.PatchX -ne ([int]$westOrSouth.PatchX + 1) -or [int]$eastOrNorth.PatchY -ne [int]$westOrSouth.PatchY) {
                    throw "Trial $amplitudeText m X seam did not cross adjacent start-relative patches."
                }
            }
            else {
                if ([int]$eastOrNorth.PatchY -ne ([int]$westOrSouth.PatchY + 1) -or [int]$eastOrNorth.PatchX -ne [int]$westOrSouth.PatchX) {
                    throw "Trial $amplitudeText m Y seam did not cross adjacent start-relative patches."
                }
            }
            $heightDelta = [math]::Abs([double]::Parse($eastOrNorth.FinalHeightCm, $culture) - [double]::Parse($westOrSouth.FinalHeightCm, $culture))
            $collisionDelta = [math]::Abs([double]::Parse($eastOrNorth.CollisionTriangleHeightCm, $culture) - [double]::Parse($westOrSouth.CollisionTriangleHeightCm, $culture))
            if ($heightDelta -gt 5.0 -or $collisionDelta -gt 5.0) {
                throw "Trial $amplitudeText m exceeded the 5 cm seed-$seed $axis patch-seam tolerance."
            }
            $summaryRows.Add([pscustomobject]@{
                AmplitudeM = $amplitudeText
                Site = ("S" + $seed + '_Patch' + $axis)
                Samples = 2
                ProfileHeightRangeCm = ''
                MaxTriangleSlopeDegrees = ''
                MaxTriangleGradePercent = ''
                SeamHeightDeltaCm = $heightDelta
                SeamCollisionDeltaCm = $collisionDelta
                OutputCsv = $outputPath
            })
        }
    }
}

$summaryPath = Join-Path $outputRoot 'summary.csv'
if ($summaryPath.Length -ge 260) { throw "Path exceeds MAX_PATH: $summaryPath" }
$summaryRows | Export-Csv -LiteralPath $summaryPath -NoTypeInformation -Encoding UTF8
Write-Output "Meadows rolling profiles exported to $outputRoot"
Write-Output "Summary: $summaryPath"
