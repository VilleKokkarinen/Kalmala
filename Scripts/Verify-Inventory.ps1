param([int]$Port = 17849)
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path $env:TEMP ('KalmalaInventory-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaInventoryTest'
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
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Inventory server: Passed=0|Harvest inventory: Passed=0|Inventory remote: Empty=0|Inventory owner: Rejected=0') { throw 'Inventory verification failed.' }
        $ownerIndex = $clientText.IndexOf('Inventory owner: Rejected=1 Wood=7 Slots=1')
        if ([regex]::Matches($serverText, 'Inventory server: Passed=1 Wood=7 Slots=1').Count -eq 2 `
            -and [regex]::Matches($serverText, 'Harvest inventory: Passed=1 Materials=3 Range=1 Full=1 Malformed=1 Duplicate=1 SparseDelta=1').Count -eq 2 `
            -and $ownerIndex -ge 0 -and $clientText.IndexOf('Inventory remote: Empty=1', $ownerIndex) -gt $ownerIndex `
            -and $serverText.Contains('Inventory presentation: Owner=1 Wood=7 ReadOnly=1') `
            -and $clientText.Contains('Inventory presentation: Owner=1 Wood=7 ReadOnly=1')) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Inventory host/client scenario timed out.' }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418 Revision=4') { throw 'Client world identity mismatch.' }
    Write-Output 'PASS: server grants/removals and rejection checks, owner-only contents, rejected local client mutations, remote privacy after replication, and read-only local presentation on both peers.'
}
finally {
    foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    Write-Output "Scenario logs: $output"
}
