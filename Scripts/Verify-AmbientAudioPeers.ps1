[CmdletBinding()]
param(
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [int]$Port = 18306
)

$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$output = Join-Path $env:TEMP ('KalmalaAmbientAudioPeers-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$common = '-game -nullrhi -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaAmbientAudioTest'
$server = $null
$client = $null
try {
    $server = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited) { throw 'Listen server exited during ambient-audio startup.' }
        if ((Test-Path $serverLog) -and (Select-String -LiteralPath $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Ambient-audio listen server startup timed out.' }

    $client = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'An ambient-audio peer exited before verification.' }
        $serverText = if (Test-Path $serverLog) { Get-Content -LiteralPath $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content -LiteralPath $clientLog -Raw } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:') { throw 'Ambient-audio host/client verification failed; inspect the retained logs.' }
        $serverStarted = $serverText -match 'Ambient audio runtime: ComponentCreated=1 Local=1 Asset=WindBed Looping=1'
        $clientStarted = $clientText -match 'Ambient audio runtime: ComponentCreated=1 Local=1 Asset=WindBed Looping=1'
        $serverWaterProbed = $serverText -match 'Ambient audio water context: Probed=1 Local=1 Visible=[01] ComponentCreated=[01] Asset=(WaterBed|None)'
        $clientWaterProbed = $clientText -match 'Ambient audio water context: Probed=1 Local=1 Visible=[01] ComponentCreated=[01] Asset=(WaterBed|None)'
        $serverWaterActive = $serverText -match 'Ambient audio water context: Probed=1 Local=1 Visible=1 ComponentCreated=1 Asset=WaterBed'
        $clientWaterActive = $clientText -match 'Ambient audio water context: Probed=1 Local=1 Visible=1 ComponentCreated=1 Asset=WaterBed'
        $clientJoined = $clientText -match 'Client received world-generation identity: Seed=418'
        $waterActive = $serverWaterActive -or $clientWaterActive
        $scenarioReady = $serverStarted -and $clientStarted -and $serverWaterProbed -and $clientWaterProbed -and $waterActive -and $clientJoined
        if ($scenarioReady) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Both local wind components, both water-context probes, at least one visible-water loop, and the connected client identity were not observed before timeout.' }

    Write-Output 'PASS: host and client created local wind audio and probed independently; a peer near visible water created the local WaterBed loop, with no gameplay request or replicated audio state.'
}
finally {
    foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    Write-Output "Scenario logs: $output"
}
