param([int]$Port = 18149, [string]$OutputDirectory = '', [string]$Project = '')
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot

$ErrorActionPreference = 'Stop'
$project = if ($Project) { $Project } else { Join-Path ($repoPaths.ProjectRoot) 'Kalmala.uproject' }
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaRatPeer-' + [guid]::NewGuid().ToString('N')) }
if ([IO.Path]::GetFullPath($output).Length -gt 180) { throw "Rat peer output root is too long for nested Windows paths: $output" }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$hostShaderDir = Join-Path $output 'Host\ShaderWorkingDir'
$clientShaderDir = Join-Path $output 'Client\ShaderWorkingDir'
$remoteShaderDir = Join-Path $output 'RemoteClient\ShaderWorkingDir'
$restartShaderDir = Join-Path $output 'Restart\ShaderWorkingDir'
foreach ($shaderDir in @($hostShaderDir, $clientShaderDir, $remoteShaderDir, $restartShaderDir)) {
    New-Item -ItemType Directory -Path $shaderDir -Force | Out-Null
}
$serverLog = Join-Path $output 'server.log'; $clientLog = Join-Path $output 'client.log'
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaRatPeerTest'
$server = $null; $client = $null; $remoteClient = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -ShaderWorkingDir=`"$hostShaderDir`" -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do { if ($server.HasExited) { throw 'Rat listen server exited during startup.' }; if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }; Start-Sleep -Milliseconds 500 } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Rat listen server readiness timed out.' }
    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -ShaderWorkingDir=`"$clientShaderDir`" -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $remoteClientLog = Join-Path $output 'remote-client.log'
    $remoteClient = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=1000 $common -ShaderWorkingDir=`"$remoteShaderDir`" -abslog=`"$remoteClientLog`" -UserDir=`"$output\RemoteClient`""
    $deadline = (Get-Date).AddSeconds(90); $ready = $false
    do {
        if ($server.HasExited -or $client.HasExited -or $remoteClient.HasExited) { throw 'A Rat peer exited before verification.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        $remoteClientText = if (Test-Path $remoteClientLog) { Get-Content $remoteClientLog -Raw } else { '' }
        $peerText = $clientText + $remoteClientText
        if (($serverText + $peerText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Combat verification FAILED:|Rat verification FAILED:|Rat verification server: Passed=0') { throw 'Rat peer verification failed; inspect retained logs.' }
        $ready = $serverText -match 'Rat verification server: Passed=1 SeedReproduced=1 BoundedActivation=1 InvalidRejected=1 ActionSerial=1 Health=0\.0 PlayerHealth=[0-9]+\.0 Defeated=1 Saved=1 FullPackEscrow=1 RetryDelivered=1 OwnerOnly=1 BoneFragment=1 RatTail=1 RemoteBoneFragment=0 RemoteRatTail=0 OtherPeerBoneFragments=0 OtherPeerRatTails=0 OtherPeers=(?:[2-9]|[1-9][0-9]+)'
        $ready = $ready -and $clientText -match 'Rat verification client rejected invalid owned attack without target data\.'
        $ready = $ready -and $remoteClientText -match 'Rat verification client rejected invalid owned attack without target data\.'
        $ready = $ready -and $clientText -match 'Rat verification client observed shared action serial=1\.'
        $ready = $ready -and $remoteClientText -match 'Rat verification client observed shared action serial=1\.'
        $ready = $ready -and $clientText -match 'Rat verification client observed relevant wildlife defeat\.'
        $ready = $ready -and $remoteClientText -match 'Rat verification client observed relevant wildlife defeat\.'
        $ready = $ready -and $clientText -match 'Rat verification client observed replicated player health=[0-9]+\.0\.'
        $ready = $ready -and $remoteClientText -match 'Rat verification client observed replicated player health=[0-9]+\.0\.'
        if ($ready) { break }; Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if (!$ready) {
        Write-Output ("Rat peer timeout detail: ServerPassed={0} InvalidRejected={1} SharedAction={2} Defeat={3} ReplicatedHealth={4}" -f
            ($serverText -match 'Rat verification server: Passed=1'),
            ($clientText -match 'Rat verification client rejected invalid owned attack without target data\.' -and $remoteClientText -match 'Rat verification client rejected invalid owned attack without target data\.'),
            ($clientText -match 'Rat verification client observed shared action serial=1\.' -and $remoteClientText -match 'Rat verification client observed shared action serial=1\.'),
            ($clientText -match 'Rat verification client observed relevant wildlife defeat\.' -and $remoteClientText -match 'Rat verification client observed relevant wildlife defeat\.'),
            ($clientText -match 'Rat verification client observed replicated player health=[0-9]+\.0\.' -and $remoteClientText -match 'Rat verification client observed replicated player health=[0-9]+\.0\.'))
        throw 'Rat peer scenario timed out.'
    }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Rat peer client identity mismatch.' }
    if ($remoteClientText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Rat observer identity mismatch.' }
    $spawnMatch = [regex]::Match($serverText, 'Rat verification server: Passed=1[^\r\n]* SpawnId=([0-9]+/-?\d+/-?\d+/\d+)')
    if (!$spawnMatch.Success) { throw 'Rat peer result did not include the defeated spawn identity.' }
    $spawnId = $spawnMatch.Groups[1].Value
    $spawnIdParts = $spawnId.Split('/')
    $spawnKeyX = [int]$spawnIdParts[1]; $spawnKeyY = [int]$spawnIdParts[2]
    Stop-Process -Id $client.Id; Stop-Process -Id $remoteClient.Id; Stop-Process -Id $server.Id; $client = $null; $remoteClient = $null; $server = $null
    $restartLog = Join-Path $output 'restart.log'
    $restart = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 -game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaReconnectVerification=WildlifeVerify -KalmalaReconnectKeyX=$spawnKeyX -KalmalaReconnectKeyY=$spawnKeyY -KalmalaReconnectSpawnId=`"$spawnId`" -ShaderWorkingDir=`"$restartShaderDir`" -abslog=`"$restartLog`" -UserDir=`"$output\Host`""
    if (!$restart.WaitForExit(120000)) { Stop-Process -Id $restart.Id; throw 'Rat world-state restart timed out.' }
    $restartText = Get-Content $restartLog -Raw
    if ($restartText -notmatch 'Reconnect verification passed: defeated generated wildlife spawn .* remained absent after listen-server restart\.') { throw 'Rat defeat did not survive restart.' }
    Write-Output 'PASS: the generated Rat hit the server host; one real mace attack defeated it, its full-pack Rat Tail claim retried only to the owner, peers saw the replicated defeat without the reward, and that exact Rat remained absent after restart.'
}
finally { foreach ($peer in @($client, $remoteClient, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }; Write-Output "Peer logs: $output" }
