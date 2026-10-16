param(
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [switch]$ForestDeliveryOnly,
    [string]$Project,
    [int]$Port = 17849
)
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot

$ErrorActionPreference = 'Stop'
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
    $server = $null
    $client = $null
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
            if ($shared.Count -eq 9) { break }
            Start-Sleep -Milliseconds 500
        } while ((Get-Date) -lt $deadline)
        if ((Get-Date) -ge $deadline) { throw "Exact forest replacement/suppression delivery timed out: $($shared.Count)/9 matching eighteen-tree patches. Inspect $output." }
        Write-Output "PASS: $($shared.Count) populated forest patches match host/client exact IDs, transforms, shape, source, standing state and zero cosmetics."
    }
    finally {
        foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
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
