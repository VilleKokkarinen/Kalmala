param([int]$Port = 17920, [int]$Width = 1280, [int]$Height = 720, [int]$TextScale = 100, [int]$Contrast = 0, [switch]$ReducedMotion)
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path $env:TEMP ('kh-' + [guid]::NewGuid().ToString('N').Substring(0,8))
New-Item -ItemType Directory -Path $output | Out-Null
$motionArgument = if ($ReducedMotion) { '-KalmalaHotbarReducedMotion' } else { '' }
$reducedMotionValue = if ($ReducedMotion) { 1 } else { 0 }
$common = "-game -windowed -RenderOffscreen -ForceRes -ResX=$Width -ResY=$Height -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaHotbarScale=$TextScale -KalmalaHotbarContrast=$Contrast $motionArgument -ExecCmds=`"t.MaxFPS 60`""
$server = $null; $client = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -KalmalaHotbarCapture=`"$output/host`" -UserDir=`"$output/H`" -ShaderWorkingDir=`"$output/hs`" -abslog=`"$output/host.log`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        if ($server.HasExited) { throw 'Host exited during startup.' }
        if ((Test-Path "$output/host.log") -and (Select-String "$output/host.log" -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Host readiness timed out.' }
    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -KalmalaHotbarCapture=`"$output/client`" -UserDir=`"$output/C`" -ShaderWorkingDir=`"$output/cs`" -abslog=`"$output/client.log`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A peer exited during presentation.' }
        $ready = $true
        foreach ($peer in @('host','client')) {
            foreach ($phase in @('empty','populated','expired','details','icons','started','refreshed','ended')) { $ready = $ready -and (Test-Path "$output/$peer-$phase.png") }
        }
        if ($ready) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if (!$ready) { throw 'Status screenshot phases timed out.' }
    foreach ($peer in @('host','client')) {
        $log = Get-Content "$output/$peer.log" -Raw
        if ($log -match 'Fatal error:|Assertion failed:|Ensure condition failed:') { throw "$peer reported an engine failure." }
        if ($log -notmatch 'Hotbar owner snapshot:.*Local=1.*WeatherValid=1') { throw "$peer did not read its local pawn and valid replicated weather." }
        foreach ($phase in @('empty','expired')) {
            if ($log -notmatch "Hotbar fixture: Phase=$phase Entries=0 ReadOnly=1") { throw "$peer $phase did not remove entries." }
        }
        if ($log -notmatch 'Hotbar details: Focusable=1 OwnerLocal=1 LiveDetails=1' -or $log -notmatch 'Catalogue icon gallery: ItemsAndTools=48 ReadOnly=1') { throw "$peer details/gallery coverage failed." }
        if ($log -notmatch 'Hotbar fixture: Phase=populated Entries=6 ReadOnly=1 Bounds=(\d+),(\d+),(\d+),(\d+) Scale=\d+ Dpi=([\d.]+)') { throw "$peer did not paint six non-focusable entries." }
        $left=[int]$Matches[1]; $top=[int]$Matches[2]; $right=[int]$Matches[3]; $bottom=[int]$Matches[4]
        $dpi=[double]::Parse($Matches[5],[Globalization.CultureInfo]::InvariantCulture)
        if ($left -lt 0 -or $top -le (232*$dpi) -or $right -gt $Width -or $bottom -gt $Height) { throw "$peer hotbar bounds overlap minimap or exceed viewport: $left,$top,$right,$bottom" }
        foreach ($cue in @(@('started',1),@('refreshed',2),@('ended',3))) {
            $phase=$cue[0]; $kind=$cue[1]
            $pattern = "Hotbar transition fixture: Phase=$phase Kind=$kind Alpha=([\d.]+) ReducedMotion=$reducedMotionValue Passed=1"
            if ($log -notmatch $pattern) { throw "$peer did not render and retain its $phase cue." }
            $alpha=[double]::Parse($Matches[1],[Globalization.CultureInfo]::InvariantCulture)
            if ($ReducedMotion -and [Math]::Abs($alpha - 1.0) -gt 0.001) { throw "$peer reduced-motion $phase cue was not static at full alpha." }
        }
    }
    Write-Output "PASS: owner-local host/client empty, six-effect, removal and start/refresh/end cues at ${Width}x${Height}, text $TextScale%, contrast $Contrast, reduced motion $reducedMotionValue. UI fixture only; inspect captures."
}
finally {
    foreach ($process in @($client,$server)) { if ($null -ne $process -and !$process.HasExited) { Stop-Process -Id $process.Id } }
    Write-Output "Scenario logs: $output"
}
