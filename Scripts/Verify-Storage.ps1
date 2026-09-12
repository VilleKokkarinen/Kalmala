param([int]$Port = 17971)
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path $env:TEMP ('KalmalaStorage-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaStorageTest -ExecCmds="t.MaxFPS 60"'
$script:originalIds = @()

function Invoke-StorageRun([int]$Run) {
    $serverLog = Join-Path $output "server-$Run.log"
    $clientLog = Join-Path $output "client-$Run.log"
    $restore = if ($Run -eq 2) { '-KalmalaStorageRestore' } else { '' }
    $server = $null; $client = $null
    try {
        $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common $restore -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
        $deadline = (Get-Date).AddSeconds(90)
        do {
            if ($server.HasExited) { throw "Run $Run server exited during startup." }
            $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
            $listenMatch = [regex]::Match([string]$serverText, 'GameNetDriver.*listening on port (\d+)')
            if ($listenMatch.Success) { $boundPort = [int]$listenMatch.Groups[1].Value; break }
            Start-Sleep -Milliseconds 500
        } while ((Get-Date) -lt $deadline)
        if ((Get-Date) -ge $deadline) { throw "Run $Run server readiness timed out." }
        $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$boundPort -WorldSeed=999 $common $restore -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
        $deadline = (Get-Date).AddSeconds(100)
        $ready = $false
        do {
            if ($server.HasExited -or $client.HasExited) { throw "Run $Run peer exited before verification." }
            $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
            $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
            if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Storage fixture FAILED:') { throw "Run $Run failed; inspect retained logs." }
            $ready = [regex]::Matches($serverText, 'Storage server final: Passed=1 Wood=3').Count -eq 2 `
                -and $serverText.Contains('Storage owner final: Passed=1 Authority=1 Private=1 Wood=3') `
                -and $clientText.Contains('Storage owner final: Passed=1 Authority=0 Private=1 Wood=3') `
                -and $clientText.Contains('Storage owner closed: Authority=0 Cleared=1')
            if ($ready) { break }
            Start-Sleep -Milliseconds 500
        } while ((Get-Date) -lt $deadline)
        if (!$ready) { throw "Run $Run storage RPC scenario timed out." }
        if (!$clientText.Contains('Client received world-generation identity: Seed=418')) { throw 'Client identity mismatch.' }
        $ids = @([regex]::Matches($serverText, 'Storage server ready: Restore=\d Id=([0-9a-f-]+) Wood=3') | ForEach-Object { $_.Groups[1].Value } | Sort-Object)
        if ($ids.Count -ne 2 -or $ids[0] -eq $ids[1]) { throw 'Expected two distinct chest IDs.' }
        if ($Run -eq 1) { $script:originalIds = $ids }
        elseif (@(Compare-Object $script:originalIds $ids).Count -ne 0) { throw 'Restart changed stable chest IDs.' }
        Write-Output "PASS run ${Run}: paid workbench/chests, validated owner RPC transfers, private snapshots, exact saved contents and stable IDs."
    }
    finally {
        foreach ($peer in @($client,$server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    }
}

try { Invoke-StorageRun 1; Invoke-StorageRun 2 }
finally { Write-Output "Scenario logs: $output" }
