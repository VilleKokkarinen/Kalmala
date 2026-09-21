$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$wavePath = Join-Path $projectRoot 'Content\Kalmala\Audio\Source\WindBed.wav'
$assetPath = Join-Path $projectRoot 'Content\Kalmala\Audio\WindBed.uasset'
$waterWavePath = Join-Path $projectRoot 'Content\Kalmala\Audio\Source\WaterBed.wav'
$waterAssetPath = Join-Path $projectRoot 'Content\Kalmala\Audio\WaterBed.uasset'
$fireWavePath = Join-Path $projectRoot 'Content\Kalmala\Audio\Source\FireBed.wav'
$fireAssetPath = Join-Path $projectRoot 'Content\Kalmala\Audio\FireBed.uasset'
$biomeWavePath = Join-Path $projectRoot 'Content\Kalmala\Audio\Source\BiomeBed.wav'
$biomeAssetPath = Join-Path $projectRoot 'Content\Kalmala\Audio\BiomeBed.uasset'
$rainWavePath = Join-Path $projectRoot 'Content\Kalmala\Audio\Source\RainBed.wav'
$rainAssetPath = Join-Path $projectRoot 'Content\Kalmala\Audio\RainBed.uasset'
$wetCueWavePath = Join-Path $projectRoot 'Content\Kalmala\Audio\Source\WetStatusCue.wav'
$wetCueAssetPath = Join-Path $projectRoot 'Content\Kalmala\Audio\WetStatusCue.uasset'
$supportCueWavePath = Join-Path $projectRoot 'Content\Kalmala\Audio\Source\SupportAcceptedCue.wav'
$supportCueAssetPath = Join-Path $projectRoot 'Content\Kalmala\Audio\SupportAcceptedCue.uasset'
$combatCueWavePath = Join-Path $projectRoot 'Content\Kalmala\Audio\Source\CombatResultCue.wav'
$combatCueAssetPath = Join-Path $projectRoot 'Content\Kalmala\Audio\CombatResultCue.uasset'
$discoveryCueWavePath = Join-Path $projectRoot 'Content\Kalmala\Audio\Source\DiscoveryAcknowledgedCue.wav'
$discoveryCueAssetPath = Join-Path $projectRoot 'Content\Kalmala\Audio\DiscoveryAcknowledgedCue.uasset'
$interactionAcceptedCueWavePath = Join-Path $projectRoot 'Content\Kalmala\Audio\Source\InteractionAcceptedCue.wav'
$interactionAcceptedCueAssetPath = Join-Path $projectRoot 'Content\Kalmala\Audio\InteractionAcceptedCue.uasset'
$interactionRejectedCueWavePath = Join-Path $projectRoot 'Content\Kalmala\Audio\Source\InteractionRejectedCue.wav'
$interactionRejectedCueAssetPath = Join-Path $projectRoot 'Content\Kalmala\Audio\InteractionRejectedCue.uasset'
$sourcePath = Join-Path $projectRoot 'Source\KalmalaUI\Private\KalmalaAmbientAudioSubsystem.cpp'
$headerPath = Join-Path $projectRoot 'Source\KalmalaUI\Public\KalmalaAmbientAudioSubsystem.h'
$supportSourcePath = Join-Path $projectRoot 'Source\KalmalaGameplay\Private\KalmalaSupportMagicComponent.cpp'
$combatSourcePath = Join-Path $projectRoot 'Source\KalmalaGameplay\Private\KalmalaCombatComponent.cpp'
$discoverySourcePath = Join-Path $projectRoot 'Source\KalmalaGameplay\Private\KalmalaDiscoveryProgressComponent.cpp'
$craftingSourcePath = Join-Path $projectRoot 'Source\KalmalaGameplay\Private\KalmalaCraftingComponent.cpp'
$inventorySourcePath = Join-Path $projectRoot 'Source\KalmalaGameplay\Private\KalmalaInventoryComponent.cpp'

foreach ($path in @($wavePath, $assetPath, $waterWavePath, $waterAssetPath, $fireWavePath, $fireAssetPath, $biomeWavePath, $biomeAssetPath, $rainWavePath, $rainAssetPath, $wetCueWavePath, $wetCueAssetPath, $supportCueWavePath, $supportCueAssetPath, $combatCueWavePath, $combatCueAssetPath, $discoveryCueWavePath, $discoveryCueAssetPath, $interactionAcceptedCueWavePath, $interactionAcceptedCueAssetPath, $interactionRejectedCueWavePath, $interactionRejectedCueAssetPath, $sourcePath, $headerPath, $supportSourcePath, $combatSourcePath, $discoverySourcePath, $craftingSourcePath, $inventorySourcePath)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Ambient audio deliverable is missing: $path"
    }
}

[byte[]]$bytes = [System.IO.File]::ReadAllBytes($wavePath)
if ($bytes.Length -lt 44 -or [System.Text.Encoding]::ASCII.GetString($bytes, 0, 4) -ne 'RIFF' -or
    [System.Text.Encoding]::ASCII.GetString($bytes, 8, 4) -ne 'WAVE') {
    throw 'WindBed.wav is not a valid RIFF/WAVE file.'
}
$channels = [BitConverter]::ToInt16($bytes, 22)
$sampleRate = [BitConverter]::ToInt32($bytes, 24)
$bitsPerSample = [BitConverter]::ToInt16($bytes, 34)
$dataLength = [BitConverter]::ToInt32($bytes, 40)
if ($channels -ne 1 -or $sampleRate -ne 22050 -or $bitsPerSample -ne 16 -or $dataLength -ne 352800) {
    throw "Unexpected WindBed format: channels=$channels rate=$sampleRate bits=$bitsPerSample bytes=$dataLength"
}

[byte[]]$waterBytes = [System.IO.File]::ReadAllBytes($waterWavePath)
if ($waterBytes.Length -lt 44 -or [System.Text.Encoding]::ASCII.GetString($waterBytes, 0, 4) -ne 'RIFF' -or
    [System.Text.Encoding]::ASCII.GetString($waterBytes, 8, 4) -ne 'WAVE') {
    throw 'WaterBed.wav is not a valid RIFF/WAVE file.'
}
$waterChannels = [BitConverter]::ToInt16($waterBytes, 22)
$waterSampleRate = [BitConverter]::ToInt32($waterBytes, 24)
$waterBitsPerSample = [BitConverter]::ToInt16($waterBytes, 34)
$waterDataLength = [BitConverter]::ToInt32($waterBytes, 40)
if ($waterChannels -ne 1 -or $waterSampleRate -ne 22050 -or $waterBitsPerSample -ne 16 -or $waterDataLength -ne 352800) {
    throw "Unexpected WaterBed format: channels=$waterChannels rate=$waterSampleRate bits=$waterBitsPerSample bytes=$waterDataLength"
}

$fireBytes = [System.IO.File]::ReadAllBytes($fireWavePath)
if ($fireBytes.Length -lt 44 -or [System.Text.Encoding]::ASCII.GetString($fireBytes, 0, 4) -ne 'RIFF' -or
    [System.Text.Encoding]::ASCII.GetString($fireBytes, 8, 4) -ne 'WAVE') {
    throw 'FireBed.wav is not a valid RIFF/WAVE file.'
}
$fireChannels = [BitConverter]::ToInt16($fireBytes, 22)
$fireSampleRate = [BitConverter]::ToInt32($fireBytes, 24)
$fireBitsPerSample = [BitConverter]::ToInt16($fireBytes, 34)
$fireDataLength = [BitConverter]::ToInt32($fireBytes, 40)
if ($fireChannels -ne 1 -or $fireSampleRate -ne 22050 -or $fireBitsPerSample -ne 16 -or $fireDataLength -ne 352800) {
    throw "Unexpected FireBed format: channels=$fireChannels rate=$fireSampleRate bits=$fireBitsPerSample bytes=$fireDataLength"
}

$biomeBytes = [System.IO.File]::ReadAllBytes($biomeWavePath)
if ($biomeBytes.Length -lt 44 -or [System.Text.Encoding]::ASCII.GetString($biomeBytes, 0, 4) -ne 'RIFF' -or
    [System.Text.Encoding]::ASCII.GetString($biomeBytes, 8, 4) -ne 'WAVE') {
    throw 'BiomeBed.wav is not a valid RIFF/WAVE file.'
}
$biomeChannels = [BitConverter]::ToInt16($biomeBytes, 22)
$biomeSampleRate = [BitConverter]::ToInt32($biomeBytes, 24)
$biomeBitsPerSample = [BitConverter]::ToInt16($biomeBytes, 34)
$biomeDataLength = [BitConverter]::ToInt32($biomeBytes, 40)
if ($biomeChannels -ne 1 -or $biomeSampleRate -ne 22050 -or $biomeBitsPerSample -ne 16 -or $biomeDataLength -ne 352800) {
    throw "Unexpected BiomeBed format: channels=$biomeChannels rate=$biomeSampleRate bits=$biomeBitsPerSample bytes=$biomeDataLength"
}

$rainBytes = [System.IO.File]::ReadAllBytes($rainWavePath)
if ($rainBytes.Length -lt 44 -or [System.Text.Encoding]::ASCII.GetString($rainBytes, 0, 4) -ne 'RIFF' -or
    [System.Text.Encoding]::ASCII.GetString($rainBytes, 8, 4) -ne 'WAVE') {
    throw 'RainBed.wav is not a valid RIFF/WAVE file.'
}
$rainChannels = [BitConverter]::ToInt16($rainBytes, 22)
$rainSampleRate = [BitConverter]::ToInt32($rainBytes, 24)
$rainBitsPerSample = [BitConverter]::ToInt16($rainBytes, 34)
$rainDataLength = [BitConverter]::ToInt32($rainBytes, 40)
if ($rainChannels -ne 1 -or $rainSampleRate -ne 22050 -or $rainBitsPerSample -ne 16 -or $rainDataLength -ne 352800) {
    throw "Unexpected RainBed format: channels=$rainChannels rate=$rainSampleRate bits=$rainBitsPerSample bytes=$rainDataLength"
}

$wetCueBytes = [System.IO.File]::ReadAllBytes($wetCueWavePath)
if ($wetCueBytes.Length -lt 44 -or [System.Text.Encoding]::ASCII.GetString($wetCueBytes, 0, 4) -ne 'RIFF' -or
    [System.Text.Encoding]::ASCII.GetString($wetCueBytes, 8, 4) -ne 'WAVE') {
    throw 'WetStatusCue.wav is not a valid RIFF/WAVE file.'
}
$wetCueChannels = [BitConverter]::ToInt16($wetCueBytes, 22)
$wetCueSampleRate = [BitConverter]::ToInt32($wetCueBytes, 24)
$wetCueBitsPerSample = [BitConverter]::ToInt16($wetCueBytes, 34)
$wetCueDataLength = [BitConverter]::ToInt32($wetCueBytes, 40)
if ($wetCueChannels -ne 1 -or $wetCueSampleRate -ne 22050 -or $wetCueBitsPerSample -ne 16 -or $wetCueDataLength -ne 35280) {
    throw "Unexpected WetStatusCue format: channels=$wetCueChannels rate=$wetCueSampleRate bits=$wetCueBitsPerSample bytes=$wetCueDataLength"
}

$supportCueBytes = [System.IO.File]::ReadAllBytes($supportCueWavePath)
if ($supportCueBytes.Length -lt 44 -or [System.Text.Encoding]::ASCII.GetString($supportCueBytes, 0, 4) -ne 'RIFF' -or
    [System.Text.Encoding]::ASCII.GetString($supportCueBytes, 8, 4) -ne 'WAVE') {
    throw 'SupportAcceptedCue.wav is not a valid RIFF/WAVE file.'
}
$supportCueChannels = [BitConverter]::ToInt16($supportCueBytes, 22)
$supportCueSampleRate = [BitConverter]::ToInt32($supportCueBytes, 24)
$supportCueBitsPerSample = [BitConverter]::ToInt16($supportCueBytes, 34)
$supportCueDataLength = [BitConverter]::ToInt32($supportCueBytes, 40)
if ($supportCueChannels -ne 1 -or $supportCueSampleRate -ne 22050 -or $supportCueBitsPerSample -ne 16 -or $supportCueDataLength -ne 35280) {
    throw "Unexpected SupportAcceptedCue format: channels=$supportCueChannels rate=$supportCueSampleRate bits=$supportCueBitsPerSample bytes=$supportCueDataLength"
}

[byte[]]$combatCueBytes = [System.IO.File]::ReadAllBytes($combatCueWavePath)
if ($combatCueBytes.Length -lt 44 -or [System.Text.Encoding]::ASCII.GetString($combatCueBytes, 0, 4) -ne 'RIFF' -or
    [System.Text.Encoding]::ASCII.GetString($combatCueBytes, 8, 4) -ne 'WAVE') {
    throw 'CombatResultCue.wav is not a valid RIFF/WAVE file.'
}
$combatCueChannels = [BitConverter]::ToInt16($combatCueBytes, 22)
$combatCueSampleRate = [BitConverter]::ToInt32($combatCueBytes, 24)
$combatCueBitsPerSample = [BitConverter]::ToInt16($combatCueBytes, 34)
$combatCueDataLength = [BitConverter]::ToInt32($combatCueBytes, 40)
if ($combatCueChannels -ne 1 -or $combatCueSampleRate -ne 22050 -or $combatCueBitsPerSample -ne 16 -or $combatCueDataLength -ne 17640) {
    throw "Unexpected CombatResultCue format: channels=$combatCueChannels rate=$combatCueSampleRate bits=$combatCueBitsPerSample bytes=$combatCueDataLength"
}

[byte[]]$discoveryCueBytes = [System.IO.File]::ReadAllBytes($discoveryCueWavePath)
if ($discoveryCueBytes.Length -lt 44 -or [System.Text.Encoding]::ASCII.GetString($discoveryCueBytes, 0, 4) -ne 'RIFF' -or
    [System.Text.Encoding]::ASCII.GetString($discoveryCueBytes, 8, 4) -ne 'WAVE') {
    throw 'DiscoveryAcknowledgedCue.wav is not a valid RIFF/WAVE file.'
}
$discoveryCueChannels = [BitConverter]::ToInt16($discoveryCueBytes, 22)
$discoveryCueSampleRate = [BitConverter]::ToInt32($discoveryCueBytes, 24)
$discoveryCueBitsPerSample = [BitConverter]::ToInt16($discoveryCueBytes, 34)
$discoveryCueDataLength = [BitConverter]::ToInt32($discoveryCueBytes, 40)
if ($discoveryCueChannels -ne 1 -or $discoveryCueSampleRate -ne 22050 -or $discoveryCueBitsPerSample -ne 16 -or $discoveryCueDataLength -ne 22050) {
    throw "Unexpected DiscoveryAcknowledgedCue format: channels=$discoveryCueChannels rate=$discoveryCueSampleRate bits=$discoveryCueBitsPerSample bytes=$discoveryCueDataLength"
}

foreach ($cue in @(
    @{ Name = 'InteractionAcceptedCue'; Path = $interactionAcceptedCueWavePath },
    @{ Name = 'InteractionRejectedCue'; Path = $interactionRejectedCueWavePath }
)) {
    [byte[]]$cueBytes = [System.IO.File]::ReadAllBytes($cue.Path)
    if ($cueBytes.Length -lt 44 -or [System.Text.Encoding]::ASCII.GetString($cueBytes, 0, 4) -ne 'RIFF' -or
        [System.Text.Encoding]::ASCII.GetString($cueBytes, 8, 4) -ne 'WAVE') {
        throw "$($cue.Name).wav is not a valid RIFF/WAVE file."
    }
    $cueChannels = [BitConverter]::ToInt16($cueBytes, 22)
    $cueSampleRate = [BitConverter]::ToInt32($cueBytes, 24)
    $cueBitsPerSample = [BitConverter]::ToInt16($cueBytes, 34)
    $cueDataLength = [BitConverter]::ToInt32($cueBytes, 40)
    if ($cueChannels -ne 1 -or $cueSampleRate -ne 22050 -or $cueBitsPerSample -ne 16 -or $cueDataLength -ne 17640) {
        throw "Unexpected $($cue.Name) format: channels=$cueChannels rate=$cueSampleRate bits=$cueBitsPerSample bytes=$cueDataLength"
    }
}

$source = Get-Content -LiteralPath $sourcePath -Raw
$header = Get-Content -LiteralPath $headerPath -Raw
$supportSource = Get-Content -LiteralPath $supportSourcePath -Raw
$combatSource = Get-Content -LiteralPath $combatSourcePath -Raw
$discoverySource = Get-Content -LiteralPath $discoverySourcePath -Raw
$craftingSource = Get-Content -LiteralPath $craftingSourcePath -Raw
$inventorySource = Get-Content -LiteralPath $inventorySourcePath -Raw
foreach ($required in @(
    '/Game/Kalmala/Audio/WindBed.WindBed',
    '/Game/Kalmala/Audio/WaterBed.WaterBed',
    '/Game/Kalmala/Audio/FireBed.FireBed',
    '/Game/Kalmala/Audio/BiomeBed.BiomeBed',
    '/Game/Kalmala/Audio/RainBed.RainBed',
    '/Game/Kalmala/Audio/WetStatusCue.WetStatusCue',
    '/Game/Kalmala/Audio/SupportAcceptedCue.SupportAcceptedCue',
    '/Game/Kalmala/Audio/CombatResultCue.CombatResultCue',
    '/Game/Kalmala/Audio/DiscoveryAcknowledgedCue.DiscoveryAcknowledgedCue',
    '/Game/Kalmala/Audio/InteractionAcceptedCue.InteractionAcceptedCue',
    '/Game/Kalmala/Audio/InteractionRejectedCue.InteractionRejectedCue',
    'IsLocalController()',
    'GetLocalPlayer()',
    'bLooping = true',
    'CreateSound2D',
    'StopAmbientAudio',
    'FKalmalaOceanSampler::Sample',
    'FKalmalaLakeBasin::IsVisibleWater',
    'LineTraceSingleByChannel',
    'WaterMaximumDistance',
    'SetVolumeMultiplier',
    'TActorIterator<AKalmalaCampfire>',
    'Hearth->GetIsReplicated()',
    'Hearth->IsLit()',
    'FireMaximumDistance',
    'FireProbeInterval',
    'FireBed->bLooping = true',
    'FKalmalaWorldFieldSampler::Sample',
    'FKalmalaBiomeClassifier::Classify',
    'BiomeProbeInterval',
    'BiomeBed->bLooping = true',
    'Weather.WindStrength',
    'Weather.PrecipitationIntensity',
    'RainMinimumIntensity',
    'RainBed->bLooping = true',
    'GetWeatherState()',
    'UpdateSupportAcceptedCue',
    'GetFeedbackSerial()',
    'EKalmalaSupportFeedback::Accepted',
    'UpdateCombatResultCue',
    'GetCombatComponent()',
    'EKalmalaCombatFeedback::Hit',
    'EKalmalaCombatFeedback::Defeat',
    'UpdateDiscoveryAcknowledgementCue',
    'GetDiscoveryProgressComponent()',
    'EKalmalaDiscoveryFeedback::LandmarkFound',
    'EKalmalaDiscoveryFeedback::ScrollFound',
    'WetStatusId',
    'PlaySound2D',
    'SetPitchMultiplier',
    'EKalmalaBiome::Meadows',
    'EKalmalaBiome::ShimmeringLakes',
    'EKalmalaBiome::Elderwood',
    'EKalmalaBiome::MossyMire',
    'EKalmalaBiome::FreezingTundra',
    'EKalmalaBiome::ThunderMountains',
    'EKalmalaBiome::Ocean',
    'UpdateInteractionResultCue',
    'UpdateGatheringResultCue',
    'GetResultSerial()',
    'WasLastResultAccepted()',
    'LastGatheringQuantities',
    'InteractionCueMinimumInterval',
    'InventoryIncrease=1'
)) {
    if ($source -notmatch [regex]::Escape($required)) {
        throw "Ambient audio runtime source is missing required contract: $required"
    }
}
if ($header -notmatch 'ULocalPlayerSubsystem' -or $header -notmatch 'SampleVisibleFireStrength' -or $header -notmatch 'UpdateWetStatusCue' -or
    $header -notmatch 'UpdateSupportAcceptedCue' -or $header -match 'UPROPERTY\s*\(\s*Replicated' -or
    $source -match 'ServerRPC|SaveGame|DOREPLIFETIME' -or
    $supportSource -notmatch 'DOREPLIFETIME_CONDITION\(UKalmalaSupportMagicComponent, Feedback, COND_OwnerOnly\)' -or
    $supportSource -notmatch 'DOREPLIFETIME_CONDITION\(UKalmalaSupportMagicComponent, FeedbackSerial, COND_OwnerOnly\)' -or
    $header -notmatch 'UpdateCombatResultCue' -or
    $combatSource -notmatch 'DOREPLIFETIME_CONDITION\(UKalmalaCombatComponent, Feedback, COND_OwnerOnly\)' -or
    $combatSource -notmatch 'DOREPLIFETIME_CONDITION\(UKalmalaCombatComponent, FeedbackSerial, COND_OwnerOnly\)' -or
    $discoverySource -notmatch 'DOREPLIFETIME_CONDITION\(UKalmalaDiscoveryProgressComponent, Feedback, COND_OwnerOnly\)' -or
    $discoverySource -notmatch 'DOREPLIFETIME_CONDITION\(UKalmalaDiscoveryProgressComponent, FeedbackSerial, COND_OwnerOnly\)' -or
    $craftingSource -notmatch 'DOREPLIFETIME_CONDITION\(UKalmalaCraftingComponent, LastResult, COND_OwnerOnly\)' -or
    $craftingSource -notmatch 'DOREPLIFETIME_CONDITION\(UKalmalaCraftingComponent, ResultSerial, COND_OwnerOnly\)' -or
    $craftingSource -notmatch 'DOREPLIFETIME_CONDITION\(UKalmalaCraftingComponent, bLastResultAccepted, COND_OwnerOnly\)' -or
    $inventorySource -notmatch 'DOREPLIFETIME_CONDITION\(UKalmalaInventoryComponent, Stacks, COND_OwnerOnly\)') {
    throw 'Ambient audio must stay local and must not add network or gameplay persistence state.'
}

Write-Output 'PASS: original wind, water, fire, biome, and rain beds are 8-second mono PCM; WetStatusCue and SupportAcceptedCue are 0.8 seconds, discovery is 0.5 seconds, and combat/interaction cues are 0.4 seconds. Accepted owner-only landmark/scroll feedback triggers the local discovery acknowledgment.'
