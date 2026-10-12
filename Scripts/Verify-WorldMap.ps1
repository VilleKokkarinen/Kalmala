param([int]$BasePort = 17844, [switch]$Overview, [switch]$SingleResolution)

$ErrorActionPreference = 'Stop'
$project = Join-Path $PSScriptRoot '..\Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path ([System.IO.Path]::GetTempPath()) ('KalmalaWorldMap-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$coverageCaseOutput = $null
$coveragePort = $null
$coverageCommon = $null

for ($index = 0; $index -lt $(if ($SingleResolution) { 1 } else { 3 }); $index++) {
    $resolution = @(@(1024, 768), @(1280, 720), @(2560, 1080))[$index]
    $label = "$($resolution[0])x$($resolution[1])"
    $caseOutput = Join-Path $output $label
    New-Item -ItemType Directory -Path $caseOutput | Out-Null
    $serverLog = Join-Path $caseOutput 'host.log'
    $clientLog = Join-Path $caseOutput 'client.log'
    $serverShot = Join-Path $caseOutput 'host.png'
    $clientShot = Join-Path $caseOutput 'client.png'
    $common = "-game -windowed -RenderOffscreen -ForceRes -ResX=$($resolution[0]) -ResY=$($resolution[1]) -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaWorldMapVerification -KalmalaWorldMapProfile"
    if ($Overview) { $common += " -KalmalaWorldOverviewVerification" }
    if ($index -eq 0) {
        $coverageCaseOutput = $caseOutput
        $coveragePort = $BasePort
        $coverageCommon = $common
    }
    $server = $null
    $client = $null
    try {
        $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$($BasePort + $index) -WorldSeed=418 $common -KalmalaWorldMapScreenshot=`"$serverShot`" -abslog=`"$serverLog`" -UserDir=`"$caseOutput\Host`""
        $deadline = (Get-Date).AddSeconds(60)
        do {
            if ($server.HasExited) { throw "Host exited before accepting connections at $label." }
            if ((Test-Path $serverLog) -and (Select-String -Path $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
            Start-Sleep -Milliseconds 500
        } while ((Get-Date) -lt $deadline)
        if ((Get-Date) -ge $deadline) { throw "Host readiness timed out at $label." }
        $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$($BasePort + $index) -WorldSeed=999 $common -KalmalaWorldMapScreenshot=`"$clientShot`" -abslog=`"$clientLog`" -UserDir=`"$caseOutput\Client`""
        $deadline = (Get-Date).AddSeconds(240)
        do {
            if ($server.HasExited -or $client.HasExited) { throw "A rendered peer exited at $label." }
            $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
            $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
            $serverTileMatch = [regex]::Match($serverText, 'World map tile presentation: Seed=418 Tiles=([1-9][0-9]*) Fingerprint=([0-9]+) PollOnly=1\.')
            $clientTileMatch = [regex]::Match($clientText, 'World map tile presentation: Seed=418 Tiles=([1-9][0-9]*) Fingerprint=([0-9]+) PollOnly=1\.')
            if ($serverTileMatch.Success -and $clientTileMatch.Success -and
                ($serverTileMatch.Groups[1].Value -ne $clientTileMatch.Groups[1].Value -or
                    $serverTileMatch.Groups[2].Value -ne $clientTileMatch.Groups[2].Value)) {
                throw "Host/client tile count or presentation fingerprint disagrees at $label."
            }
            if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:') { throw "Unreal reported an error at $label." }
            if ($serverText -match 'World map verification: Open=1 Input=1 ZoomMin=1 ZoomMax=1 Pan=1 Recenter=1.' -and
                $serverText -match 'World map marker filters: PointerTargets=1 Keyboard=1 Controller=1 PinsPreserved=1.' -and
                $clientText -match 'World map marker filters: PointerTargets=1 Keyboard=1 Controller=1 PinsPreserved=1.' -and
                $serverText -match 'World map gameplay exploration: Closed=1 Cells=[1-9][0-9]* Tiles=0' -and
                $clientText -match 'World map gameplay exploration: Closed=1 Cells=[1-9][0-9]* Tiles=0' -and
                $serverText -match 'World map fog presentation: Seed=418 Cells=[1-9][0-9]* Loaded=0 Remote=0\.' -and
                $clientText -match 'World map fog presentation: Seed=418 Cells=[1-9][0-9]* Loaded=0 Remote=0\.' -and
                $clientText -match "Client received world-generation identity: Seed=418" -and
                $clientText -match 'World map verification: Open=1 Input=1 ZoomMin=1 ZoomMax=1 Pan=1 Recenter=1.' -and
                $serverText -match 'World map paint verification: FullViewport=1 .*ReadyTiles=[1-9][0-9]* Fog=1' -and
                $clientText -match 'World map paint verification: FullViewport=1 .*ReadyTiles=[1-9][0-9]* Fog=1' -and
                $serverTileMatch.Success -and $clientTileMatch.Success -and
                $serverText -match 'World map profile: OpenMs=.*WorkerTotalMs=.*GameThreadTotalMs=.*Tiles=[1-9][0-9]* CacheBytes=[0-9]+\.' -and
                $clientText -match 'World map profile: OpenMs=.*WorkerTotalMs=.*GameThreadTotalMs=.*Tiles=[1-9][0-9]* CacheBytes=[0-9]+\.' -and
                (Test-Path $serverShot) -and (Test-Path $clientShot) -and
                (!$Overview -or ($serverText -match 'World overview: Fits=1 Reveal=1' -and $clientText -match 'World overview: Fits=1 Reveal=1'))) { break }
            Start-Sleep -Milliseconds 500
        } while ((Get-Date) -lt $deadline)
        if ((Get-Date) -ge $deadline) { throw "Rendered host/client world-map verification timed out at $label." }
        Write-Output "PASS: $label host/client map input, identity, matching tile fingerprint, private fog, full-viewport painting, profile metrics, and screenshots."
    }
    finally {
        foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    }
}

if ($null -eq $coverageCaseOutput -or $null -eq $coveragePort -or $null -eq $coverageCommon) { throw 'No rendered case is available for saved-coverage verification.' }
$restartServerLog = Join-Path $coverageCaseOutput 'host-restart.log'
$restartClientLog = Join-Path $coverageCaseOutput 'client-restart.log'
$hostUserDir = Join-Path $coverageCaseOutput 'Host'
$clientUserDir = Join-Path $coverageCaseOutput 'Client'
$server = $null
$client = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$coveragePort -WorldSeed=418 $coverageCommon -abslog=`"$restartServerLog`" -UserDir=`"$hostUserDir`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited) { throw 'Restarted host exited before accepting connections.' }
        if ((Test-Path $restartServerLog) -and (Select-String -Path $restartServerLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Restarted host readiness timed out.' }
    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$coveragePort -WorldSeed=999 $coverageCommon -abslog=`"$restartClientLog`" -UserDir=`"$clientUserDir`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A restarted peer exited before personal-coverage verification.' }
        $restartServerText = if (Test-Path $restartServerLog) { Get-Content $restartServerLog -Raw } else { '' }
        $restartClientText = if (Test-Path $restartClientLog) { Get-Content $restartClientLog -Raw } else { '' }
        if (($restartServerText + $restartClientText) -match 'Fatal error:|Assertion failed:') { throw 'A restarted Unreal peer reported a fatal error.' }
        if ($restartServerText -match 'World map fog presentation: Seed=418 Cells=[0-9]+ Loaded=1 Remote=0\.' -and
            $restartClientText -match 'Client received world-generation identity: Seed=418' -and
            $restartClientText -match 'World map fog presentation: Seed=418 Cells=[0-9]+ Loaded=1 Remote=0\.') { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Restarted host/client personal-coverage verification timed out.' }
    Write-Output 'PASS: personal coverage reloaded for both peers after restart without revealing remote terrain.'
}
finally {
    foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
}
Write-Output "Scenario logs and screenshots: $output"
