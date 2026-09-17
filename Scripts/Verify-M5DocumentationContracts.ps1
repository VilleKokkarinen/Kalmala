[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$checks = @(
    'Verify-OnboardingContract.ps1',
    'Verify-SettingsAccessibilityContract.ps1',
    'Verify-PresentationOwnership.ps1',
    'Verify-AudioCueContract.ps1',
    'Verify-LocalInputContract.ps1'
)

$passed = @()
foreach ($check in $checks) {
    $checkPath = Join-Path $PSScriptRoot $check
    if (-not (Test-Path -LiteralPath $checkPath -PathType Leaf)) {
        throw "M5 documentation check is missing: $check"
    }
    try {
        & $checkPath
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
