param(
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [int]$Port = 17991,
    [ValidateRange(1, 4)][int]$GeneratorRevision = 4
)
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$output = Join-Path $env:TEMP ('KalmalaConstructionMovement-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaConstructionMovementTest -ExecCmds="t.MaxFPS 60"'
$server = $null
$client = $null
try {
    $server = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 -GeneratorRevision=$GeneratorRevision $common -abslog=`"$serverLog`" -UserDir=`"$output/Host`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited) { throw 'Server exited during startup.' }
        if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Server startup timed out.' }
    $client = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -abslog=`"$clientLog`" -UserDir=`"$output/Client`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A construction movement peer exited.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Construction (movement|roof): Passed=0') { throw 'Construction movement verification failed; inspect logs.' }
        $localPass = $serverText -match 'Construction movement: Passed=1 Authority=1 Local=1' -and $clientText -match 'Construction movement: Passed=1 Authority=0 Local=1'
        $remotePass = $serverText -match 'Construction movement: Passed=1 Authority=1 Local=0'
        $roofPass = $serverText -match 'Construction roof: Passed=1 Authority=1 Local=1' `
            -and $serverText -match 'Construction roof: Passed=1 Authority=1 Local=0' `
            -and $clientText -match 'Construction roof: Passed=1 Authority=0 Local=1'
        if ($localPass -and $remotePass -and $roofPass) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Construction movement verification timed out.' }
    if ($clientText -notmatch "Client received world-generation identity: Seed=418 Revision=$GeneratorRevision") { throw 'Client world identity mismatch.' }
    $pattern = 'Construction movement: Passed=1 Authority={0} Local={1} Player=(\d+) Floor=(\S+) Wall=(\S+) Grounded=1 Travel=[\d.]+ Y=([\d.-]+)'
    $remote = [regex]::Match($serverText, ($pattern -f 1, 0))
    $owner = [regex]::Match($clientText, ($pattern -f 0, 1))
    if (!$remote.Success -or !$owner.Success) { throw 'Missing remote movement evidence.' }
    foreach ($index in 1..3) {
        if ($remote.Groups[$index].Value -ne $owner.Groups[$index].Value) { throw 'Player or construction identity mismatch.' }
    }
    if ([Math]::Abs([double]$remote.Groups[4].Value - [double]$owner.Groups[4].Value) -gt 3) { throw 'Server/client stopping position mismatch.' }
    $roofPattern = 'Construction roof: Passed=1 Authority={0} Local={1} Player=(\d+) Roof=(\S+) Airborne=1 Landed=1 Peak=([\d.]+) Ceiling=([\d.]+)'
    $remoteRoof = [regex]::Match($serverText, ($roofPattern -f 1, 0))
    $ownerRoof = [regex]::Match($clientText, ($roofPattern -f 0, 1))
    if (!$remoteRoof.Success -or !$ownerRoof.Success) { throw 'Missing roof evidence.' }
    foreach ($index in @(1, 2, 4)) {
        if ($remoteRoof.Groups[$index].Value -ne $ownerRoof.Groups[$index].Value) { throw 'Roof identity or ceiling mismatch.' }
    }
    if ($remoteRoof.Groups[1].Value -ne $remote.Groups[1].Value) { throw 'Roof and wall player mismatch.' }
    if ([Math]::Abs([double]$remoteRoof.Groups[3].Value - [double]$ownerRoof.Groups[3].Value) -gt 10) { throw 'Server/client jump peak mismatch.' }
    Write-Output 'PASS: both owners walk on floors, stop at windbreaks, jump beneath replicated roofs and land; server confirms remote identities and movement.'
}
finally {
    foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    Write-Output "Scenario logs: $output"
}
