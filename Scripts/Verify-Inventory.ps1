param([int]$Port = 17849, [switch]$Rendered, [int]$Width = 1280, [int]$Height = 720,
    [int]$TextScale = 100, [int]$Contrast = 0, [switch]$NotificationReview)
$ErrorActionPreference = 'Stop'
if ($NotificationReview -and !$Rendered) { throw 'Notification review requires -Rendered.' }
$panelRemovalCheck = Join-Path $PSScriptRoot 'Verify-InventoryPanelRemoval.ps1'
& $panelRemovalCheck
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path $env:TEMP ('KalmalaInventory-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$hostShaderDir = Join-Path $output 'Host\ShaderWorkingDir'
$clientShaderDir = Join-Path $output 'Client\ShaderWorkingDir'
if ($Rendered) { New-Item -ItemType Directory -Path $hostShaderDir, $clientShaderDir -Force | Out-Null }
$common = "-game -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaInventoryTest -KalmalaNotificationBaselineAudit -KalmalaUIDeveloperTextScale=$TextScale -KalmalaUIDeveloperContrast=$Contrast"
if ($Rendered) { $common += " -windowed -RenderOffscreen -ForceRes -ResX=$Width -ResY=$Height" } else { $common += ' -nullrhi' }
$hostNotificationCapture = if ($NotificationReview) { "-KalmalaNotificationCapture=`"$output\Host\notice`"" } else { '' }
$clientNotificationCapture = if ($NotificationReview) { "-KalmalaNotificationCapture=`"$output\Client\notice`"" } else { '' }
$hostShader = if ($Rendered) { "-ShaderWorkingDir=`"$hostShaderDir`"" } else { '' }
$clientShader = if ($Rendered) { "-ShaderWorkingDir=`"$clientShaderDir`"" } else { '' }
$server = $null
$client = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common $hostShader $hostNotificationCapture -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited) { throw 'Listen server exited during startup.' }
        if ((Test-Path $serverLog) -and (Select-String $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Listen server readiness timed out.' }
    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common $clientShader $clientNotificationCapture -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A peer exited before verification.' }
        $serverText = if (Test-Path $serverLog) { [string](Get-Content -LiteralPath $serverLog -Raw) } else { '' }
        $clientText = if (Test-Path $clientLog) { [string](Get-Content -LiteralPath $clientLog -Raw) } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Inventory server: Passed=0|Harvest inventory: Passed=0|Inventory remote: Empty=0|Inventory owner: Rejected=0') { throw 'Inventory verification failed.' }
        $ownerIndex = $clientText.IndexOf('Inventory owner: Rejected=1 Wood=7 Slots=1')
        if ([regex]::Matches($serverText, 'Inventory server: Passed=1 Wood=7 Slots=1').Count -eq 2 `
            -and [regex]::Matches($serverText, 'Harvest inventory: Passed=1 Materials=3 Range=1 Full=1 Malformed=1 Duplicate=1 SparseDelta=1').Count -eq 2 `
            -and $ownerIndex -ge 0 -and $clientText.IndexOf('Inventory remote: Empty=1', $ownerIndex) -gt $ownerIndex `
            -and (!$NotificationReview -or (
                $serverText -match 'Notification combined layout: Complete=1 OwnerLocal=1 PeerPrivateHidden=1 Passive=1 Rows=3' `
                -and $clientText -match 'Notification combined layout: Complete=1 OwnerLocal=1 PeerPrivateHidden=1 Passive=1 Rows=3' `
                -and $serverText -match 'Notification modal fixture: Collapsed=1 Rows=3' `
                -and $clientText -match 'Notification modal fixture: Collapsed=1 Rows=3' `
                -and $serverText -match 'Notification restored fixture: Visible=1 Rows=3' `
                -and $clientText -match 'Notification restored fixture: Visible=1 Rows=3'))) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Inventory host/client scenario timed out.' }
    if ($clientText -notmatch 'Item gain receipts owner: Present=1 Bounded=1' -or
        $clientText -notmatch 'Item gain receipts remote: Empty=1' -or
        ($serverText + $clientText) -match 'Item gain receipts remote: Empty=0') {
        throw 'Accepted item-gain receipts did not preserve owner delivery/privacy.'
    }
    if ($serverText -notmatch 'Notification owner baseline: Silent=1 Rows=0 Sources=5' -or
        $clientText -notmatch 'Notification owner baseline: Silent=1 Rows=0 Sources=5') {
        throw 'A peer replayed existing owner state while establishing notification baselines.'
    }
    if ($NotificationReview) {
        foreach ($peer in @(@{ Name = 'Host'; Text = $serverText }, @{ Name = 'Client'; Text = $clientText })) {
            if ($peer.Text -notmatch 'Notification reconnect baseline: Silent=1 Rows=0 Sources=3' -or
                $peer.Text -notmatch 'Notification combined fixture: Ready=1 Rows=3 Skill=1 Item=1 Discovery=1' -or
                $peer.Text -notmatch "Notification combined layout: Complete=1 OwnerLocal=1 PeerPrivateHidden=1 Passive=1 Rows=3 Scale=$TextScale Contrast=$Contrast Motion=Static" -or
                $peer.Text -notmatch 'Notification modal fixture: Collapsed=1 Rows=3' -or
                $peer.Text -notmatch 'Notification restored fixture: Visible=1 Rows=3') {
                throw "$($peer.Name) peer did not verify its combined notification, modal, static-motion and owner-local presentation."
            }
            $otherOwner = if ($peer.Owner -eq 'Host') { 'Client' } else { 'Host' }
            if ($peer.Text.Contains("$otherOwner owner discovery")) { throw "$($peer.Name) review fixture exposed the other peer's private label." }
        }
    }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Client world identity mismatch.' }
    if ($Rendered -and $NotificationReview) {
        $noticeCaptures = @(
            "$output\Host\notice-combined.png", "$output\Host\notice-modal.png", "$output\Host\notice-restored.png",
            "$output\Client\notice-combined.png", "$output\Client\notice-modal.png", "$output\Client\notice-restored.png")
        $deadline = (Get-Date).AddSeconds(20)
        while (($noticeCaptures | Where-Object { !(Test-Path $_) }).Count -gt 0 -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 250 }
        if (($noticeCaptures | Where-Object { !(Test-Path $_) }).Count -gt 0) { throw 'Combined notification captures timed out on a peer.' }
        foreach ($capture in $noticeCaptures) {
            if ((Get-Item -LiteralPath $capture).Length -le 32) { throw "Notification capture is empty: $capture" }
        }
        # Capture settling can outlast the transaction checks; retain fresh final peer output.
        $serverText = Get-Content $serverLog -Raw
        $clientText = Get-Content $clientLog -Raw
    }
    Write-Output 'PASS: inventory grants/rejections, owner privacy, notification baselines, and legacy-panel absence preflight.'
    if ($NotificationReview) { Write-Output 'PASS: both peers rendered combined skill/item/discovery rows, modal collapse/restore, passive text, and unique owner-local review labels.' }
}
finally {
    foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    Write-Output "Scenario logs: $output"
}
