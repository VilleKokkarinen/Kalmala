param([int]$Port = 18167, [string]$OutputDirectory = '', [string]$Project = '')
$ErrorActionPreference = 'Stop'
$projectPath = if ($Project) { $Project } else { Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject' }
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaOceanSkiffWeather-' + [guid]::NewGuid().ToString('N')) }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$hostLog = Join-Path $output 'host.log'
$clientLog = Join-Path $output 'client.log'
$hostShaderDir = Join-Path $output 'Host\ShaderWorkingDir'
$clientShaderDir = Join-Path $output 'Client\ShaderWorkingDir'
New-Item -ItemType Directory -Path $hostShaderDir -Force | Out-Null
New-Item -ItemType Directory -Path $clientShaderDir -Force | Out-Null
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaOceanWeatherPeerTest'
$hostProcess = $null
$client = $null
try {
    $hostProcess = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -ShaderWorkingDir=`"$hostShaderDir`" -abslog=`"$hostLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($hostProcess.HasExited) { throw 'Ocean skiff weather listen server exited during startup.' }
        if ((Test-Path -LiteralPath $hostLog) -and (Select-String -LiteralPath $hostLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Ocean skiff weather listen server readiness timed out.' }

    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" 127.0.0.1:$Port -WorldSeed=999 $common -ShaderWorkingDir=`"$clientShaderDir`" -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(90)
    $ready = $false
    do {
        if ($hostProcess.HasExited -or $client.HasExited) { throw 'An ocean skiff weather peer exited before verification.' }
        $hostText = if (Test-Path -LiteralPath $hostLog) { Get-Content -LiteralPath $hostLog -Raw } else { '' }
        $clientText = if (Test-Path -LiteralPath $clientLog) { Get-Content -LiteralPath $clientLog -Raw } else { '' }
        if (($hostText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Ocean weather peer verification FAILED:|Passed=0') {
            throw 'Ocean skiff weather peer verification failed; inspect retained logs.'
        }
        $hostCrosswind = $hostText -match 'Ocean weather peer result: Authority=1 Cycle=7001 Direction=90 Strength=1\.000 WindRate=4\.000 CounterRate=-3\.000 ClientForgeryRejected=0 Passed=1'
        $hostCalm = $hostText -match 'Ocean weather peer result: Authority=1 Cycle=7002 Direction=0 Strength=0\.000 WindRate=0\.000 .*ClientForgeryRejected=0 Passed=1'
        $clientCrosswind = $clientText -match 'Ocean weather peer result: Authority=0 Cycle=7001 Direction=90 Strength=1\.000 WindRate=4\.000 CounterRate=-3\.000 ClientForgeryRejected=1 Passed=1'
        $clientCalm = $clientText -match 'Ocean weather peer result: Authority=0 Cycle=7002 Direction=0 Strength=0\.000 WindRate=0\.000 .*ClientForgeryRejected=1 Passed=1'
        $serverComplete = $hostText -match 'Ocean weather peer verification server states complete\.'
        $ready = $hostCrosswind -and $hostCalm -and $clientCrosswind -and $clientCalm -and $serverComplete
        if ($ready) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)

    if (!$ready) { throw 'Ocean skiff weather host/client agreement timed out.' }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Ocean skiff weather client did not adopt the server world identity.' }
    if ($clientText -match 'Client received weather cycle 7003') { throw 'Client-forged weather state was replicated back to the client.' }
    Write-Output 'PASS: host and client derived matching crosswind pressure and calm recovery from server weather; the client could not replace it with a forged calm state.'
}
finally {
    foreach ($peer in @($client, $hostProcess)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Peer logs: $output"
}
