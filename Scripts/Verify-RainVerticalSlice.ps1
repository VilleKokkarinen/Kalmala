param([int]$Port = 18119, [string]$OutputDirectory = '')
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaRainVerticalSlice-' + [guid]::NewGuid().ToString('N')) }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$serverLog = Join-Path $output 'server.log'; $clientLog = Join-Path $output 'client.log'
# A retained output directory is useful for comparison, but the next run must
# not interpret a previous peer's failure as its own startup result.
Remove-Item -LiteralPath $serverLog, $clientLog -Force -ErrorAction SilentlyContinue
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaRainVerticalSliceTest -ExecCmds="t.MaxFPS 60"'
$server = $null; $client = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do { if ($server.HasExited) { throw 'Listen server exited during startup.' }; if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }; Start-Sleep -Milliseconds 500 } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Listen server readiness timed out.' }
    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(90); $ready = $false
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A peer exited before rain vertical-slice verification.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }; $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Rain vertical slice (FAILED|server: Passed=0|client: Passed=0)') { throw 'Rain vertical-slice verification failed; inspect retained logs.' }
        $ready = $serverText -match 'Rain vertical slice server: Passed=1 WaterWet=1 RainWet=1 ExposedHealth=50\.0 RoofedHealth=100\.0 RoofHealth=100\.0 FireState=1 Roofed=1 WetRemoved=1'
        $ready = $ready -and $clientText -match 'Rain vertical slice client: Passed=1 Wet=0 ExposedHealth=50\.0 RoofedHealth=100\.0 RoofHealth=100\.0 FireState=1 Roofed=1'
        if ($ready) { break }; Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if (!$ready) { throw 'Rain vertical-slice scenario timed out.' }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Client identity mismatch.' }
    Write-Output 'PASS: host and conflicting-seed client observed the server-owned water/rain Wet loop, roof protection, capped rain wear, smoulder/reignite, and heat recovery.'
}
finally { foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }; Write-Output "Scenario logs: $output" }
