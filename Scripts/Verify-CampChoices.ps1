param(
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [int]$Port = 17841
)
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$output = Join-Path $env:TEMP ('KalmalaCampChoices-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -KalmalaCampChoiceTest -forcelogflush -ExecCmds="t.MaxFPS 60"'
$server = $null
$client = $null
try {
    $server = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 -GeneratorRevision=1 $common -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited) { throw 'Listen server exited before accepting connections.' }
        if ((Test-Path $serverLog) -and (Select-String -Path $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Listen server readiness timed out.' }
    $client = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A peer exited before scenario completion.' }
        $serverText = Get-Content $serverLog -Raw
        if ($serverText -match 'Camp choice FAILED') { throw 'Server scenario assertions failed.' }
        if ($serverText -match 'Camp choice server PASSED') { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Two-player scenario timed out.' }
    $clientText = Get-Content $clientLog -Raw
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418 Revision=1') { throw 'Client world identity did not match the server.' }
    $weatherPattern = 'weather cycle \d+: Start=[\d.]+ Duration=[\d.]+ Precipitation=[\d.]+ WindDirection=\d+ WindStrength=[\d.]+\.'
    $weather = [regex]::Match($serverText, $weatherPattern).Value
    if (!$weather -or !$clientText.Contains($weather)) { throw 'Client weather did not match the server.' }
    $sites = [regex]::Matches($serverText, 'Camp choice site \d pawn=(\S+)')
    if ($sites.Count -ne 2) { throw 'Expected exactly two camp occupants.' }
    foreach ($site in $sites) {
        $pawn = $site.Groups[1].Value
        $pattern = 'Camp choice client ' + [regex]::Escape($pawn) + ': Wetness=[\d.]+ Warmth=[\d.]+ Travel=[\d.]+\.'
        $matches = @([regex]::Matches($clientText, $pattern) | Where-Object { $serverText.Contains($_.Value.Replace('Camp choice client ', 'Camp choice server ')) })
        if ($matches.Count -lt 10) { throw "Insufficient matching replicated samples for $pawn." }
        Write-Output "$pawn matched $($matches.Count) server exposure snapshots."
    }
    if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:') { throw 'Unreal reported a fatal error.' }
    Select-String -Path $serverLog -Pattern 'Camp choice site|Camp choice unprepared|Camp choice prepared|Camp choice server PASSED' | ForEach-Object { $_.Line }
    Write-Output 'PASS: two camp choices, normal fire recovery, matching client weather and both pawn states.'
}
finally {
    foreach ($peer in @($client, $server)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Scenario logs: $output"
}
