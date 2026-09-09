param(
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [int]$Port = 17843
)

$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$output = Join-Path $env:TEMP ('KalmalaWorldMapTiles-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$common = '-game -windowed -RenderOffscreen -ForceRes -ResX=1280 -ResY=720 -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaWorldMapVerification'
$server = $null
$client = $null
try {
    $server = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 -GeneratorRevision=4 $common -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited) { throw 'Listen server exited before accepting connections.' }
        if ((Test-Path $serverLog) -and (Select-String -Path $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Listen server readiness timed out.' }
    $client = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 -GeneratorRevision=1 $common -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A peer exited before world-map tile verification.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
    if (($serverText -match 'World map tile presentation: Seed=418 Revision=4 Tiles=\d+ Fingerprint=\d+ PollOnly=1.') -and
            ($serverText -match 'World map fog presentation: Seed=418 Revision=4 Cells=\d+ Loaded=0 Remote=0.') -and
            ($clientText -match 'Client received world-generation identity: Seed=418 Revision=4') -and
            ($clientText -match 'World map tile presentation: Seed=418 Revision=4 Tiles=\d+ Fingerprint=\d+ PollOnly=1.') -and
            ($clientText -match 'World map fog presentation: Seed=418 Revision=4 Cells=\d+ Loaded=0 Remote=0.')) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Host/client world-map tile presentation timed out.' }
    $serverMatch = [regex]::Match($serverText, 'World map tile presentation: Seed=418 Revision=4 Tiles=(\d+) Fingerprint=(\d+) PollOnly=1.')
    $clientMatch = [regex]::Match($clientText, 'World map tile presentation: Seed=418 Revision=4 Tiles=(\d+) Fingerprint=(\d+) PollOnly=1.')
    if (!$serverMatch.Success -or !$clientMatch.Success -or $serverMatch.Groups[1].Value -ne $clientMatch.Groups[1].Value -or $serverMatch.Groups[2].Value -ne $clientMatch.Groups[2].Value) { throw 'Host/client tile count or deterministic presentation fingerprint disagrees.' }
    if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:') { throw 'Unreal reported a fatal error.' }
    Write-Output "PASS: $($serverMatch.Value) matches the conflicting-seed client after authoritative identity replication; each peer kept remote terrain unexplored."
    foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    $client = $null
    $server = $null
    $restartServerLog = Join-Path $output 'server-restart.log'
    $restartClientLog = Join-Path $output 'client-restart.log'
    $server = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 -GeneratorRevision=4 $common -abslog=`"$restartServerLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited) { throw 'Restarted listen server exited before accepting connections.' }
        if ((Test-Path $restartServerLog) -and (Select-String -Path $restartServerLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Restarted listen server readiness timed out.' }
    $client = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 -GeneratorRevision=1 $common -abslog=`"$restartClientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A restarted peer exited before personal-coverage verification.' }
        $restartServerText = if (Test-Path $restartServerLog) { Get-Content $restartServerLog -Raw } else { '' }
        $restartClientText = if (Test-Path $restartClientLog) { Get-Content $restartClientLog -Raw } else { '' }
        if (($restartServerText -match 'World map fog presentation: Seed=418 Revision=4 Cells=\d+ Loaded=1 Remote=0.') -and
            ($restartClientText -match 'Client received world-generation identity: Seed=418 Revision=4') -and
            ($restartClientText -match 'World map fog presentation: Seed=418 Revision=4 Cells=\d+ Loaded=1 Remote=0.')) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Restarted host/client personal-coverage verification timed out.' }
    if (($restartServerText + $restartClientText) -match 'Fatal error:|Assertion failed:') { throw 'Restarted Unreal peer reported a fatal error.' }
    Write-Output 'PASS: matching personal coverage reloaded after restart without revealing remote terrain.'
}
finally {
    foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    Write-Output "Scenario logs: $output"
}
