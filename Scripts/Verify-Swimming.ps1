param([int]$Port = 17844)

$ErrorActionPreference = 'Stop'
$project = Join-Path $PSScriptRoot '..\Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path ([System.IO.Path]::GetTempPath()) ("KalmalaSwimming-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -KalmalaSwimmingTest'
$server = $null
$client = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -abslog=`"$serverLog`" -UserDir=`"$output/Host`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited) { throw 'Server exited during startup.' }
        if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Server startup timed out.' }
    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -abslog=`"$clientLog`" -UserDir=`"$output/Client`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A swimming test peer exited.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:') { throw 'Swimming verification failed; inspect logs.' }
        $serverPass = $serverText -match 'Swimming test owner entered generated ocean. Authority=1.'
        $clientPass = $clientText -match 'Swimming test owner entered generated ocean. Authority=0.'
        if ($serverPass -and $clientPass) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Swimming verification timed out.' }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418 Revision=1') { throw 'Client world identity mismatch.' }
    Write-Output 'PASS: host and client entered the shared generated ocean using server-authoritative movement.'
}
finally {
    foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    Write-Output "Scenario logs: $output"
}
