param(
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [int]$Port = 18461,
    [ValidateSet(1024, 1280)][int]$Width = 1280,
    [ValidateSet(720, 768)][int]$Height = 720,
    [ValidateSet(80, 90, 100, 110, 120)][int]$HostInterfaceScale = 90,
    [ValidateSet(80, 90, 100, 110, 120)][int]$ClientInterfaceScale = 110,
    [string]$Document = '',
    [switch]$ContractOnly
)
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot


function Invoke-SettingsAccessibilityContract([string]$DocumentPath) {
    $projectRoot = $repoPaths.ProjectRoot
    if ([string]::IsNullOrWhiteSpace($DocumentPath)) {
        $DocumentPath = Join-Path $projectRoot 'docs\02-technical-architecture.md'
    }

    if (-not (Test-Path -LiteralPath $DocumentPath -PathType Leaf)) {
        throw "Settings/accessibility contract was not found: $DocumentPath"
    }

    $text = Get-Content -LiteralPath $DocumentPath -Raw
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
        'M13 Settings groups' = '(?s)Keyboard & Mouse.*Controller.*Graphics.*Audio.*Accessibility'
        'Empty Gameplay group stays hidden' = 'Gameplay stays hidden while no candidate option is selected'
        'Inventory backplate reused' = '(?s)`InventoryPanelImage` shared with the Inventory\s+backplate'
        'Shared-theme button states' = 'shared-theme hover, pressed, selected, and focus states'
        'Audio option group' = 'Audio'
        'Local master volume' = 'focusable master-volume slider'
        'Mute and restore' = 'mute/restore button'
        'Ambient/music/feedback category levels' = 'Ambient, music, and interaction/combat feedback each have a focusable 0–100% slider with a live percentage label'
        'Ambient loop mix' = '(?s)ambient value scales the local.*wind, rain, water, fire, and biome loops'
        'Interaction/combat cue mix' = '(?s)interaction/combat value.*scales owner-local.*one-shots'
        'Current music playback limit' = 'there is no music track in the current runtime'
        'Local audio configuration' = 'local `GameUserSettings` config'
        'Local primary output' = 'primary output-volume multiplier'
        'Control remapping' = 'per-action binding choices|bounded keyboard/controller choices|bounded local remapping'
        'Control restore defaults' = 'restore.?defaults|Restore default controls'
        'Control local input ownership' = 'owning local\s+`UPlayerInput`|owning `UPlayerInput`'
        'Control config persistence' = 'store(?:s)? only allowlisted local choices|local overrides'
        'Control baseline preservation' = 'DefaultInput\.ini.*never rewritten|DefaultInput\.ini.*unchanged'
        'Text scale option' = 'text scale'
        'Text-scale choices' = '50%, 75%, 100%, 125%, and 150%'
        'Whole-interface scale choices' = '80%,\s*90%,\s*100%,\s*110%,\s*or 120%'
        'Interface-scale default and local persistence' = '(?s)defaulting to 100%.*existing local `GameUserSettings` configuration'
        'Interface scale layered over project defaults' = 'multiplier over the project.s\s+existing DPI curve and `UUserInterfaceSettings::ApplicationScale`'
        'Separate text and interface scale' = 'It remains separate from text scale'
        'Reduced-motion default and storage' = '(?s)Reduced motion.*defaults to Off.*existing local `GameUserSettings` configuration'
        'Reduced motion overrides decorative effects' = 'overrides theme animation defaults for the options-panel slide, button/card\s+highlight transitions, and animated scrolling'
        'Immediate static feedback under reduced motion' = 'Static focus, selection,\s+contrast, labels, status changes, and menu actions remain immediate'
        'Fresh-process persistence check' = '(?s)restarts both peers.*confirms both choices.*reload'
        'Contrast option' = 'contrast'
        'Contrast choices' = 'Low, Standard, or High contrast'
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
    # Markdown line wrapping must not change the prose contract.
    $contractText = $text -replace '\s+', ' '
    foreach ($term in $requiredTerms.GetEnumerator()) {
        if ($contractText -notmatch $term.Value) {
            throw "Settings/accessibility contract is missing $($term.Key)"
        }
    }

    if ($contractText -notmatch 'Escape') {
        throw 'Settings/accessibility contract does not define the modal Escape path'
    }
    if ($contractText -notmatch 'Cancel.*apply.*reset|apply.*reset.*actions') {
        throw 'Settings/accessibility contract does not define reversible local changes'
    }
    if ($contractText -notmatch 'SettingsAccessibility\.ps1' -or $contractText -notmatch 'physical\s+controller\s+hardware') {
        throw 'Settings/accessibility contract does not state the rendered probe and its remaining runtime limits'
    }
    if ($contractText -notmatch '1024x768.*1280x720|1280x720.*1024x768') {
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
}

$ErrorActionPreference = 'Stop'
Invoke-SettingsAccessibilityContract $Document
if ($ContractOnly) { return }
if (($Width -eq 1024 -and $Height -ne 768) -or ($Width -eq 1280 -and $Height -ne 720)) {
    throw 'Use the supported 1024x768 or 1280x720 viewport pair.'
}
Add-Type -AssemblyName System.Drawing
$project = Join-Path ($repoPaths.ProjectRoot) 'Kalmala.uproject'
$output = Join-Path $env:TEMP ('KalmalaSettingsAccessibility-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$hostShaderDir = Join-Path $output 'HostShader'
$clientShaderDir = Join-Path $output 'ClientShader'
New-Item -ItemType Directory -Path $hostShaderDir, $clientShaderDir | Out-Null
$common = "-game -windowed -RenderOffscreen -ForceRes -ResX=$Width -ResY=$Height -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaSettingsAccessibilityTest"
$server = $null
$client = $null
$completed = $false

function Read-Log([string]$Path) {
    if (Test-Path $Path) { return Get-Content -Raw $Path }
    return ''
}

function Assert-SettingsConfig([string]$UserDirectory, [string]$Role, [int]$InterfaceScale, [bool]$ReducedMotion) {
    $config = Get-ChildItem -Path $UserDirectory -Filter 'GameUserSettings.ini' -File -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -eq $config) { throw "$Role did not write a local GameUserSettings.ini." }
    $text = Get-Content -Raw $config.FullName
    $expected = @(
        'LocalMasterVolume=0.5',
        'LocalAmbientVolume=0.25',
        'LocalMusicVolume=0.5',
        'LocalInteractionCombatVolume=0.75',
        'LocalTextScalePercent=150',
        "LocalInterfaceScalePercent=$InterfaceScale",
        "LocalReducedMotionEnabled=$($ReducedMotion.ToString())",
        'LocalContrastMode=1',
        'LocalFeedbackMode=1',
        'LocalInput_Interact_Keyboard=F',
        'LocalInput_Interact_Controller=Gamepad_FaceButton_Right'
    )
    foreach ($value in $expected) {
        if ($text -notmatch [regex]::Escape($value)) { throw "$Role local config is missing $value ($($config.FullName))." }
    }
}

try {
    $server = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -KalmalaSettingsInterfaceScale=$HostInterfaceScale -ShaderWorkingDir=`"$hostShaderDir`" -KalmalaSettingsScreenshot=`"$output\Host\settings`" -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited) { throw 'Settings accessibility host exited during startup.' }
        if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Settings accessibility host startup timed out.' }

    $client = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -KalmalaSettingsInterfaceScale=$ClientInterfaceScale -ShaderWorkingDir=`"$clientShaderDir`" -KalmalaSettingsScreenshot=`"$output\Client\settings`" -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(120)
    $captures = @(
        "$output\Host\settings-accessibility.png", "$output\Host\settings-keyboard-mouse.png", "$output\Host\settings-controller.png",
        "$output\Host\settings-audio.png", "$output\Host\settings-escape.png", "$output\Host\settings-graphics.png",
        "$output\Host\settings-motion.png", "$output\Host\settings-hud.png",
        "$output\Client\settings-accessibility.png", "$output\Client\settings-keyboard-mouse.png", "$output\Client\settings-controller.png",
        "$output\Client\settings-audio.png", "$output\Client\settings-escape.png", "$output\Client\settings-graphics.png",
        "$output\Client\settings-motion.png", "$output\Client\settings-hud.png"
    )
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A settings accessibility peer exited.' }
        $serverText = Read-Log $serverLog
        $clientText = Read-Log $clientLog
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Settings accessibility: FAIL') {
            throw 'Settings accessibility verification failed; inspect the retained logs.'
        }
        $hostComplete = $serverText -match 'Settings accessibility: Authority=1 Completed=1 GameplayStable=1 InputRestored=1'
        $clientComplete = $clientText -match 'Settings accessibility: Authority=0 Completed=1 GameplayStable=1 InputRestored=1'
        $hostStages = $serverText -match "Authority=1 Stage=Accessibility .*Open=1 .*FocusTargets=1 .*MoveIgnored=1 .*LookIgnored=1 .*LocalRoundTrip=1 InputApplied=1 InterfaceScale=$HostInterfaceScale ReducedMotion=0" -and
            $serverText -match 'Authority=1 Stage=RememberedOptionsTab Selected=4 Restored=4 Focused=1 ValuesUnchanged=1' -and
            $serverText -match 'Authority=1 Stage=KeyboardMouse .*Open=1 .*FocusTargets=1 .*FocusableControls=1 .*Keyboard=.*Escape=1' -and
            $serverText -match 'Authority=1 Stage=Controller .*Open=1 .*FocusTargets=1 .*FocusableControls=1 .*Binding=.*Escape=1' -and
            $serverText -match 'Authority=1 Stage=Audio .*Open=1 .*FocusTargets=1 .*Master=0.50 Ambient=0.25 Music=0.50 InteractionCombat=0.75' -and
            $serverText -match 'Authority=1 Stage=OpeningStart Animated=1 PanelY=-32.00 ExpectedY=-32.00 Duration=0.18 Travel=32.00 FocusQueued=1 MoveIgnored=1 LookIgnored=1 CenterAnchored=1' -and
            $serverText -match 'Authority=1 Stage=OpeningFocus Focused=1 FocusTargets=1 FocusDuringAnimation=1 Frame=1' -and
            $serverText -match 'Authority=1 Stage=OpeningResize Applied=1 Viewport=1600x900 .*CenterAnchored=1' -and
            $serverText -match 'Authority=1 Stage=OpeningCloseReopen Interrupted=1 Reopened=1 CloseReset=1 .*Focused=1 FocusDuringAnimation=1 CenterAnchored=1' -and
            $serverText -match "Authority=1 Stage=OpeningResizeRestore Restored=1 Viewport=${Width}x${Height} .*CenterAnchored=1" -and
            $serverText -match 'Authority=1 Stage=OpeningAnimation Completed=1 Animated=1 Intermediate=1 Interrupted=1 Resize=1600x900 Restored=1 FinalY=-?0.00 Focused=1 CenterAnchored=1'
        $hostStages = $hostStages -and $serverText -match 'Authority=1 Stage=MotionPreference ReducedMotion=1 Animated=0 Focused=1 PanelFits=1' -and
            $serverText -match "Authority=1 Stage=HUDScale InterfaceScale=$HostInterfaceScale ReducedMotion=1 InputRestored=1 Viewport=${Width}x${Height}"
        $clientStages = $clientText -match "Authority=0 Stage=Accessibility .*Open=1 .*FocusTargets=1 .*MoveIgnored=1 .*LookIgnored=1 .*LocalRoundTrip=1 InputApplied=1 InterfaceScale=$ClientInterfaceScale ReducedMotion=0" -and
            $clientText -match 'Authority=0 Stage=RememberedOptionsTab Selected=4 Restored=4 Focused=1 ValuesUnchanged=1' -and
            $clientText -match 'Authority=0 Stage=KeyboardMouse .*Open=1 .*FocusTargets=1 .*FocusableControls=1 .*Keyboard=.*Escape=1' -and
            $clientText -match 'Authority=0 Stage=Controller .*Open=1 .*FocusTargets=1 .*FocusableControls=1 .*Binding=.*Escape=1' -and
            $clientText -match 'Authority=0 Stage=Audio .*Open=1 .*FocusTargets=1 .*Master=0.50 Ambient=0.25 Music=0.50 InteractionCombat=0.75' -and
            $clientText -match 'Authority=0 Stage=OpeningStart Animated=1 PanelY=-32.00 ExpectedY=-32.00 Duration=0.18 Travel=32.00 FocusQueued=1 MoveIgnored=1 LookIgnored=1 CenterAnchored=1' -and
            $clientText -match 'Authority=0 Stage=OpeningFocus Focused=1 FocusTargets=1 FocusDuringAnimation=1 Frame=1' -and
            $clientText -match 'Authority=0 Stage=OpeningResize Applied=1 Viewport=1600x900 .*CenterAnchored=1' -and
            $clientText -match 'Authority=0 Stage=OpeningCloseReopen Interrupted=1 Reopened=1 CloseReset=1 .*Focused=1 FocusDuringAnimation=1 CenterAnchored=1' -and
            $clientText -match "Authority=0 Stage=OpeningResizeRestore Restored=1 Viewport=${Width}x${Height} .*CenterAnchored=1" -and
            $clientText -match 'Authority=0 Stage=OpeningAnimation Completed=1 Animated=1 Intermediate=1 Interrupted=1 Resize=1600x900 Restored=1 FinalY=-?0.00 Focused=1 CenterAnchored=1'
        $clientStages = $clientStages -and $clientText -match 'Authority=0 Stage=MotionPreference ReducedMotion=0 Animated=1 Focused=1 PanelFits=1' -and
            $clientText -match "Authority=0 Stage=HUDScale InterfaceScale=$ClientInterfaceScale ReducedMotion=0 InputRestored=1 Viewport=${Width}x${Height}"
        $hostBackgrounds = $serverText -match 'Authority=1 Stage=Accessibility .*PanelImages=1 ContrastImageSuppressed=1' -and
            $serverText -match 'Authority=1 Stage=StandardBackgroundCapture Loaded=1 Contrast=0' -and
            $serverText -match 'Authority=1 Stage=EscapeBackgroundCapture Transparent=1 Contrast=0 OptionList=1 Focused=1 FocusCue=1' -and
            $serverText -match 'Authority=1 Stage=GraphicsBackgroundCapture Loaded=1 Contrast=0 FocusTargets=1' -and
            $serverText -match 'Authority=1 Stage=ContrastRestore Mode=1 ImageSuppressed=1 HomeTransparent=0 HomeFocusCue=1'
        $clientBackgrounds = $clientText -match 'Authority=0 Stage=Accessibility .*PanelImages=1 ContrastImageSuppressed=1' -and
            $clientText -match 'Authority=0 Stage=StandardBackgroundCapture Loaded=1 Contrast=0' -and
            $clientText -match 'Authority=0 Stage=EscapeBackgroundCapture Transparent=1 Contrast=0 OptionList=1 Focused=1 FocusCue=1' -and
            $clientText -match 'Authority=0 Stage=GraphicsBackgroundCapture Loaded=1 Contrast=0 FocusTargets=1' -and
            $clientText -match 'Authority=0 Stage=ContrastRestore Mode=1 ImageSuppressed=1 HomeTransparent=0 HomeFocusCue=1'
        $capturesReady = ($captures | Where-Object { !(Test-Path $_) -or (Get-Item $_).Length -le 32 }).Count -eq 0
        if ($hostComplete -and $clientComplete -and $hostStages -and $clientStages -and $hostBackgrounds -and $clientBackgrounds -and $capturesReady) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Settings accessibility verification timed out.' }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Client world identity mismatch.' }
    foreach ($capture in $captures) {
        $bytes = [System.IO.File]::ReadAllBytes($capture)
        if ($bytes.Length -lt 8 -or $bytes[0] -ne 0x89 -or $bytes[1] -ne 0x50 -or $bytes[2] -ne 0x4e -or $bytes[3] -ne 0x47) {
            throw "Invalid settings capture: $capture"
        }
        $bitmap = [System.Drawing.Bitmap]::FromFile($capture)
        try {
            if ($capture -match '-hud\.png$') { continue }
            if ($capture -match '-escape\.png$') { continue }
            # The centered panel keeps at least a 16-unit margin, including at 1024x768.
            $backdropPixel = $bitmap.GetPixel(5, 5)
            if ($backdropPixel.R -gt 80 -or $backdropPixel.G -gt 80 -or $backdropPixel.B -gt 80) {
                throw "Settings modal backdrop is not visible in capture: $capture (RGB $($backdropPixel.R),$($backdropPixel.G),$($backdropPixel.B))."
            }
        }
        finally {
            $bitmap.Dispose()
        }
    }
    $completed = $true
}
finally {
    foreach ($peer in @($client, $server)) {
        if ($null -ne $peer -and !$peer.HasExited) {
            Stop-Process -Id $peer.Id
            Wait-Process -Id $peer.Id -Timeout 30 -ErrorAction SilentlyContinue
        }
    }
    Write-Output "Scenario logs: $output"
}

if (!$completed) { throw 'Settings accessibility verification did not complete.' }
Assert-SettingsConfig (Join-Path $output 'Host') 'Host' $HostInterfaceScale $true
Assert-SettingsConfig (Join-Path $output 'Client') 'Client' $ClientInterfaceScale $false

$reloadServerLog = Join-Path $output 'reload-server.log'
$reloadClientLog = Join-Path $output 'reload-client.log'
$reloadCommon = $common -replace ' -KalmalaSettingsAccessibilityTest', ''
$reloadServer = $null
$reloadClient = $null
$reloadPassed = $false
try {
    $reloadServer = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $reloadCommon -KalmalaSettingsPreferenceReloadTest -KalmalaExpectedInterfaceScale=$HostInterfaceScale -KalmalaExpectedReducedMotion=1 -KalmalaExpectedContrast=1 -ShaderWorkingDir=`"$hostShaderDir`" -abslog=`"$reloadServerLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($reloadServer.HasExited) { throw 'Settings preference reload host exited during startup.' }
        if ((Test-Path $reloadServerLog) -and (Select-String $reloadServerLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Settings preference reload host startup timed out.' }

    $reloadClient = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $reloadCommon -KalmalaSettingsPreferenceReloadTest -KalmalaExpectedInterfaceScale=$ClientInterfaceScale -KalmalaExpectedReducedMotion=0 -KalmalaExpectedContrast=1 -ShaderWorkingDir=`"$clientShaderDir`" -abslog=`"$reloadClientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($reloadServer.HasExited -or $reloadClient.HasExited) { throw 'A settings preference reload peer exited.' }
        $hostReload = Read-Log $reloadServerLog
        $clientReload = Read-Log $reloadClientLog
        if (($hostReload + $clientReload) -match 'Fatal error:|Assertion failed:|Settings accessibility reload: FAIL') {
            throw 'Settings preference reload verification failed; inspect the retained logs.'
        }
        $hostReloaded = $hostReload -match "Settings accessibility reload: Authority=1 Completed=1 InterfaceScale=$HostInterfaceScale ReducedMotion=1 Contrast=1 .*ScaleApplied=1"
        $clientReloaded = $clientReload -match "Settings accessibility reload: Authority=0 Completed=1 InterfaceScale=$ClientInterfaceScale ReducedMotion=0 Contrast=1 .*ScaleApplied=1"
        if ($hostReloaded -and $clientReloaded) { $reloadPassed = $true; break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if (!$reloadPassed) { throw 'Saved interface scale and reduced-motion choices did not reload in fresh peers.' }
}
finally {
    foreach ($peer in @($reloadClient, $reloadServer)) {
        if ($null -ne $peer -and !$peer.HasExited) {
            Stop-Process -Id $peer.Id
            Wait-Process -Id $peer.Id -Timeout 30 -ErrorAction SilentlyContinue
        }
    }
}

Write-Output "PASS: host/client settings panels reflowed at ${Width}x${Height}; the 0.18s slide survived resize and close/reopen with immediate focus; owner-local 80-120% interface scale and reduced-motion paths passed; saved choices reloaded after process restart; standard contrast, high contrast, input restoration, gameplay stability and peer privacy checks passed; fourteen PNG captures retained at $output."
