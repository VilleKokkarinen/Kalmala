param(
    [ValidateRange(1024, 65535)][int]$Port = 18169,
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
    Join-Path $env:TEMP ('KalmalaSupportMagicPeer-' + [guid]::NewGuid().ToString('N'))
}
if ($output.Length -gt 140) { throw "Support-magic peer output root is too long: $output" }
if (Test-Path -LiteralPath $output) { throw "Output directory must be new: $output" }
$hostDir = Join-Path $output 'Host'
$ownerDir = Join-Path $output 'Owner'
$observerDir = Join-Path $output 'Observer'
$serverLog = Join-Path $output 'server.log'
$ownerLog = Join-Path $output 'owner.log'
$observerLog = Join-Path $output 'observer.log'
New-Item -ItemType Directory -Path $hostDir, $ownerDir, $observerDir -Force | Out-Null
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaSupportMagicPeerTest -ExecCmds="t.MaxFPS 60"'
$server = $null
$owner = $null
$observer = $null

function Read-PeerLog([string]$Path) {
    if (Test-Path -LiteralPath $Path) { Get-Content -LiteralPath $Path -Raw } else { '' }
}

function Assert-PeersRunning($Peers) {
    foreach ($peer in $Peers) {
        if ($peer.HasExited) { throw "Support-magic peer exited before acceptance: $($peer.ExitCode). Logs: $output" }
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
    if (!$ready) { throw "Support-magic listen server readiness timed out. Logs: $output" }

    $owner = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -UserDir=`"$ownerDir`" -abslog=`"$ownerLog`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        Assert-PeersRunning @($server, $owner)
        $joined = [regex]::Matches((Read-PeerLog $serverLog), 'LogNet: Join succeeded:').Count -ge 1
        if (!$joined) { Start-Sleep -Milliseconds 500 }
    } while (!$joined -and (Get-Date) -lt $deadline)
    if (!$joined) { throw "Support-magic owner did not join before observer launch. Logs: $output" }

    $observer = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=1000 $common -UserDir=`"$observerDir`" -abslog=`"$observerLog`""
    $deadline = (Get-Date).AddSeconds(240)
    $passed = $false
    do {
        Assert-PeersRunning @($server, $owner, $observer)
        $serverText = Read-PeerLog $serverLog
        $ownerText = Read-PeerLog $ownerLog
        $observerText = Read-PeerLog $observerLog
        $allText = "$serverText`n$ownerText`n$observerText"
        if ($allText -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Support magic peer .*Passed=0|Support magic peer verification FAILED:') {
            throw "Support-magic peer verification failed; inspect retained logs: $output"
        }
        $passed = $serverText -match 'Support magic peer server: Passed=1 RemoteOwner=1 Slot=9 FirstCast=1 CooldownReject=1 NoRepeatPayment=1 PublicShield=1 PrivateOwnerState=1 ObserverConnected=1'
        $passed = $passed -and $ownerText -match 'Support magic peer owner: Passed=1 FirstCastAccepted=1 RepeatRejected=1 LearnedPrivate=1 CooldownPrivate=1 FeedbackPrivate=1 PublicShield=1'
        $passed = $passed -and $observerText -match 'Support magic peer observer: Passed=1 PublicEffect=1 LearnedPrivate=0 CooldownPrivate=0 FeedbackPrivate=0 SerialPrivate=0'
        $identity = $ownerText.Contains('Client received world-generation identity: Seed=418') -and $observerText.Contains('Client received world-generation identity: Seed=418')
        if (!($passed -and $identity)) { Start-Sleep -Milliseconds 500 }
    } while (!($passed -and $identity) -and (Get-Date) -lt $deadline)
    if (!($passed -and $identity)) { throw "Support-magic peer acceptance or world identity timed out. Logs: $output" }
    Write-Output 'PASS: remote hotbar activation, cooldown payment rejection, and observer privacy.'
}
finally {
    foreach ($peer in @($observer, $owner, $server)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Support-magic peer logs: $output"
}
