param([int]$Port = 17869, [switch]$Rendered)
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path $env:TEMP ('KalmalaCrafting-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$common = '-game -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaCraftingTest -ExecCmds="t.MaxFPS 60"'
if ($Rendered) { $common += ' -windowed -RenderOffscreen -ResX=1280 -ResY=720 -ForceRes' } else { $common += ' -nullrhi' }
$serverCapture = if ($Rendered) { "-KalmalaCraftingCapture=`"$output\host.png`"" } else { '' }
$clientCapture = if ($Rendered) { "-KalmalaCraftingCapture=`"$output\client.png`"" } else { '' }
$server = $null
$client = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common $serverCapture -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited) { throw 'Listen server exited during startup.' }
        if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Listen server readiness timed out.' }
    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common $clientCapture -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(120)
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A peer exited before verification.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Crafting fixture FAILED:|Crafting [^\r\n]*Passed=0|Restored=0') { throw 'Crafting verification failed; inspect retained logs.' }
        $ready = [regex]::Matches($serverText, 'Crafting server gates: Passed=1').Count -eq 2 `
            -and [regex]::Matches($serverText, 'Crafting server final: Passed=1').Count -eq 2 `
            -and $serverText.Contains('Crafting owner final: Passed=1 Authority=1 Fuel=2 Slots=1') `
            -and $clientText.Contains('Crafting owner final: Passed=1 Authority=0 Fuel=2 Slots=1') `
            -and $serverText.Contains('Crafting presentation: Passed=1 Restored=1') `
            -and $clientText.Contains('Crafting presentation: Passed=1 Restored=1')
        $ready = $ready -and [regex]::Matches($serverText, 'Crafting RPC: Recipe=Forged Batch=1 Accepted=0').Count -eq 2 `
            -and [regex]::Matches($serverText, 'Crafting RPC: Recipe=Fuel Batch=2147483647 Accepted=0').Count -eq 2 `
            -and [regex]::Matches($serverText, 'Crafting placement RPC: Accepted=0').Count -eq 2
        foreach ($state in @('Fuel=60 Lit=1 Wet=0 Warmth=1', 'Fuel=48 Lit=0 Wet=96 Warmth=0')) {
            $serverNames = [regex]::Matches($serverText, ('Crafting fire server: Name=(\S+) ' + [regex]::Escape($state))) | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique
            $clientNames = [regex]::Matches($clientText, ('Crafting fire client: Name=(\S+) ' + [regex]::Escape($state))) | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique
            $ready = $ready -and @($serverNames).Count -eq 2 -and @($clientNames).Count -eq 2
        }
        if ($Rendered) { $ready = $ready -and (Test-Path "$output\host.png") -and (Test-Path "$output\client.png") }
        if ($ready) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if (!$ready) { throw 'Crafting host/client scenario timed out.' }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Client identity mismatch.' }
    Write-Output 'PASS: server validation/payment/atomicity gates; overlapping owner RPC crafting; exact final inventory; two matching dry and rain-extinguished fires; local menu input restoration.'
}
finally {
    foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    Write-Output "Scenario logs: $output"
}
