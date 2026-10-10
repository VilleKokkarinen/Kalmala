[CmdletBinding()]
param(
    [string]$Document = ''
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($Document)) {
    $Document = Join-Path $projectRoot 'docs\02-technical-architecture.md'
}

if (-not (Test-Path -LiteralPath $Document -PathType Leaf)) {
    throw "Settings/accessibility contract was not found: $Document"
}

$text = Get-Content -LiteralPath $Document -Raw
if ($text.Contains('## Consolidated system contracts')) {
    $section = [regex]::Match($text, '(?ms)^## Settings and accessibility\r?\n(.*?)(?=^## Presentation ownership\r?$)')
    if (-not $section.Success) { throw 'Consolidated settings/accessibility section was not found' }
    $text = $section.Groups[1].Value
}
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
    'Whole-interface scale choices' = '80%,\s*90%,\s*100%,\s*110%,\s*or 120%'
    'Interface-scale default and local persistence' = '(?s)defaulting to 100%.*existing local `GameUserSettings` configuration'
    'Interface scale layered over project defaults' = 'multiplier over the project.s\s+existing DPI curve and `UUserInterfaceSettings::ApplicationScale`'
    'Separate text and interface scale' = 'It remains separate from text scale'
    'Reduced-motion default and storage' = '(?s)Reduced motion.*defaults to Off.*existing local `GameUserSettings` configuration'
    'Reduced motion overrides decorative effects' = 'overrides theme animation defaults for the options-panel slide, button/card\s+highlight transitions, and animated scrolling'
    'Immediate static feedback under reduced motion' = 'Static focus, selection,\s+contrast, labels, status changes, and menu actions remain immediate'
    'Fresh-process persistence check' = '(?s)restarts both peers.*confirms both choices.*reload'
    'Contrast option' = 'contrast'
    'Contrast choices' = 'Standard or High contrast'
    'Readable modal scaling' = 'auto-wrapped labels and buttons.*larger bounded panel'
    'Contrast palette' = 'backdrop, panel, button surfaces, text, and focusable state controls'
    'Non-colour feedback' = 'colour-independent feedback'
    'Feedback choices' = 'Text only.*Text \+ markers'
    'Marker overlay reflow' = 'follows the scaled viewport'
    'Feedback state coverage' = '(?s)Wet, hearth, construction,.*combat, discovery, and support'
    'Owner-only feedback overlay' = 'owner-only overlay'
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
if ($text -notmatch 'Verify-SettingsAccessibility\.ps1' -or $text -notmatch 'physical\s+controller\s+hardware') {
    throw 'Settings/accessibility contract does not state the rendered probe and its remaining runtime limits'
}
if ($text -notmatch '1024x768.*1280x720|1280x720.*1024x768') {
    throw 'Settings/accessibility acceptance does not cover both supported verification viewports'
}

$feedbackSource = Join-Path $projectRoot 'Source\KalmalaUI\Private\KalmalaAccessibilityFeedbackSubsystem.cpp'
if (-not (Test-Path -LiteralPath $feedbackSource -PathType Leaf)) {
    throw "Accessibility feedback source was not found: $feedbackSource"
}
$feedbackSourceText = Get-Content -LiteralPath $feedbackSource -Raw
foreach ($anchor in @(
    'UKalmalaAccessibilityFeedbackSubsystem',
    'GetFeedbackMode',
    'COLOUR-INDEPENDENT FEEDBACK',
    'MarkerLine(TEXT("HEARTH")',
    'MarkerLine(TEXT("CONSTRUCTION")',
    'GetViewportScale',
    'LastFeedbackViewportSize',
    'SetPositionInViewport',
    'IsLocalController'
)) {
    if ($feedbackSourceText -notmatch [regex]::Escape($anchor)) {
        throw "Accessibility feedback source is missing anchor: $anchor"
    }
}
foreach ($movedFeedback in @('MarkerLine(TEXT("COMBAT")', 'MarkerLine(TEXT("DISCOVERY")', 'MarkerLine(TEXT("SUPPORT")')) {
    if ($feedbackSourceText.Contains($movedFeedback)) {
        throw "Accessibility overlay still duplicates dedicated HUD feedback: $movedFeedback"
    }
}

$hotbarSource = Get-Content (Join-Path $projectRoot 'Source\KalmalaUI\Private\KalmalaStatusHotbarWidget.cpp') -Raw
foreach ($anchor in @('WetStatusId', 'SteadyMealStatusId', 'HearthShield', 'HeatIntensity', 'Storm', 'HitTestInvisible')) {
    if (!$hotbarSource.Contains($anchor)) { throw "Shared status hotbar is missing accessible status coverage: $anchor" }
}
Write-Output 'PASS: settings/accessibility contract covers option groups, input access, non-colour feedback, interface scale, reduced motion, local persistence, and server authority.'
