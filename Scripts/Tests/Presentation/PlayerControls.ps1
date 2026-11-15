param(
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [int]$Port = 17843,
    [switch]$Rendered,
    [switch]$WetStamina,
    [switch]$MovementAudio,
    [switch]$OceanTraversalAudio,
    [switch]$WorldBoundary
)
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot

$ErrorActionPreference = 'Stop'
if ($WorldBoundary -and ($Rendered -or $WetStamina -or $MovementAudio -or $OceanTraversalAudio)) {
    throw 'WorldBoundary is a focused headless scenario and cannot be combined with the other PlayerControls switches.'
}
$tutorialCheck = Join-Path $repoPaths.TestsRoot 'Presentation\TutorialRouteFree.ps1'
& $tutorialCheck
$project = Join-Path ($repoPaths.ProjectRoot) 'Kalmala.uproject'
$output = Join-Path $env:TEMP ('KalmalaPlayerControls-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$observerLog = Join-Path $output 'observer.log'
$renderer = if ($Rendered) { '-windowed -RenderOffscreen -ForceRes -ResX=1280 -ResY=720 -KalmalaMinimapVerification' } else { '-nullrhi' }
$common = "-game $renderer -nosound -unattended -nosplash -DDC-ForceMemoryCache -KalmalaPlayerControlsTest"
if ($WorldBoundary) { $common += ' -KalmalaWorldBoundaryTest' }
if ($WetStamina) { $common += ' -KalmalaWetStaminaTest' }
if ($MovementAudio) { $common += ' -KalmalaMovementAudioTest' }
if ($OceanTraversalAudio) { $common += ' -KalmalaOceanTraversalAudioTest' }
$server = $null
$client = $null
$observer = $null
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
    if ($WorldBoundary) {
        $ownerDeadline = (Get-Date).AddSeconds(60)
        do {
            if ($server.HasExited -or $client.HasExited) { throw 'The boundary owner peer exited during login.' }
            $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
            if ($serverText -match 'World boundary fixture assigned peer role: Role=Owner') { break }
            Start-Sleep -Milliseconds 250
        } while ((Get-Date) -lt $ownerDeadline)
        if ((Get-Date) -ge $ownerDeadline) { throw 'The boundary owner did not finish login.' }
        $observer = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -KalmalaMinimapScreenshot=`"$output/observer.png`" -abslog=`"$observerLog`" -UserDir=`"$output/Observer`""
    }
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited -or $client.HasExited -or ($null -ne $observer -and $observer.HasExited)) { throw 'A controls test peer exited.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        $observerText = if (Test-Path $observerLog) { Get-Content $observerLog -Raw } else { '' }
        if (($serverText + $clientText + $observerText) -match 'Fatal error:|Assertion failed:|Controls local result: FAIL|World boundary result: Role=\w+ Passed=0') { throw 'Controls verification failed; inspect logs.' }
        $localPass = $serverText -match 'Controls local result: PASS Authority=1 Parts=12 Jump=1' -and $clientText -match 'Controls local result: PASS Authority=0 Parts=12 Jump=1'
        $remotePass = $serverText -match 'Controls server sprint: Remote=1' -and $serverText -match 'Controls server jump: Remote=1' -and $serverText -match 'Controls server release: Remote=1'
        $boundaryPass = $false
        if ($WorldBoundary) {
            $positionPattern = 'World boundary result: Role=(?<role>Server|Owner|Observer) Passed=(?<pass>[01]) X=(?<x>[-+]?\d+(?:\.\d+)?) Y=(?<y>[-+]?\d+(?:\.\d+)?) Z=(?<z>[-+]?\d+(?:\.\d+)?) Radius=(?<radius>[-+]?\d+(?:\.\d+)?) Limit=(?<limit>[-+]?\d+(?:\.\d+)?)'
            $serverBoundary = [regex]::Match($serverText, $positionPattern)
            $ownerBoundary = [regex]::Match($clientText, $positionPattern)
            $observerBoundary = [regex]::Match($observerText, $positionPattern)
            if ($serverBoundary.Success -and $ownerBoundary.Success -and $observerBoundary.Success) {
                $serverX = [double]$serverBoundary.Groups['x'].Value
                $serverY = [double]$serverBoundary.Groups['y'].Value
                $serverZ = [double]$serverBoundary.Groups['z'].Value
                $ownerX = [double]$ownerBoundary.Groups['x'].Value
                $ownerY = [double]$ownerBoundary.Groups['y'].Value
                $ownerZ = [double]$ownerBoundary.Groups['z'].Value
                $observerX = [double]$observerBoundary.Groups['x'].Value
                $observerY = [double]$observerBoundary.Groups['y'].Value
                $observerZ = [double]$observerBoundary.Groups['z'].Value
                $ownerError = [Math]::Sqrt([Math]::Pow($serverX - $ownerX, 2) + [Math]::Pow($serverY - $ownerY, 2) + [Math]::Pow($serverZ - $ownerZ, 2))
                $observerError = [Math]::Sqrt([Math]::Pow($serverX - $observerX, 2) + [Math]::Pow($serverY - $observerY, 2) + [Math]::Pow($serverZ - $observerZ, 2))
                $serverBounded = [double]$serverBoundary.Groups['radius'].Value -le [double]$serverBoundary.Groups['limit'].Value + 1.0
                $ownerBounded = [double]$ownerBoundary.Groups['radius'].Value -le [double]$ownerBoundary.Groups['limit'].Value + 1.0
                $observerBounded = [double]$observerBoundary.Groups['radius'].Value -le [double]$observerBoundary.Groups['limit'].Value + 5.0
                $serverProjectionPass = $serverText -match 'World boundary projection: Role=Server RemovedOutward=1 PreservedTangential=1 PreservedVertical=1'
                $ownerProjectionPass = $clientText -match 'World boundary projection: Role=Owner RemovedOutward=1 PreservedTangential=1 PreservedVertical=1'
                $boundaryPass = $serverBoundary.Groups['pass'].Value -eq '1' -and $ownerBoundary.Groups['pass'].Value -eq '1' -and $observerBoundary.Groups['pass'].Value -eq '1' -and $serverBounded -and $ownerBounded -and $observerBounded -and $ownerError -le 300.0 -and $observerError -le 300.0 -and $serverProjectionPass -and $ownerProjectionPass
            }
        }
        $controlsPass = if ($WorldBoundary) { $boundaryPass } else { $localPass -and $remotePass }
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
        $oceanTraversalAudioPass = !$OceanTraversalAudio -or (
            [regex]::Matches($serverText, 'Ocean traversal audio local: PawnAuthority=1 Event=Entry CueSubmitted=1 Asset=GeneratedOceanEntryCue').Count -eq 1 -and
            [regex]::Matches($serverText, 'Ocean traversal audio local: PawnAuthority=1 Event=Exit CueSubmitted=1 Asset=GeneratedOceanExitCue').Count -eq 1 -and
            [regex]::Matches($clientText, 'Ocean traversal audio local: PawnAuthority=0 Event=Entry CueSubmitted=1 Asset=GeneratedOceanEntryCue').Count -eq 1 -and
            [regex]::Matches($clientText, 'Ocean traversal audio local: PawnAuthority=0 Event=Exit CueSubmitted=1 Asset=GeneratedOceanExitCue').Count -eq 1
        )
        if (($serverText + $clientText) -match 'Ocean traversal audio local:.*CueSubmitted=0') { throw 'A generated-ocean traversal cue asset did not load; inspect logs.' }
        if ($controlsPass -and $renderPass -and $wetPass -and $movementAudioPass -and $oceanTraversalAudioPass) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Player controls/movement audio verification timed out.' }
    if ($clientText -notmatch "Client received world-generation identity: Seed=418") { throw 'Client world identity mismatch.' }
    if ($WorldBoundary -and $observerText -notmatch "Client received world-generation identity: Seed=418") { throw 'Observer world identity mismatch.' }
    foreach ($match in [regex]::Matches($serverText, 'Controls server sprint: Remote=\d Speed=([\d.]+) Base=([\d.]+)')) {
        $statusFactor = if ($WetStamina) { 0.92 } else { 1.0 }
        if ([Math]::Abs([double]$match.Groups[1].Value - 1.5 * $statusFactor * [double]$match.Groups[2].Value) -gt 0.2) { throw 'Server sprint did not retain its status multiplier.' }
    }
    if ($WorldBoundary) {
        Write-Output 'PASS: remote owner and server remain inside the finite-world capsule margin, retain tangent and vertical movement, and converge with the observer transform.'
    }
    elseif ($MovementAudio -and $OceanTraversalAudio) {
        Write-Output 'PASS: host/client models and controls plus owner-local footfall, jump, landing, and generated-ocean entry/exit cues; remote movement remains server-observed.'
    }
    elseif ($MovementAudio) {
        Write-Output 'PASS: host/client models and controls plus owner-local footfall, jump, and landing cues; remote movement remains server-observed.'
    }
    elseif ($OceanTraversalAudio) {
        Write-Output 'PASS: host and client each submitted exactly one owner-local generated-ocean entry and exit cue.'
    }
    else {
        Write-Output 'PASS: host/client models, bound jump/sprint/release, landing, and server-observed remote movement.'
    }
}
finally {
    foreach ($peer in @($observer, $client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    Write-Output "Scenario logs: $output"
}
