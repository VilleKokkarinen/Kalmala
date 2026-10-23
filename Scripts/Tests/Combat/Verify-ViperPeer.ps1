param([int]$Port = 18166, [string]$OutputDirectory = '', [string]$Project = '')
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot

$ErrorActionPreference = 'Stop'
$project = if ($Project) { $Project } else { Join-Path $repoPaths.ProjectRoot 'Kalmala.uproject' }
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaViperPeer-' + [guid]::NewGuid().ToString('N')) }
if ([IO.Path]::GetFullPath($output).Length -gt 180) { throw "Viper peer output root is too long for nested Windows paths: $output" }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$hostShaderDir = Join-Path $output 'Host\ShaderWorkingDir'
$clientShaderDir = Join-Path $output 'Client\ShaderWorkingDir'
$observerShaderDir = Join-Path $output 'Observer\ShaderWorkingDir'
$restartShaderDir = Join-Path $output 'Restart\ShaderWorkingDir'
foreach ($shaderDir in @($hostShaderDir, $clientShaderDir, $observerShaderDir, $restartShaderDir)) {
    New-Item -ItemType Directory -Path $shaderDir -Force | Out-Null
}
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$observerLog = Join-Path $output 'observer.log'
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaViperPeerTest'
$server = $null; $client = $null; $observer = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -ShaderWorkingDir=`"$hostShaderDir`" -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited) { throw "Viper listen server exited during startup with code $($server.ExitCode)." }
        if ((Test-Path $serverLog) -and (Select-String -Path $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Viper listen server readiness timed out.' }

    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -ShaderWorkingDir=`"$clientShaderDir`" -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $observer = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=1000 $common -ShaderWorkingDir=`"$observerShaderDir`" -abslog=`"$observerLog`" -UserDir=`"$output\Observer`""
    $deadline = (Get-Date).AddSeconds(240); $passed = $false
    do {
        if ($server.HasExited -or $client.HasExited -or $observer.HasExited) {
            $codes = @($server, $client, $observer) | ForEach-Object { if ($_.HasExited) { $_.ExitCode } else { 'running' } }
            throw "Viper peer exited before acceptance completed; exit codes: $($codes -join ',')."
        }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        $observerText = if (Test-Path $observerLog) { Get-Content $observerLog -Raw } else { '' }
        $allText = $serverText + $clientText + $observerText
        if ($allText -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Combat verification FAILED:|Viper verification FAILED:') {
            throw 'Viper peer verification failed or Unreal terminated abnormally; inspect retained logs.'
        }
        $serverPassed = $serverText -match 'Viper verification server: Passed=1 SeedReproduced=1 BoundedActivation=1 InvalidRejected=1 ActionSerial=4 WeakPoison=1 Defeated=1 Saved=1 FullPackEscrow=1 RetryDelivered=1 OwnerOnly=1 BoneFragment=1 ViperFang=1 RemoteBoneFragment=0 RemoteViperFang=0 OtherPeerBoneFragments=0 OtherPeerViperFangs=0 OtherPeers=2 Owner=.* SpawnId=([0-9]+/-?\d+/-?\d+/\d+)'
        $peerText = $clientText + $observerText
        $peerEvidence = $peerText -match 'Viper verification client rejected invalid owned attack without target data\.'
        $peerEvidence = $peerEvidence -and $peerText -match 'Viper verification client observed shared action serial=4\.'
        $peerEvidence = $peerEvidence -and $peerText -match 'Viper verification client observed relevant wildlife defeat\.'
        $peerEvidence = $peerEvidence -and $peerText -match 'Viper verification client observed replicated player health=\d+\.0\.'
        $passed = $serverPassed -and $peerEvidence
        if ($passed) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if (!$passed) { throw 'Viper peer scenario timed out; inspect retained logs.' }

    if ($clientText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Viper peer client identity mismatch.' }
    if ($observerText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Viper observer identity mismatch.' }
    $spawnMatch = [regex]::Match($serverText, 'Viper verification server: Passed=1[^\r\n]* SpawnId=([0-9]+/-?\d+/-?\d+/\d+)')
    if (!$spawnMatch.Success) { throw 'Viper peer result did not include the defeated spawn identity.' }
    $spawnId = $spawnMatch.Groups[1].Value
    $spawnIdParts = $spawnId.Split('/')
    $spawnKeyX = [int]$spawnIdParts[1]; $spawnKeyY = [int]$spawnIdParts[2]
    Stop-Process -Id $client.Id; Stop-Process -Id $observer.Id; Stop-Process -Id $server.Id
    $client = $null; $observer = $null; $server = $null

    $restartLog = Join-Path $output 'restart.log'
    $restart = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 -game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaReconnectVerification=WildlifeVerify -KalmalaReconnectKeyX=$spawnKeyX -KalmalaReconnectKeyY=$spawnKeyY -KalmalaReconnectSpawnId=`"$spawnId`" -ShaderWorkingDir=`"$restartShaderDir`" -abslog=`"$restartLog`" -UserDir=`"$output\Host`""
    if (!$restart.WaitForExit(120000)) { Stop-Process -Id $restart.Id; throw 'Viper world-state restart timed out.' }
    $restartText = Get-Content $restartLog -Raw
    if ($restartText -notmatch 'Reconnect verification passed: defeated generated wildlife spawn .* remained absent after listen-server restart\.') {
        throw 'Viper defeat did not survive the same-world listen-server restart.'
    }
    Write-Output 'PASS: a generated dry-shore Viper landed its weak poison bite; its full-pack Bone Fragment and Viper Fang claim retried only to the owner, both clients saw the replicated defeat without rewards, and that exact Viper remained absent after restart.'
}
finally {
    foreach ($peer in @($client, $observer, $server)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Peer logs: $output"
}
