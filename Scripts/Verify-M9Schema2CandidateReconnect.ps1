param([int]$Port = 19683, [string]$ProjectPath = '')
$ErrorActionPreference = 'Stop'
$project = if ([string]::IsNullOrWhiteSpace($ProjectPath)) {
    Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
} else {
    (Resolve-Path -LiteralPath $ProjectPath).Path
}
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path $env:TEMP ('KalmalaM9Schema2Reconnect-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$hostDir = Join-Path $output 'Host'
$ownerDir = Join-Path $output 'Owner'
$observerDir = Join-Path $output 'Observer'
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaM9Schema2PeerTest'

function Read-Log([string]$Path) {
    if (Test-Path -LiteralPath $Path) { return Get-Content -LiteralPath $Path -Raw }
    return ''
}

function Stop-Peer($Peer) {
    if ($null -ne $Peer -and !$Peer.HasExited) {
        Stop-Process -Id $Peer.Id
        $Peer.WaitForExit()
    }
}

function Wait-Listen($Process, [string]$LogPath) {
    $deadline = (Get-Date).AddSeconds(120)
    do {
        if ($Process.HasExited) { throw 'Schema-2 listen server exited during startup.' }
        if ((Test-Path -LiteralPath $LogPath) -and (Select-String -LiteralPath $LogPath -Pattern 'GameNetDriver.*listening on port' -Quiet)) { return }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    throw 'Schema-2 listen server readiness timed out.'
}

function Wait-ForLogs($Processes, [string[]]$LogPaths, [scriptblock]$Condition, [string]$Description) {
    $deadline = (Get-Date).AddSeconds(120)
    do {
        foreach ($Peer in $Processes) {
            if ($null -ne $Peer -and $Peer.HasExited) { throw "${Description}: a peer exited before the verification completed." }
        }
        $texts = @($LogPaths | ForEach-Object { Read-Log $_ })
        $combined = $texts -join "`n"
        if ($combined -match 'Passed=0|Fatal error:|Assertion failed:|Ensure condition failed:') {
            throw "$Description reported a failure."
        }
        if (& $Condition $texts) { return $texts }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    throw "$Description timed out."
}

function Invoke-Phase([string]$Phase) {
    $serverLog = Join-Path $output "$Phase-server.log"
    $ownerLog = Join-Path $output "$Phase-owner.log"
    $observerLog = Join-Path $output "$Phase-observer.log"
    $server = $null
    $owner = $null
    $observer = $null
    try {
        $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -KalmalaM9Schema2Phase=$Phase -abslog=`"$serverLog`" -UserDir=`"$hostDir`""
        Wait-Listen $server $serverLog
        $worldPattern = "M9 schema-2 candidate world: Phase=$Phase Passed=1 Records=3 Attachment=1 DryingLine=1 NormalSchema=2"
        Wait-ForLogs @($server) @($serverLog) { param($texts) $texts[0] -match $worldPattern } "$Phase world-candidate check" | Out-Null

        $owner = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -KalmalaM9Schema2Phase=$Phase -KalmalaM9Schema2ClientRole=Owner -abslog=`"$ownerLog`" -UserDir=`"$ownerDir`""
        $replayResult = if ($Phase -eq 'Resume') { 1 } else { 0 }
        $ownerServerPattern = "M9 schema-2 candidate player: Phase=$Phase Role=Owner Passed=1 CandidateReady=1 Discovery=1 Claim=1 ReplayRejected=$replayResult ServerTools=3 NormalSchema=2 NormalTools=4 NormalFacts=1"
        Wait-ForLogs @($server, $owner) @($serverLog, $ownerLog) { param($texts) $texts[0] -match $ownerServerPattern } "$Phase owner-player check" | Out-Null

        $observer = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -KalmalaM9Schema2Phase=$Phase -KalmalaM9Schema2ClientRole=Observer -abslog=`"$observerLog`" -UserDir=`"$observerDir`""
        $texts = Wait-ForLogs @($server, $owner, $observer) @($serverLog, $ownerLog, $observerLog) {
            param($current)
            $serverOkay = $current[0] -match $ownerServerPattern -and $current[0] -match "M9 schema-2 candidate player: Phase=$Phase Role=Observer Passed=1 CandidateLoaded=0 ServerTools=4 NormalSchema=2 NormalTools=0 NormalFacts=0"
            $ownerOkay = $current[1] -match "M9 schema-2 candidate client: Phase=$Phase Role=Owner Passed=1 OwnedTools=3 OtherOwnersHidden=1"
            $observerOkay = $current[2] -match "M9 schema-2 candidate client: Phase=$Phase Role=Observer Passed=1 OwnedTools=4 OtherOwnersHidden=1"
            $worldIdentityOkay = $current[1] -match 'Client received world-generation identity: Seed=418' -and $current[2] -match 'Client received world-generation identity: Seed=418'
            return $serverOkay -and $ownerOkay -and $observerOkay -and $worldIdentityOkay
        } "$Phase host/client candidate reconnect check"
        return $texts
    }
    finally {
        Stop-Peer $observer
        Stop-Peer $owner
        Stop-Peer $server
    }
}

try {
    Invoke-Phase 'Seed' | Out-Null
    Invoke-Phase 'Resume' | Out-Null
    Write-Output 'PASS: schema-2 world and player state survived a listen-server restart; a schema-1 player slot migrated its discovery/effect facts, current M9 claims and owner tools persisted, replay was rejected, other owners saw no tool details, and both normal slots used schema 2.'
}
finally {
    Write-Output "Scenario logs: $output"
}
