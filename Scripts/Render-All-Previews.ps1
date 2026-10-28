$ErrorActionPreference = 'Stop'

$menuRenderer = Join-Path $PSScriptRoot 'Render-Menu.ps1'
$creatureRenderer = Join-Path $PSScriptRoot 'Render-Creatures.ps1'
$cache = Join-Path $env:TEMP 'kmall'
$menuOutput = Join-Path $cache 'png/Menus'
$creatureOutput = Join-Path $cache 'png/Creatures'

try {
    Write-Host "Building KalmalaEditor modules once and rendering menu previews to $menuOutput..."
    & $menuRenderer -Cache $cache -Output $menuOutput -BuildModules

    Write-Host "Rendering live-creature previews to $creatureOutput using the same compiled cache..."
    & $creatureRenderer -Cache $cache -Output $creatureOutput

    Write-Host "All previews completed. Menus: $menuOutput  Creatures: $creatureOutput  Logs: $(Join-Path $cache 'run')"
    exit 0
}
catch {
    Write-Host ("Preview batch failed: {0}" -f $_.Exception.Message) -ForegroundColor Red
    exit 1
}
