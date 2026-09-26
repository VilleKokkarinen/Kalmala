param(
    [int]$Port = 18169,
    [string]$OutputDirectory = '',
    [string]$Project = '',
    [int]$TimeoutSeconds = 660
)

$ErrorActionPreference = 'Stop'
$projectPath = if ($Project) { $Project } else { Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject' }
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$baseOutput = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP 'KalmalaOceanSkiffOrigin' }
$output = Join-Path $baseOutput ('Run-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output -Force | Out-Null
$hostLog = Join-Path $output 'host.log'
$clientLog = Join-Path $output 'client.log'
$hostShaderDir = Join-Path $output 'Host\ShaderWorkingDir'
$clientShaderDir = Join-Path $output 'Client\ShaderWorkingDir'
New-Item -ItemType Directory -Path $hostShaderDir -Force | Out-Null
New-Item -ItemType Directory -Path $clientShaderDir -Force | Out-Null
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaOceanJourneyPeerTest -KalmalaOceanWeatherPeerTest'
$hostProcess = $null
$client = $null
try {
    $hostProcess = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -ShaderWorkingDir=`"$hostShaderDir`" -abslog=`"$hostLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($hostProcess.HasExited) { throw 'Origin-stability listen server exited during startup.' }
        if ((Test-Path -LiteralPath $hostLog) -and (Select-String -LiteralPath $hostLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Origin-stability listen server readiness timed out.' }

    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" 127.0.0.1:$Port -WorldSeed=999 $common -ShaderWorkingDir=`"$clientShaderDir`" -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $ready = $false
    do {
        if ($hostProcess.HasExited -or $client.HasExited) { throw 'An origin-stability peer exited before verification completed.' }
        $hostText = if (Test-Path -LiteralPath $hostLog) { Get-Content -LiteralPath $hostLog -Raw } else { '' }
        $clientText = if (Test-Path -LiteralPath $clientLog) { Get-Content -LiteralPath $clientLog -Raw } else { '' }
        if (($hostText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Ocean journey peer verification FAILED:|Ocean origin stability verification FAILED:|Passed=0') {
            throw 'Origin-stability peer verification failed; inspect retained logs.'
        }

        $hostJourney = $hostText -match 'Ocean journey peer verification passed: Seed=418 Seats=Helm,Passenger Distance=2[3-9][0-9]{4} .* ActivePatches=(?:[1-9]|1[0-9]|2[0-5]) Crosswind=1 Calm=1 OriginShift=inactive'
        $hostReplica = $hostText -match 'Ocean journey peer replica observed travel: Authority=1 Seat=Passenger Mode=Underway Distance=1[0-9]{5,} Seed=418'
        $clientAttach = $clientText -match 'Ocean journey peer replica attached: Authority=0 Seat=Helm Mode=[0-2] Seed=418'
        $clientTravel = $clientText -match 'Ocean journey peer replica observed travel: Authority=0 Seat=Helm Mode=Underway Distance=1[0-9]{5,} Seed=418'
        $clientStop = $clientText -match 'Ocean journey peer replica observed stop: Authority=0 Seat=Helm Mode=Moored Distance=2[3-9][0-9]{4} Seed=418'
        $hostCrosswind = $hostText -match 'Ocean weather peer result: Authority=1 Cycle=7001 Direction=90 Strength=1\.000 .*Passed=1'
        $clientCrosswind = $clientText -match 'Ocean weather peer result: Authority=0 Cycle=7001 Direction=90 Strength=1\.000 .*ClientForgeryRejected=1 Passed=1'
        $hostCalm = $hostText -match 'Ocean weather peer result: Authority=1 Cycle=7002 Direction=0 Strength=0\.000 .*Passed=1'
        $clientCalm = $clientText -match 'Ocean weather peer result: Authority=0 Cycle=7002 Direction=0 Strength=0\.000 .*ClientForgeryRejected=1 Passed=1'
        $hostOrigin = $hostText -match 'Ocean origin stability verification passed: Peer=ListenServer Seat=Passenger Distance=2[3-9][0-9]{4} StartOrigin=\(0,0,0\) EndOrigin=\(0,0,0\) OriginStable=1'
        $clientOrigin = $clientText -match 'Ocean origin stability verification passed: Peer=RemoteClient Seat=Helm Distance=2[3-9][0-9]{4} StartOrigin=\(0,0,0\) EndOrigin=\(0,0,0\) OriginStable=1'
        $ready = $hostJourney -and $hostReplica -and $clientAttach -and $clientTravel -and $clientStop -and $hostCrosswind -and $clientCrosswind -and $hostCalm -and $clientCalm -and $hostOrigin -and $clientOrigin
        if ($ready) { break }
        Start-Sleep -Seconds 1
    } while ((Get-Date) -lt $deadline)

    if (!$ready) { throw "Origin-stability peer verification timed out after $TimeoutSeconds seconds; inspect retained logs." }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418') {
        throw 'Origin-stability client did not adopt the server world identity.'
    }
    Write-Output 'PASS: host and client retained origin (0,0,0) through the server-authoritative 2.4 km streamed voyage.'
}
finally {
    foreach ($peer in @($client, $hostProcess)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Origin-stability logs: $output"
}
