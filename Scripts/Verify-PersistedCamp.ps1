param(
    [ValidateRange(1024, 65535)][int]$Port = 18031
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot
$results = @()

function Invoke-RequiredScenario([string]$Name, [string]$Script, [int]$ScenarioPort) {
    Write-Output "Starting persisted-camp preflight: $Name"
    & (Join-Path $PSScriptRoot $Script) -Port $ScenarioPort
    if ($LASTEXITCODE -ne 0) { throw "$Name preflight failed with exit code $LASTEXITCODE." }
    $script:results += $Name
}

# These are deliberately separate, retained-user-dir scenarios. They establish
# the existing two-player contracts before the later single-session M2 fixture
# joins gathering, crafting, placement, storage, weather and restart evidence.
Invoke-RequiredScenario 'crafting-and-hearth' 'Verify-Crafting.ps1' $Port
Invoke-RequiredScenario 'storage-and-workbench' 'Verify-Storage.ps1' ($Port + 1)
Invoke-RequiredScenario 'shelter-collision-and-exposure' 'Verify-ConstructionMovement.ps1' ($Port + 2)

if ($results.Count -ne 3) { throw 'Persisted-camp preflight did not complete every required contract.' }
Write-Output 'PASS: persisted-camp preflight confirmed existing two-player hearth/crafting, storage/workbench, and shelter/exposure contracts. A single-session gathered-camp acceptance fixture remains required.'
