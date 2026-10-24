param([int]$Port = 18185, [string]$OutputDirectory = '', [string]$Project = '', [long]$WorldSeed = 445)
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot

$ErrorActionPreference = 'Stop'
$project = if ($Project) { $Project } else { Join-Path $repoPaths.ProjectRoot 'Kalmala.uproject' }
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaMosquitoPeer-' + [guid]::NewGuid().ToString('N')) }
if ([IO.Path]::GetFullPath($output).Length -gt 180) { throw "Mosquito peer output root is too long for nested Windows paths: $output" }
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
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaMosquitoPeerTest'
$server = $null; $client = $null; $observer = $null
try {
    foreach ($shaderDir in @($hostShaderDir, $clientShaderDir, $observerShaderDir)) {
        New-Item -ItemType Directory -Path $shaderDir -Force | Out-Null
    }
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=$WorldSeed $common -ShaderWorkingDir=`"$hostShaderDir`" -UserDir=`"$hostDir`" -abslog=`"$serverLog`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited) { throw "Mosquito listen server exited during startup with code $($server.ExitCode)." }
        if ((Test-Path $serverLog) -and (Select-String -Path $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Mosquito listen server readiness timed out.' }

    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -ShaderWorkingDir=`"$clientShaderDir`" -UserDir=`"$clientDir`" -abslog=`"$clientLog`""
    $observer = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=1000 $common -ShaderWorkingDir=`"$observerShaderDir`" -UserDir=`"$observerDir`" -abslog=`"$observerLog`""
    $deadline = (Get-Date).AddSeconds(240)
    $passed = $false
    $serverPassedAt = $null
    do {
        if ($server.HasExited -or $client.HasExited -or $observer.HasExited) {
            $codes = @($server, $client, $observer) | ForEach-Object { if ($_.HasExited) { $_.ExitCode } else { 'running' } }
            throw "Mosquito peer exited before acceptance completed; exit codes: $($codes -join ',')."
        }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        $observerText = if (Test-Path $observerLog) { Get-Content $observerLog -Raw } else { '' }
        $allText = $serverText + $clientText + $observerText
        if ($allText -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Mosquito verification FAILED:') {
            throw 'Mosquito peer verification failed or Unreal terminated abnormally; inspect retained logs.'
        }

        $serverPassed = $serverText -match 'Mosquito verification server: Passed=1 Seed=\d+ Bites=\d+ Damage=\d+\.\d+ WeakPoisonSeconds=\d+\.\d+ OtherPeers=2 DodgeAccepted=1 DodgeRejected=1 RollExpired=1 HeldTorchRepelled=1 HeldTorchOffNoProtection=1 CampfireUnlitNoProtection=1 CampfireLitRepelled=1 SpawnId=\d+/-?\d+/-?\d+/\d+'
        if ($serverPassed -and $null -eq $serverPassedAt) { $serverPassedAt = Get-Date }
        $clientHealth = [regex]::Matches($clientText, 'Mosquito verification client observed replicated player health=(\d+(?:\.\d+)?)\.') |
            Where-Object { [double]$_.Groups[1].Value -lt 99.9 } | Select-Object -First 1
        $observerHealth = [regex]::Matches($observerText, 'Mosquito verification client observed replicated player health=(\d+(?:\.\d+)?)\.') |
            Where-Object { [double]$_.Groups[1].Value -lt 99.9 } | Select-Object -First 1
        $clientPoison = $clientText -match 'Mosquito verification client observed replicated weak poison remaining=\d+\.\d+'
        $observerPoison = $observerText -match 'Mosquito verification client observed replicated weak poison remaining=\d+\.\d+'
        $poisonOwnerPrivacyVerified = $false
        if ($null -ne $serverPassedAt)
        {
            $poisonOwnerPrivacyVerified = ((Get-Date) - $serverPassedAt).TotalSeconds -ge 2.0
        }
        $passed = $serverPassed -and $null -ne $clientHealth -and $null -ne $observerHealth
        $passed = $passed -and ($clientPoison -xor $observerPoison) -and $poisonOwnerPrivacyVerified
        if ($passed) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)

    if (!$passed) {
        Write-Output ("Mosquito peer timeout detail: ServerPassed={0} ClientHealth={1} ClientPoison={2} ObserverHealth={3} ObserverPoison={4}" -f
            $serverPassed, ($null -ne $clientHealth), $clientPoison, ($null -ne $observerHealth), $observerPoison)
        throw 'Mosquito swarm live peer acceptance timed out; inspect retained logs.'
    }
    Write-Output 'PASS: a generated Mossy Mire swarm replicated bites and owner-only poison, accepted one dodge, rejected its cooldown retry, and respected held-torch and placed-campfire states.'
}
finally {
    foreach ($peer in @($client, $observer, $server)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Peer logs: $output"
}
