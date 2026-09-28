param([int]$Port = 23841, [string]$OutputDirectory = '')
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaM9ExplorationRewardsPeer-' + [guid]::NewGuid().ToString('N')) }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaM9ExplorationRewardPeerTest'
$server = $null
$client = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited) { throw 'M9 exploration reward listen server exited during startup.' }
        if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'M9 exploration reward listen server readiness timed out.' }

    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(120)
    $ready = $false
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'An M9 exploration reward peer exited before verification.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|M9 exploration reward verification FAILED:|M9 exploration reward verification server: Passed=0') {
            throw 'M9 exploration reward peer verification failed; inspect retained logs.'
        }
        $ready = $serverText -match 'M9 exploration reward verification server: Passed=1 Deterministic=1 DistantRejected=1 ForgedRejected=1 ActorReplayRejected=1 DirectReplayRejected=1 IndependentPlayer=1 OwnerOnlyState=1'
        $ready = $ready -and $clientText -match 'Client received world-generation identity: Seed=418'
        if ($ready) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)

    if (!$ready) { throw 'M9 exploration reward peer scenario timed out.' }
    Write-Output 'PASS: deterministic land reward placement; distant, forged, and replayed claims rejected; exact owner rewards and separate co-op claims verified.'
}
finally {
    foreach ($peer in @($client, $server)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Peer logs: $output"
}
