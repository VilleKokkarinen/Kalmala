param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Catalogue', 'Status')]
    [string]$Category,

    [ValidatePattern('^[A-Za-z][A-Za-z0-9]*$')]
    [string]$Id
)

$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))

if ($Category -eq 'Catalogue') {
    $manifestPath = Join-Path $PSScriptRoot 'Data\catalogue-icon-manifest.csv'
    $runtimePath = Join-Path $repoRoot 'Source\KalmalaUI\Private\KalmalaIconWidget.cpp'
    $assetDir = Join-Path $repoRoot 'Content\Kalmala\UI\Icons\Items'
    $sourceDir = Join-Path $repoRoot 'Content\Kalmala\UI\Source\IconOriginals'
    $preparedDir = Join-Path $repoRoot 'Content\Kalmala\UI\Source\Icons'
    $idColumn = 'canonical_id'
    $expectedCount = 86
}
else {
    $manifestPath = Join-Path $PSScriptRoot 'Data\status-icon-manifest.csv'
    $runtimePath = Join-Path $repoRoot 'Source\KalmalaUI\Private\KalmalaStatusIconLibrary.cpp'
    $assetDir = Join-Path $repoRoot 'Content\Kalmala\UI\Icons\Status'
    $sourceDir = Join-Path $repoRoot 'Content\Kalmala\UI\Source\IconOriginals\Status'
    $preparedDir = Join-Path $repoRoot 'Content\Kalmala\UI\Source\Icons\Status'
    $idColumn = 'icon_id'
    $expectedCount = 9
}

foreach ($path in @($manifestPath, $runtimePath, $assetDir, $sourceDir, $preparedDir)) {
    if ($path.Length -ge 260) {
        throw "Path must be shorter than 260 characters: $path"
    }
}

$rows = @(Import-Csv -LiteralPath $manifestPath)
$runtimeSource = [IO.File]::ReadAllText($runtimePath)

function Get-PngInfo([string]$Path, [bool]$Require64) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Missing icon file: $Path"
    }

    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 26 -or [Convert]::ToBase64String($bytes[0..7]) -ne 'iVBORw0KGgo=') {
        throw "Not a valid PNG signature: $Path"
    }

    $width = [uint32](([int64]$bytes[16] -shl 24) -bor ([int64]$bytes[17] -shl 16) -bor ([int64]$bytes[18] -shl 8) -bor [int64]$bytes[19])
    $height = [uint32](([int64]$bytes[20] -shl 24) -bor ([int64]$bytes[21] -shl 16) -bor ([int64]$bytes[22] -shl 8) -bor [int64]$bytes[23])
    if ($bytes[24] -ne 8 -or $bytes[25] -ne 6) {
        throw "Expected 8-bit RGBA PNG: $Path"
    }
    if ($Require64 -and ($width -ne 64 -or $height -ne 64)) {
        throw "Prepared icon must be exactly 64x64: $Path ($width x $height)"
    }
    if (-not $Require64 -and ($width -le 64 -or $height -le 64)) {
        throw "Retained original must be larger than 64x64: $Path ($width x $height)"
    }

    Add-Type -AssemblyName System.Drawing
    $bitmap = [Drawing.Bitmap]::new($Path)
    try {
        $hasTransparentPixel = $false
        for ($y = 0; $y -lt $bitmap.Height -and -not $hasTransparentPixel; $y++) {
            for ($x = 0; $x -lt $bitmap.Width; $x++) {
                if ($bitmap.GetPixel($x, $y).A -lt 255) {
                    $hasTransparentPixel = $true
                    break
                }
            }
        }
        if (-not $hasTransparentPixel) {
            throw "Expected at least one transparent pixel: $Path"
        }
    }
    finally {
        $bitmap.Dispose()
    }

    return [pscustomobject]@{ Width = $width; Height = $height }
}

function Test-IconRow($Row, [bool]$RequirePackage) {
    $id = $Row.$idColumn.Trim()
    $assetId = if ($Category -eq 'Catalogue' -and $Row.texture_asset_id -and $Row.texture_asset_id.Trim()) {
        $Row.texture_asset_id.Trim()
    } else { $id }
    if ($Category -eq 'Catalogue' -and -not $runtimeSource.Contains('TEXT("' + $id + '")')) {
        throw "Runtime catalogue mapping is missing canonical ID '$id'."
    }

    $sourcePath = Join-Path $sourceDir "$assetId.png"
    $preparedPath = Join-Path $preparedDir "$assetId.png"
    $packagePath = Join-Path $assetDir "$assetId.uasset"
    foreach ($path in @($sourcePath, $preparedPath, $packagePath)) {
        if ($path.Length -ge 260) {
            throw "Path must be shorter than 260 characters: $path"
        }
    }

    $originalInfo = Get-PngInfo -Path $sourcePath -Require64 $false
    $preparedInfo = Get-PngInfo -Path $preparedPath -Require64 $true
    if ($RequirePackage -and -not (Test-Path -LiteralPath $packagePath -PathType Leaf)) {
        throw "Imported Texture2D package is missing: $packagePath"
    }

    if ($Category -eq 'Catalogue') {
        Write-Output "Validated $id asset=$assetId source=$($originalInfo.Width)x$($originalInfo.Height) RGBA final=$($preparedInfo.Width)x$($preparedInfo.Height) RGBA transparent=1 target=/Game/Kalmala/UI/Icons/Items/$assetId.$assetId"
    }
    else {
        Write-Output "Validated status icon $id entry=$($Row.entry_id) batch=$($Row.batch) source=$($originalInfo.Width)x$($originalInfo.Height) RGBA final=$($preparedInfo.Width)x$($preparedInfo.Height) RGBA transparent=1 target=/Game/Kalmala/UI/Icons/Status/$id.$id"
    }
}

if ($PSBoundParameters.ContainsKey('Id')) {
    $identityRows = @($rows | Where-Object { $_.$idColumn -ceq $Id })
    if ($identityRows.Count -ne 1) {
        throw "Expected exactly one pinned $Category icon row for '$Id'; found $($identityRows.Count)."
    }
    Test-IconRow -Row $identityRows[0] -RequirePackage ($Category -eq 'Catalogue')
    return
}

if ($rows.Count -ne $expectedCount) {
    throw "Expected $expectedCount pinned $Category icon identities; found $($rows.Count)."
}

if ($Category -eq 'Catalogue') {
    $ids = @($rows | ForEach-Object { $_.$idColumn.Trim() })
    if (@($ids | Select-Object -Unique).Count -ne $expectedCount) {
        throw 'Catalogue canonical IDs must be unique.'
    }
    $packageIds = @($rows | ForEach-Object {
        if ($_.texture_asset_id -and $_.texture_asset_id.Trim()) { $_.texture_asset_id.Trim() }
        else { $_.$idColumn.Trim() }
    })
    if (@($packageIds | Select-Object -Unique).Count -ne $expectedCount) {
        throw 'Catalogue texture asset IDs must be unique.'
    }
}
else {
    $expectedBatchCounts = @{ '01' = 4; '02' = 4; '03' = 1 }
    foreach ($batch in $expectedBatchCounts.Keys) {
        $count = @($rows | Where-Object { $_.batch.Trim() -eq $batch }).Count
        if ($count -ne $expectedBatchCounts[$batch]) {
            throw "Status batch $batch has $count rows instead of $($expectedBatchCounts[$batch])."
        }
    }

    $entryIds = @($rows | ForEach-Object { $_.entry_id.Trim() })
    $ids = @($rows | ForEach-Object { $_.$idColumn.Trim() })
    if (@($entryIds | Select-Object -Unique).Count -ne $expectedCount -or @($ids | Select-Object -Unique).Count -ne $expectedCount) {
        throw 'Status entry IDs and image IDs must each be unique.'
    }
    if (-not $runtimeSource.Contains('/Game/Kalmala/UI/Icons/Status/%s.%s')) {
        throw 'The status runtime texture path does not match the imported package directory.'
    }
}

$packages = @(Get-ChildItem -LiteralPath $assetDir -Filter '*.uasset' -File)
if ($Category -eq 'Catalogue') {
    $expectedAssetIds = @($rows | ForEach-Object {
        if ($_.texture_asset_id -and $_.texture_asset_id.Trim()) { $_.texture_asset_id.Trim() }
        else { $_.$idColumn.Trim() }
    })
    $packageIds = @($packages | ForEach-Object { $_.BaseName })
    $missingPackages = @($expectedAssetIds | Where-Object { $_ -notin $packageIds })
    $unlistedPackages = @($packageIds | Where-Object { $_ -notin $expectedAssetIds })
    # Preserve retired Fibre and String packages; neither is a current item or icon-ledger identity.
    $unexpectedPackages = @($unlistedPackages | Where-Object { $_ -notin @('Fibre', 'String') })
    if ($missingPackages.Count -gt 0) {
        throw "Missing imported catalogue textures: $($missingPackages -join ', ')."
    }
    if ($unexpectedPackages.Count -gt 0) {
        throw "Unlisted catalogue texture packages: $($unexpectedPackages -join ', ')."
    }
}
elseif ($packages.Count -ne $expectedCount) {
    throw "Expected $expectedCount imported $Category textures; found $($packages.Count)."
}

foreach ($row in $rows) {
    if ($Category -eq 'Status') {
        $mappingNeedle = 'TEXT("' + $row.entry_id.Trim() + '"), TEXT("' + $row.$idColumn.Trim() + '")'
        if (-not $runtimeSource.Contains($mappingNeedle)) {
            throw "Status runtime map is missing $($row.entry_id) -> $($row.$idColumn)."
        }
    }
    Test-IconRow -Row $row -RequirePackage $true
}

foreach ($package in $packages) {
    $expectedPackageIds = if ($Category -eq 'Catalogue') { $packageIds } else { $ids }
    if ($package.BaseName -notin $expectedPackageIds) {
        throw "Unexpected $Category texture package: $($package.Name)"
    }
}

if ($Category -eq 'Catalogue') {
    Write-Output "PASS: all $expectedCount catalogue identities map to validated originals, prepared PNGs, and imported Texture2D packages; retired Fibre and String packages remain unlisted."
}
else {
    Write-Output 'PASS: all nine manifest entries map to unique imported Texture2D packages and validated transparent PNGs.'
}
