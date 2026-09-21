param(
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [int]$Port = 17843,
    [switch]$Rendered,
    [switch]$WetStamina,
    [switch]$MovementAudio
)
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$output = Join-Path $env:TEMP ('KalmalaPlayerControls-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$renderer = if ($Rendered) { '-windowed -RenderOffscreen -ForceRes -ResX=1280 -ResY=720 -KalmalaMinimapVerification' } else { '-nullrhi' }
$common = "-game $renderer -nosound -unattended -nosplash -DDC-ForceMemoryCache -KalmalaPlayerControlsTest"
if ($WetStamina) { $common += ' -KalmalaWetStaminaTest' }
if ($MovementAudio) { $common += ' -KalmalaMovementAudioTest' }
$server = $null
$client = $null
try {
    $server = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -KalmalaMinimapScreenshot=`"$output/host.png`" -abslog=`"$serverLog`" -UserDir=`"$output/Host`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited) { throw 'Server exited during startup.' }
        if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Server startup timed out.' }
    $client = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -KalmalaMinimapScreenshot=`"$output/client.png`" -abslog=`"$clientLog`" -UserDir=`"$output/Client`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A controls test peer exited.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Controls local result: FAIL') { throw 'Controls verification failed; inspect logs.' }
        $localPass = $serverText -match 'Controls local result: PASS Authority=1 Parts=9 Jump=1' -and $clientText -match 'Controls local result: PASS Authority=0 Parts=9 Jump=1'
        $remotePass = $serverText -match 'Controls server sprint: Remote=1' -and $serverText -match 'Controls server jump: Remote=1' -and $serverText -match 'Controls server release: Remote=1'
        $renderPass = !$Rendered -or ((Test-Path "$output/host.png") -and (Test-Path "$output/client.png"))
        $wetPass = !$WetStamina -or ($serverText -match 'Wet stamina: Passed=1 Authority=1 Remote=0' -and $serverText -match 'Wet stamina: Passed=1 Authority=1 Remote=1' -and $clientText -match 'Wet stamina: Passed=1 Authority=0 Remote=0')
        $movementAudioPass = !$MovementAudio -or (
            $serverText -match 'Movement audio local: PawnAuthority=1 Event=Footfall CueSubmitted=1 Asset=MovementFootfallCue' -and
            $serverText -match 'Movement audio local: PawnAuthority=1 Event=Jump CueSubmitted=1 Asset=MovementJumpCue' -and
            $serverText -match 'Movement audio local: PawnAuthority=1 Event=Landing CueSubmitted=1 Asset=MovementLandingCue' -and
            $clientText -match 'Movement audio local: PawnAuthority=0 Event=Footfall CueSubmitted=1 Asset=MovementFootfallCue' -and
            $clientText -match 'Movement audio local: PawnAuthority=0 Event=Jump CueSubmitted=1 Asset=MovementJumpCue' -and
            $clientText -match 'Movement audio local: PawnAuthority=0 Event=Landing CueSubmitted=1 Asset=MovementLandingCue'
        )
        if ($localPass -and $remotePass -and $renderPass -and $wetPass -and $movementAudioPass) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Player controls/movement audio verification timed out.' }
    if ($clientText -notmatch "Client received world-generation identity: Seed=418") { throw 'Client world identity mismatch.' }
    foreach ($match in [regex]::Matches($serverText, 'Controls server sprint: Remote=\d Speed=([\d.]+) Base=([\d.]+)')) {
        $statusFactor = if ($WetStamina) { 0.9 } else { 1.0 }
        if ([Math]::Abs([double]$match.Groups[1].Value - 1.5 * $statusFactor * [double]$match.Groups[2].Value) -gt 0.2) { throw 'Server sprint did not retain its status multiplier.' }
    }
    if ($MovementAudio) {
        Write-Output 'PASS: host/client models and controls plus owner-local footfall, jump, and landing cues; remote movement remains server-observed.'
    }
    else {
        Write-Output 'PASS: host/client models, bound jump/sprint/release, landing, and server-observed remote movement.'
    }
}
finally {
    foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    Write-Output "Scenario logs: $output"
}
