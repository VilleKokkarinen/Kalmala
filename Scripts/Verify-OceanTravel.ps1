param([int]$Port = 17845)

$ErrorActionPreference = 'Stop'
$project = Join-Path $PSScriptRoot '..\Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path ([System.IO.Path]::GetTempPath()) ("KalmalaOceanTravel-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -KalmalaOceanTravelTest'
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
    $deadline = (Get-Date).AddSeconds(300)
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'An ocean-travel test peer exited.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ocean travel test could not (resolve|find)|Ocean travel terrain audit failed') { throw 'Ocean-travel verification failed; inspect logs.' }
        $serverPass = $serverText -match 'Ocean travel test owner entered open ocean. Authority=1.' -and $serverText -match 'Ocean travel test owner reached the seeded island. Authority=1.' -and $serverText -match 'Ocean travel terrain audit passed: .* Authority=1.'
        $clientPass = $clientText -match 'Ocean travel test owner entered open ocean. Authority=0.' -and $clientText -match 'Ocean travel test owner reached the seeded island. Authority=0.' -and $clientText -match 'Ocean travel terrain audit passed: .* Authority=0.'
        if ($serverPass -and $clientPass) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Ocean-travel verification timed out.' }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Client world identity mismatch.' }
    if (($serverText + $clientText) -match 'ClientAdjustPosition|movement base') { throw 'Ocean-travel verification found a terrain/movement disagreement; inspect logs.' }
    Write-Output 'PASS: host and client crossed the shared generated ocean with complete, duplicate-free terrain neighborhoods.'
}
finally {
    foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    Write-Output "Scenario logs: $output"
}
