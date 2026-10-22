param(
    [ValidateSet('Both', 'Badger', 'MoonBear')][string]$Archetype = 'Both',
    [int]$Port = 18270,
    [string]$OutputDirectory = '',
    [string]$Project = ''
)
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot
$ErrorActionPreference = 'Stop'
$projectPath = if ($Project) { $Project } else { Join-Path $repoPaths.ProjectRoot 'Kalmala.uproject' }
if ($Archetype -eq 'Both') {
    if ($Port -lt 1 -or $Port -ge 65535) { throw 'Choose a starting port from 1 through 65534 for the combined run.' }
    $batchOutput = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaElderwood-' + [guid]::NewGuid().ToString('N')) }
    $badgerOutput = Join-Path $batchOutput 'Badger'
    $moonBearOutput = Join-Path $batchOutput 'MoonBear'
    foreach ($path in @($batchOutput, $badgerOutput, $moonBearOutput)) {
        if ($path.Length -ge 260) { throw "Windows MAX_PATH limit reached for test output: $path" }
    }
    & $PSCommandPath -Archetype Badger -Port $Port -OutputDirectory $badgerOutput -Project $projectPath
    & $PSCommandPath -Archetype MoonBear -Port ($Port + 1) -OutputDirectory $moonBearOutput -Project $projectPath
    Write-Output 'PASS: combined Elderwood Badger and Moon Bear peer acceptance.'
    return
}
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$label = if ($Archetype -eq 'MoonBear') { 'Moon Bear' } else { 'Badger' }
$switch = if ($Archetype -eq 'MoonBear') { '-KalmalaMoonBearPeerTest' } else { '-KalmalaBadgerPeerTest' }
$expectedReward = if ($Archetype -eq 'MoonBear') {
    'BoneFragment=3 BadgerFur=0 BadgerMeat=0 RemoteBoneFragment=0 RemoteBadgerFur=0 RemoteBadgerMeat=0'
} else {
    'BoneFragment=1 BadgerFur=1 BadgerMeat=1 RemoteBoneFragment=0 RemoteBadgerFur=0 RemoteBadgerMeat=0'
}
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaElderwood-' + $Archetype + '-' + [guid]::NewGuid().ToString('N')) }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$serverShaderDir = Join-Path $output 'Host\ShaderWorkingDir'
$clientShaderDir = Join-Path $output 'Client\ShaderWorkingDir'
$observerShaderDir = Join-Path $output 'Observer\ShaderWorkingDir'
$restartShaderDir = Join-Path $output 'Restart\ShaderWorkingDir'
foreach ($path in @($serverShaderDir, $clientShaderDir, $observerShaderDir, $restartShaderDir)) {
    New-Item -ItemType Directory -Path $path -Force | Out-Null
}
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$observerLog = Join-Path $output 'observer.log'
$server = $null; $client = $null; $observer = $null; $restart = $null
try {
    $common = "-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush $switch"
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -ShaderWorkingDir=`"$serverShaderDir`" -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited) { throw "${label} listen server exited during startup with code $($server.ExitCode)." }
        if ((Test-Path $serverLog) -and (Select-String -Path $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw "${label} listen server readiness timed out." }

    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" 127.0.0.1:$Port -WorldSeed=999 $common -ShaderWorkingDir=`"$clientShaderDir`" -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $observer = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" 127.0.0.1:$Port -WorldSeed=1000 $common -ShaderWorkingDir=`"$observerShaderDir`" -abslog=`"$observerLog`" -UserDir=`"$output\Observer`""
    $deadline = (Get-Date).AddSeconds(240); $passed = $false
    do {
        if ($server.HasExited -or $client.HasExited -or $observer.HasExited) {
            $codes = @($server, $client, $observer) | Where-Object { $null -ne $_ } | ForEach-Object { if ($_.HasExited) { $_.ExitCode } else { 'running' } }
            $codes = $codes -join ','
            throw "${label} peer exited before acceptance completed; exit codes: $codes."
        }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        $observerText = if (Test-Path $observerLog) { Get-Content $observerLog -Raw } else { '' }
        $allText = $serverText + $clientText + $observerText
        $hasCrash = $allText -match 'Fatal error:|Assertion failed:|Ensure condition failed:'
        $hasFixtureFailure = $serverText -match [regex]::Escape("$label verification FAILED:")
        if ($hasCrash -or $hasFixtureFailure) {
            throw "${label} peer acceptance failed or Unreal terminated abnormally; inspect retained logs."
        }
        $resultPattern = [regex]::Escape("$label verification server: Passed=1") + "[^\r\n]*FullPackEscrow=1 RetryDelivered=1 OwnerOnly=1 $expectedReward[^\r\n]* SpawnId=([0-9]+/-?\d+/-?\d+/\d+)"
        $result = [regex]::Match($serverText, $resultPattern)
        $peerText = $clientText + $observerText
        $peerEvidence = ($peerText -match [regex]::Escape("$label verification client rejected invalid owned attack without target data.")) -and
            ($peerText -match [regex]::Escape("$label verification client observed shared action serial=4.")) -and
            ($peerText -match [regex]::Escape("$label verification client observed relevant wildlife defeat."))
        $passed = $result.Success -and $peerEvidence
        if ($passed) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if (!$passed) { throw "${label} live peer acceptance timed out; inspect retained logs." }

    $spawnId = $result.Groups[1].Value
    $parts = $spawnId.Split('/')
    $keyX = [int]$parts[1]; $keyY = [int]$parts[2]
    Stop-Process -Id $client.Id; Stop-Process -Id $observer.Id; Stop-Process -Id $server.Id
    $client = $null; $observer = $null; $server = $null

    $restartLog = Join-Path $output 'restart.log'
    $restart = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 -game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaReconnectVerification=WildlifeVerify -KalmalaReconnectKeyX=$keyX -KalmalaReconnectKeyY=$keyY -KalmalaReconnectSpawnId=`"$spawnId`" -ShaderWorkingDir=`"$restartShaderDir`" -abslog=`"$restartLog`" -UserDir=`"$output\Host`""
    if (!$restart.WaitForExit(120000)) { Stop-Process -Id $restart.Id; throw "${label} world-state restart timed out." }
    $restartText = Get-Content $restartLog -Raw
    if ($restartText -notmatch 'Reconnect verification passed: defeated generated wildlife spawn .* remained absent after listen-server restart\.') {
        throw "${label} defeat did not survive the same-world listen-server restart."
    }
    Write-Output "PASS: $label defeated on the server; its full-pack reward stayed escrowed, retried atomically to the credited owner after capacity was freed, remained private to the two other clients, and the spawn stayed defeated after restart."
}
finally {
    foreach ($process in @($client, $observer, $server, $restart)) {
        if ($null -ne $process -and !$process.HasExited) { Stop-Process -Id $process.Id }
    }
    Write-Output "Peer logs: $output"
}
