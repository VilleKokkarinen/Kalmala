param([int]$Port = 18129, [string]$OutputDirectory = '')
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaBoarPeer-' + [guid]::NewGuid().ToString('N')) }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$serverLog = Join-Path $output 'server.log'; $clientLog = Join-Path $output 'client.log'
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaBoarPeerTest'
$server = $null; $client = $null; $remoteClient = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do { if ($server.HasExited) { throw 'Boar listen server exited during startup.' }; if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }; Start-Sleep -Milliseconds 500 } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Boar listen server readiness timed out.' }
    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $remoteClientLog = Join-Path $output 'remote-client.log'
    $remoteClient = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=1000 $common -abslog=`"$remoteClientLog`" -UserDir=`"$output\RemoteClient`""
    $deadline = (Get-Date).AddSeconds(90); $ready = $false
    do {
        if ($server.HasExited -or $client.HasExited -or $remoteClient.HasExited) { throw 'A Boar peer exited before verification.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }; $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }; $remoteClientText = if (Test-Path $remoteClientLog) { Get-Content $remoteClientLog -Raw } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Boar verification FAILED:|Boar verification server: Passed=0') { throw 'Boar peer verification failed; inspect retained logs.' }
        $ready = $serverText -match 'Boar verification server: Passed=1 SeedReproduced=1 BoundedActivation=1 InvalidRejected=1 ActionSerial=4 Health=0\.0 PlayerHealth=[0-9]+\.0 Defeated=1 Saved=1 Ash=0 RemoteAsh=0 Meat=1 Hide=1 RemoteMeat=0 RemoteHide=0'
        $ready = $ready -and $clientText -match 'Boar verification client rejected invalid owned attack without target data\.'
        $ready = $ready -and $clientText -match 'Boar verification client observed shared action serial=4\.'
        $ready = $ready -and $clientText -match 'Boar verification client observed relevant wildlife defeat\.'
        $ready = $ready -and $clientText -match 'Boar verification client observed replicated player health=[0-9]+\.0\.'
        if ($ready) { break }; Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if (!$ready) { throw 'Boar peer scenario timed out.' }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Boar peer client identity mismatch.' }
    Stop-Process -Id $client.Id; Stop-Process -Id $remoteClient.Id; Stop-Process -Id $server.Id; $client = $null; $remoteClient = $null; $server = $null
    $restartLog = Join-Path $output 'restart.log'
    $restart = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 -game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaReconnectVerification=WildlifeBoarVerify -abslog=`"$restartLog`" -UserDir=`"$output\Host`""
    if (!$restart.WaitForExit(120000)) { Stop-Process -Id $restart.Id; throw 'Boar world-state restart timed out.' }
    $restartText = Get-Content $restartLog -Raw
    if ($restartText -notmatch 'Reconnect verification passed: defeated generated wildlife spawn .* remained absent after listen-server restart\.') { throw 'Boar defeat did not survive restart.' }
    Write-Output 'PASS: seed-reproduced bounded boar activation charged and damaged a player server-side; the client saw only replicated combat/health/defeat state, its target-free attack was rejected, meat/hide stayed owner-only, and the defeat survived restart.'
}
finally { foreach ($peer in @($client, $remoteClient, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }; Write-Output "Peer logs: $output" }
