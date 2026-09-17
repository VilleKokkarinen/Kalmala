[CmdletBinding()]
param(
    [string]$Document = ''
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($Document)) {
    $Document = Join-Path $projectRoot 'docs\14-settings-and-accessibility.md'
}

if (-not (Test-Path -LiteralPath $Document -PathType Leaf)) {
    throw "Settings/accessibility contract was not found: $Document"
}

$text = Get-Content -LiteralPath $Document -Raw
$requiredSections = @(
    '## Existing shell and option groups',
    '## Accessibility requirements',
    '## Local storage and authority boundary',
    '## Acceptance and limits'
)
foreach ($section in $requiredSections) {
    if ($text -notmatch [regex]::Escape($section)) {
        throw "Settings/accessibility contract is missing section: $section"
    }
}

$requiredTerms = @{
    'Video option group' = 'Video'
    'Audio option group' = 'Audio'
    'Controls option group' = 'Controls'
    'Text scale option' = 'text scale'
    'Contrast option' = 'contrast'
    'Non-colour feedback' = 'colour-independent feedback'
    'Keyboard/controller access' = 'keyboard.*controller|controller.*keyboard'
    'Focus navigation' = 'visible focus|stable tab/order navigation'
    'Text audio equivalent' = 'text equivalents.*current value|readable text confirmation'
    'Local save boundary' = 'world saves.*player progression saves|not be written.*world saves'
    'No RPC boundary' = 'sends no RPC'
    'Server authority' = 'server continues to own gameplay outcomes'
    'No hidden client outcome' = 'cannot use settings to select a target'
}
foreach ($term in $requiredTerms.GetEnumerator()) {
    if ($text -notmatch $term.Value) {
        throw "Settings/accessibility contract is missing $($term.Key)"
    }
}

if ($text -notmatch 'Escape') {
    throw 'Settings/accessibility contract does not define the modal Escape path'
}
if ($text -notmatch 'Cancel.*apply.*reset|apply.*reset.*actions') {
    throw 'Settings/accessibility contract does not define reversible local changes'
}
if ($text -notmatch 'Runtime UI.*persistence verification remain queued') {
    throw 'Settings/accessibility contract does not state its runtime verification limit'
}

Write-Output 'PASS: settings/accessibility contract covers option groups, input access, non-colour feedback, local persistence, and server authority.'
