param([int]$Port = 17847)
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot


$ErrorActionPreference = 'Stop'
$project = Join-Path ($repoPaths.ProjectRoot) 'Kalmala.uproject'
$mapHeaderPath = Join-Path $repoPaths.ProjectRoot 'Source\KalmalaGameplay\Public\KalmalaMapAwarenessComponent.h'
$mapSourcePath = Join-Path $repoPaths.ProjectRoot 'Source\KalmalaGameplay\Private\KalmalaMapAwarenessComponent.cpp'
$gameModePath = Join-Path $repoPaths.ProjectRoot 'Source\KalmalaGameplay\Private\KalmalaGameMode.cpp'
$worldMapPath = Join-Path $repoPaths.ProjectRoot 'Source\KalmalaUI\Private\KalmalaWorldMapWidget.cpp'
$mapHeader = Get-Content -LiteralPath $mapHeaderPath -Raw
$mapSource = Get-Content -LiteralPath $mapSourcePath -Raw
$gameMode = Get-Content -LiteralPath $gameModePath -Raw
$worldMap = Get-Content -LiteralPath $worldMapPath -Raw
$mapContractSources = $mapHeader + [Environment]::NewLine + $mapSource + [Environment]::NewLine + $gameMode + [Environment]::NewLine + $worldMap
foreach ($obsoleteMapApi in @(
    'ServerSetSharingEnabled', 'bMapSharingEnabled', 'AKalmalaMapPlayerState', 'FKalmalaMapPeerMarker',
    'SetSharingEnabled', 'IsSharingEnabled', 'IsSharingRequested', 'GetPeerMarkers', 'GetStatusText', 'bLocalSharingRequested'
)) {
    if ($mapContractSources.IndexOf($obsoleteMapApi, [StringComparison]::Ordinal) -ge 0) {
        throw "Retired map location-sharing API remains in production source: $obsoleteMapApi"
    }
}
foreach ($requiredMapPattern in @(
    @{ Pattern = 'UFUNCTION\(Server, Unreliable\)\s*void\s+ServerRequestPing'; Name = 'server-validated temporary ping request' },
    @{ Pattern = 'DOREPLIFETIME_CONDITION\(UKalmalaMapAwarenessComponent, Inbox, COND_OwnerOnly\)'; Name = 'owner-only ping inbox' },
    @{ Pattern = 'PingCooldown\s*=\s*2\.0'; Name = 'two-second sender cooldown' },
    @{ Pattern = 'PingLifetime\s*=\s*10\.0'; Name = 'ten-second ping lifetime' },
    @{ Pattern = 'MaxInboxPings\s*=\s*16'; Name = 'sixteen-ping inbox bound' },
    @{ Pattern = 'Recipient->Inbox\.Num\(\) > MaxInboxPings'; Name = 'bounded recipient inbox write' },
    @{ Pattern = 'DrawMapPings[\s\S]*GetVisiblePings'; Name = 'temporary ping rendering' }
)) {
    if ($mapContractSources -notmatch $requiredMapPattern.Pattern) {
        throw "Map ping contract is missing: $($requiredMapPattern.Name)"
    }
}
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path $env:TEMP ('KalmalaMapAwareness-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaMapAwarenessTest'
$server = $null
$client = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited) { throw 'Listen server exited during startup.' }
        if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Listen server readiness timed out.' }
    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A peer exited before verification.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:') { throw 'Unreal reported an assertion.' }
        if ($serverText -match 'Map awareness verification: Seen=4 Expired=1' -and $clientText -match 'Map awareness verification: Seen=4 Expired=1') { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Host/client awareness scenario timed out.' }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Client world identity mismatch.' }
    if ([regex]::Matches($serverText, 'Map ping request: Result=Accepted').Count -ne 4) { throw 'Expected two accepted pings from each connected player.' }
    foreach ($result in @('Malformed', 'OutOfBounds', 'Excessive')) {
        if ([regex]::Matches($serverText, "Map ping request: Result=$result").Count -ne 2) { throw "Expected server rejection from both host and remote owner: $result" }
    }
    foreach ($peerText in @($serverText, $clientText)) {
        if ($peerText -notmatch 'Map awareness ping-only default: Pings=\d+') { throw 'Ping-only map verification did not start.' }
        if ($peerText -notmatch 'Map awareness verification: Seen=4 Expired=1') { throw 'Ten-second ping expiry verification missing.' }
    }
    $pattern = 'Map ping observed: Sender=(\d+) Sequence=(\d+) Issued=([\d.]+) Expires=([\d.]+) X=([-\d.]+) Y=([-\d.]+)'
    $hostPings = [regex]::Matches($serverText, $pattern)
    $clientPings = [regex]::Matches($clientText, $pattern)
    if ($hostPings.Count -ne 4 -or $clientPings.Count -ne 4) { throw 'Expected exactly four relayed pings per peer.' }
    for ($index = 0; $index -lt 4; $index++) {
        if ($hostPings[$index].Value -ne $clientPings[$index].Value) { throw 'Host/client ping order, position, or authoritative expiry disagrees.' }
        $issued = [double]::Parse($hostPings[$index].Groups[3].Value, [cultureinfo]::InvariantCulture)
        $expires = [double]::Parse($hostPings[$index].Groups[4].Value, [cultureinfo]::InvariantCulture)
        if ([math]::Abs($expires - $issued - 10) -gt 0.00001) { throw 'Server lifetime differs from ten seconds.' }
    }
    if (-not ($hostPings | Where-Object { [double]::Parse($_.Groups[5].Value, [cultureinfo]::InvariantCulture) -eq 1000000.0 })) { throw 'Host did not receive the accepted remote-world ping.' }
    if (-not ($clientPings | Where-Object { [double]::Parse($_.Groups[5].Value, [cultureinfo]::InvariantCulture) -eq 1000000.0 })) { throw 'Client did not receive the accepted remote-world ping.' }
    Write-Output 'PASS: ping-only map contract, owner-only inboxes, world-bound validation, remote-world relay, per-sender cooldown, host/client ordering, and ten-second expiry.'
}
finally {
    foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    Write-Output "Scenario logs: $output"
}
