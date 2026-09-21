param([int]$Port = 18143, [string]$OutputDirectory = '')
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaDiscoveryPeer-' + [guid]::NewGuid().ToString('N')) }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$serverLog = Join-Path $output 'server.log'; $clientLog = Join-Path $output 'client.log'
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaDiscoveryPeerTest'
$server = $null; $client = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do { if ($server.HasExited) { throw 'Discovery listen server exited during startup.' }; if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }; Start-Sleep -Milliseconds 500 } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Discovery listen server readiness timed out.' }
    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(90); $ready = $false
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A discovery peer exited before verification.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }; $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Discovery verification FAILED:|Discovery verification server: Passed=0|received undiscovered remote progress') { throw 'Discovery peer verification failed; inspect retained logs.' }
        $ready = $serverText -match 'Discovery verification server: Passed=1 SeedReproduced=1 DifferentSeed=1 DistantRejected=1 DuplicateRejected=1 OwnerFeedbackSerial=2 RemoteFeedbackSerial=0'
        $ready = $ready -and $serverText -match 'Ambient audio discovery result: Local=1 Feedback=(LandmarkFound|ScrollFound) Serial=1 CueSubmitted=1 Asset=DiscoveryAcknowledgedCue'
        $ready = $ready -and $serverText -match 'Ambient audio discovery result: Local=1 Feedback=AlreadyFound Serial=2 CueSubmitted=0 Asset=None'
        $ready = $ready -and $clientText -match 'Discovery verification client retained no undiscovered remote progress feedback\.'
        $ready = $ready -and $clientText -notmatch 'Ambient audio discovery result:'
        if ($ready) { break }; Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if (!$ready) { throw 'Discovery peer scenario timed out.' }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Discovery peer client identity mismatch.' }
    Write-Output 'PASS: same-seed descriptor reproduction and different-seed variation held; distant and duplicate claims were rejected, the entitled owner submitted the discovery cue, and the remote peer received no private feedback or cue.'
}
finally { foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }; Write-Output "Peer logs: $output" }
