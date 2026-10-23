param([int]$Port = 18209, [string]$OutputDirectory = '', [string]$Project = '', [long]$WorldSeed = 445)
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot

$ErrorActionPreference = 'Stop'
$project = if ($Project) { $Project } else { Join-Path $repoPaths.ProjectRoot 'Kalmala.uproject' }
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaReefMawPeer-' + [guid]::NewGuid().ToString('N')) }
if ([IO.Path]::GetFullPath($output).Length -gt 180) { throw "Reef Maw peer output root is too long for nested Windows paths: $output" }
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
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaReefMawPeerTest'
$server = $null; $client = $null; $observer = $null
try {
    foreach ($shaderDir in @($hostShaderDir, $clientShaderDir, $observerShaderDir)) {
        New-Item -ItemType Directory -Path $shaderDir -Force | Out-Null
    }
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=$WorldSeed $common -ShaderWorkingDir=`"$hostShaderDir`" -UserDir=`"$hostDir`" -abslog=`"$serverLog`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited) { throw "Reef Maw listen server exited during startup with code $($server.ExitCode)." }
        if ((Test-Path $serverLog) -and (Select-String -Path $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Reef Maw listen server readiness timed out.' }

    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -ShaderWorkingDir=`"$clientShaderDir`" -UserDir=`"$clientDir`" -abslog=`"$clientLog`""
    $joinDeadline = (Get-Date).AddSeconds(60)
    $firstRemoteJoined = $false
    do {
        if ($server.HasExited -or $client.HasExited) {
            throw 'Reef Maw listen server or first client exited before the first remote join.'
        }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $firstRemoteJoined = [regex]::Matches($serverText, 'LogNet: Join succeeded:').Count -ge 1
        if (!$firstRemoteJoined) { Start-Sleep -Milliseconds 500 }
    } while (!$firstRemoteJoined -and (Get-Date) -lt $joinDeadline)
    if (!$firstRemoteJoined) { throw 'Reef Maw first remote client did not join before observer launch.' }

    $observer = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=1000 $common -ShaderWorkingDir=`"$observerShaderDir`" -UserDir=`"$observerDir`" -abslog=`"$observerLog`""
    $deadline = (Get-Date).AddSeconds(240)
    $passed = $false
    do {
        if ($server.HasExited -or $client.HasExited -or $observer.HasExited) {
            $codes = @($server, $client, $observer) | ForEach-Object { if ($_.HasExited) { $_.ExitCode } else { 'running' } }
            throw "Reef Maw peer exited before acceptance completed; exit codes: $($codes -join ',')."
        }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        $observerText = if (Test-Path $observerLog) { Get-Content $observerLog -Raw } else { '' }
        $allText = $serverText + $clientText + $observerText
        if ($allText -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Reef Maw verification FAILED:') {
            throw 'Reef Maw peer verification failed or Unreal terminated abnormally; inspect retained logs.'
        }

        $fixtureMatch = [regex]::Match($serverText, 'Reef Maw verification fixture armed: Seed=\d+ SpawnId=(\S+) StartDistanceCm=(\d+(?:\.\d+)?) SpawnDepthCm=(\d+(?:\.\d+)?) HostDistanceCm=(\d+(?:\.\d+)?) Observers=2\.')
        $serverMatch = [regex]::Match($serverText, 'Reef Maw verification server: Passed=1 Seed=\d+ Telegraph=1 Bite=1 Recovery=1 HostDamage=(\d+(?:\.\d+)?) BiteDistanceCm=(\d+(?:\.\d+)?) SpawnId=(\S+)')
        $clientTelegraph = $fixtureMatch.Success -and $clientText -match [regex]::Escape("Reef Maw verification client observed replicated telegraph: SpawnId=$($fixtureMatch.Groups[1].Value).")
        $observerTelegraph = $fixtureMatch.Success -and $observerText -match [regex]::Escape("Reef Maw verification client observed replicated telegraph: SpawnId=$($fixtureMatch.Groups[1].Value).")
        $clientBite = $fixtureMatch.Success -and $clientText -match [regex]::Escape("Reef Maw verification client observed replicated bite: SpawnId=$($fixtureMatch.Groups[1].Value).")
        $observerBite = $fixtureMatch.Success -and $observerText -match [regex]::Escape("Reef Maw verification client observed replicated bite: SpawnId=$($fixtureMatch.Groups[1].Value).")
        $passed = $fixtureMatch.Success -and [double]$fixtureMatch.Groups[2].Value -ge 15000.0
        $passed = $passed -and [double]$fixtureMatch.Groups[3].Value -ge 150.0
        $passed = $passed -and [double]$fixtureMatch.Groups[4].Value -gt 180.0 -and [double]$fixtureMatch.Groups[4].Value -le 500.0
        $passed = $passed -and $serverMatch.Success -and [double]$serverMatch.Groups[1].Value -ge 49.0
        $passed = $passed -and [double]$serverMatch.Groups[1].Value -le 51.0
        $passed = $passed -and [double]$serverMatch.Groups[2].Value -gt 0.0 -and [double]$serverMatch.Groups[2].Value -le 180.0
        $passed = $passed -and $serverMatch.Groups[3].Value -eq $fixtureMatch.Groups[1].Value
        $passed = $passed -and $clientTelegraph -and $observerTelegraph -and $clientBite -and $observerBite
        if ($passed) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)

    if (!$passed) {
        Write-Output ("Reef Maw peer timeout detail: Fixture={0} ServerMatch={1} ClientTelegraph={2} ObserverTelegraph={3} ClientBite={4} ObserverBite={5}" -f
            $fixtureMatch.Success, $serverMatch.Success, $clientTelegraph, $observerTelegraph, $clientBite, $observerBite)
        throw 'Reef Maw live peer acceptance timed out; inspect retained logs.'
    }
    Write-Output 'PASS: a generated deep-Ocean Reef Maw bit a swimming host for 50 Stab, recovered, and replicated its telegraph and bite to both clients.'
}
finally {
    foreach ($peer in @($client, $observer, $server)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Peer logs: $output"
}
