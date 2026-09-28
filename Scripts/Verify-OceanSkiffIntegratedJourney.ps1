param(
    [int]$Port = 18172,
    [string]$OutputDirectory = '',
    [string]$Project = '',
    [int]$TimeoutSeconds = 600
)

$ErrorActionPreference = 'Stop'
$projectPath = if ($Project) { $Project } else { Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject' }
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaOceanSkiffIntegratedJourney-' + [guid]::NewGuid().ToString('N')) }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$hostLog = Join-Path $output 'host.log'
$clientLog = Join-Path $output 'client.log'
$hostShaderDir = Join-Path $output 'Host\ShaderWorkingDir'
$clientShaderDir = Join-Path $output 'Client\ShaderWorkingDir'
New-Item -ItemType Directory -Path $hostShaderDir -Force | Out-Null
New-Item -ItemType Directory -Path $clientShaderDir -Force | Out-Null
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaOceanDiscoveryDisembarkPeerTest -KalmalaOceanIntegratedJourneyPeerTest -KalmalaOceanWeatherPeerTest'
$hostProcess = $null
$client = $null
try {
    $hostProcess = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -ShaderWorkingDir=`"$hostShaderDir`" -abslog=`"$hostLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($hostProcess.HasExited) { throw 'Integrated journey listen server exited during startup.' }
        if ((Test-Path -LiteralPath $hostLog) -and (Select-String -LiteralPath $hostLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Integrated journey listen server readiness timed out.' }

    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" 127.0.0.1:$Port -WorldSeed=999 $common -ShaderWorkingDir=`"$clientShaderDir`" -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $ready = $false
    do {
        if ($hostProcess.HasExited -or $client.HasExited) { throw 'An integrated journey peer exited before verification completed.' }
        $hostText = if (Test-Path -LiteralPath $hostLog) { Get-Content -LiteralPath $hostLog -Raw } else { '' }
        $clientText = if (Test-Path -LiteralPath $clientLog) { Get-Content -LiteralPath $clientLog -Raw } else { '' }
        if (($hostText + $clientText) -match 'Ocean integrated journey .*FAILED:|Ocean discovery-stop peer verification FAILED:|Fatal error:|Assertion failed:|Ensure condition failed:') {
            throw 'Integrated ocean journey verification failed; inspect retained logs.'
        }

        $serverJourney = $hostText -match 'Ocean integrated journey server passed: Seed=418 Players=2 Discovery=.+ Reward=(?:Wood|Fibre|Stone):[12] Claims=2 Distance=2[3-4][0-9]{4} Crosswind=1 Calm=1 PatchStart=\((-?[0-9]+),(-?[0-9]+)\) PatchEnd=\((-?[0-9]+),(-?[0-9]+)\) ActivePatches=(?:[1-9]|1[0-9]|2[0-5]) DryShoreHost=1 DryShoreHelm=1 Mode=Moored Disembarked=2'
        $clientJourney = $clientText -match 'Ocean integrated journey peer replica passed: Authority=0 Seat=Helm Discovery=.+ DiscoveryFeedback=LandmarkFound Reward=(?:Wood|Fibre|Stone):[12] Distance=2[3-4][0-9]{4} Crosswind=1 Calm=1 DryShore=1 Disembarked=1 EmptySeats=1 Mode=Moored'
        $clientWorld = $clientText -match 'Client received world-generation identity: Seed=418'
        $hostWeather = $hostText -match 'Ocean weather peer result: Authority=1 Cycle=7001 Direction=90 Strength=1\.000 .*Passed=1' -and $hostText -match 'Ocean weather peer result: Authority=1 Cycle=7002 Direction=0 Strength=0\.000 .*Passed=1'
        $clientWeather = $clientText -match 'Ocean weather peer result: Authority=0 Cycle=7001 Direction=90 Strength=1\.000 .*ClientForgeryRejected=1 Passed=1' -and $clientText -match 'Ocean weather peer result: Authority=0 Cycle=7002 Direction=0 Strength=0\.000 .*ClientForgeryRejected=1 Passed=1'
        $ready = $serverJourney -and $clientJourney -and $clientWorld -and $hostWeather -and $clientWeather
        if ($ready) { break }
        Start-Sleep -Seconds 1
    } while ((Get-Date) -lt $deadline)

    if (!$ready) { throw "Integrated ocean journey did not satisfy the two-peer checks within $TimeoutSeconds seconds; inspect retained logs." }
    Write-Output 'PASS: both peers observed the accepted weather states, discovery reward, 2.4 km skiff route, patch transition, and dry-shore disembark.'
}
finally {
    foreach ($peer in @($client, $hostProcess)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Integrated journey logs: $output"
}
