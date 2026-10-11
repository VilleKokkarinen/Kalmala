$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$manifestPath = Join-Path $PSScriptRoot 'Data\catalogue-icon-manifest.csv'
$assetDir = Join-Path $repoRoot 'Content\Kalmala\UI\Icons\Items'
$rows = @(Import-Csv -LiteralPath $manifestPath)
if ($rows.Count -ne 48) { throw "Expected 48 pinned catalogue identities; found $($rows.Count)." }
$ids = @($rows | ForEach-Object { $_.canonical_id.Trim() })
if (@($ids | Select-Object -Unique).Count -ne 48) { throw 'Catalogue canonical IDs must be unique.' }
$packages = @(Get-ChildItem -LiteralPath $assetDir -Filter '*.uasset' -File)
if ($packages.Count -ne 48) { throw "Expected 48 imported catalogue textures; found $($packages.Count)." }
foreach ($id in $ids) {
    $path = Join-Path $assetDir "$id.uasset"
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Imported package missing for canonical ID $id." }
    & (Join-Path $PSScriptRoot 'Validate-CatalogueIcon.ps1') -Id $id
}
foreach ($package in $packages) {
    if ($package.BaseName -notin $ids) { throw "Unexpected catalogue texture package: $($package.Name)" }
}
Write-Output 'PASS: all 48 catalogue identities map to validated originals, prepared PNGs, and imported Texture2D packages.'