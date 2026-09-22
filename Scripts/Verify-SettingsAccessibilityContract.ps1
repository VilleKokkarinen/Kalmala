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
    'Local master volume' = 'master-volume cycle in\s+25% steps'
    'Mute and restore' = 'mute/restore button'
    'Ambient/music/feedback category levels' = '(?s)Ambient, music, and interaction/combat feedback each have.*0%, 25%, 50%, 75%, and 100%'
    'Ambient loop mix' = '(?s)ambient value scales the local.*wind, rain, water, fire, and biome loops'
    'Interaction/combat cue mix' = '(?s)interaction/combat value.*scales owner-local.*one-shots'
    'Current music playback limit' = 'there is no music track in the current runtime'
    'Local audio configuration' = 'local `GameUserSettings` config'
    'Local primary output' = 'primary output-volume multiplier'
    'Controls option group' = 'Controls'
    'Control remapping' = 'bounded keyboard/controller choices|bounded local remapping'
    'Control restore defaults' = 'restore.?defaults|Restore default controls'
    'Control local input ownership' = 'owning local\s+`UPlayerInput`|owning `UPlayerInput`'
    'Control config persistence' = 'stores only allowlisted local choices|local overrides'
    'Control baseline preservation' = 'DefaultInput\.ini.*never rewritten|DefaultInput\.ini.*unchanged'
    'Text scale option' = 'text scale'
    'Text-scale choices' = '100%, 125%, and 150%'
    'Contrast option' = 'contrast'
    'Contrast choices' = 'Standard or High contrast'
    'Readable modal scaling' = 'auto-wrapped labels and buttons.*larger bounded panel'
    'Contrast palette' = 'backdrop, panel, button surfaces, text, and focusable state controls'
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
if ($text -notmatch '(?s)Colour-independent feedback preferences and the full\s+rendered settings-persistence verification remain queued') {
    throw 'Settings/accessibility contract does not state its remaining runtime verification limit'
}

Write-Output 'PASS: settings/accessibility contract covers option groups, input access, non-colour feedback, local persistence, and server authority.'
