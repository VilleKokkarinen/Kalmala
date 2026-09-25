param(
    [int]$Port = 18170,
    [string]$OutputDirectory = '',
    [string]$Project = '',
    [int]$TimeoutSeconds = 120
)

$ErrorActionPreference = 'Stop'
$projectPath = if ($Project) { $Project } else { Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject' }
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaOceanSkiffDiscoveryDisembark-' + [guid]::NewGuid().ToString('N')) }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$hostLog = Join-Path $output 'host.log'
$clientLog = Join-Path $output 'client.log'
$hostShaderDir = Join-Path $output 'Host\ShaderWorkingDir'
$clientShaderDir = Join-Path $output 'Client\ShaderWorkingDir'
New-Item -ItemType Directory -Path $hostShaderDir -Force | Out-Null
New-Item -ItemType Directory -Path $clientShaderDir -Force | Out-Null
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaOceanDiscoveryDisembarkPeerTest'
$hostProcess = $null
$client = $null
try {
    $hostProcess = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -ShaderWorkingDir=`"$hostShaderDir`" -abslog=`"$hostLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($hostProcess.HasExited) { throw 'Ocean discovery/disembark listen server exited during startup.' }
        if ((Test-Path -LiteralPath $hostLog) -and (Select-String -LiteralPath $hostLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Ocean discovery/disembark listen server readiness timed out.' }

    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" 127.0.0.1:$Port -WorldSeed=999 $common -ShaderWorkingDir=`"$clientShaderDir`" -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $ready = $false
    do {
        if ($hostProcess.HasExited -or $client.HasExited) { throw 'An ocean discovery/disembark peer exited before verification completed.' }
        $hostText = if (Test-Path -LiteralPath $hostLog) { Get-Content -LiteralPath $hostLog -Raw } else { '' }
        $clientText = if (Test-Path -LiteralPath $clientLog) { Get-Content -LiteralPath $clientLog -Raw } else { '' }
        if (($hostText + $clientText) -match 'Ocean discovery-stop peer verification FAILED:|Fatal error:|Assertion failed:|Ensure condition failed:') {
            throw 'Ocean discovery/disembark peer verification failed; inspect retained logs.'
        }

        $serverAccepted = $hostText -match 'Ocean discovery-stop server passed: Seed=418 Players=2 Discovery=.+ Reward=(?:Wood|Fibre|Stone):[12] Claims=2 Mode=Moored Disembarked=2'
        $clientAccepted = $clientText -match 'Ocean discovery-stop peer replica passed: Authority=0 Seat=Helm Discovery=.+ DiscoveryFeedback=LandmarkFound Reward=(?:Wood|Fibre|Stone):[12] Disembarked=1 EmptySeats=1 Mode=Moored'
        $clientWorld = $clientText -match 'Client received world-generation identity: Seed=418'
        $ready = $serverAccepted -and $clientAccepted -and $clientWorld
        if ($ready) { break }
        Start-Sleep -Seconds 1
    } while ((Get-Date) -lt $deadline)

    if (!$ready) { throw "Ocean discovery/disembark peer verification timed out after $TimeoutSeconds seconds; inspect retained logs." }
    Write-Output 'PASS: both peers retained one server-selected sea-discovery reward and replicated the safely moored, empty skiff after disembark.'
}
finally {
    foreach ($peer in @($client, $hostProcess)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Discovery/disembark logs: $output"
}
