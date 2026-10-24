param([int]$Port = 19309, [Parameter(Mandatory)][string]$OutputDirectory)
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot
$ErrorActionPreference = 'Stop'
$project = Join-Path $repoPaths.ProjectRoot 'Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path $output) { throw 'Use a fresh output directory for each A9 peer run.' }
if ($output.Length -gt 130) { throw 'Use a short output root to keep generated paths below MAX_PATH.' }
New-Item -ItemType Directory -Path $output | Out-Null
$peers = @()
$logs = @('host','one','two') | ForEach-Object { Join-Path $output "$_`.log" }
$common = '-game -nullrhi -nosound -unattended -nosplash -NoZenAutoLaunch -DDC=NoZenLocalFallback -forcelogflush -KalmalaA9PeerTest'
function Read-Log([string]$Path) { if (Test-Path $Path) { [string](Get-Content -LiteralPath $Path -Raw) } else { '' } }
function Stop-Peers { foreach ($peer in $script:peers) { if (!$peer.HasExited) { Stop-Process -Id $peer.Id } }; $script:peers = @() }
try {
    $peers += Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -abslog=`"$($logs[0])`" -UserDir=`"$output\host`""
    $deadline = (Get-Date).AddSeconds(120)
    while ((Read-Log $logs[0]) -notmatch 'GameNetDriver.*listening on port') {
        if ($peers[0].HasExited -or (Get-Date) -gt $deadline) { throw 'A9 host startup failed.' }
        Start-Sleep -Milliseconds 500
    }
    foreach ($index in 1,2) {
        $peers += Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -abslog=`"$($logs[$index])`" -UserDir=`"$output\c$index`""
    }
    $deadline = (Get-Date).AddSeconds(240)
    do {
        $texts = @($logs | ForEach-Object { Read-Log $_ })
        if (($texts -join "`n") -match 'Fatal error:|Assertion failed:|Ensure condition failed:|A9 peer FAILED:') { throw 'A9 peer failure; inspect retained logs.' }
        if (@($peers | Where-Object HasExited).Count) { throw 'An A9 peer exited early.' }
        $ready = $texts[0] -match 'A9 peer mining: Passed=1 TotalGem=1 SavedOutcome=3 OwningRequests=2' -and $texts[0] -match 'A9 peer treasure: Passed=1 TotalGarnet=1 SavedClaim=1'
        foreach ($index in 1,2) {
            $ready = $ready -and $texts[$index] -match 'A9 peer owner stats: Passed=1 .*ForeignGearHidden=1' -and $texts[$index] -match 'A9 peer owning interaction request: Target=KalmalaDiscoveryActor' -and $texts[$index] -match 'A9 peer replicated node: .*Hits=1 Outcome=0' -and $texts[$index] -match 'A9 peer replicated node: .*Hits=3 Outcome=2'
        }
        if ($ready) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if (!$ready) { throw 'A9 peer scenario timed out.' }
    $gem = [regex]::Match($texts[0], 'A9 peer mining: Passed=1 [^\r\n]*Node=([^\r\n]+)').Groups[1].Value.Trim()
    $treasure = [regex]::Match($texts[0], 'A9 peer treasure: Passed=1 [^\r\n]*Treasure=([^\r\n]+)').Groups[1].Value.Trim()
    Stop-Peers
    $restartLog = Join-Path $output 'restart.log'
    $peers += Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype -WorldSeed=418 -game -nullrhi -nosound -unattended -nosplash -NoZenAutoLaunch -DDC=NoZenLocalFallback -forcelogflush -KalmalaA9RestartCheck -A9GemId=$gem -A9TreasureId=$treasure -abslog=`"$restartLog`" -UserDir=`"$output\host`""
    $deadline = (Get-Date).AddSeconds(120)
    do {
        $restart = Read-Log $restartLog
        if ($restart -match 'Fatal error:|Assertion failed:|Ensure condition failed:|A9 restart: Passed=0') { throw 'A9 restart failed.' }
        if ($restart -match 'A9 restart: Passed=1 GemClaimed=1 TreasureClaimed=1') { break }
        if ($peers[0].HasExited) { throw 'A9 restart process exited early.' }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ($restart -notmatch 'A9 restart: Passed=1 GemClaimed=1 TreasureClaimed=1') { throw 'A9 restart timed out.' }
    Write-Output 'PASS: two owning clients mined, retried full packs, contended for one gem and treasure reward, observed private accessory stats and replicated hit progress; a fresh process restored both claimed ledger entries.'
}
finally { Stop-Peers; Write-Output "A9 evidence: $output" }
