[CmdletBinding()]
param(
    [string]$Document = ''
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($Document)) {
    $Document = Join-Path $projectRoot 'docs\13-onboarding-and-tutorial.md'
}

if (-not (Test-Path -LiteralPath $Document -PathType Leaf)) {
    throw "Onboarding contract was not found: $Document"
}

$text = Get-Content -LiteralPath $Document -Raw
$requiredSections = @(
    '## Prompt rules',
    '## Route-free beat matrix',
    '## Presentation and accessibility acceptance',
    '## Authority, privacy, and persistence'
)
foreach ($section in $requiredSections) {
    if ($text -notmatch [regex]::Escape($section)) {
        throw "Onboarding contract is missing section: $section"
    }
}

$beats = @(
    'Arrive',
    'Interact',
    'Gather',
    'Prepare',
    'Weather',
    'Explore',
    'Optional encounter',
    'Discovery',
    'Support magic',
    'Return'
)
$beatRows = @()
foreach ($beat in $beats) {
    $row = @($text -split "`r?`n" | Where-Object { $_ -match "^\| $([regex]::Escape($beat)) \|" })
    if ($row.Count -ne 1) {
        throw "Expected exactly one route-free beat row for '$beat'; found $($row.Count)"
    }
    if (($row[0].ToCharArray() | Where-Object { $_ -eq '|' }).Count -lt 5) {
        throw "Beat row '$beat' does not contain all four contract columns"
    }
    $beatRows += $row[0]
}

$requiredFragments = @(
    'dismissible prompt behavior',
    'local presentation state' ,
    'colour-independent cue',
    'server authority boundary',
    'no hidden-content disclosure',
    'no new RPC boundary'
)
$fragmentPatterns = @{
    'dismissible prompt behavior' = 'dismissible'
    'local presentation state' = 'local UI state'
    'colour-independent cue' = 'non-colour'
    'server authority boundary' = 'server.*authoritative|server continues to own'
    'no hidden-content disclosure' = 'hidden.*population|hidden.*creatures|undiscovered.*rewards'
    'no new RPC boundary' = 'no prompt sends\s+a new RPC'
}
foreach ($fragment in $requiredFragments) {
    if ($text -notmatch $fragmentPatterns[$fragment]) {
        throw "Onboarding contract is missing $fragment"
    }
}

if ($text -notmatch 'must not.*route|never.*route|no.*route') {
    throw 'Onboarding contract does not state the route-free boundary'
}
if ($text -notmatch 'keyboard/controller input label') {
    throw 'Onboarding contract does not require keyboard/controller labels'
}
if ($text -notmatch 'never be\s+included in the gameplay save schema') {
    throw 'Onboarding contract does not protect local prompt history from gameplay saves'
}

Write-Output "PASS: onboarding contract has $($beatRows.Count) route-free beats, accessibility cues, authority boundaries, and local-persistence limits."
