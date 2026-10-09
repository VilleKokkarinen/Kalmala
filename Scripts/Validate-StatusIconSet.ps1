$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$manifestPath = Join-Path $repoRoot 'docs\status-icon-manifest.csv'
$lookupPath = Join-Path $repoRoot 'Source\KalmalaUI\Private\KalmalaStatusIconLibrary.cpp'
$assetDir = Join-Path $repoRoot 'Content\Kalmala\UI\Icons\Status'
$rows = @(Import-Csv -LiteralPath $manifestPath)
if ($rows.Count -ne 9) {
    throw "Expected nine pinned status identities; found $($rows.Count)."
}

$expectedBatchCounts = @{ '01' = 4; '02' = 4; '03' = 1 }
foreach ($batch in $expectedBatchCounts.Keys) {
    $count = @($rows | Where-Object { $_.batch.Trim() -eq $batch }).Count
    if ($count -ne $expectedBatchCounts[$batch]) {
        throw "Batch $batch has $count rows instead of $($expectedBatchCounts[$batch])."
    }
}

$entryIds = @($rows | ForEach-Object { $_.entry_id.Trim() })
$iconIds = @($rows | ForEach-Object { $_.icon_id.Trim() })
if (@($entryIds | Select-Object -Unique).Count -ne 9 -or @($iconIds | Select-Object -Unique).Count -ne 9) {
    throw 'Status entry IDs and image IDs must each be unique.'
}
$lookupSource = [IO.File]::ReadAllText($lookupPath)
if (!$lookupSource.Contains('/Game/Kalmala/UI/Icons/Status/%s.%s')) {
    throw 'The runtime texture path does not match the imported status package directory.'
}

foreach ($row in $rows) {
    $entryId = $row.entry_id.Trim()
    $iconId = $row.icon_id.Trim()
    $assetPath = Join-Path $assetDir "$iconId.uasset"
    if ($assetPath.Length -ge 260) {
        throw "MAX_PATH violation: $assetPath"
    }
    if (!(Test-Path -LiteralPath $assetPath -PathType Leaf)) {
        throw "Imported Texture2D package is missing for $entryId -> $iconId."
    }
    $mappingNeedle = 'TEXT("' + $entryId + '"), TEXT("' + $iconId + '")'
    if (!$lookupSource.Contains($mappingNeedle)) {
        throw "Runtime map is missing $entryId -> $iconId."
    }
    & (Join-Path $PSScriptRoot 'Validate-StatusIcon.ps1') -Id $iconId
}

$packages = @(Get-ChildItem -LiteralPath $assetDir -Filter '*.uasset' -File)
if ($packages.Count -ne 9) {
    throw "Expected exactly nine imported status textures; found $($packages.Count)."
}
foreach ($package in $packages) {
    if ($package.BaseName -notin $iconIds) {
        throw "Unexpected status texture package: $($package.Name)"
    }
}
Write-Output 'PASS: all nine manifest entries map to unique imported Texture2D packages and validated transparent PNGs.'
