param([int]$Port = 17847)

$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path $env:TEMP ('KalmalaMapAwareness-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaMapAwarenessTest'
$server = $null
$client = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 -GeneratorRevision=4 $common -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited) { throw 'Listen server exited during startup.' }
        if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Listen server readiness timed out.' }
    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 -GeneratorRevision=1 $common -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A peer exited before verification.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:') { throw 'Unreal reported an assertion.' }
        if ($serverText -match 'Map awareness revoked: Peers=0 Pings=0' -and $clientText -match 'Map awareness revoked: Peers=0 Pings=0') { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Host/client awareness scenario timed out.' }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418 Revision=4') { throw 'Client world identity mismatch.' }
    foreach ($result in @('Unauthorized', 'Malformed', 'Distant', 'Excessive')) {
        if ([regex]::Matches($serverText, "Map ping request: Result=$result").Count -ne 2) { throw "Expected rejection on both host and remote owner: $result" }
    }
    foreach ($peerText in @($serverText, $clientText)) {
        if ($peerText -notmatch 'Map awareness default: Private=1 Peers=0 Pings=0') { throw 'Default privacy failed.' }
        if ($peerText -notmatch 'Map awareness verification: Peers=\d Seen=4 Expired=1') { throw 'Expiry verification missing.' }
    }
    $pattern = 'Map ping observed: Sender=(\d+) Sequence=(\d+) Issued=([\d.]+) Expires=([\d.]+) X=([-\d.]+) Y=([-\d.]+)'
    $hostPings = [regex]::Matches($serverText, $pattern)
    $clientPings = [regex]::Matches($clientText, $pattern)
    if ($hostPings.Count -ne 4 -or $clientPings.Count -ne 4) { throw 'Expected exactly four relayed pings per peer.' }
    for ($index = 0; $index -lt 4; $index++) {
        if ($hostPings[$index].Value -ne $clientPings[$index].Value) { throw 'Host/client ping order, position, or authoritative expiry disagrees.' }
        $issued = [double]::Parse($hostPings[$index].Groups[3].Value, [cultureinfo]::InvariantCulture)
        $expires = [double]::Parse($hostPings[$index].Groups[4].Value, [cultureinfo]::InvariantCulture)
        if ([math]::Abs($expires - $issued - 6) -gt 0.00001) { throw 'Server lifetime differs from six seconds.' }
    }
    Write-Output 'PASS: default privacy, owner consent, host/remote rejection matrix, four matching ordered relays with six-second expiry, and opt-out clearing.'
}
finally {
    foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    Write-Output "Scenario logs: $output"
}
