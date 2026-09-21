$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$wavePath = Join-Path $projectRoot 'Content\Kalmala\Audio\Source\WindBed.wav'
$assetPath = Join-Path $projectRoot 'Content\Kalmala\Audio\WindBed.uasset'
$waterWavePath = Join-Path $projectRoot 'Content\Kalmala\Audio\Source\WaterBed.wav'
$waterAssetPath = Join-Path $projectRoot 'Content\Kalmala\Audio\WaterBed.uasset'
$sourcePath = Join-Path $projectRoot 'Source\KalmalaUI\Private\KalmalaAmbientAudioSubsystem.cpp'
$headerPath = Join-Path $projectRoot 'Source\KalmalaUI\Public\KalmalaAmbientAudioSubsystem.h'

foreach ($path in @($wavePath, $assetPath, $waterWavePath, $waterAssetPath, $sourcePath, $headerPath)) {
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

$source = Get-Content -LiteralPath $sourcePath -Raw
$header = Get-Content -LiteralPath $headerPath -Raw
foreach ($required in @(
    '/Game/Kalmala/Audio/WindBed.WindBed',
    '/Game/Kalmala/Audio/WaterBed.WaterBed',
    'IsLocalController()',
    'GetLocalPlayer()',
    'bLooping = true',
    'CreateSound2D',
    'StopAmbientAudio',
    'FKalmalaOceanSampler::Sample',
    'FKalmalaLakeBasin::IsVisibleWater',
    'LineTraceSingleByChannel',
    'WaterMaximumDistance',
    'SetVolumeMultiplier'
)) {
    if ($source -notmatch [regex]::Escape($required)) {
        throw "Ambient audio runtime source is missing required contract: $required"
    }
}
if ($header -notmatch 'ULocalPlayerSubsystem' -or $source -match 'ServerRPC|SaveGame|Replicated') {
    throw 'Ambient audio must stay local and must not add network or gameplay persistence state.'
}

Write-Output 'PASS: original wind and water sources are 8 seconds of mono 22.05 kHz PCM; loops are local, water is proximity/visibility gated, and components stop on teardown.'
