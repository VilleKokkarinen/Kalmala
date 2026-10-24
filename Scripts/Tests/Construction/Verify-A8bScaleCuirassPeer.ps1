param([int]$Port = 17901, [string]$OutputDirectory = '', [string]$Project = '')
$ErrorActionPreference = 'Stop'
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot

$projectPath = if ($Project) { $Project } else { Join-Path $repoPaths.ProjectRoot 'Kalmala.uproject' }
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('A8bScalePeer-' + [guid]::NewGuid().ToString('N')) }
if ([IO.Path]::GetFullPath($output).Length -gt 180) { throw "A8b Scale Cuirass peer output root is too long: $output" }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$hostDir = Join-Path $output 'Host'
$ownerDir = Join-Path $output 'Owner'
$observerDir = Join-Path $output 'Observer'
$serverLog = Join-Path $output 'server.log'
$ownerLog = Join-Path $output 'owner.log'
$observerLog = Join-Path $output 'observer.log'
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaA8bScaleCuirassPeerTest -ExecCmds="t.MaxFPS 60"'
New-Item -ItemType Directory -Path $hostDir, $ownerDir, $observerDir -Force | Out-Null
$server = $null
$owner = $null
$observer = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -UserDir=`"$hostDir`" -abslog=`"$serverLog`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited) { throw "Scale Cuirass listen server exited with code $($server.ExitCode)." }
        if ((Test-Path $serverLog) -and (Select-String -Path $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Scale Cuirass listen server readiness timed out.' }

    $owner = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" 127.0.0.1:$Port -WorldSeed=999 $common -UserDir=`"$ownerDir`" -abslog=`"$ownerLog`""
    $joinDeadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited -or $owner.HasExited) { throw 'Scale Cuirass server or owner exited before the owner joined.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        if ([regex]::Matches($serverText, 'LogNet: Join succeeded:').Count -ge 1) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $joinDeadline)
    if ((Get-Date) -ge $joinDeadline) { throw 'Scale Cuirass owner did not join before the observer launch.' }

    $observer = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" 127.0.0.1:$Port -WorldSeed=1000 $common -UserDir=`"$observerDir`" -abslog=`"$observerLog`""
    $deadline = (Get-Date).AddSeconds(240)
    $passed = $false
    do {
        if ($server.HasExited -or $owner.HasExited -or $observer.HasExited) {
            $codes = @($server, $owner, $observer) | ForEach-Object { if ($_.HasExited) { $_.ExitCode } else { 'running' } }
            throw "A8b Scale Cuirass peer process exited before acceptance; exit codes: $($codes -join ',')."
        }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $ownerText = if (Test-Path $ownerLog) { Get-Content $ownerLog -Raw } else { '' }
        $observerText = if (Test-Path $observerLog) { Get-Content $observerLog -Raw } else { '' }
        $allText = "$serverText`n$ownerText`n$observerText"
        if ($allText -match 'Fatal error:|Assertion failed:|Ensure condition failed:|A8b Scale Cuirass peer FAILED:|A8b Scale Cuirass peer (server|owner|listen host|observer): Passed=0') {
            throw 'A8b Scale Cuirass peer verification failed; inspect retained logs.'
        }
        $passed = $serverText.Contains('A8b Scale Cuirass peer server: Passed=1 Craft=1 Payment=1 Output=1 Guid=1 Equip=1 Armor=20 Damage=10 Condition=1') `
            -and $serverText.Contains('A8b Scale Cuirass peer listen host: Passed=1 HostItems=Stable') `
            -and $ownerText.Contains('A8b Scale Cuirass peer owner: Passed=1 Authority=0 Craft=1 Payment=1 Equip=1 ArmorDamage=1 Condition=1') `
            -and $observerText.Contains('A8b Scale Cuirass peer observer: Passed=1 Authority=0 PublicArmorDamage=1 TargetPrivate=1')
        if (!$passed) { Start-Sleep -Milliseconds 500 }
    } while (!$passed -and (Get-Date) -lt $deadline)
    if (!$passed) { throw 'A8b Scale Cuirass host/owner/observer acceptance timed out.' }
    if (($ownerText -notmatch 'Client received world-generation identity: Seed=418') -or ($observerText -notmatch 'Client received world-generation identity: Seed=418')) {
        throw 'Scale Cuirass peer client world identity mismatch.'
    }
    Write-Output 'PASS: the remote owner crafted and equipped the Scale Cuirass through server RPCs; 20 Armor reduced accepted damage, condition wear and peer privacy passed.'
}
finally {
    foreach ($peer in @($observer, $owner, $server)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Scenario logs: $output"
}
