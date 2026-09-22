param(
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [int]$Port = 18461
)
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$output = Join-Path $env:TEMP ('KalmalaSettingsAccessibility-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$hostShaderDir = Join-Path $output 'HostShader'
$clientShaderDir = Join-Path $output 'ClientShader'
New-Item -ItemType Directory -Path $hostShaderDir, $clientShaderDir | Out-Null
$common = '-game -windowed -RenderOffscreen -ForceRes -ResX=1280 -ResY=720 -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaSettingsAccessibilityTest'
$server = $null
$client = $null
$completed = $false

function Read-Log([string]$Path) {
    if (Test-Path $Path) { return Get-Content -Raw $Path }
    return ''
}

function Assert-SettingsConfig([string]$UserDirectory, [string]$Role) {
    $config = Get-ChildItem -Path $UserDirectory -Filter 'GameUserSettings.ini' -File -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -eq $config) { throw "$Role did not write a local GameUserSettings.ini." }
    $text = Get-Content -Raw $config.FullName
    $expected = @(
        'LocalMasterVolume=0.5',
        'LocalAmbientVolume=0.25',
        'LocalMusicVolume=0.5',
        'LocalInteractionCombatVolume=0.75',
        'LocalTextScalePercent=150',
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
    $server = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -ShaderWorkingDir=`"$hostShaderDir`" -KalmalaSettingsScreenshot=`"$output\Host\settings`" -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited) { throw 'Settings accessibility host exited during startup.' }
        if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Settings accessibility host startup timed out.' }

    $client = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -ShaderWorkingDir=`"$clientShaderDir`" -KalmalaSettingsScreenshot=`"$output\Client\settings`" -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(90)
    $captures = @(
        "$output\Host\settings-settings.png", "$output\Host\settings-controls.png", "$output\Host\settings-audio.png",
        "$output\Client\settings-settings.png", "$output\Client\settings-controls.png", "$output\Client\settings-audio.png"
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
        $hostStages = $serverText -match 'Authority=1 Stage=Settings .*Open=1 .*FocusTargets=1 .*MoveIgnored=1 .*LookIgnored=1 .*LocalRoundTrip=1 InputApplied=1' -and
            $serverText -match 'Authority=1 Stage=Controls .*Open=1 .*FocusTargets=1 .*FocusableControls=1 .*Escape=1' -and
            $serverText -match 'Authority=1 Stage=Audio .*Open=1 .*FocusTargets=1 .*Master=0.50 Ambient=0.25 Music=0.50 InteractionCombat=0.75'
        $clientStages = $clientText -match 'Authority=0 Stage=Settings .*Open=1 .*FocusTargets=1 .*MoveIgnored=1 .*LookIgnored=1 .*LocalRoundTrip=1 InputApplied=1' -and
            $clientText -match 'Authority=0 Stage=Controls .*Open=1 .*FocusTargets=1 .*FocusableControls=1 .*Escape=1' -and
            $clientText -match 'Authority=0 Stage=Audio .*Open=1 .*FocusTargets=1 .*Master=0.50 Ambient=0.25 Music=0.50 InteractionCombat=0.75'
        $capturesReady = ($captures | Where-Object { !(Test-Path $_) -or (Get-Item $_).Length -le 32 }).Count -eq 0
        if ($hostComplete -and $clientComplete -and $hostStages -and $clientStages -and $capturesReady) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Settings accessibility verification timed out.' }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Client world identity mismatch.' }
    foreach ($capture in $captures) {
        $bytes = [System.IO.File]::ReadAllBytes($capture)
        if ($bytes.Length -lt 8 -or $bytes[0] -ne 0x89 -or $bytes[1] -ne 0x50 -or $bytes[2] -ne 0x4e -or $bytes[3] -ne 0x47) {
            throw "Invalid settings capture: $capture"
        }
    }
    $completed = $true
}
finally {
    foreach ($peer in @($client, $server)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Scenario logs: $output"
}

if (!$completed) { throw 'Settings accessibility verification did not complete.' }
Assert-SettingsConfig (Join-Path $output 'Host') 'Host'
Assert-SettingsConfig (Join-Path $output 'Client') 'Client'
Write-Output 'PASS: host/client settings tabs opened; local values persisted; keyboard/controller mappings stayed local; gameplay and replicated identity remained stable; six PNG captures retained.'
