param([int]$Port = 17849, [switch]$Rendered, [int]$Width = 1280, [int]$Height = 720,
    [int]$TextScale = 100, [int]$Contrast = 0, [switch]$EquipmentView)
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path $env:TEMP ('KalmalaInventory-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$hostShaderDir = Join-Path $output 'Host\ShaderWorkingDir'
$clientShaderDir = Join-Path $output 'Client\ShaderWorkingDir'
if ($Rendered) { New-Item -ItemType Directory -Path $hostShaderDir, $clientShaderDir -Force | Out-Null }
$common = "-game -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaInventoryTest -KalmalaUIDeveloperTextScale=$TextScale -KalmalaUIDeveloperContrast=$Contrast"
if ($Rendered) { $common += " -windowed -RenderOffscreen -ForceRes -ResX=$Width -ResY=$Height" } else { $common += ' -nullrhi' }
if ($EquipmentView) { $common += ' -KalmalaEquipmentView' }
$scrollState = if ($EquipmentView) { '[01]' } else { '1' }
$hostShader = if ($Rendered) { "-ShaderWorkingDir=`"$hostShaderDir`"" } else { '' }
$clientShader = if ($Rendered) { "-ShaderWorkingDir=`"$clientShaderDir`"" } else { '' }
$hostCapture = if ($Rendered) { "-KalmalaInventoryCapture=`"$output\Host\inventory`"" } else { '' }
$clientCapture = if ($Rendered) { "-KalmalaInventoryCapture=`"$output\Client\inventory`"" } else { '' }
$server = $null
$client = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common $hostShader $hostCapture -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited) { throw 'Listen server exited during startup.' }
        if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Listen server readiness timed out.' }
    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common $clientShader $clientCapture -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A peer exited before verification.' }
        $serverText = if (Test-Path $serverLog) { Get-Content $serverLog -Raw } else { '' }
        $clientText = if (Test-Path $clientLog) { Get-Content $clientLog -Raw } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Inventory server: Passed=0|Harvest inventory: Passed=0|Inventory remote: Empty=0|Inventory owner: Rejected=0') { throw 'Inventory verification failed.' }
        $ownerIndex = $clientText.IndexOf('Inventory owner: Rejected=1 Wood=7 Slots=1')
        if ([regex]::Matches($serverText, 'Inventory server: Passed=1 Wood=7 Slots=1').Count -eq 2 `
            -and [regex]::Matches($serverText, 'Harvest inventory: Passed=1 Materials=3 Range=1 Full=1 Malformed=1 Duplicate=1 SparseDelta=1').Count -eq 2 `
            -and $ownerIndex -ge 0 -and $clientText.IndexOf('Inventory remote: Empty=1', $ownerIndex) -gt $ownerIndex `
            -and $serverText.Contains('Inventory presentation: Owner=1 Wood=7 ReadOnly=1') `
            -and $clientText.Contains('Inventory presentation: Owner=1 Wood=7 ReadOnly=1') `
            -and $serverText -match "Inventory grid: PackSlots=16 Filled=1 Empty=15 CarriedTools=\d+ Scrollable=$scrollState ReadOnly=1" `
            -and $clientText -match "Inventory grid: PackSlots=16 Filled=1 Empty=15 CarriedTools=\d+ Scrollable=$scrollState ReadOnly=1") { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Inventory host/client scenario timed out.' }
    if ($clientText -notmatch 'Item gain receipts owner: Present=1 Bounded=1' -or
        $clientText -notmatch 'Item gain receipts remote: Empty=1' -or
        ($serverText + $clientText) -match 'Item gain receipts remote: Empty=0') {
        throw 'Accepted item-gain receipts did not preserve owner delivery/privacy.'
    }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Client world identity mismatch.' }
    if ($Rendered) {
        $captures = @("$output\Host\inventory-empty.png", "$output\Host\inventory-filled.png",
            "$output\Client\inventory-empty.png", "$output\Client\inventory-filled.png")
        $deadline = (Get-Date).AddSeconds(35)
        while (($captures | Where-Object { !(Test-Path $_) }).Count -gt 0 -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 250 }
        if (($captures | Where-Object { !(Test-Path $_) }).Count -gt 0) { throw 'Inventory empty/filled slot screenshots timed out.' }
        foreach ($capture in $captures) {
            if ((Get-Item -LiteralPath $capture).Length -le 32) { throw "Inventory capture is empty: $capture" }
        }
        # Capture settling can outlast the transaction checks; validate fresh fixture logs.
        $serverText = Get-Content $serverLog -Raw
        $clientText = Get-Content $clientLog -Raw
        . (Join-Path $PSScriptRoot 'Read-InventoryCapture.ps1')
        $captureScrollState = if ($PSBoundParameters.ContainsKey('EquipmentView') -and $PSBoundParameters['EquipmentView']) { '[01]' } else { '1' }
        $captureLogs = Read-KalmalaInventoryCaptureLogs -ServerLog $serverLog -ClientLog $clientLog -ScrollState $captureScrollState
        $serverText = $captureLogs.Server
        $clientText = $captureLogs.Client
        foreach ($peerText in @($serverText, $clientText)) {
            if ($peerText -notmatch 'Inventory grid fixture: State=Empty PackSlots=16 Filled=0 Empty=16 CarriedTools=0' `
                -or $peerText -notmatch "Inventory grid fixture: State=Filled PackSlots=16 Filled=1 Empty=15 CarriedTools=\d+ Scrollable=$scrollState") {
                throw 'Inventory empty/filled cell fixture did not render the expected fixed capacity.'
            }
        }
    }
    Write-Output "PASS: inventory capacity/empty cells and live owner slots at ${Width}x${Height}, text $TextScale%, contrast $Contrast; server grants/rejections, privacy, and read-only presentation passed."
}
finally {
    foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    Write-Output "Scenario logs: $output"
}
