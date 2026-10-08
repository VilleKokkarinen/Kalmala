param(
    [Parameter(Mandatory=$true)]
    [ValidatePattern('^[A-Za-z][A-Za-z0-9]*$')]
    [string]$Id
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$manifestPath = Join-Path $repoRoot 'docs/catalogue-icon-manifest.csv'
$manifestRows = @(Import-Csv -LiteralPath $manifestPath)
$identityRows = @($manifestRows | Where-Object { $_.canonical_id -ceq $Id })
if ($identityRows.Count -ne 1) {
    throw "Expected exactly one pinned manifest row for canonical ID '$Id'; found $($identityRows.Count)."
}

$sourcePath = Join-Path $repoRoot "Content/Kalmala/UI/Source/IconOriginals/$Id.png"
$preparedPath = Join-Path $repoRoot "Content/Kalmala/UI/Source/Icons/$Id.png"
$importTargetPath = Join-Path $repoRoot "Content/Kalmala/UI/Icons/Items/$Id.uasset"
foreach ($path in @($manifestPath, $sourcePath, $preparedPath, $importTargetPath)) {
    if ($path.Length -ge 260) {
        throw "Path must be shorter than 260 characters: $path"
    }
}

function Get-PngInfo([string]$Path, [bool]$bRequire64) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Missing icon file: $Path"
    }

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 26 -or [System.Convert]::ToBase64String($bytes[0..7]) -ne 'iVBORw0KGgo=') {
        throw "Not a valid PNG signature: $Path"
    }

    $width = [uint32](([int64]$bytes[16] -shl 24) -bor ([int64]$bytes[17] -shl 16) -bor ([int64]$bytes[18] -shl 8) -bor [int64]$bytes[19])
    $height = [uint32](([int64]$bytes[20] -shl 24) -bor ([int64]$bytes[21] -shl 16) -bor ([int64]$bytes[22] -shl 8) -bor [int64]$bytes[23])
    if ($bytes[24] -ne 8 -or $bytes[25] -ne 6) {
        throw "Expected 8-bit RGBA PNG color type 6: $Path"
    }
    if ($bRequire64 -and ($width -ne 64 -or $height -ne 64)) {
        throw "Prepared icon must be exactly 64x64: $Path ($width x $height)"
    }
    if (-not $bRequire64 -and ($width -le 64 -or $height -le 64)) {
        throw "Retained original must be larger than 64x64: $Path ($width x $height)"
    }

    Add-Type -AssemblyName System.Drawing
    $bitmap = [System.Drawing.Bitmap]::new($Path)
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

$originalInfo = Get-PngInfo -Path $sourcePath -bRequire64 $false
$preparedInfo = Get-PngInfo -Path $preparedPath -bRequire64 $true
Write-Output "Validated $Id source=$($originalInfo.Width)x$($originalInfo.Height) RGBA final=$($preparedInfo.Width)x$($preparedInfo.Height) RGBA transparent=1 target=/Game/Kalmala/UI/Icons/Items/$Id.$Id"
