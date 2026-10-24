param([int]$Port = 18204, [string]$OutputDirectory = '', [string]$Project = '', [long]$WorldSeed = 445)
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot

$ErrorActionPreference = 'Stop'
$project = if ($Project) { $Project } else { Join-Path $repoPaths.ProjectRoot 'Kalmala.uproject' }
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaPolarBearPeer-' + [guid]::NewGuid().ToString('N')) }
if ([IO.Path]::GetFullPath($output).Length -gt 180) { throw "Polar Bear peer output root is too long for nested Windows paths: $output" }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$hostDir = Join-Path $output 'Host'
$clientDir = Join-Path $output 'Client'
$observerDir = Join-Path $output 'Observer'
$hostShaderDir = Join-Path $output 'Host\ShaderWorkingDir'
$clientShaderDir = Join-Path $output 'Client\ShaderWorkingDir'
$observerShaderDir = Join-Path $output 'Observer\ShaderWorkingDir'
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$observerLog = Join-Path $output 'observer.log'
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaPolarBearPeerTest'
$server = $null; $client = $null; $observer = $null
try {
    foreach ($shaderDir in @($hostShaderDir, $clientShaderDir, $observerShaderDir)) {
        New-Item -ItemType Directory -Path $shaderDir -Force | Out-Null
    }
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=$WorldSeed $common -ShaderWorkingDir=`"$hostShaderDir`" -UserDir=`"$hostDir`" -abslog=`"$serverLog`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited) { throw "Polar Bear listen server exited during startup with code $($server.ExitCode)." }
        if ((Test-Path $serverLog) -and (Select-String -Path $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Polar Bear listen server readiness timed out.' }

    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -ShaderWorkingDir=`"$clientShaderDir`" -UserDir=`"$clientDir`" -abslog=`"$clientLog`""
    $joinDeadline = (Get-Date).AddSeconds(60)
    $firstRemoteJoined = $false
    do {
        if ($server.HasExited -or $client.HasExited) {
            throw 'Polar Bear listen server or first client exited before the first remote join.'
        }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $firstRemoteJoined = [regex]::Matches($serverText, 'LogNet: Join succeeded:').Count -ge 1
        if (!$firstRemoteJoined) { Start-Sleep -Milliseconds 500 }
    } while (!$firstRemoteJoined -and (Get-Date) -lt $joinDeadline)
    if (!$firstRemoteJoined) { throw 'Polar Bear first remote client did not join before observer launch.' }

    $observer = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=1000 $common -ShaderWorkingDir=`"$observerShaderDir`" -UserDir=`"$observerDir`" -abslog=`"$observerLog`""
    $deadline = (Get-Date).AddSeconds(240)
    $passed = $false
    do {
        if ($server.HasExited -or $client.HasExited -or $observer.HasExited) {
            $codes = @($server, $client, $observer) | ForEach-Object { if ($_.HasExited) { $_.ExitCode } else { 'running' } }
            throw "Polar Bear peer exited before acceptance completed; exit codes: $($codes -join ',')."
        }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        $observerText = if (Test-Path $observerLog) { Get-Content $observerLog -Raw } else { '' }
        $allText = $serverText + $clientText + $observerText
        if ($allText -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Polar Bear verification FAILED:') {
            throw 'Polar Bear peer verification failed or Unreal terminated abnormally; inspect retained logs.'
        }

        $fixtureMatch = [regex]::Match($serverText, 'Polar Bear verification fixture armed: Seed=\d+ SpawnId=(\S+) StartDistanceCm=(\d+(?:\.\d+)?) HostDistanceCm=(\d+(?:\.\d+)?) Observers=2\.')
        $serverMatch = [regex]::Match($serverText, 'Polar Bear verification server: Passed=1 Seed=\d+ Telegraph=1 Charge=1 Retreat=1 Recovery=1 HostDamage=(\d+(?:\.\d+)?) SpawnId=(\S+)')
        $clientCue = $fixtureMatch.Success -and $clientText -match [regex]::Escape("Polar Bear verification client observed replicated telegraph: SpawnId=$($fixtureMatch.Groups[1].Value).")
        $observerCue = $fixtureMatch.Success -and $observerText -match [regex]::Escape("Polar Bear verification client observed replicated telegraph: SpawnId=$($fixtureMatch.Groups[1].Value).")
        $passed = $serverMatch.Success -and [double]$serverMatch.Groups[1].Value -gt 0.0
        $passed = $passed -and $fixtureMatch.Success -and [double]$fixtureMatch.Groups[2].Value -ge 15000.0 -and [double]$fixtureMatch.Groups[3].Value -le 220.0
        $passed = $passed -and $serverMatch.Groups[2].Value -eq $fixtureMatch.Groups[1].Value -and $clientCue -and $observerCue
        if ($passed) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)

    if (!$passed) {
        Write-Output ("Polar Bear peer timeout detail: Fixture={0} ServerMatch={1} ClientCue={2} ObserverCue={3}" -f
            $fixtureMatch.Success, $serverMatch.Success, $clientCue, $observerCue)
        throw 'Polar Bear live peer acceptance timed out; inspect retained logs.'
    }
    Write-Output 'PASS: a generated 750-health Tundra Polar Bear passed the 150 m start exclusion, replicated its telegraph to both clients, and completed an accepted charge, retreat, and recovery.'
}
finally {
    foreach ($peer in @($client, $observer, $server)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Peer logs: $output"
}
