[CmdletBinding()]
param(
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [int]$Port = 18306,
    [switch]$WeatherExposureOnly
)

$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$output = Join-Path $env:TEMP ('KalmalaAmbientAudioPeers-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$common = '-game -nullrhi -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaAmbientAudioTest'
$server = $null
$client = $null
try {
    $server = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited) { throw 'Listen server exited during ambient-audio startup.' }
        if ((Test-Path $serverLog) -and (Select-String -LiteralPath $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Ambient-audio listen server startup timed out.' }

    $client = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'An ambient-audio peer exited before verification.' }
        $serverText = if (Test-Path $serverLog) { Get-Content -LiteralPath $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content -LiteralPath $clientLog -Raw } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:') { throw 'Ambient-audio host/client verification failed; inspect the retained logs.' }
        $serverStarted = $serverText -match 'Ambient audio runtime: ComponentCreated=1 Local=1 Asset=WindBed Looping=1'
        $clientStarted = $clientText -match 'Ambient audio runtime: ComponentCreated=1 Local=1 Asset=WindBed Looping=1'
        $serverWaterProbed = $serverText -match 'Ambient audio water context: Probed=1 Local=1 Visible=[01] ComponentCreated=[01] Asset=(WaterBed|None)'
        $clientWaterProbed = $clientText -match 'Ambient audio water context: Probed=1 Local=1 Visible=[01] ComponentCreated=[01] Asset=(WaterBed|None)'
        $serverWaterActive = $serverText -match 'Ambient audio water context: Probed=1 Local=1 Visible=1 ComponentCreated=1 Asset=WaterBed'
        $clientWaterActive = $clientText -match 'Ambient audio water context: Probed=1 Local=1 Visible=1 ComponentCreated=1 Asset=WaterBed'
        $serverFireProbed = $serverText -match 'Ambient audio fire context: Probed=1 Local=1 Visible=[01] ComponentCreated=[01] Asset=(FireBed|None)'
        $clientFireProbed = $clientText -match 'Ambient audio fire context: Probed=1 Local=1 Visible=[01] ComponentCreated=[01] Asset=(FireBed|None)'
        $serverFireActive = $serverText -match 'Ambient audio fire context: Probed=1 Local=1 Visible=1 ComponentCreated=1 Asset=FireBed'
        $clientFireActive = $clientText -match 'Ambient audio fire context: Probed=1 Local=1 Visible=1 ComponentCreated=1 Asset=FireBed'
        $serverBiomeActive = $serverText -match 'Ambient audio biome context: Probed=1 Local=1 Biome=[A-Za-z]+ ComponentCreated=1 Asset=BiomeBed Pitch=[0-9]+\.[0-9]+'
        $clientBiomeActive = $clientText -match 'Ambient audio biome context: Probed=1 Local=1 Biome=[A-Za-z]+ ComponentCreated=1 Asset=BiomeBed Pitch=[0-9]+\.[0-9]+'
        $serverWeatherActive = $serverText -match 'Ambient audio weather context: Local=1 Precipitation=0\.04 WindStrength=0\.80 RainComponentCreated=1 Asset=RainBed'
        $clientWeatherActive = $clientText -match 'Ambient audio weather context: Local=1 Precipitation=0\.04 WindStrength=0\.80 RainComponentCreated=1 Asset=RainBed'
        $serverWetCueActive = $serverText -match 'Ambient audio exposure context: Local=1 Wet=1 CueSubmitted=1 Asset=WetStatusCue'
        $clientWetCueActive = $clientText -match 'Ambient audio exposure context: Local=1 Wet=1 CueSubmitted=1 Asset=WetStatusCue'
        $serverSupportAcceptedCount = [regex]::Matches($serverText, 'Ambient audio verification server support result: Learned=1 Accepted=1 Serial=1').Count
        $serverSupportAccepted = $serverSupportAcceptedCount -ge 2
        $serverSupportCueActive = $serverText -match 'Ambient audio support context: Local=1 Feedback=Accepted Serial=1 CueSubmitted=1 Asset=SupportAcceptedCue'
        $clientSupportCueActive = $clientText -match 'Ambient audio support context: Local=1 Feedback=Accepted Serial=1 CueSubmitted=1 Asset=SupportAcceptedCue'
        $serverInteractionAccepted = $serverText -match 'Ambient audio interaction result: Local=1 Feedback=Accepted Serial=1 CueSubmitted=1 Asset=InteractionAcceptedCue'
        $clientInteractionAccepted = $clientText -match 'Ambient audio interaction result: Local=1 Feedback=Accepted Serial=1 CueSubmitted=1 Asset=InteractionAcceptedCue'
        $serverGatheringAccepted = $serverText -match 'Ambient audio gathering result: Local=1 InventoryIncrease=1 CueSubmitted=1 Asset=InteractionAcceptedCue'
        $clientGatheringAccepted = $clientText -match 'Ambient audio gathering result: Local=1 InventoryIncrease=1 CueSubmitted=1 Asset=InteractionAcceptedCue'
        $serverInteractionRejected = $serverText -match 'Ambient audio interaction result: Local=1 Feedback=Unavailable Serial=2 CueSubmitted=1 Asset=InteractionRejectedCue'
        $clientInteractionRejected = $clientText -match 'Ambient audio interaction result: Local=1 Feedback=Unavailable Serial=2 CueSubmitted=1 Asset=InteractionRejectedCue'
        $testCraftAcceptedCount = [regex]::Matches($serverText, 'Ambient audio verification server crafted fuel: Accepted=1 ResultSerial=1').Count
        $testGatherAcceptedCount = [regex]::Matches($serverText, 'Ambient audio verification server gathered: Accepted=1 DuplicateRejected=1').Count
        $testCraftRejectedCount = [regex]::Matches($serverText, 'Ambient audio verification server rejected craft: Rejected=1 ResultSerial=2').Count
        $interactionFixturePassed = $testCraftAcceptedCount -ge 2 -and $testGatherAcceptedCount -ge 2 -and $testCraftRejectedCount -ge 2
        $testHearthSpawned = $serverText -match 'Ambient audio verification server spawned lit hearth: Lit=1'
        $wetStatusApplied = $serverText -match 'Ambient audio verification server applied Wet status: Wet=1'
        $clientJoined = $clientText -match 'Client received world-generation identity: Seed=418'
        $waterActive = $serverWaterActive -or $clientWaterActive
        $waterRequirementSatisfied = $WeatherExposureOnly -or $waterActive
        $fireActive = $serverFireActive -and $clientFireActive
        $scenarioReady = $serverStarted -and $clientStarted -and $serverWaterProbed -and $clientWaterProbed `
            -and $waterRequirementSatisfied `
            -and $serverFireProbed -and $clientFireProbed -and $testHearthSpawned -and $fireActive `
            -and $serverBiomeActive -and $clientBiomeActive `
            -and $serverWeatherActive -and $clientWeatherActive -and $wetStatusApplied `
            -and $serverWetCueActive -and $clientWetCueActive `
            -and $serverSupportAccepted `
            -and $serverSupportCueActive -and $clientSupportCueActive `
            -and $interactionFixturePassed `
            -and $serverInteractionAccepted -and $clientInteractionAccepted `
            -and $serverGatheringAccepted -and $clientGatheringAccepted `
            -and $serverInteractionRejected -and $clientInteractionRejected -and $clientJoined
        if ($scenarioReady) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) {
        if ($WeatherExposureOnly) { throw 'Local wind/fire/biome/weather cues, Wet/support/interaction/gathering feedback, or connected client identity was not observed before timeout.' }
        throw 'Local wind, visible water/fire, sampled-biome/weather cues, Wet/support/interaction/gathering feedback, or connected client identity was not observed before timeout.'
    }

    if ($WeatherExposureOnly) {
        Write-Output 'PASS: host and client created local ambient cues and owner-local Wet, support, accepted crafting, gathered-item, and rejected crafting cues; audio stayed local.'
    }
    else {
        Write-Output 'PASS: host and client created local ambience from visible water/hearth context, sampled biome, weather, and owner-local interaction/gathering results; all cues stayed local without audio replication.'
    }
}
finally {
    foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    Write-Output "Scenario logs: $output"
}
