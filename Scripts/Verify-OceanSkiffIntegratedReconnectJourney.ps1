param(
    [int]$Port = 18176,
    [string]$OutputDirectory = '',
    [string]$Project = '',
    [int]$TimeoutSeconds = 900
)

$ErrorActionPreference = 'Stop'
$projectPath = if ($Project) { $Project } else { Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject' }
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaOceanSkiffIntegratedReconnectJourney-' + [guid]::NewGuid().ToString('N')) }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$processes = [System.Collections.Generic.List[System.Diagnostics.Process]]::new()
$allLogs = [System.Collections.Generic.List[string]]::new()
$failurePattern = 'FAILED:|Fatal error:|Assertion failed:|Ensure condition failed:'

function Start-OceanPeer {
    param(
        [string]$Role,
        [string]$Phase,
        [string]$Url,
        [string]$UserDirectory,
        [string]$PortArgument,
        [switch]$Journey,
        [string]$ExpectedX,
        [string]$ExpectedY
    )

    $log = Join-Path $output "$Phase-$Role.log"
    $shaderDirectory = Join-Path $output "$Phase-$Role-ShaderWorkingDir"
    New-Item -ItemType Directory -Path $UserDirectory, $shaderDirectory -Force | Out-Null
    $common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaOceanDiscoveryDisembarkPeerTest -KalmalaOceanIntegratedReconnectPeerTest'
    if ($Journey) {
        $common += ' -KalmalaOceanIntegratedJourneyPeerTest'
        if ($Role -ne 'late') { $common += ' -KalmalaOceanWeatherPeerTest' }
    }
    else {
        $common += " -KalmalaOceanReconnectPhase=$Phase -KalmalaOceanReconnectExpectedX=$ExpectedX -KalmalaOceanReconnectExpectedY=$ExpectedY"
    }
    $arguments = "`"$projectPath`" $Url -WorldSeed=$(if ($Role -eq 'host') { '418' } else { '999' }) $common"
    if ($PortArgument) { $arguments += " $PortArgument" }
    $arguments += " -ShaderWorkingDir=`"$shaderDirectory`" -abslog=`"$log`" -UserDir=`"$UserDirectory`""
    $process = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList $arguments
    $processes.Add($process)
    $allLogs.Add($log)
    return @{ Process = $process; Log = $log }
}

function Get-LogText {
    param([string]$Path)
    if (Test-Path -LiteralPath $Path) { return Get-Content -LiteralPath $Path -Raw }
    return ''
}

function Wait-ForLogPatterns {
    param(
        [hashtable[]]$Checks,
        [System.Diagnostics.Process[]]$ActiveProcesses,
        [int]$Seconds,
        [string]$FailureMessage
    )
    $deadline = (Get-Date).AddSeconds($Seconds)
    do {
        foreach ($process in $ActiveProcesses) {
            if ($process.HasExited) { throw "$FailureMessage An Unreal peer exited before verification completed." }
        }
        foreach ($path in $allLogs) {
            if ((Get-LogText $path) -match $failurePattern) {
                throw "$FailureMessage Inspect retained logs for the fixture failure."
            }
        }
        $complete = $true
        foreach ($check in $Checks) {
            if ((Get-LogText $check.Log) -notmatch $check.Pattern) { $complete = $false; break }
        }
        if ($complete) { return }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    throw "$FailureMessage Timed out after $Seconds seconds; inspect retained logs."
}

function Stop-OceanPeer {
    param([System.Diagnostics.Process]$Process)
    if ($null -eq $Process) { return }
    try {
        if (!$Process.HasExited) {
            Stop-Process -Id $Process.Id
            if (!$Process.WaitForExit(30000)) { throw "Unreal peer $($Process.Id) did not stop." }
        }
    }
    catch [System.InvalidOperationException] { }
}

function Read-JourneyOutcome {
    param([string]$Log)
    $text = Get-LogText $Log
    $pattern = 'Ocean integrated reconnect journey server passed: Seed=418 HostId=(?<host>[0-9a-f]{8}) HelmId=(?<helm>[0-9a-f]{8}) Players=3 Discovery=(?<discovery>[^ ]+) Reward=(?<item>Wood|Fibre|Stone):(?<quantity>[12]) Claims=2 Distance=(?<distance>[0-9]+).*StopX=(?<x>-?[0-9]+) StopY=(?<y>-?[0-9]+)'
    $match = [regex]::Match($text, $pattern)
    if (!$match.Success) { throw 'Could not parse the integrated voyage identity, discovery, reward, and accepted moored stop.' }
    return $match
}

function Read-ResumeIdentity {
    param([string]$Log)
    $text = Get-LogText $Log
    $pattern = 'Ocean skiff reconnect resume server passed: Seed=418 VesselActors=1 VesselId=ocean-skiff:primary HostId=(?<host>[0-9a-f]{8}) HelmId=(?<helm>[0-9a-f]{8}) Seats=Helm,Passenger Discovery=(?<discovery>[^ ]+) Reward=(?<item>Wood|Fibre|Stone):(?<quantity>[12]) ClaimState=AlreadyFound InventoryQuantity=0'
    $match = [regex]::Match($text, $pattern)
    if (!$match.Success) { throw 'Could not parse the resumed vessel, identities, and discovery replay result.' }
    return $match
}

$hostDirectory = Join-Path $output 'Host'
$ownerDirectory = Join-Path $output 'ReturningOwner'
$lateDirectory = Join-Path $output 'LateJoiner'
$hostPeer = $null
$owner = $null
$late = $null
try {
    $hostPeer = Start-OceanPeer -Role 'host' -Phase 'Journey' -Url '/Game/Kalmala/Maps/Prototype/L_Prototype?listen?Name=KalmalaReconnectHost' -UserDirectory $hostDirectory -PortArgument "-port=$Port" -Journey
    Wait-ForLogPatterns -Checks @(@{ Log = $hostPeer.Log; Pattern = 'GameNetDriver.*listening on port' }) -ActiveProcesses @($hostPeer.Process) -Seconds 90 -FailureMessage 'Integrated journey listen-server startup failed.'
    $owner = Start-OceanPeer -Role 'owner' -Phase 'Journey' -Url "127.0.0.1:$Port`?Name=KalmalaReconnectOwner" -UserDirectory $ownerDirectory -Journey
    Wait-ForLogPatterns -Checks @(
        @{ Log = $hostPeer.Log; Pattern = 'Ocean integrated journey started: Seed=418 Discovery=' },
        @{ Log = $owner.Log; Pattern = 'Client received world-generation identity: Seed=418' }
    ) -ActiveProcesses @($hostPeer.Process, $owner.Process) -Seconds 120 -FailureMessage 'Integrated voyage setup failed.'

    $late = Start-OceanPeer -Role 'late' -Phase 'Journey' -Url "127.0.0.1:$Port`?Name=KalmalaReconnectLate" -UserDirectory $lateDirectory -Journey
    Wait-ForLogPatterns -Checks @(
        @{ Log = $hostPeer.Log; Pattern = 'Ocean integrated journey late-join server passed: Seed=418 LateJoinId=[0-9a-f]{8} VesselActors=1 Underway=1 OriginalSeats=1 Attached=0 RewardLeak=0' },
        @{ Log = $late.Log; Pattern = 'Ocean integrated journey late-join peer passed: Authority=0 Seed=418 VesselActors=1 Underway=1 Seats=OriginalPeers LatePlayerAttached=0 RewardLeak=0' }
    ) -ActiveProcesses @($hostPeer.Process, $owner.Process, $late.Process) -Seconds 120 -FailureMessage 'Underway late-join privacy check failed.'

    Wait-ForLogPatterns -Checks @(
        @{ Log = $hostPeer.Log; Pattern = 'Ocean integrated reconnect journey server passed: Seed=418 HostId=[0-9a-f]{8} HelmId=[0-9a-f]{8} Players=3 Discovery=.+ Claims=2 Distance=2[3-4][0-9]{4} Crosswind=1 Calm=1 .*LateJoin=Underway RewardLeak=0 Seats=Helm,Passenger Mode=Moored StopX=-?[0-9]+ StopY=-?[0-9]+' },
        @{ Log = $owner.Log; Pattern = 'Ocean integrated reconnect journey peer passed: Authority=0 Seat=Helm Discovery=.+ DiscoveryFeedback=LandmarkFound Reward=(?:Wood|Fibre|Stone):[12] Distance=2[3-4][0-9]{4} Crosswind=1 Calm=1 Attached=1 Seats=Helm,Passenger Mode=Moored' },
        @{ Log = $owner.Log; Pattern = 'Client received world-generation identity: Seed=418' },
        @{ Log = $hostPeer.Log; Pattern = 'Ocean weather peer result: Authority=1 Cycle=7001 Direction=90 Strength=1\.000 .*Passed=1' },
        @{ Log = $hostPeer.Log; Pattern = 'Ocean weather peer result: Authority=1 Cycle=7002 Direction=0 Strength=0\.000 .*Passed=1' },
        @{ Log = $owner.Log; Pattern = 'Ocean weather peer result: Authority=0 Cycle=7001 Direction=90 Strength=1\.000 .*ClientForgeryRejected=1 Passed=1' },
        @{ Log = $owner.Log; Pattern = 'Ocean weather peer result: Authority=0 Cycle=7002 Direction=0 Strength=0\.000 .*ClientForgeryRejected=1 Passed=1' }
    ) -ActiveProcesses @($hostPeer.Process, $owner.Process, $late.Process) -Seconds $TimeoutSeconds -FailureMessage 'Integrated voyage did not preserve discovery, weather, late-join, and occupied-seat state.'

    $journeyIdentity = Read-JourneyOutcome -Log $hostPeer.Log
    Stop-OceanPeer $late.Process; $late = $null
    Stop-OceanPeer $owner.Process; $owner = $null
    Stop-OceanPeer $hostPeer.Process; $hostPeer = $null

    $invariant = [System.Globalization.CultureInfo]::InvariantCulture
    $expectedX = [double]::Parse($journeyIdentity.Groups['x'].Value, $invariant).ToString('0', $invariant)
    $expectedY = [double]::Parse($journeyIdentity.Groups['y'].Value, $invariant).ToString('0', $invariant)
    $hostPeer = Start-OceanPeer -Role 'host' -Phase 'Resume' -Url '/Game/Kalmala/Maps/Prototype/L_Prototype?listen?Name=KalmalaReconnectHost' -UserDirectory $hostDirectory -PortArgument "-port=$Port" -ExpectedX $expectedX -ExpectedY $expectedY
    Wait-ForLogPatterns -Checks @(@{ Log = $hostPeer.Log; Pattern = 'GameNetDriver.*listening on port' }) -ActiveProcesses @($hostPeer.Process) -Seconds 90 -FailureMessage 'Restarted journey listen-server startup failed.'
    $owner = Start-OceanPeer -Role 'owner' -Phase 'Resume' -Url "127.0.0.1:$Port`?Name=KalmalaReconnectOwner" -UserDirectory $ownerDirectory -ExpectedX $expectedX -ExpectedY $expectedY
    Wait-ForLogPatterns -Checks @(
        @{ Log = $hostPeer.Log; Pattern = 'Ocean skiff reconnect resume server passed: Seed=418 VesselActors=1 VesselId=ocean-skiff:primary HostId=[0-9a-f]{8} HelmId=[0-9a-f]{8} Seats=Helm,Passenger Discovery=.+ Reward=(?:Wood|Fibre|Stone):[12] ClaimState=AlreadyFound InventoryQuantity=0' },
        @{ Log = $owner.Log; Pattern = 'Ocean skiff reconnect resume owner peer passed: Authority=0 Seat=Helm Seed=418 DiscoveryFeedback=AlreadyFound Reward=(?:Wood|Fibre|Stone):0 VesselActors=1' },
        @{ Log = $owner.Log; Pattern = 'Client received world-generation identity: Seed=418' }
    ) -ActiveProcesses @($hostPeer.Process, $owner.Process) -Seconds $TimeoutSeconds -FailureMessage 'Authenticated journey restart/reconnect failed.'

    $resumeIdentity = Read-ResumeIdentity -Log $hostPeer.Log
    foreach ($key in @('host', 'helm', 'discovery', 'item', 'quantity')) {
        if ($journeyIdentity.Groups[$key].Value -ne $resumeIdentity.Groups[$key].Value) {
            throw "The $key identity or discovery value changed across the integrated journey restart/reconnect."
        }
    }

    $late = Start-OceanPeer -Role 'late' -Phase 'Resume' -Url "127.0.0.1:$Port`?Name=KalmalaReconnectLate" -UserDirectory $lateDirectory -ExpectedX $expectedX -ExpectedY $expectedY
    Wait-ForLogPatterns -Checks @(
        @{ Log = $hostPeer.Log; Pattern = 'Ocean skiff reconnect resume late-join server passed: Seed=418 LateJoinId=[0-9a-f]{8} VesselActors=1 Helm=OriginalOwner Passenger=Host Attached=0 RewardLeak=0' },
        @{ Log = $late.Log; Pattern = 'Ocean skiff reconnect resume late-join peer passed: Authority=0 Seed=418 VesselActors=1 Seats=OriginalPeers LatePlayerAttached=0 RewardLeak=0' },
        @{ Log = $late.Log; Pattern = 'Client received world-generation identity: Seed=418' }
    ) -ActiveProcesses @($hostPeer.Process, $owner.Process, $late.Process) -Seconds $TimeoutSeconds -FailureMessage 'Post-restart late-join privacy check failed.'

    Write-Output 'PASS: the late peer joined during the 2.4 km voyage without private rewards; the original authenticated seats and moored stop restored, and discovery replay was rejected after restart.'
}
finally {
    foreach ($peer in @($late, $owner, $hostPeer)) {
        if ($null -ne $peer) { Stop-OceanPeer $peer.Process }
    }
    Write-Output "Integrated voyage reconnect logs: $output"
}
