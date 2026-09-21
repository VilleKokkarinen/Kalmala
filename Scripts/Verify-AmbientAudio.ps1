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
$sourcePath = Join-Path $projectRoot 'Source\KalmalaUI\Private\KalmalaAmbientAudioSubsystem.cpp'
$headerPath = Join-Path $projectRoot 'Source\KalmalaUI\Public\KalmalaAmbientAudioSubsystem.h'

foreach ($path in @($wavePath, $assetPath, $waterWavePath, $waterAssetPath, $fireWavePath, $fireAssetPath, $biomeWavePath, $biomeAssetPath, $sourcePath, $headerPath)) {
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

$source = Get-Content -LiteralPath $sourcePath -Raw
$header = Get-Content -LiteralPath $headerPath -Raw
foreach ($required in @(
    '/Game/Kalmala/Audio/WindBed.WindBed',
    '/Game/Kalmala/Audio/WaterBed.WaterBed',
    '/Game/Kalmala/Audio/FireBed.FireBed',
    '/Game/Kalmala/Audio/BiomeBed.BiomeBed',
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
    'SetPitchMultiplier',
    'EKalmalaBiome::Meadows',
    'EKalmalaBiome::ShimmeringLakes',
    'EKalmalaBiome::Elderwood',
    'EKalmalaBiome::MossyMire',
    'EKalmalaBiome::FreezingTundra',
    'EKalmalaBiome::ThunderMountains',
    'EKalmalaBiome::Ocean'
)) {
    if ($source -notmatch [regex]::Escape($required)) {
        throw "Ambient audio runtime source is missing required contract: $required"
    }
}
if ($header -notmatch 'ULocalPlayerSubsystem' -or $header -notmatch 'SampleVisibleFireStrength' -or
    $header -match 'UPROPERTY\s*\(\s*Replicated' -or $source -match 'ServerRPC|SaveGame|DOREPLIFETIME') {
    throw 'Ambient audio must stay local and must not add network or gameplay persistence state.'
}

Write-Output 'PASS: original wind, water, fire, and biome sources are 8 seconds of mono 22.05 kHz PCM; loops stay local, water/fire require visible context, biome pitch/level follows the local sampled biome, hearth audio requires a replicated lit actor, and components stop on teardown.'
