param([int]$Port = 18041)
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path $env:TEMP ('KalmalaPersistedCampRestart-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output -Force | Out-Null
$hostDir = Join-Path $output 'Host'
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -ExecCmds="t.MaxFPS 60"'
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
function Wait-Listen($process, $log) {
    $deadline = (Get-Date).AddSeconds(90)
    do { if ($process.HasExited) { throw 'Listen server exited during startup.' }; if ((Test-Path $log) -and (Select-String $log -Pattern 'GameNetDriver.*listening on port' -Quiet)) { return }; Start-Sleep -Milliseconds 500 } while ((Get-Date) -lt $deadline)
    throw 'Listen server readiness timed out.'
}
function Invoke-ReconnectMode([string]$mode, [string]$name) {
    $log = Join-Path $output "$name.log"
    $process = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -KalmalaReconnectVerification=$mode -abslog=`"$log`" -UserDir=`"$hostDir`""
    if (!$process.WaitForExit(120000)) { Stop-Process -Id $process.Id; throw "Reconnect $mode timed out." }
    $text = Get-Content $log -Raw
    if ($text -match 'Reconnect verification failed:|Fatal error:|Assertion failed:') { throw "Reconnect $mode failed." }
    return $text
}
function Invoke-PersistedCampBuild {
    $buildCommon = "$common -KalmalaPersistedCampTest"
    $buildServer = $null
    $buildClient = $null
    try {
        $buildServer = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $buildCommon -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
        $deadline = (Get-Date).AddSeconds(90)
        do {
            if ($buildServer.HasExited) { throw 'Listen server exited during startup.' }
            if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
            Start-Sleep -Milliseconds 500
        } while ((Get-Date) -lt $deadline)
        if ((Get-Date) -ge $deadline) { throw 'Listen server readiness timed out.' }

        $buildClient = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $buildCommon -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
        $deadline = (Get-Date).AddSeconds(120)
        $ready = $false
        do {
            if ($buildServer.HasExited -or $buildClient.HasExited) { throw 'A peer exited before persisted-camp build verification.' }
            $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
            $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
            if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Persisted camp build (server|owner): Passed=0') {
                throw 'Persisted-camp build verification failed; inspect retained logs.'
            }
            $ready = [regex]::Matches($serverText, 'Persisted camp build server: Passed=1 Player=\d+ Gathered=1 Hearth=1 Kits=1 Built=1 Storage=1 Paid=1 Fuel=60').Count -eq 2 `
                -and $serverText -match 'Persisted camp build owner: Passed=1 Authority=1 Player=\d+ EmptyPack=1 Fuel=60 Constructions=10 StorageWood=1 Weather=77/0\.75/0/1\.00 Shelter=\d+\.\d+ Wetness=\d+\.\d+ Warmth=\d+\.\d+' `
                -and $clientText -match 'Persisted camp build owner: Passed=1 Authority=0 Player=\d+ EmptyPack=1 Fuel=60 Constructions=10 StorageWood=1 Weather=77/0\.75/0/1\.00 Shelter=-1\.00 Wetness=\d+\.\d+ Warmth=\d+\.\d+' `
                -and $clientText -match 'Persisted camp client authority probe: Passed=1 Wet=0 FloorHealth=[\d.]+ RoofHealth=100\.0 FireState=0 Fuel=60 Weather=77/0\.75 SaveOwner=0'
            if ($ready) { break }
            Start-Sleep -Milliseconds 500
        } while ((Get-Date) -lt $deadline)
        if (!$ready) { throw 'Persisted-camp shared build scenario timed out.' }
        if ($clientText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Client identity mismatch.' }
        Write-Output 'PASS: both players observed the shared gathered camp; the live client could not alter Wet, construction rain/roof state, hearth state/fuel, weather, or access server save owners.'
    }
    finally {
        foreach ($peer in @($buildClient, $buildServer)) {
            if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
        }
    }
}
$server = $null; $client = $null
try {
    Invoke-PersistedCampBuild
    $firstLog = Get-Content $serverLog -Raw
    $ids = @([regex]::Matches($firstLog, 'Persisted camp construction: Id=([0-9a-f-]+) Kit=') | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique)
    if ($ids.Count -ne 10) { throw 'Initial camp did not create exactly ten unique persisted constructions.' }
    $serverLog = Join-Path $output 'restart-server.log'; $clientLog = Join-Path $output 'restart-client.log'
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -KalmalaPersistedCampRestoreTest -abslog=`"$serverLog`" -UserDir=`"$hostDir`""
    Wait-Listen $server $serverLog
    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -KalmalaPersistedCampRestoreTest -abslog=`"$clientLog`" -UserDir=`"$(Join-Path $output 'ClientRestore')`""
    $deadline = (Get-Date).AddSeconds(120); $ready = $false
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A peer exited during camp restoration.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }; $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        if (($serverText + $clientText) -match 'Passed=0|Fatal error:|Assertion failed:|Ensure condition failed:') { throw 'Persisted-camp restoration reported failure.' }
        $restored = @([regex]::Matches($serverText, 'Construction restored: Id=([0-9a-f-]+) Kit=') | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique)
        $replicated = @([regex]::Matches($clientText, 'Construction replicated: Id=([0-9a-f-]+) Kit=') | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique)
        $ready = $restored.Count -eq 10 -and $replicated.Count -eq 10 -and @(Compare-Object $ids $restored).Count -eq 0 -and @(Compare-Object $ids $replicated).Count -eq 0 `
            -and [regex]::Matches($serverText, 'Persisted camp restore server: Passed=1 .*Constructions=10 StorageWood=1').Count -eq 2 `
            -and $clientText -match 'Persisted camp restore owner: Passed=1 Authority=0 .*Constructions=10 StorageWood=1'
        if (!$ready) { Start-Sleep -Milliseconds 500 }
    } while (!$ready -and (Get-Date) -lt $deadline)
    if (!$ready) { throw 'Persisted-camp same-identity restart timed out.' }
    Stop-Process -Id $client.Id; Stop-Process -Id $server.Id; $client = $null; $server = $null
    $harvest = Invoke-ReconnectMode 'Harvest' 'harvest'
    $verify = Invoke-ReconnectMode 'Verify' 'verify'
    if ($harvest -notmatch 'harvested generated node' -or $verify -notmatch 'harvested generated node .* remained absent') { throw 'Sparse harvest delta did not survive the camp restart.' }
    $crossLog = Join-Path $output 'cross-world.log'
    $cross = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=419 $common -KalmalaPersistedCampRestoreTest -abslog=`"$crossLog`" -UserDir=`"$hostDir`""
    Start-Sleep -Seconds 12
    if (!$cross.HasExited) { Stop-Process -Id $cross.Id }
    if ((Get-Content $crossLog -Raw) -match 'Construction restored:') { throw 'Cross-world construction reuse was not rejected.' }
    Write-Output 'PASS: the shared gathered camp restored exact construction IDs and chest contents to a conflicting-seed reconnecting client; no duplicate actors/items appeared, a real sparse harvest delta survived the same host save directory, and seed 419 restored no camp.'
}
finally { foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }; Write-Output "Scenario logs: $output" }
