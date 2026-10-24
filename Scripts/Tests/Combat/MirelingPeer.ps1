param([int]$Port = 18128, [string]$OutputDirectory = '', [switch]$Rendered, [string]$Project = '')
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot

$ErrorActionPreference = 'Stop'
$project = if ($Project) { $Project } else { Join-Path ($repoPaths.ProjectRoot) 'Kalmala.uproject' }
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaMirelingPeer-' + [guid]::NewGuid().ToString('N')) }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$hostShaderDir = Join-Path $output 'Host\ShaderWorkingDir'
$clientShaderDir = Join-Path $output 'Client\ShaderWorkingDir'
$remoteShaderDir = Join-Path $output 'RemoteClient\ShaderWorkingDir'
$restartShaderDir = Join-Path $output 'Restart\ShaderWorkingDir'
foreach ($shaderDir in @($hostShaderDir, $clientShaderDir, $remoteShaderDir, $restartShaderDir)) {
    New-Item -ItemType Directory -Path $shaderDir -Force | Out-Null
}
$serverLog = Join-Path $output 'server.log'; $clientLog = Join-Path $output 'client.log'
$renderer = if ($Rendered) { '-windowed -RenderOffscreen -ForceRes -ResX=1280 -ResY=720' } else { '-nullrhi' }
$common = "-game $renderer -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaMirelingPeerTest"
$serverCapture = "-ShaderWorkingDir=`"$hostShaderDir`"" + $(if ($Rendered) { " -KalmalaMirelingScreenshot=`"$output\mireling-host.png`"" } else { '' })
$clientCapture = "-ShaderWorkingDir=`"$clientShaderDir`""
$remoteCapture = "-ShaderWorkingDir=`"$remoteShaderDir`""
$server = $null; $client = $null; $remoteClient = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common $serverCapture -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do { if ($server.HasExited) { throw 'Mireling listen server exited during startup.' }; if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }; Start-Sleep -Milliseconds 500 } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Mireling listen server readiness timed out.' }
    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common $clientCapture -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $remoteClientLog = Join-Path $output 'remote-client.log'
    $remoteClient = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=1000 $common $remoteCapture -abslog=`"$remoteClientLog`" -UserDir=`"$output\RemoteClient`""
    $deadline = (Get-Date).AddSeconds(90); $ready = $false
    do {
        if ($server.HasExited -or $client.HasExited -or $remoteClient.HasExited) { throw 'A Mireling peer exited before verification.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }; $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }; $remoteClientText = if (Test-Path $remoteClientLog) { Get-Content $remoteClientLog -Raw } else { '' }
        $peerText = $clientText + $remoteClientText
        if (($serverText + $peerText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Mireling verification FAILED:|Combat verification FAILED:|Mireling verification server: Passed=0') { throw 'Mireling peer verification failed; inspect retained logs.' }
        $ready = $serverText -match 'Mireling verification server: Passed=1 SeedReproduced=1 BoundedActivation=1 (?:HerdAlert=0 )?InvalidRejected=1 ActionSerial=4 Health=0\.0 PlayerHealth=[0-9]+\.0 Defeated=1 Saved=1 Meat=0 Hide=0 BoneFragment=0 RemoteMeat=0 RemoteHide=0 RemoteBoneFragment=0 DeerMeat=0 DeerHide=0 DeerSinew=0 RemoteDeerMeat=0 RemoteDeerHide=0 RemoteDeerSinew=0'
        $ready = $ready -and $peerText -match 'Mireling verification client rejected invalid owned attack without target data\.'
        $ready = $ready -and $peerText -match 'Mireling verification client observed shared action serial=4\.'
        $ready = $ready -and $peerText -match 'Mireling verification client observed relevant wildlife defeat\.'
        $ready = $ready -and $peerText -match 'Mireling verification client observed replicated player health=[0-9]+\.0\.'
        if ($ready) { break }; Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if (!$ready) {
        Write-Output ("Mireling peer timeout detail: ServerPassed={0} InvalidRejected={1} SharedAction={2} Defeat={3} Health={4}" -f
            ($serverText -match 'Mireling verification server: Passed=1'),
            ($peerText -match 'Mireling verification client rejected invalid owned attack without target data\.'),
            ($peerText -match 'Mireling verification client observed shared action serial=4\.'),
            ($peerText -match 'Mireling verification client observed relevant wildlife defeat\.'),
            ($peerText -match 'Mireling verification client observed replicated player health=[0-9]+\.0\.'))
        throw 'Mireling peer scenario timed out.'
    }
    if (($clientText + $remoteClientText) -notmatch 'Client received world-generation identity: Seed=418') { throw 'Mireling peer client identity mismatch.' }
    $spawnMatch = [regex]::Match($serverText, 'Mireling verification server: Passed=1[^\r\n]* SpawnId=([0-9]+/-?\d+/-?\d+/\d+)')
    if (!$spawnMatch.Success) { throw 'Mireling peer result did not include the defeated spawn identity.' }
    $spawnId = $spawnMatch.Groups[1].Value
    $spawnIdParts = $spawnId.Split('/')
    $spawnKeyX = [int]$spawnIdParts[1]; $spawnKeyY = [int]$spawnIdParts[2]
    if ($Rendered) {
        $renderDeadline = (Get-Date).AddSeconds(20)
        do {
            $captureReady = Test-Path -LiteralPath (Join-Path $output 'mireling-host.png')
            if ($captureReady) { break }
            Start-Sleep -Milliseconds 250
        } while ((Get-Date) -lt $renderDeadline)
        if (!$captureReady) { throw 'Rendered Mireling capture was not produced after the test arranged the target.' }
        $capture = Join-Path $output 'mireling-host.png'
        if ((Get-Item -LiteralPath $capture).Length -lt 4096) { throw "Rendered Mireling capture is unexpectedly small: $capture" }
    }
    Stop-Process -Id $client.Id; Stop-Process -Id $remoteClient.Id; Stop-Process -Id $server.Id; $client = $null; $remoteClient = $null; $server = $null
    $restartLog = Join-Path $output 'restart.log'
    $restart = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 -game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaReconnectVerification=WildlifeVerify -KalmalaReconnectKeyX=$spawnKeyX -KalmalaReconnectKeyY=$spawnKeyY -KalmalaReconnectSpawnId=`"$spawnId`" -ShaderWorkingDir=`"$restartShaderDir`" -abslog=`"$restartLog`" -UserDir=`"$output\Host`""
    if (!$restart.WaitForExit(120000)) { Stop-Process -Id $restart.Id; throw 'Mireling world-state restart timed out.' }
    $restartText = Get-Content $restartLog -Raw
    if ($restartText -notmatch 'Reconnect verification passed: defeated generated wildlife spawn .* remained absent after listen-server restart\.') { throw 'Mireling defeat did not survive restart.' }
    if ($Rendered) {
        Write-Output "PASS: the host captured the positioned Mireling target in $output\mireling-host.png; bounded host/client combat, owner-only rewards, and same-world defeat persistence also passed."
    }
    else {
        Write-Output 'PASS: seed-reproduced bounded Mireling activation damaged a player server-side; the client saw only replicated combat/health/defeat state, its target-free attack was rejected, the reward stayed owner-only, and the defeat survived restart.'
    }
}
finally { foreach ($peer in @($client, $remoteClient, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }; Write-Output "Peer logs: $output" }
