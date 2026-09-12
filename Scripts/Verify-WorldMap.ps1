param([int]$BasePort = 17844, [switch]$Overview, [switch]$SingleResolution)

$ErrorActionPreference = 'Stop'
$project = Join-Path $PSScriptRoot '..\Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path ([System.IO.Path]::GetTempPath()) ('KalmalaWorldMap-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null

for ($index = 0; $index -lt $(if ($SingleResolution) { 1 } else { 3 }); $index++) {
    $resolution = @(@(1024, 768), @(1280, 720), @(2560, 1080))[$index]
    $label = "$($resolution[0])x$($resolution[1])"
    $caseOutput = Join-Path $output $label
    New-Item -ItemType Directory -Path $caseOutput | Out-Null
    $serverLog = Join-Path $caseOutput 'host.log'
    $clientLog = Join-Path $caseOutput 'client.log'
    $serverShot = Join-Path $caseOutput 'host.png'
    $clientShot = Join-Path $caseOutput 'client.png'
    $common = "-game -windowed -RenderOffscreen -ForceRes -ResX=$($resolution[0]) -ResY=$($resolution[1]) -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaWorldMapVerification"
    if ($Overview) { $common += " -KalmalaWorldOverviewVerification" }
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
            if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:') { throw "Unreal reported an error at $label." }
            if ($serverText -match 'World map verification: Open=1 Input=1 ZoomMin=1 ZoomMax=1 Pan=1 Recenter=1.' -and
                $serverText -match 'World map gameplay exploration: Closed=1 Cells=[1-9][0-9]* Tiles=0' -and
                $clientText -match 'World map gameplay exploration: Closed=1 Cells=[1-9][0-9]* Tiles=0' -and
                $clientText -match "Client received world-generation identity: Seed=418" -and
                $clientText -match 'World map verification: Open=1 Input=1 ZoomMin=1 ZoomMax=1 Pan=1 Recenter=1.' -and
                $serverText -match 'World map paint verification: FullViewport=1 .*ReadyTiles=[1-9][0-9]* Fog=1' -and
                $clientText -match 'World map paint verification: FullViewport=1 .*ReadyTiles=[1-9][0-9]* Fog=1' -and
                (Test-Path $serverShot) -and (Test-Path $clientShot) -and
                (!$Overview -or ($serverText -match 'World overview: Fits=1 Reveal=1' -and $clientText -match 'World overview: Fits=1 Reveal=1'))) { break }
            Start-Sleep -Milliseconds 500
        } while ((Get-Date) -lt $deadline)
        if ((Get-Date) -ge $deadline) { throw "Rendered host/client world-map verification timed out at $label." }
        Write-Output "PASS: $label host/client map input, authoritative identity, full-viewport terrain/fog painting, and screenshots."
    }
    finally {
        foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    }
}
Write-Output "Scenario logs and screenshots: $output"
