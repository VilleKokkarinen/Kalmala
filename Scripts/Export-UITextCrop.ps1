param(
    [Parameter(Mandatory)][string]$Source,
    [Parameter(Mandatory)][string]$Output,
    [Parameter(Mandatory)][int]$X,
    [Parameter(Mandatory)][int]$Y,
    [Parameter(Mandatory)][int]$Width,
    [Parameter(Mandatory)][int]$Height,
    [ValidateRange(1, 8)][int]$Scale = 4
)
$ErrorActionPreference = 'Stop'
# A review aid only: copy source pixels into integer-sized blocks, without smoothing.
# The original capture remains the evidence; enlargement does not establish acceptance.
Add-Type -AssemblyName System.Drawing
$sourcePath = (Resolve-Path -LiteralPath $Source).Path
$outputPath = [IO.Path]::GetFullPath($Output)
if ($sourcePath.Length -ge 260 -or $outputPath.Length -ge 260) { throw 'Crop path exceeds MAX_PATH.' }
if ($sourcePath -eq $outputPath -or (Test-Path -LiteralPath $outputPath)) { throw 'Output must be a new file.' }
if ([IO.Path]::GetExtension($outputPath) -ne '.png') { throw 'Output must be PNG.' }
$inputHash = (Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256).Hash
$image = $null
$crop = $null
$enlarged = $null
try {
    $image = [Drawing.Bitmap]::new($sourcePath)
    if ($X -lt 0 -or $Y -lt 0 -or $Width -le 0 -or $Height -le 0 `
        -or [long]$X + $Width -gt $image.Width -or [long]$Y + $Height -gt $image.Height) {
        throw 'Crop rectangle must lie within the source capture.'
    }
    if ([long]$Width * $Height * $Scale * $Scale -gt 16777216) { throw 'Review crop exceeds 16 million pixels.' }
    $crop = $image.Clone([Drawing.Rectangle]::new($X, $Y, $Width, $Height), [Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $enlarged = [Drawing.Bitmap]::new($Width * $Scale, $Height * $Scale)
    # Explicit pixel copies avoid interpolation, half-pixel offsets and font reconstruction.
    for ($row = 0; $row -lt $Height; $row++) {
        for ($column = 0; $column -lt $Width; $column++) {
            $pixel = $crop.GetPixel($column, $row)
            for ($dy = 0; $dy -lt $Scale; $dy++) {
                for ($dx = 0; $dx -lt $Scale; $dx++) {
                    $enlarged.SetPixel($column * $Scale + $dx, $row * $Scale + $dy, $pixel)
                }
            }
        }
    }
    $enlarged.Save($outputPath, [Drawing.Imaging.ImageFormat]::Png)
}
finally {
    foreach ($bitmap in @($enlarged, $crop, $image)) { if ($null -ne $bitmap) { $bitmap.Dispose() } }
}
if ((Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256).Hash -ne $inputHash) { throw 'Source capture changed during review.' }
[pscustomobject]@{
    Source = $sourcePath; SHA256 = $inputHash; Output = $outputPath
    X = $X; Y = $Y; Width = $Width; Height = $Height; Scale = $Scale
}
