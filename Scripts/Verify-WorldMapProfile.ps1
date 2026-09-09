param([int]$Port = 17847)

$ErrorActionPreference = 'Stop'
$project = Join-Path $PSScriptRoot '..\Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path ([System.IO.Path]::GetTempPath()) ('KalmalaWorldMapProfile-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'host.log'
$clientLog = Join-Path $output 'client.log'
$common = '-game -windowed -RenderOffscreen -ForceRes -ResX=1280 -ResY=720 -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaWorldMapVerification -KalmalaWorldMapProfile'
$server = $null
$client = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 -GeneratorRevision=4 $common -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited) { throw 'Listen server exited before accepting connections.' }
        if ((Test-Path $serverLog) -and (Select-String -Path $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Listen-server readiness timed out.' }
    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 -GeneratorRevision=1 $common -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A profile peer exited.' }
        $serverText = if (Test-Path $serverLog) { Get-Content -Raw $serverLog } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content -Raw $clientLog } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:') { throw 'Unreal reported a profile failure.' }
        if ($serverText -match 'World map profile: OpenMs=.*WorkerTotalMs=.*GameThreadTotalMs=.*Tiles=\d+ CacheBytes=\d+.' -and
            $clientText -match 'Client received world-generation identity: Seed=418 Revision=4' -and
            $clientText -match 'World map profile: OpenMs=.*WorkerTotalMs=.*GameThreadTotalMs=.*Tiles=\d+ CacheBytes=\d+.') { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Full-map profile timed out.' }
    Write-Output 'PASS: captured host/client full-map open, worker, game-thread, cache, and late-join identity metrics.'
}
finally {
    foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    Write-Output "Profile logs: $output"
}
