param(
    [ValidateRange(1024, 65535)][int]$Port = 18173,
    [string]$OutputDirectory = '',
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
)
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot
$ErrorActionPreference = 'Stop'

$project = Join-Path $repoPaths.ProjectRoot 'Kalmala.uproject'
if (!(Test-Path -LiteralPath $project -PathType Leaf)) { throw "Project not found: $project" }
if (!(Test-Path -LiteralPath $Editor -PathType Leaf)) { throw "Editor not found: $Editor" }
$output = if ($OutputDirectory) { [IO.Path]::GetFullPath($OutputDirectory) } else {
    Join-Path $env:TEMP ('KalmalaCampBedPeer-' + [guid]::NewGuid().ToString('N'))
}
if ($output.Length -gt 140) { throw "Camp Bed peer output root is too long: $output" }
if (Test-Path -LiteralPath $output) { throw "Output directory must be new: $output" }
$hostDir = Join-Path $output 'Host'
$ownerDir = Join-Path $output 'Owner'
$serverLog = Join-Path $output 'server.log'
$ownerLog = Join-Path $output 'owner.log'
New-Item -ItemType Directory -Path $hostDir, $ownerDir -Force | Out-Null
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaCampBedRestPeerTest -ExecCmds="t.MaxFPS 60"'
$server = $null
$owner = $null

function Read-PeerLog([string]$Path) {
    if (Test-Path -LiteralPath $Path) { Get-Content -LiteralPath $Path -Raw } else { '' }
}

function Assert-PeersRunning($Peers) {
    foreach ($peer in $Peers) {
        if ($peer.HasExited) { throw "Camp Bed peer exited before acceptance: $($peer.ExitCode). Logs: $output" }
    }
}

try {
    $server = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -UserDir=`"$hostDir`" -abslog=`"$serverLog`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        Assert-PeersRunning @($server)
        $ready = (Read-PeerLog $serverLog) -match 'GameNetDriver.*listening on port'
        if (!$ready) { Start-Sleep -Milliseconds 500 }
    } while (!$ready -and (Get-Date) -lt $deadline)
    if (!$ready) { throw "Camp Bed listen server readiness timed out. Logs: $output" }

    $owner = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -KalmalaCampBedRestPeerOwner -UserDir=`"$ownerDir`" -abslog=`"$ownerLog`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        Assert-PeersRunning @($server, $owner)
        $joined = (Read-PeerLog $serverLog) -match 'LogNet: Join succeeded:'
        if (!$joined) { Start-Sleep -Milliseconds 500 }
    } while (!$joined -and (Get-Date) -lt $deadline)
    if (!$joined) { throw "Camp Bed remote owner did not join. Logs: $output" }

    $deadline = (Get-Date).AddSeconds(90)
    $passed = $false
    do {
        Assert-PeersRunning @($server, $owner)
        $serverText = Read-PeerLog $serverLog
        $ownerText = Read-PeerLog $ownerLog
        $allText = "$serverText`n$ownerText"
        if ($allText -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Camp Bed rest peer owner: Passed=0|Camp Bed rest peer server: Passed=0') {
            throw "Camp Bed rest peer verification failed; inspect retained logs: $output"
        }
        $serverPassed = $serverText.Contains('Camp Bed rest peer server: Passed=1 RateRejected=1 Accepted=1 ListenHostUnposed=1 RemoteAuthorityUnposed=1')
        $ownerPassed = $ownerText.Contains('Camp Bed rest peer owner: Passed=1 RateRejectedPoseUnchanged=1 AcceptedPose=1 Stationary=1')
        $identity = $ownerText.Contains('Client received world-generation identity: Seed=418')
        $passed = $serverPassed -and $ownerPassed -and $identity
        if (!$passed) { Start-Sleep -Milliseconds 500 }
    } while (!$passed -and (Get-Date) -lt $deadline)
    if (!$passed) { throw "Camp Bed rest peer acceptance or world identity timed out. Logs: $output" }
    Write-Output 'PASS: rate-rejected rest stays standing; accepted rest poses only the remote owner.'
}
finally {
    foreach ($peer in @($owner, $server)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Camp Bed rest peer logs: $output"
}
