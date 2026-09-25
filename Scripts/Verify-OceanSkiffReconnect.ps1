param(
    [int]$Port = 18171,
    [string]$OutputDirectory = '',
    [string]$Project = '',
    [int]$TimeoutSeconds = 180
)

$ErrorActionPreference = 'Stop'
$projectPath = if ($Project) { $Project } else { Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject' }
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaOceanSkiffReconnect-' + [guid]::NewGuid().ToString('N')) }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$processes = [System.Collections.Generic.List[System.Diagnostics.Process]]::new()
$allLogs = [System.Collections.Generic.List[string]]::new()
$failurePattern = 'Ocean skiff reconnect peer verification FAILED:|Fatal error:|Assertion failed:|Ensure condition failed:'

function Start-OceanPeer {
    param(
        [string]$Role,
        [string]$Phase,
        [string]$Url,
        [string]$UserDirectory,
        [string]$PortArgument
    )

    $log = Join-Path $output "$Phase-$Role.log"
    $shaderDirectory = Join-Path $output "$Phase-$Role-ShaderWorkingDir"
    New-Item -ItemType Directory -Path $UserDirectory, $shaderDirectory -Force | Out-Null
    $common = "-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaOceanDiscoveryDisembarkPeerTest -KalmalaOceanReconnectPhase=$Phase"
    $arguments = "`"$projectPath`" $Url -WorldSeed=418 $common"
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

function Read-ServerIdentity {
    param([string]$Log, [string]$Phase)
    $text = Get-LogText $Log
    $pattern = "Ocean skiff reconnect $Phase server passed: Seed=418 VesselActors=1 VesselId=ocean-skiff:primary HostId=(?<host>[0-9a-f]{8}) HelmId=(?<helm>[0-9a-f]{8}) Seats=Helm,Passenger Discovery=(?<discovery>[^ ]+) Reward=(?<item>Wood|Fibre|Stone):(?<quantity>[12]) ClaimState=(?<claim>LandmarkFound|AlreadyFound) InventoryQuantity=(?<inventory>[0-9]+)"
    $match = [regex]::Match($text, $pattern)
    if (!$match.Success) { throw "Could not parse the $Phase server identity and discovery record." }
    return $match
}

$hostDirectory = Join-Path $output 'Host'
$ownerDirectory = Join-Path $output 'ReturningOwner'
$lateDirectory = Join-Path $output 'LateJoiner'
$hostPeer = $null
$owner = $null
$late = $null
try {
    $hostPeer = Start-OceanPeer -Role 'host' -Phase 'Seed' -Url '/Game/Kalmala/Maps/Prototype/L_Prototype?listen?Name=KalmalaReconnectHost' -UserDirectory $hostDirectory -PortArgument "-port=$Port"
    Wait-ForLogPatterns -Checks @(@{ Log = $hostPeer.Log; Pattern = 'GameNetDriver.*listening on port' }) -ActiveProcesses @($hostPeer.Process) -Seconds 90 -FailureMessage 'Seed listen-server startup failed.'
    $owner = Start-OceanPeer -Role 'owner' -Phase 'Seed' -Url "127.0.0.1:$Port`?Name=KalmalaReconnectOwner" -UserDirectory $ownerDirectory
    Wait-ForLogPatterns -Checks @(
        @{ Log = $hostPeer.Log; Pattern = 'Ocean skiff reconnect seed server passed: Seed=418 VesselActors=1 VesselId=ocean-skiff:primary HostId=[0-9a-f]{8} HelmId=[0-9a-f]{8} Seats=Helm,Passenger Discovery=.+ Reward=(?:Wood|Fibre|Stone):[12] ClaimState=LandmarkFound InventoryQuantity=[12]' },
        @{ Log = $owner.Log; Pattern = 'Ocean skiff reconnect seed owner peer passed: Authority=0 Seat=Helm Seed=418 DiscoveryFeedback=LandmarkFound Reward=(?:Wood|Fibre|Stone):[12] VesselActors=1' },
        @{ Log = $owner.Log; Pattern = 'Client received world-generation identity: Seed=418' }
    ) -ActiveProcesses @($hostPeer.Process, $owner.Process) -Seconds $TimeoutSeconds -FailureMessage 'Initial skiff and discovery setup failed.'

    $late = Start-OceanPeer -Role 'late' -Phase 'Seed' -Url "127.0.0.1:$Port`?Name=KalmalaReconnectLate" -UserDirectory $lateDirectory
    Wait-ForLogPatterns -Checks @(
        @{ Log = $hostPeer.Log; Pattern = 'Ocean skiff reconnect seed late-join server passed: Seed=418 LateJoinId=[0-9a-f]{8} VesselActors=1 Helm=OriginalOwner Passenger=Host Attached=0 RewardLeak=0' },
        @{ Log = $late.Log; Pattern = 'Ocean skiff reconnect seed late-join peer passed: Authority=0 Seed=418 VesselActors=1 Seats=OriginalPeers LatePlayerAttached=0 RewardLeak=0' },
        @{ Log = $late.Log; Pattern = 'Client received world-generation identity: Seed=418' }
    ) -ActiveProcesses @($hostPeer.Process, $owner.Process, $late.Process) -Seconds $TimeoutSeconds -FailureMessage 'Seed-session late join failed.'

    $seedIdentity = Read-ServerIdentity -Log $hostPeer.Log -Phase 'seed'
    Stop-OceanPeer $late.Process; $late = $null
    Stop-OceanPeer $owner.Process; $owner = $null
    Stop-OceanPeer $hostPeer.Process; $hostPeer = $null

    $hostPeer = Start-OceanPeer -Role 'host' -Phase 'Resume' -Url '/Game/Kalmala/Maps/Prototype/L_Prototype?listen?Name=KalmalaReconnectHost' -UserDirectory $hostDirectory -PortArgument "-port=$Port"
    Wait-ForLogPatterns -Checks @(@{ Log = $hostPeer.Log; Pattern = 'GameNetDriver.*listening on port' }) -ActiveProcesses @($hostPeer.Process) -Seconds 90 -FailureMessage 'Restarted listen-server startup failed.'
    $owner = Start-OceanPeer -Role 'owner' -Phase 'Resume' -Url "127.0.0.1:$Port`?Name=KalmalaReconnectOwner" -UserDirectory $ownerDirectory
    Wait-ForLogPatterns -Checks @(
        @{ Log = $hostPeer.Log; Pattern = 'Ocean skiff reconnect resume server passed: Seed=418 VesselActors=1 VesselId=ocean-skiff:primary HostId=[0-9a-f]{8} HelmId=[0-9a-f]{8} Seats=Helm,Passenger Discovery=.+ Reward=(?:Wood|Fibre|Stone):[12] ClaimState=AlreadyFound InventoryQuantity=0' },
        @{ Log = $owner.Log; Pattern = 'Ocean skiff reconnect resume owner peer passed: Authority=0 Seat=Helm Seed=418 DiscoveryFeedback=AlreadyFound Reward=(?:Wood|Fibre|Stone):0 VesselActors=1' },
        @{ Log = $owner.Log; Pattern = 'Client received world-generation identity: Seed=418' }
    ) -ActiveProcesses @($hostPeer.Process, $owner.Process) -Seconds $TimeoutSeconds -FailureMessage 'Authenticated restart/reconnect verification failed.'

    $resumeIdentity = Read-ServerIdentity -Log $hostPeer.Log -Phase 'resume'
    foreach ($key in @('host', 'helm', 'discovery', 'item', 'quantity')) {
        if ($seedIdentity.Groups[$key].Value -ne $resumeIdentity.Groups[$key].Value) {
            throw "The $key identity or discovery value changed across the authenticated restart/reconnect."
        }
    }

    $late = Start-OceanPeer -Role 'late' -Phase 'Resume' -Url "127.0.0.1:$Port`?Name=KalmalaReconnectLate" -UserDirectory $lateDirectory
    Wait-ForLogPatterns -Checks @(
        @{ Log = $hostPeer.Log; Pattern = 'Ocean skiff reconnect resume late-join server passed: Seed=418 LateJoinId=[0-9a-f]{8} VesselActors=1 Helm=OriginalOwner Passenger=Host Attached=0 RewardLeak=0' },
        @{ Log = $late.Log; Pattern = 'Ocean skiff reconnect resume late-join peer passed: Authority=0 Seed=418 VesselActors=1 Seats=OriginalPeers LatePlayerAttached=0 RewardLeak=0' },
        @{ Log = $late.Log; Pattern = 'Client received world-generation identity: Seed=418' }
    ) -ActiveProcesses @($hostPeer.Process, $owner.Process, $late.Process) -Seconds $TimeoutSeconds -FailureMessage 'Post-restart late join failed.'

    Write-Output 'PASS: late join saw one vessel and no private reward; stable authenticated peers restored their saved seats after restart, and the original owner replay was rejected without another inventory grant.'
}
finally {
    foreach ($peer in @($late, $owner, $hostPeer)) {
        if ($null -ne $peer) { Stop-OceanPeer $peer.Process }
    }
    Write-Output "Ocean skiff reconnect logs: $output"
}
