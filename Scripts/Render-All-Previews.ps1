$ErrorActionPreference = 'Stop'

$menuRenderer = Join-Path $PSScriptRoot 'Render-Menu.ps1'
$creatureRenderer = Join-Path $PSScriptRoot 'Render-Creatures.ps1'
$cache = Join-Path $env:TEMP 'kmall'

try {
    Write-Host 'Building KalmalaEditor modules once and rendering all menu previews...'
    & $menuRenderer -Cache $cache -BuildModules

    Write-Host 'Rendering all live-creature previews using the same compiled cache...'
    & $creatureRenderer -Cache $cache

    Write-Host "All previews completed. PNGs and logs are under $cache."
    exit 0
}
catch {
    Write-Host ("Preview batch failed: {0}" -f $_.Exception.Message) -ForegroundColor Red
    exit 1
}
