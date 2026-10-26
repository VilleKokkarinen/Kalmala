param(
    [Parameter(Mandatory)][string]$Project,
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe',
    [int]$TimeoutSeconds = 900
)

$ErrorActionPreference = 'Stop'
$culture = [Globalization.CultureInfo]::InvariantCulture
$projectPath = [IO.Path]::GetFullPath($Project)
$inputPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'BiomeHillTransects.csv'))
$editorPath = [IO.Path]::GetFullPath($Editor)
$outputRoot = Join-Path $env:TEMP ('bhp' + [guid]::NewGuid().ToString('N').Substring(0, 8))
$expectedSamples = 88

foreach ($path in @($projectPath, $inputPath, $editorPath, $outputRoot)) {
    if ($path.Length -ge 260) { throw "Path exceeds MAX_PATH: $path" }
}
if (-not (Test-Path -LiteralPath $projectPath -PathType Leaf)) { throw "Project file not found: $projectPath" }
if (-not (Test-Path -LiteralPath $editorPath -PathType Leaf)) { throw "Unreal Editor commandlet not found: $editorPath" }
if (-not (Test-Path -LiteralPath $inputPath -PathType Leaf)) { throw "Transect input not found: $inputPath" }

$inputRows = @(Import-Csv -LiteralPath $inputPath)
if ($inputRows.Count -ne $expectedSamples) { throw "Expected $expectedSamples fixed transect rows, found $($inputRows.Count)." }
$siteGroups = @($inputRows | Group-Object Site)
if ($siteGroups.Count -ne 8 -or @($siteGroups | Where-Object { $_.Count -ne 11 }).Count -ne 0) {
    throw 'Transect fixture must contain eight 11-sample profiles.'
}

New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
$summaryRows = [Collections.Generic.List[object]]::new()

foreach ($amplitudeM in @(4, 6, 8)) {
    $trialName = 'hill-' + $amplitudeM.ToString($culture) + 'm'
    $trialDirectory = Join-Path $outputRoot $trialName
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
        "-MeadowHillAmplitudeM=$($amplitudeM.ToString($culture))",
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
            throw "Hill profile trial $amplitudeM m timed out. See $logPath"
        }
        if ($process.ExitCode -ne 0) {
            $tail = if (Test-Path -LiteralPath $logPath) { (Get-Content -LiteralPath $logPath -Tail 30) -join "`n" } else { '(no log created)' }
            throw "Hill profile trial $amplitudeM m failed with exit $($process.ExitCode).`n$tail"
        }
    }
    finally {
        if ($null -ne $process -and -not $process.HasExited) {
            Stop-Process -Id $process.Id -Force
            $process.WaitForExit()
        }
    }

    if (-not (Test-Path -LiteralPath $outputPath -PathType Leaf) -or -not (Test-Path -LiteralPath $logPath -PathType Leaf)) {
        throw "Trial $amplitudeM m did not produce both numeric output and a log."
    }
    $logText = Get-Content -LiteralPath $logPath -Raw
    $marker = 'Meadow hill sample trial accepted: AmplitudeM=' + $amplitudeM.ToString('0.000', $culture)
    if ($logText.IndexOf($marker, [StringComparison]::Ordinal) -lt 0) {
        throw "Fresh hill-trial commandlet marker was not found for $amplitudeM m; the loaded binary may be stale. See $logPath"
    }
    if ($logText -notmatch ('Exported ' + $expectedSamples + ' numeric world-generation samples')) {
        throw "Trial $amplitudeM m did not export the expected sample count. See $logPath"
    }

    $rows = @(Import-Csv -LiteralPath $outputPath)
    if ($rows.Count -ne $expectedSamples) { throw "Trial $amplitudeM m returned $($rows.Count) rows, expected $expectedSamples." }
    foreach ($row in $rows) {
        foreach ($column in @('FinalHeightCm', 'MeadowsWeight', 'WaterLevelCm', 'CollisionTriangleHeightCm', 'CollisionTriangleSlopeDegrees', 'CollisionTriangleGradePercent')) {
            $number = 0.0
            $parsed = [double]::TryParse([string]$row.$column, [Globalization.NumberStyles]::Float, $culture, [ref]$number)
            if (-not $parsed -or [double]::IsNaN($number) -or [double]::IsInfinity($number)) {
                throw "Trial $amplitudeM m has a non-finite $column for $($row.Site)."
            }
        }
    }

    foreach ($group in $rows | Group-Object Site) {
        $heights = @($group.Group | ForEach-Object { [double]::Parse($_.FinalHeightCm, $culture) })
        $slopes = @($group.Group | ForEach-Object { [double]::Parse($_.CollisionTriangleSlopeDegrees, $culture) })
        $summaryRows.Add([pscustomobject]@{
            AmplitudeM = $amplitudeM
            Site = $group.Name
            Samples = $group.Count
            FinalHeightRangeCm = (($heights | Measure-Object -Maximum).Maximum - ($heights | Measure-Object -Minimum).Minimum)
            MaxTriangleSlopeDegrees = ($slopes | Measure-Object -Maximum).Maximum
            OutputCsv = $outputPath
        })
    }
}

$summaryPath = Join-Path $outputRoot 'summary.csv'
$summaryRows | Export-Csv -LiteralPath $summaryPath -NoTypeInformation -Encoding UTF8
Write-Output "Meadows hill profiles exported to $outputRoot"
Write-Output "Summary: $summaryPath"
