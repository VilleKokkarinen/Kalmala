[CmdletBinding()]
param(
    [string]$Document = ''
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($Document)) {
    $Document = Join-Path $projectRoot 'docs\16-audio-cue-contract.md'
}

if (-not (Test-Path -LiteralPath $Document -PathType Leaf)) {
    throw "Audio cue contract was not found: $Document"
}

$text = Get-Content -LiteralPath $Document -Raw
$requiredSections = @(
    '## Cue matrix',
    '## Original asset and mix requirements',
    '## No-build audit and runtime limits',
    '## Multiplayer and persistence boundary'
)
foreach ($section in $requiredSections) {
    if ($text -notmatch [regex]::Escape($section)) {
        throw "Audio cue contract is missing section: $section"
    }
}

$cueGroups = @(
    'Ambient wilderness',
    'Movement and traversal',
    'Weather and exposure',
    'Interaction and gathering',
    'Combat',
    'Discovery',
    'Support magic',
    'Camp and storage'
)
$cueRows = @()
foreach ($cueGroup in $cueGroups) {
    $row = @($text -split "`r?`n" | Where-Object { $_ -match "^\| $([regex]::Escape($cueGroup)) \|" })
    if ($row.Count -ne 1) {
        throw "Expected exactly one cue row for '$cueGroup'; found $($row.Count)"
    }
    if (($row[0].ToCharArray() | Where-Object { $_ -eq '|' }).Count -lt 6) {
        throw "Cue row '$cueGroup' does not contain all five contract columns"
    }
    $cueRows += $row[0]
}

$requiredTerms = @{
    'project-owned audio rule' = 'project-owned'
    'non-audio accessibility fallback' = 'non-audio equivalent|text/shape equivalent'
    'local audio settings' = 'Audio settings contract|master/music/ambient/interaction-combat'
    'server authority' = 'server remains authoritative'
    'hidden-content privacy' = 'hidden.*population|undiscovered IDs|private state'
    'no RPC boundary' = 'no request payload|without adding an RPC'
    'no save boundary' = 'no save field|gameplay save schemas'
    'silence fallback' = 'muted or unavailable device'
    'accepted replicated weather cue' = 'FKalmalaWeatherState::WindStrength|accepted replicated.*precipitation'
    'owner-scoped Wet status cue' = 'State\.Wet|WetStatusCue'
    'owner-only accepted support cue' = '(?s)owner-only.*support.*FeedbackSerial.*Accepted|FeedbackSerial.*Accepted.*owner-only'
    'owner-only combat result cue' = '(?s)owner-only.*combat.*FeedbackSerial.*EKalmalaCombatFeedback::Hit.*Defeat|FeedbackSerial.*EKalmalaCombatFeedback::Hit.*Defeat.*owner-only'
}
foreach ($term in $requiredTerms.GetEnumerator()) {
    if ($text -notmatch $term.Value) {
        throw "Audio cue contract is missing $($term.Key)"
    }
}

if ($text -match 'third party') {
    # These words are allowed only when the document states that the source is forbidden.
    if ($text -notmatch 'Do not copy music') {
        throw 'Audio cue contract contains an unqualified external-audio reference'
    }
}
if ($text -notmatch 'does not create sound assets|does not.*launch Unreal') {
    throw 'Audio cue contract does not state its no-build limitation'
}

Write-Output "PASS: audio cue contract has $($cueRows.Count) cue groups, non-audio fallbacks, project ownership, and multiplayer boundaries."
