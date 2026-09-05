param(
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [int]$Port = 17842,
    [switch]$Rendered,
    [int]$Width = 1280,
    [int]$Height = 720
)
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$output = Join-Path $env:TEMP ('KalmalaMinimap-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush'
if ($Rendered) {
    $common = "-game -windowed -RenderOffscreen -ForceRes -ResX=$Width -ResY=$Height -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaMinimapVerification"
}
$server = $null
$client = $null
try {
    $server = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -KalmalaMinimapScreenshot=`"$output/host.png`" -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited) { throw 'Listen server exited before accepting connections.' }
        if ((Test-Path $serverLog) -and (Select-String -Path $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Listen server readiness timed out.' }
    $client = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -KalmalaMinimapScreenshot=`"$output/client.png`" -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A peer exited before minimap verification.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        $identityReady = $clientText -match 'Client received world-generation identity: Seed=418 Revision=1'
        $renderReady = !$Rendered -or (($serverText -match 'Minimap painted:') -and ($clientText -match 'Minimap painted:') -and (Test-Path "$output/host.png") -and (Test-Path "$output/client.png"))
        if ($identityReady -and $renderReady) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Host/client minimap presentation timed out.' }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418 Revision=1') { throw 'Client did not receive the server world identity.' }
    if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:') { throw 'Unreal reported a fatal error.' }
    if ($Rendered) {
        foreach ($peerText in @($serverText, $clientText)) {
            if ($peerText -notmatch 'Minimap input verification: Min=1 Max=1 Modal=1 Resume=1') { throw 'Bound wheel input or CommonUI modal ownership failed.' }
            if ($peerText -match 'CommonUI Input routing will not function correctly') { throw 'CommonUI viewport routing is not configured.' }
            if ($peerText -notmatch 'Minimap painted:.*Size=208x208 Bounds=(-?\d+),(-?\d+),(-?\d+),(-?\d+) Samples=16641') { throw 'Minimap did not paint at the expected size and detail.' }
            $left = [int]$Matches[1]; $top = [int]$Matches[2]; $right = [int]$Matches[3]; $bottom = [int]$Matches[4]
            if ($left -lt ($Width / 2) -or $top -lt 0 -or $right -gt $Width -or $bottom -gt $Height) { throw 'Actual minimap geometry is outside the top-right viewport.' }
        }
        Write-Output 'PASS: both peers painted filled top-right minimaps and captured HUD screenshots; inspect host.png and client.png.'
    }
    Write-Output 'PASS: conflicting-seed client received the server identity for the local-only minimap model.'
}
finally {
    foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    Write-Output "Scenario logs: $output"
}
