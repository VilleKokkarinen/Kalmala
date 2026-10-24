param([string]$EditorCmd = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe')
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot

$ErrorActionPreference = 'Stop'
$project = Join-Path ($repoPaths.ProjectRoot) 'Kalmala.uproject'
$proof = Join-Path $env:TEMP ('KalmalaMapExportProof-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $proof | Out-Null
$inputPath = Join-Path $proof 'preview.params'
$log = Join-Path $proof 'export.log'
$baseline = "WorldSeed=418`nSize=64`n"
[IO.File]::WriteAllText($inputPath, $baseline)
$process = $null
function Wait-Report([string]$Pattern, [int]$Count) {
    $deadline = (Get-Date).AddSeconds(90)
    do {
        $text = if (Test-Path -LiteralPath $log) { Get-Content -LiteralPath $log -Raw } else { '' }
        if ([regex]::Matches($text, $Pattern).Count -ge $Count) { return }
        if ($process.HasExited) { throw "Exporter exited before expected report: $log" }
        Start-Sleep -Milliseconds 100
    } while ((Get-Date) -lt $deadline)
    throw "Expected report timed out: $Pattern ($log)"
}
function Get-Hashes {
    $result = @{}
    foreach ($name in @('MasterLandWater.png','LandWaterCrop.png','Biomes.png')) {
        $path = Join-Path $proof $name
        $bytes = [IO.File]::ReadAllBytes($path)
        if ([BitConverter]::ToString($bytes[0..7]) -ne '89-50-4E-47-0D-0A-1A-0A') { throw "Invalid PNG: $name" }
        $result[$name] = (Get-FileHash -LiteralPath $path).Hash
    }
    return $result
}
try {
    $process = Start-Process $EditorCmd -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" -run=ExportWorldMaps -Watch -Verify -MaxExports=4 -Parameters=`"$inputPath`" -Output=`"$proof`" -unattended -nop4 -nosplash -nullrhi -NoZenAutoLaunch -DDC=NoZenLocalFallback -LocalDataCachePath=`"$proof/DDC`" -UserDir=`"$proof/User`" -abslog=`"$log`" -forcelogflush"
    Wait-Report 'World PNG export complete:' 1
    $original = Get-Hashes
    $defaultSummary = Get-Content -LiteralPath (Join-Path $proof 'LastRender.txt') -Raw
    if ($defaultSummary -notmatch 'ReliefBroadScale=100000\.000' -or $defaultSummary -notmatch 'MeadowHillAmplitude=600\.000') {
        throw 'LastRender.txt did not report the documented terrain-control defaults'
    }
    [IO.File]::WriteAllText($inputPath, ($baseline + "ReliefBroadScaleKm=0.49`n"))
    Wait-Report 'Invalid world-map parameters' 1
    $invalid = Get-Hashes
    foreach ($name in $original.Keys) { if ($original[$name] -ne $invalid[$name]) { throw 'Out-of-range relief scale modified output' } }
    [IO.File]::WriteAllText($inputPath, ($baseline + "MeadowHillAmplitudeM=10.01`n"))
    Wait-Report 'Invalid world-map parameters' 2
    $invalid = Get-Hashes
    foreach ($name in $original.Keys) { if ($original[$name] -ne $invalid[$name]) { throw 'Out-of-range biome control modified output' } }
    [IO.File]::WriteAllText($inputPath, "Size=NaN`nUnknownParameter=42`n")
    Wait-Report 'Invalid world-map parameters' 3
    $invalid = Get-Hashes
    foreach ($name in $original.Keys) { if ($original[$name] -ne $invalid[$name]) { throw 'Invalid input modified output' } }
    [IO.File]::WriteAllText($inputPath, ($baseline + "ReliefBroadScaleKm=1.25`nReliefMiddleScaleKm=0.18`nReliefLocalScaleKm=0.04`nMeadowHillScaleM=200`nMeadowHillAmplitudeM=8`nMeadowRollingScaleM=50`nMeadowRollingAmplitudeM=1.5`nMeadowFloorScaleM=15`nMeadowFloorAmplitudeM=0.25`n"))
    Wait-Report 'World PNG export complete:' 2
    $terrainControlsOnly = Get-Hashes
    foreach ($name in $original.Keys) { if ($original[$name] -ne $terrainControlsOnly[$name]) { throw 'Staged terrain controls changed fast-map pixels before height integration' } }
    $terrainSummary = Get-Content -LiteralPath (Join-Path $proof 'LastRender.txt') -Raw
    if ($terrainSummary -notmatch 'ReliefBroadScale=125000\.000' -or $terrainSummary -notmatch 'MeadowHillAmplitude=800\.000') {
        throw 'LastRender.txt did not report the requested terrain controls'
    }
    [IO.File]::WriteAllText($inputPath, ($baseline + "MasterSeed=42`nMasterWavelengthKm=5.5`nLandThreshold=0.1`nBiomeScaleKm=1.4`nWarpStrengthKm=0.2`nLakesMinimumKm=3`nLakesMaximumKm=16`nMireMinimumKm=3`nMireMaximumKm=16`nWetlandHumidityMinimum=0.3`nWetlandHumidityFull=0.6`n"))
    Wait-Report 'World PNG export complete:' 3
    $changed = Get-Hashes
    foreach ($name in $original.Keys) { if ($original[$name] -eq $changed[$name]) { throw "No tuning variation: $name" } }
    [IO.File]::WriteAllText($inputPath, $baseline)
    Wait-Report 'World PNG export complete:' 4
    if (!$process.WaitForExit(10000) -or $process.ExitCode -ne 0) { throw 'Exporter failed to finish cleanly' }
    $restored = Get-Hashes
    foreach ($name in $original.Keys) { if ($original[$name] -ne $restored[$name]) { throw "Stale tuning/crop cache: $name" } }
    Write-Output 'PASS: PNG signatures, exact fast/full pixels, shared wetland weights, relief-control validation/reporting, unchanged fast-map pixels, warm reload, independent tuning variation, invalid-input preservation and restored-default hashes.'
    Select-String -LiteralPath $log -Pattern 'World PNG export complete:' | ForEach-Object { $_.Line }
}
finally {
    if ($null -ne $process -and !$process.HasExited) { Stop-Process -Id $process.Id }
    Write-Output "Proof output: $proof"
}
