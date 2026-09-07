param(
    [string]$EditorCmd = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe',
    [switch]$SkipPeers
)
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$output = Join-Path $env:TEMP ('KalmalaRegionalProof-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
function Invoke-Editor([string]$Name, [string]$Arguments) {
    $log = Join-Path $output ($Name + '.log')
    $process = $null
    try {
        $process = Start-Process $EditorCmd -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" -unattended -nop4 -nosplash -nullrhi -NoZenAutoLaunch -DDC=NoZenLocalFallback -LocalDataCachePath=`"$output/DDC`" -UserDir=`"$output/User-$Name`" -abslog=`"$log`" $Arguments"
        $deadline = (Get-Date).AddMinutes(8)
        while (!$process.WaitForExit(1000)) { if ((Get-Date) -gt $deadline) { throw "$Name timed out: $log" } }
        if ($process.ExitCode -ne 0) { throw "$Name exited $($process.ExitCode): $log" }
        $text = Get-Content $log -Raw
        if ($text -match 'Result=\{Fail\}|Fatal error:|Assertion failed:') { throw "$Name failed: $log" }
        if ($Name -eq 'automation' -and $text -notmatch 'Result=\{Success\} Name=\{Integrated\} Path=\{Kalmala.World.Regional.Integrated\}') { throw "Regional test did not run: $log" }
        Write-Output "PASS: $Name ($log)"
    }
    finally { if ($null -ne $process -and !$process.HasExited) { Stop-Process -Id $process.Id } }
}
try {
    Invoke-Editor 'automation' '-ExecCmds="Automation RunTests Kalmala.World.Regional+Kalmala.World.Biomes+Kalmala.World.Water+Kalmala.World.PopulationSaveGame+Kalmala.UI.Minimap.LocalPresentation" -TestExit="Automation Test Queue Empty"'
    foreach ($run in @(@('seed418-a',418), @('seed418-b',418), @('seed419',419))) {
        $directory = Join-Path $output $run[0]
        Invoke-Editor $run[0] "-run=RenderWorldGenerationVisualization -Seed=$($run[1]) -Revision=3 -Size=256 -Extent=400000 -Output=`"$directory`""
    }
    $first = Join-Path $output 'seed418-a'
    $second = Join-Path $output 'seed418-b'
    $other = Join-Path $output 'seed419'
    foreach ($file in Get-ChildItem -LiteralPath $first -Filter '*.ppm') {
        $hash = (Get-FileHash -LiteralPath $file.FullName).Hash
        if ($hash -ne (Get-FileHash -LiteralPath (Join-Path $second $file.Name)).Hash) { throw "Nondeterministic image: $($file.Name)" }
    }
    foreach ($name in @('BiomeClassification.ppm','Hydrology.ppm','Weight2.ppm','ShapedHeight.ppm')) {
        if ((Get-FileHash (Join-Path $first $name)).Hash -eq (Get-FileHash (Join-Path $other $name)).Hash) { throw "No different-seed variation: $name" }
    }
    Write-Output 'PASS: all repeated field/weight/biome/overlap/hydrology/height renders match; different seeds vary.'
    if (!$SkipPeers) { & (Join-Path $PSScriptRoot 'Verify-Minimap.ps1') -GeneratorRevision 3 }
}
finally { Write-Output "Regional proof output: $output" }
