param([int]$Port = 18031, [string]$OutputDirectory = '')
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaPersistedCampBuild-' + [guid]::NewGuid().ToString('N')) }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$serverLog = Join-Path $output 'server.log'; $clientLog = Join-Path $output 'client.log'
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaPersistedCampTest -ExecCmds="t.MaxFPS 60"'
$server = $null; $client = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do { if ($server.HasExited) { throw 'Listen server exited during startup.' }; if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }; Start-Sleep -Milliseconds 500 } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Listen server readiness timed out.' }
    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(120); $ready = $false
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A peer exited before persisted-camp build verification.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }; $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Persisted camp build (server|owner): Passed=0') { throw 'Persisted-camp build verification failed; inspect retained logs.' }
        $ready = [regex]::Matches($serverText, 'Persisted camp build server: Passed=1 Player=\d+ Gathered=1 Hearth=1 Kits=1 Built=1 Storage=1 Paid=1 Fuel=60').Count -eq 2 `
            -and $serverText -match 'Persisted camp build owner: Passed=1 Authority=1 Player=\d+ EmptyPack=1 Fuel=60 Constructions=10 StorageWood=1 Weather=77/0\.75/0/1\.00 Shelter=\d+\.\d+ Wetness=\d+\.\d+ Warmth=\d+\.\d+' `
            -and $clientText -match 'Persisted camp build owner: Passed=1 Authority=0 Player=\d+ EmptyPack=1 Fuel=60 Constructions=10 StorageWood=1 Weather=77/0\.75/0/1\.00 Shelter=-1\.00 Wetness=\d+\.\d+ Warmth=\d+\.\d+' `
            -and $clientText -match 'Persisted camp client authority probe: Passed=1 Wet=0 FloorHealth=[\d.]+ RoofHealth=100\.0 FireState=0 Fuel=60 Weather=77/0\.75 SaveOwner=0'
        if ($ready) { break }; Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if (!$ready) { throw 'Persisted-camp shared build scenario timed out.' }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Client identity mismatch.' }
    Write-Output 'PASS: both players observed the shared gathered camp; the live client could not alter Wet, construction rain/roof state, hearth state/fuel, weather, or access server save owners.'
}
finally { foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }; Write-Output "Scenario logs: $output" }
