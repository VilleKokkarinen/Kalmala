param([int]$Port = 17871)
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path $env:TEMP ('KalmalaConstructionRestore-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaCraftingTest -ExecCmds="t.MaxFPS 60"'
$script:FirstPlacementIds = @()

function Invoke-PeerRun([int]$Run, [bool]$ExpectRestore) {
    $serverLog = Join-Path $output "server-$Run.log"
    $clientLog = Join-Path $output "client-$Run.log"
    $server = $null; $client = $null
    try {
        $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
        $deadline = (Get-Date).AddSeconds(90)
        do {
            if ($server.HasExited) { throw "Run $Run listen server exited during startup." }
            if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
            Start-Sleep -Milliseconds 500
        } while ((Get-Date) -lt $deadline)
        if ((Get-Date) -ge $deadline) { throw "Run $Run listen server readiness timed out." }
        $listenMatch = Select-String -LiteralPath $serverLog -Pattern 'GameNetDriver.*listening on port (\d+)' | Select-Object -Last 1
        if ($null -eq $listenMatch -or $listenMatch.Line -notmatch 'port (\d+)') { throw "Run $Run did not report its bound listen port." }
        $clientPort = [int]$Matches[1]
        $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$clientPort -WorldSeed=999 $common -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
        $deadline = (Get-Date).AddSeconds(120)
        do {
            if ($server.HasExited -or $client.HasExited) { throw "Run $Run peer exited before verification." }
            $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
            $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
            if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Crafting fixture FAILED:') { throw "Run $Run fixture failed; inspect retained logs." }
            $accepted = [regex]::Matches($serverText, 'Construction accepted: Id=([0-9a-f-]+) Kit=FloorKit')
            $replicated = [regex]::Matches($clientText, 'Construction replicated: Id=([0-9a-f-]+) Kit=FloorKit')
            $ready = $accepted.Count -eq 2 -and $replicated.Count -ge 2
            $acceptedIds = @($accepted | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique)
            $replicatedIds = @($replicated | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique)
            $expectedIds = $acceptedIds
            if ($ExpectRestore) {
                $restored = [regex]::Matches($serverText, 'Construction restored: Id=([0-9a-f-]+) Kit=FloorKit')
                $restoredIds = @($restored | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique)
                if ($restored.Count -eq 2 -and (@(Compare-Object $script:FirstPlacementIds $restoredIds).Count -ne 0)) {
                    throw 'Restored construction IDs differ from the first run paid placements.'
                }
                $expectedIds = @($script:FirstPlacementIds + $acceptedIds | Sort-Object -Unique)
                $ready = $ready -and $restored.Count -eq 2 -and $expectedIds.Count -eq 4
            }
            $ready = $ready -and $acceptedIds.Count -eq 2 -and $replicatedIds.Count -eq $expectedIds.Count
            if ($ready) { $ready = @(Compare-Object $expectedIds $replicatedIds).Count -eq 0 }
            if ($ready) { break }
            Start-Sleep -Milliseconds 500
        } while ((Get-Date) -lt $deadline)
        if (!$ready) { throw "Run $Run construction host/client scenario timed out." }
        if ($clientText -notmatch 'Client received world-generation identity: Seed=418') { throw "Run $Run client identity mismatch." }
        if ($ExpectRestore) { Write-Output 'PASS: exact two original construction IDs restored; client received those plus two distinct newly paid placements.' }
        else {
            $script:FirstPlacementIds = $acceptedIds
            Write-Output 'PASS: client received the exact two distinct server-paid construction IDs.'
        }
    }
    finally {
        foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    }
}

try {
    Invoke-PeerRun 1 $false
    Invoke-PeerRun 2 $true
}
finally {
    Write-Output "Scenario logs: $output"
}
