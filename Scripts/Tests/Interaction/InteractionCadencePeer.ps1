param(
    [ValidateRange(1024, 65535)][int]$Port = 18164,
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
    Join-Path $env:TEMP ('KalmalaInteractionCadencePeer-' + [guid]::NewGuid().ToString('N'))
}
if ($output.Length -gt 140) { throw "Interaction-cadence peer output root is too long: $output" }
if (Test-Path -LiteralPath $output) { throw "Output directory must be new: $output" }
$hostDir = Join-Path $output 'Host'
$ownerDir = Join-Path $output 'Owner'
$serverLog = Join-Path $output 'server.log'
$ownerLog = Join-Path $output 'owner.log'
New-Item -ItemType Directory -Path $hostDir, $ownerDir -Force | Out-Null
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaInteractionCadencePeerTest -ExecCmds="t.MaxFPS 60"'
$server = $null
$owner = $null

function Read-PeerLog([string]$Path) {
    if (Test-Path -LiteralPath $Path) { Get-Content -LiteralPath $Path -Raw } else { '' }
}

function Assert-PeersRunning($Peers) {
    foreach ($peer in $Peers) {
        if ($peer.HasExited) { throw "Interaction-cadence peer exited before acceptance: $($peer.ExitCode). Logs: $output" }
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
    if (!$ready) { throw "Interaction-cadence listen server readiness timed out. Logs: $output" }

    $owner = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -KalmalaInteractionCadencePeerOwner -UserDir=`"$ownerDir`" -abslog=`"$ownerLog`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        Assert-PeersRunning @($server, $owner)
        $joined = (Read-PeerLog $serverLog) -match 'LogNet: Join succeeded:'
        if (!$joined) { Start-Sleep -Milliseconds 500 }
    } while (!$joined -and (Get-Date) -lt $deadline)
    if (!$joined) { throw "Interaction-cadence remote owner did not join. Logs: $output" }

    $deadline = (Get-Date).AddSeconds(90)
    $passed = $false
    do {
        Assert-PeersRunning @($server, $owner)
        $serverText = Read-PeerLog $serverLog
        $ownerText = Read-PeerLog $ownerLog
        $allText = "$serverText`n$ownerText"
        if ($allText -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Interaction cadence peer owner: Passed=0|Interaction cadence peer verification FAILED:') {
            throw "Interaction-cadence peer verification failed; inspect retained logs: $output"
        }
        $passed = $serverText.Contains('Interaction cadence peer server: Passed=1 RemoteOwner=1 BurstRequests=8 CadencePassedTraces=1 FirstImmediate=1 FirstDispatch=1 PacedRequest=1 BoundedDispatch=2')
        $passed = $passed -and $ownerText.Contains('Interaction cadence peer owner: Passed=1 FirstDispatch=1 BurstBounded=1 PacedDispatch=1')
        $identity = $ownerText.Contains('Client received world-generation identity: Seed=418')
        if (!($passed -and $identity)) { Start-Sleep -Milliseconds 500 }
    } while (!($passed -and $identity) -and (Get-Date) -lt $deadline)
    if (!($passed -and $identity)) { throw "Interaction-cadence acceptance or world identity timed out. Logs: $output" }
    Write-Output 'PASS: immediate remote interaction, bounded repeated requests, and normally paced retry.'
}
finally {
    foreach ($peer in @($owner, $server)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Interaction-cadence peer logs: $output"
}
