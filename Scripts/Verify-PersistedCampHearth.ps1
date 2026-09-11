param([int]$Port = 18031)
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path $env:TEMP ('KalmalaPersistedCampHearth-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'; $clientLog = Join-Path $output 'client.log'
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaPersistedCampTest -ExecCmds="t.MaxFPS 60"'
$server = $null; $client = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 -GeneratorRevision=4 $common -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do { if ($server.HasExited) { throw 'Listen server exited during startup.' }; if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }; Start-Sleep -Milliseconds 500 } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Listen server readiness timed out.' }
    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 -GeneratorRevision=1 $common -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(120); $ready = $false
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A peer exited before persisted-camp hearth verification.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }; $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Persisted camp hearth (server|owner): Passed=0') { throw 'Persisted-camp hearth verification failed; inspect retained logs.' }
        $ready = [regex]::Matches($serverText, 'Persisted camp hearth server: Passed=1 Player=\d+ Gathered=1 Crafted=1 Placed=1 Paid=1 Fuel=60').Count -eq 2 `
            -and $serverText -match 'Persisted camp hearth owner: Passed=1 Authority=1 Player=\d+ EmptyPack=1 Fuel=60' `
            -and $clientText -match 'Persisted camp hearth owner: Passed=1 Authority=0 Player=\d+ EmptyPack=1 Fuel=60'
        if ($ready) { break }; Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if (!$ready) { throw 'Persisted-camp shared hearth scenario timed out.' }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418 Revision=4') { throw 'Client identity mismatch.' }
    Write-Output 'PASS: both players harvested server-initialized nodes, crafted and paid for a shared-session hearth through normal server validation; each owner observed its replicated empty pack and 60-second fire.'
}
finally { foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }; Write-Output "Scenario logs: $output" }
