param(
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [switch]$ForestDeliveryOnly,
    [switch]$ForestStages,
    [switch]$ForestDropLateJoin,
    [switch]$ForestTrunkLateJoin,
    [switch]$ForestPickup,
    [string]$Project,
    [int]$Port = 17849
)
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot

$ErrorActionPreference = 'Stop'
if ($ForestStages -and !$ForestDeliveryOnly) { throw '-ForestStages requires -ForestDeliveryOnly.' }
if ($ForestDropLateJoin -and !$ForestStages) { throw '-ForestDropLateJoin requires -ForestStages.' }
if ($ForestTrunkLateJoin -and (!$ForestStages -or $ForestDropLateJoin)) { throw '-ForestTrunkLateJoin requires -ForestStages and excludes -ForestDropLateJoin.' }
if ($ForestPickup -and (!$ForestStages -or $ForestDropLateJoin -or $ForestTrunkLateJoin)) { throw '-ForestPickup requires -ForestStages and excludes late-join modes.' }
if (!$Project) { $Project = Join-Path ($repoPaths.ProjectRoot) 'Kalmala.uproject' }
$project = (Resolve-Path -LiteralPath $Project).Path
if ($ForestDeliveryOnly) {
    $output = Join-Path $env:TEMP ('kfpeer-' + [guid]::NewGuid().ToString('N').Substring(0, 8))
    if ((Join-Path $output 'Client/Saved/SaveGames/KalmalaPlayerDiscoveries_10323456789_ffffffff.sav').Length -ge 260) { throw 'Forest peer output exceeds MAX_PATH.' }
    if (Test-Path -LiteralPath $output) { throw 'Forest peers require a fresh output root.' }
    New-Item -ItemType Directory -Path $output | Out-Null
    $serverLog = Join-Path $output 'host.log'
    $clientLog = Join-Path $output 'client.log'
    $common = '-game -nullrhi -nosound -unattended -nosplash -NoZenAutoLaunch -DDC=NoZenLocalFallback -forcelogflush -KalmalaForestDeliveryTest -ExecCmds="t.MaxFPS 30"'
    if ($ForestStages) { $common += ' -KalmalaForestStageDriver' }
    if ($ForestTrunkLateJoin) { $common += ' -KalmalaForestHoldTrunk' }
    if ($ForestPickup) { $common += ' -KalmalaForestPickup' }
    $server = $null
    $client = $null
    $lateClient = $null
    try {
        $server = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=10323456789 $common -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
        $deadline = (Get-Date).AddSeconds(120)
        do {
            if ($server.HasExited) { throw 'Forest host exited during startup.' }
            if ((Test-Path $serverLog) -and (Select-String -LiteralPath $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
            Start-Sleep -Milliseconds 500
        } while ((Get-Date) -lt $deadline)
        if ((Get-Date) -ge $deadline) { throw 'Forest host readiness timed out.' }
        $client = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
        $deadline = (Get-Date).AddSeconds(120)
        do {
            if ($server.HasExited -or $client.HasExited) { throw 'A forest peer exited before delivery.' }
            $hostText = if (Test-Path $serverLog) { [string](Get-Content $serverLog -Raw) } else { '' }
            $clientText = if (Test-Path $clientLog) { [string](Get-Content $clientLog -Raw) } else { '' }
            if (($hostText + $clientText) -match 'Fatal error:|Assertion failed:|Forest delivery: Passed=0') { throw 'Forest peer failure; inspect logs.' }
            $matches = [regex]::Matches($hostText, 'Forest delivery: Passed=1 Authority=1 (Seed=10323456789 Trees=[1-9][0-9]* Cosmetics=0 Ids=[^\r\n]+)')
            # This fixed-seed/start-location fixture has nine populated patches,
            # each with eighteen trees. Do not accept a single delivered patch
            # or count duplicate log lines as additional patch coverage.
            $shared = @($matches | Where-Object {
                $_.Groups[1].Value -match '^Seed=10323456789 Trees=18 Cosmetics=0 '
            } | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique | Where-Object {
                $clientText.Contains('Forest delivery: Passed=1 Authority=0 ' + $_)
            })
            # The fixture reads GameState's seed itself. An identity OnRep log
            # is not guaranteed when the server seed equals its class default.
            # Require both stages for the same exact tree on both peers; a
            # trunk from one tree and a drop from another cannot satisfy this.
            $sharedStages = @([regex]::Matches($hostText, 'Forest stage delivery: Passed=1 Authority=1 Seed=10323456789 Id=([^\s]+) Stage=Trunk Cosmetics=0') |
                ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique | Where-Object {
                    $id = $_
                    $clientText.Contains("Forest stage delivery: Passed=1 Authority=0 Seed=10323456789 Id=$id Stage=Trunk Cosmetics=0") -and
                    ($ForestTrunkLateJoin -or (
                        $hostText.Contains("Forest stage delivery: Passed=1 Authority=1 Seed=10323456789 Id=$id Stage=Drop Cosmetics=0") -and
                        $clientText.Contains("Forest stage delivery: Passed=1 Authority=0 Seed=10323456789 Id=$id Stage=Drop Cosmetics=0")))
                })
            $sharedPickups = @([regex]::Matches($clientText, 'Forest pickup delivery: Passed=1 Authority=0 Seed=10323456789 Id=([^\s]+) Item=([^\s]+) Delta=([1-9][0-9]*) Expected=([1-9][0-9]*) Requests=3 DepletedRequests=3 Stable=5(?:\r?\n|$)') |
                Where-Object {
                    $id = $_.Groups[1].Value
                    $item = $_.Groups[2].Value
                    $delta = $_.Groups[3].Value
                    $expected = $_.Groups[4].Value
                    $_.Groups[3].Value -eq $_.Groups[4].Value -and $sharedStages -contains $id -and
                    $hostText.Contains("Forest forged rejection: Passed=1 Authority=1 Id=$id Item=ForestForgedItem Action=255 Delta=0 Standing=1 Requests=3 HealthUnchanged=1") -and
                    $hostText.Contains("Forest remote range rejection: Passed=1 Authority=1 Id=$id Item=$item Delta=0 Standing=1 HealthUnchanged=1 TraceMiss=1 Requests=3") -and
                    $hostText.Contains("Forest remote range stability: Passed=1 Authority=1 Seed=10323456789 Id=$id Item=$item Delta=0 Standing=1 HealthUnchanged=1 Requests=3 Stable=5") -and
                    $hostText.Contains("Forest remote drop range rejection: Passed=1 Authority=1 Id=$id Item=$item Delta=0 LiveDrop=1 TraceMiss=1 Requests=3") -and
                    $hostText.Contains("Forest remote drop range stability: Passed=1 Authority=1 Seed=10323456789 Id=$id Item=$item Delta=0 LiveDrop=1 Requests=3 Stable=5") -and
                    $hostText.Contains("Forest range rejection: Passed=1 Authority=1 Seed=10323456789 Id=$id Item=$item Delta=0 LiveDrop=1") -and
                    $hostText.Contains("Forest physical rejection: Passed=1 Authority=1 Seed=10323456789 Id=$id Item=$item Delta=0 LiveDrop=1 ForgedTrace=0") -and
                    @([regex]::Matches($hostText, "Forest sparse write: Authority=1 Seed=10323456789 Id=$([regex]::Escape($id)) Success=[01](?![0-9])")).Count -eq 1 -and
                    $hostText.Contains("Forest sparse write: Authority=1 Seed=10323456789 Id=$id Success=1") -and
                    $hostText.Contains("Forest second collector: Passed=1 Authority=1 Seed=10323456789 Id=$id Item=$item Delta=0 Requests=3 Stable=5") -and
                    $hostText.Contains("Forest pickup authority: Passed=1 Authority=1 Seed=10323456789 Id=$id Item=$item Delta=$delta Expected=$expected Stable=5") -and
                    $hostText.Contains("Forest depletion delivery: Passed=1 Authority=1 Seed=10323456789 Id=$id Collected=1") -and
                    $clientText.Contains("Forest depletion delivery: Passed=1 Authority=0 Seed=10323456789 Id=$id Collected=1")
                } | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique)
            if ($shared.Count -eq 9 -and (!$ForestStages -or $sharedStages.Count -gt 0) -and (!$ForestPickup -or $sharedPickups.Count -gt 0)) { break }
            Start-Sleep -Milliseconds 500
        } while ((Get-Date) -lt $deadline)
        if ((Get-Date) -ge $deadline) { throw "Exact forest replacement/suppression delivery timed out: $($shared.Count)/9 matching eighteen-tree patches; $($sharedStages.Count) shared Trunk/Drop IDs (required=$ForestStages); $($sharedPickups.Count) pickups (required=$ForestPickup). Inspect $output." }
        if ($ForestPickup) { Write-Output "PASS: exact-ID pickup quantity stable after three initial and three post-depletion owned requests, with collected state on both peers and zero reward for three second-player depleted attempts: $($sharedPickups -join ', ')." }
        if ($ForestStages) { Write-Output "PASS: host/client required stage delivery for exact tree IDs: $($sharedStages -join ', ')." }
        if ($ForestDropLateJoin -or $ForestTrunkLateJoin) {
            $lateStage = if ($ForestTrunkLateJoin) { 'Trunk' } else { 'Drop' }
            # Join only after both original peers observed this exact stage.
            # The observer does not need standing history; no driver on this peer.
            $lateId = $sharedStages[0]
            $lateLog = Join-Path $output 'late.log'
            $lateCommon = $common.Replace(' -KalmalaForestStageDriver', '').Replace(' -KalmalaForestHoldTrunk', '')
            $lateClient = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $lateCommon -abslog=`"$lateLog`" -UserDir=`"$output\Late`""
            $deadline = (Get-Date).AddSeconds(120)
            do {
                if ($server.HasExited -or $client.HasExited -or $lateClient.HasExited) { throw 'A forest peer exited during late join.' }
                $hostText = [string](Get-Content $serverLog -Raw)
                $clientText = [string](Get-Content $clientLog -Raw)
                $lateText = if (Test-Path $lateLog) { [string](Get-Content $lateLog -Raw) } else { '' }
                if (($hostText + $clientText + $lateText) -match 'Fatal error:|Assertion failed:|Forest delivery: Passed=0') { throw "Forest late-join failure; inspect $output." }
                $lateStageDelivered = $lateText.Contains("Forest stage delivery: Passed=1 Authority=0 Seed=10323456789 Id=$lateId Stage=$lateStage Cosmetics=0")
                if ($lateStageDelivered) { break }
                Start-Sleep -Milliseconds 500
            } while ((Get-Date) -lt $deadline)
            if (!$lateStageDelivered) { throw "Forest $lateStage late join timed out for $lateId; inspect $output." }
            Write-Output "PASS: new peer joined after $lateStage and received exact tree $lateId with zero cosmetics."
        }
        Write-Output "PASS: $($shared.Count) populated forest patches match host/client exact IDs, transforms, shape, source, standing state and zero cosmetics."
    }
    finally {
        foreach ($peer in @($lateClient, $client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
        Write-Output "Forest peer logs: $output"
    }
    return
}
$editorCmd = Join-Path (Split-Path $Editor) 'UnrealEditor-Cmd.exe'
$output = Join-Path $env:TEMP ('KalmalaBiomeExpansion-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$automationLog = Join-Path $output 'automation.log'
$automation = $null
try {
    $automation = Start-Process $editorCmd -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -ExecCmds=`"Automation RunTests Kalmala.World.BiomeExpansion.IntegratedScenario`" -TestExit=`"Automation Test Queue Empty`" -abslog=`"$automationLog`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        $automationText = if (Test-Path $automationLog) { Get-Content $automationLog -Raw } else { '' }
        if ($automationText -match 'Result=\{Success\}.*IntegratedScenario') { break }
        if ($automationText -match 'No automation tests matched|IntegratedScenario.*(Fail|Error)') { throw 'Integrated biome automation failed; inspect its log.' }
        if ($automation.HasExited) { throw 'Integrated biome automation exited before completing.' }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Integrated biome automation timed out.' }
    $automationText = Get-Content $automationLog -Raw
    if ($automationText -match 'Fatal error:|Assertion failed:') { throw 'Integrated biome automation reported a fatal error.' }
    & (Join-Path $repoPaths.TestsRoot 'Survival\Verify-CampChoices.ps1') -Editor $Editor
    if ($LASTEXITCODE -ne 0) { throw "Two-player camp scenario exited with code $LASTEXITCODE." }
    Write-Output 'PASS: all biome deterministic seam/discovery/shelter checks and two-player replicated camp recovery passed.'
}
finally {
    if ($null -ne $automation -and !$automation.HasExited) { Stop-Process -Id $automation.Id }
    Write-Output "Biome verification log: $output"
}
