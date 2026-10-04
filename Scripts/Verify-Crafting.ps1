param([int]$Port = 17869, [switch]$Rendered, [int]$Width = 1280, [int]$Height = 720,
    [int]$TextScale = 100, [int]$Contrast = 0)
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path $env:TEMP ('KalmalaCrafting-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$hostShaderDir = Join-Path $output 'Host\ShaderWorkingDir'
$clientShaderDir = Join-Path $output 'Client\ShaderWorkingDir'
New-Item -ItemType Directory -Path $hostShaderDir, $clientShaderDir -Force | Out-Null
$common = "-game -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaCraftingTest -KalmalaUIDeveloperTextScale=$TextScale -KalmalaUIDeveloperContrast=$Contrast -ExecCmds=`"t.MaxFPS 60`""
if ($Rendered) { $common += " -windowed -RenderOffscreen -ForceRes -ResX=$Width -ResY=$Height" } else { $common += ' -nullrhi' }
$hostShader = if ($Rendered) { "-ShaderWorkingDir=`"$hostShaderDir`"" } else { '' }
$clientShader = if ($Rendered) { "-ShaderWorkingDir=`"$clientShaderDir`"" } else { '' }
$serverCapture = if ($Rendered) { "-KalmalaCraftingCapture=`"$output\host.png`"" } else { '' }
$clientCapture = if ($Rendered) { "-KalmalaCraftingCapture=`"$output\client.png`"" } else { '' }
$server = $null
$client = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common $hostShader $serverCapture -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited) { throw 'Listen server exited during startup.' }
        if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Listen server readiness timed out.' }
    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common $clientShader $clientCapture -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(120)
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A peer exited before verification.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Crafting fixture FAILED:|Crafting [^\r\n]*Passed=0|M9 camp [^\r\n]*Passed=0|Restored=0') { throw 'Crafting verification failed; inspect retained logs.' }
        $ready = [regex]::Matches($serverText, 'Crafting server gates: Passed=1').Count -eq 2 `
            -and [regex]::Matches($serverText, 'Crafting server final: Passed=1').Count -eq 2 `
            -and $serverText.Contains('Crafting owner final: Passed=1 Authority=1 WorkbenchKit=2 Slots=1') `
            -and $clientText.Contains('Crafting owner final: Passed=1 Authority=0 WorkbenchKit=2 Slots=1') `
            -and $serverText.Contains('Crafting presentation: Passed=1 Restored=1') `
            -and $clientText.Contains('Crafting presentation: Passed=1 Restored=1') `
            -and $serverText.Contains('M9 tool feedback: Passed=1') `
            -and $clientText.Contains('M9 tool feedback: Passed=1') `
            -and $serverText.Contains('Inventory inspection: FocusAndKeys=1') `
            -and $clientText.Contains('Inventory inspection: FocusAndKeys=1') `
            -and $serverText.Contains('M9 camp feedback: Passed=1') `
            -and $clientText.Contains('M9 camp feedback: Passed=1')
        foreach ($peerText in @($serverText, $clientText)) {
            $ready = $ready -and $peerText.Contains('Recipe browsing: SelectionKept=1 Category=1 NoResults=1 Restored=1 SearchFocus=1')
        }
        $gridPattern = 'Build slot grid: Slots=(\d+) Unavailable=(\d+) Selected=(\d+) Focused=1 ReadOnly=1 Scrollable=1 Navigation=1'
        $serverGrid = [regex]::Match($serverText, $gridPattern)
        $clientGrid = [regex]::Match($clientText, $gridPattern)
        $serverGridSlots = if ($serverGrid.Success) { [int]$serverGrid.Groups[1].Value } else { 0 }
        $serverGridUnavailable = if ($serverGrid.Success) { [int]$serverGrid.Groups[2].Value } else { 0 }
        $clientGridSlots = if ($clientGrid.Success) { [int]$clientGrid.Groups[1].Value } else { 0 }
        $clientGridUnavailable = if ($clientGrid.Success) { [int]$clientGrid.Groups[2].Value } else { 0 }
        $ready = $ready -and $serverGrid.Success -and $clientGrid.Success `
            -and $serverGridSlots -gt 0 -and $serverGridUnavailable -gt 0 `
            -and $clientGridSlots -gt 0 -and $clientGridUnavailable -gt 0
        $ready = $ready -and [regex]::Matches($serverText, 'Crafting RPC: Recipe=Forged Batch=1 Accepted=0').Count -eq 2 `
            -and [regex]::Matches($serverText, 'Crafting RPC: Recipe=Workbench Batch=2147483647 Accepted=0').Count -eq 2 `
            -and [regex]::Matches($serverText, 'Crafting placement RPC: Accepted=0').Count -eq 2
        foreach ($statePattern in @('Fuel=60 Lit=1 Wet=0 Warmth=1 State=1', 'Fuel=48 Lit=0 Wet=(?:9[6-9]|100) Warmth=0 State=2')) {
            $serverNames = [regex]::Matches($serverText, ('Crafting fire server: Name=(\S+) ' + $statePattern)) | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique
            $clientNames = [regex]::Matches($clientText, ('Crafting fire client: Name=(\S+) ' + $statePattern)) | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique
            $ready = $ready -and @($serverNames).Count -eq 2 -and @($clientNames).Count -eq 2
        }
        if ($Rendered) {
            foreach ($peerName in @('host', 'client')) {
                foreach ($suffix in @('', '-details', '-feedback', '-inspection')) {
                    $ready = $ready -and (Test-Path "$output\$peerName$suffix.png")
                }
            }
            foreach ($peerText in @($serverText, $clientText)) {
                $ready = $ready -and $peerText.Contains('Construction feedback: Passed=1') `
                    -and $peerText.Contains('Crafting review scroll: Section=Details Passed=1') `
                    -and $peerText.Contains('Crafting review scroll: Section=Feedback Passed=1')
                $ready = $ready -and $peerText.Contains('Inventory detail review: Scrolled=1')
            }
        }
        if ($ready) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if (!$ready) { throw 'Crafting host/client scenario timed out.' }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Client identity mismatch.' }
    Write-Output 'PASS: build-grid focus/selection/navigation/unavailable states, server validation/payment/atomicity gates, camp feedback, exact inventory, matching fires, and local menu input restoration.'
}
finally {
    foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    Write-Output "Scenario logs: $output"
}
