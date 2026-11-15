[CmdletBinding()]
param(
    [ValidateRange(1024, 65535)][int]$Port = 17945,
    [string]$OutputDirectory = '',
    [string]$Project = '',
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
)
$ErrorActionPreference = 'Stop'
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot
$projectPath = if ($Project) { $Project } else { Join-Path $repoPaths.ProjectRoot 'Kalmala.uproject' }
if (!(Test-Path -LiteralPath $projectPath -PathType Leaf)) { throw "Project not found: $projectPath" }
if (!(Test-Path -LiteralPath $Editor -PathType Leaf)) { throw "Editor not found: $Editor" }
$output = if ($OutputDirectory) { [IO.Path]::GetFullPath($OutputDirectory) } else {
    Join-Path $env:TEMP ('SkillPrivacyPeer-' + [guid]::NewGuid().ToString('N'))
}
if ($output.Length -gt 180) { throw "Skill peer output root is too long: $output" }
if (Test-Path -LiteralPath $output) { throw "Output directory must be new: $output" }
$hostDir = Join-Path $output 'Host'
$ownerDir = Join-Path $output 'Owner'
$observerDir = Join-Path $output 'Observer'
$serverLog = Join-Path $output 'server.log'
$ownerLog = Join-Path $output 'owner.log'
$observerLog = Join-Path $output 'observer.log'
New-Item -ItemType Directory -Path $hostDir, $ownerDir, $observerDir -Force | Out-Null
$common = "-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaSkillPrivacyPeerTest -ExecCmds=`"t.MaxFPS 60`""
$server = $null
$owner = $null
$observer = $null
function Read-PeerLog([string]$Path) {
    if (Test-Path -LiteralPath $Path) { [string](Get-Content -LiteralPath $Path -Raw) } else { '' }
}
function Assert-PeersRunning($Peers) {
    foreach ($peer in $Peers) {
        if ($peer.HasExited) { throw "A skill privacy peer exited before acceptance: $($peer.ExitCode). Logs: $output" }
    }
}
try {
    $server = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -UserDir=`"$hostDir`" -abslog=`"$serverLog`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        Assert-PeersRunning @($server)
        $ready = (Read-PeerLog $serverLog) -match 'GameNetDriver.*listening on port'
        if (!$ready) { Start-Sleep -Milliseconds 500 }
    } while (!$ready -and (Get-Date) -lt $deadline)
    if (!$ready) { throw 'Skill privacy listen server readiness timed out.' }

    $owner = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" 127.0.0.1:$Port -WorldSeed=999 $common -UserDir=`"$ownerDir`" -abslog=`"$ownerLog`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        Assert-PeersRunning @($server, $owner)
        $joined = [regex]::Matches((Read-PeerLog $serverLog), 'LogNet: Join succeeded:').Count -ge 1
        if (!$joined) { Start-Sleep -Milliseconds 500 }
    } while (!$joined -and (Get-Date) -lt $deadline)
    if (!$joined) { throw 'Skill privacy owner did not join before observer launch.' }

    $observer = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" 127.0.0.1:$Port -WorldSeed=1000 $common -UserDir=`"$observerDir`" -abslog=`"$observerLog`""
    $deadline = (Get-Date).AddSeconds(240)
    do {
        Assert-PeersRunning @($server, $owner, $observer)
        $serverText = Read-PeerLog $serverLog
        $ownerText = Read-PeerLog $ownerLog
        $observerText = Read-PeerLog $observerLog
        $allText = "$serverText`n$ownerText`n$observerText"
        if ($allText -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Skill privacy peer FAILED:') {
            throw "Skill privacy peer verification failed; inspect retained logs: $output"
        }
        $passed = $serverText.Contains('Skill privacy peer fixture: Ready=1 RemoteOwner=1 Recipe=HearthBrothRecipe Cauldron=1 Heat=1') `
            -and $serverText.Contains('Skill privacy peer server: Passed=1 AcceptedCrafts=10 ResultDelta=10 Payment=1 Output=10 Skills=6 CookingExperience=100 Level=2 Unlock=1') `
            -and $ownerText.Contains('Skill privacy peer owner: Passed=1 Authority=0 AcceptedCrafts=10 Skills=6 CookingExperience=100 Level=2 Unlock=1 Badge=2:1 ClientWriteRejected=1') `
            -and $observerText.Contains('Skill privacy peer observer: Passed=1 Authority=0 TargetDetailed=0 Badge=2:1')
        $identity = $ownerText.Contains('Client received world-generation identity: Seed=418') `
            -and $observerText.Contains('Client received world-generation identity: Seed=418')
        if (!($passed -and $identity)) { Start-Sleep -Milliseconds 500 }
    } while (!($passed -and $identity) -and (Get-Date) -lt $deadline)
    if (!($passed -and $identity)) { throw "Skill privacy peer acceptance or world identity timed out. Logs: $output" }
    Write-Output 'PASS: ten accepted remote Hearth Broth crafts produced the exact owner-only six-skill ledger and derived peer badge; the observer received no per-skill details and a client-authored award was rejected.'
}
finally {
    foreach ($peer in @($observer, $owner, $server)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Scenario logs: $output"
}
