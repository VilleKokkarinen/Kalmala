param([int]$Port = 18166, [string]$OutputDirectory = '', [string]$Project = '')
$ErrorActionPreference = 'Stop'
$projectPath = if ($Project) { $Project } else { Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject' }
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaOceanSkiffFeedback-' + [guid]::NewGuid().ToString('N')) }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$hostLog = Join-Path $output 'host.log'
$clientLog = Join-Path $output 'client.log'
$hostShaderDir = Join-Path $output 'Host\ShaderWorkingDir'
$clientShaderDir = Join-Path $output 'Client\ShaderWorkingDir'
New-Item -ItemType Directory -Path $hostShaderDir -Force | Out-Null
New-Item -ItemType Directory -Path $clientShaderDir -Force | Out-Null
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaOceanSkiffFeedbackTest'
$hostProcess = $null
$client = $null
try {
    $hostProcess = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -ShaderWorkingDir=`"$hostShaderDir`" -abslog=`"$hostLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($hostProcess.HasExited) { throw 'Ocean skiff feedback listen server exited during startup.' }
        if ((Test-Path -LiteralPath $hostLog) -and (Select-String -LiteralPath $hostLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Ocean skiff feedback listen server readiness timed out.' }

    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" 127.0.0.1:$Port -WorldSeed=999 $common -ShaderWorkingDir=`"$clientShaderDir`" -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(90)
    $ready = $false
    do {
        if ($hostProcess.HasExited -or $client.HasExited) { throw 'An ocean skiff feedback peer exited before verification.' }
        $hostText = if (Test-Path -LiteralPath $hostLog) { Get-Content -LiteralPath $hostLog -Raw } else { '' }
        $clientText = if (Test-Path -LiteralPath $clientLog) { Get-Content -LiteralPath $clientLog -Raw } else { '' }
        if (($hostText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Ocean skiff feedback verification FAILED:|Passed=0|received another player') {
            throw 'Ocean skiff feedback peer verification failed; inspect retained logs.'
        }
        $ready = $hostText -match 'Ocean skiff feedback verification server: Passed=1 Host=ShallowLaunch Remote=SeatsFull HostSerial=1 RemoteSerial=1'
        $ready = $ready -and $hostText -match 'Ocean skiff feedback UI local: .*Serial=1 Message=.*SHALLOW WATER.*Wade or swim farther out'
        $ready = $ready -and $clientText -match 'Ocean skiff feedback UI local: .*Serial=1 Message=.*SEATS FULL.*Both skiff seats are occupied'
        $ready = $ready -and $clientText -match 'Ocean skiff feedback verification client retained no other-owner feedback\.'
        if ($ready) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)

    if (!$ready) { throw 'Ocean skiff feedback host/client scenario timed out.' }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Ocean skiff feedback client did not adopt the server world identity.' }
    Write-Output 'PASS: host and client displayed distinct server-selected coast/seat messages from their own owner-only state; the client copy of the host pawn retained no feedback.'
}
finally {
    foreach ($peer in @($client, $hostProcess)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Peer logs: $output"
}
