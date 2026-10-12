[CmdletBinding()]
param()
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot


$ErrorActionPreference = 'Stop'
$projectRoot = $repoPaths.ProjectRoot
$checks = @(
    'Presentation\Verify-OnboardingContract.ps1',
    'Presentation\Verify-SettingsAccessibility.ps1',
    'Presentation\Verify-PresentationOwnership.ps1',
    'Audio\Verify-AmbientAudio.ps1',
    'Presentation\Verify-LocalInputContract.ps1'
)

$passed = @()
foreach ($check in $checks) {
    $checkPath = Join-Path $repoPaths.TestsRoot $check
    if (-not (Test-Path -LiteralPath $checkPath -PathType Leaf)) {
        throw "M5 documentation check is missing: $check"
    }
    try {
        $checkArguments = if ($check -in @('Presentation\Verify-SettingsAccessibility.ps1', 'Audio\Verify-AmbientAudio.ps1')) { @('-ContractOnly') } else { @() }
        & $checkPath @checkArguments
        if ($null -ne $LASTEXITCODE -and $LASTEXITCODE -ne 0) {
            throw "exit code $LASTEXITCODE"
        }
        $passed += $check
    }
    catch {
        throw "M5 documentation check failed in $check : $($_.Exception.Message)"
    }
}

Write-Output "PASS: all $($passed.Count) M5 no-build documentation contracts passed in one run."
