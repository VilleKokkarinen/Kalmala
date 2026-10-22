param([int]$Port = 17849, [switch]$Rendered, [int]$Width = 1280, [int]$Height = 720,
    [int]$TextScale = 100, [int]$Contrast = 0, [switch]$NotificationReview, [switch]$ContractOnly)
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot

$ErrorActionPreference = 'Stop'
if ($NotificationReview -and !$Rendered) { throw 'Notification review requires -Rendered.' }
if ($ContractOnly -and ($Rendered -or $NotificationReview)) { throw '-ContractOnly cannot be combined with rendered or notification review options.' }

function Invoke-InventoryContractChecks {
    $projectRoot = $repoPaths.ProjectRoot
    $retiredPaths = @(
        'Source\KalmalaUI\Private\KalmalaInventorySubsystem.cpp',
        'Source\KalmalaUI\Public\KalmalaInventorySubsystem.h',
        'Source\KalmalaUI\Private\Tests\KalmalaInventoryWidgetTest.cpp',
        'Scripts\Read-InventoryCapture.ps1'
    )
    foreach ($relativePath in $retiredPaths) {
        if (Test-Path -LiteralPath (Join-Path $projectRoot $relativePath)) {
            throw "Retired persistent inventory HUD source remains: $relativePath"
        }
    }

    $uiRoot = Join-Path $projectRoot 'Source\KalmalaUI'
    $uiSources = Get-ChildItem -LiteralPath $uiRoot -Recurse -File |
        Where-Object { $_.Extension -in @('.h', '.cpp') }
    $retiredSourceTerms = @(
        'UKalmalaInventorySubsystem',
        'UKalmalaInventoryWidget',
        'SetCraftingMenuSuppressed',
        'IsCraftingMenuSuppressed',
        'Pack | Build/craft:',
        'Inventory grid fixture:',
        'Inventory presentation: Owner=1',
        'Crafting HUD overlap:',
        'BuildPreparedFoodDetails'
    )
    foreach ($sourceFile in $uiSources) {
        $source = Get-Content -LiteralPath $sourceFile.FullName -Raw
        foreach ($term in $retiredSourceTerms) {
            if ($source.Contains($term)) {
                throw "Retired panel source remains in $($sourceFile.FullName): $term"
            }
        }
    }

    $inventoryScript = Get-Content -LiteralPath $PSCommandPath -Raw
    $runtimeStart = $inventoryScript.LastIndexOf('# Live host/client inventory verification')
    if ($runtimeStart -lt 0) { throw 'Inventory peer-verification boundary marker is missing.' }
    $inventoryRuntime = $inventoryScript.Substring($runtimeStart)
    foreach ($term in @('Inventory presentation: Owner=1', 'Inventory grid fixture:', 'Read-InventoryCapture.ps1', '-EquipmentView')) {
        if ($inventoryRuntime.Contains($term)) {
            throw "Inventory runtime verification still depends on retired HUD output: $term"
        }
    }

    $menuSubsystem = Get-Content -LiteralPath (Join-Path $uiRoot 'Private\KalmalaInventoryMenuSubsystem.cpp') -Raw
    $menuWidget = Get-Content -LiteralPath (Join-Path $uiRoot 'Private\KalmalaInventoryMenuWidget.cpp') -Raw
    $menuTest = Get-Content -LiteralPath (Join-Path $uiRoot 'Private\Tests\KalmalaInventoryMenuSelectionTest.cpp') -Raw
    if (-not $menuSubsystem.Contains('CreateWidget<UKalmalaInventoryMenuWidget>') -or
        -not $menuSubsystem.Contains('InventoryWidget->Open()') -or
        -not $menuWidget.Contains('SetVisibility(ESlateVisibility::Collapsed)')) {
        throw 'The owner-local Inventory menu must remain on-demand and collapsed before opening.'
    }
    foreach ($emptyPath in @(
        'Normal gameplay keeps Inventory collapsed until opened',
        'Empty owner pack clears selection',
        'Empty Inventory retains forty grid cells',
        'Empty pack hides the stale detail panel'
    )) {
        if (-not $menuTest.Contains($emptyPath)) {
            throw "Inventory empty-state automation is missing: $emptyPath"
        }
    }

    $equipmentCoverageCases = @(
        @{
            Path = 'Source\KalmalaGameplay\Private\Tests\KalmalaInventoryHotbarTest.cpp'
            Automation = 'Kalmala.Gameplay.Inventory.MainHandHotbar'
            Assertions = @('Invalid and empty numbers preserve layout')
        },
        @{
            Path = 'Source\KalmalaGameplay\Private\Tests\KalmalaTwoHandedTest.cpp'
            Automation = 'Kalmala.Gameplay.Inventory.TwoHanded'
            Assertions = @('Full pack rejects returning two held items for bow', 'One free cell accepts hotbar two-hand swap')
        },
        @{
            Path = 'Source\KalmalaGameplay\Private\Tests\KalmalaEquipmentSaveTest.cpp'
            Automation = 'Kalmala.Gameplay.Inventory.EquipmentCurrentSaveFormat'
            Assertions = @('Current-format saves require pre-existing physical GUIDs', 'Validation does not rewrite an incompatible schema')
        },
        @{
            Path = 'Source\KalmalaGameplay\Private\Tests\KalmalaInventoryNetworkContractTest.cpp'
            Automation = 'Kalmala.Gameplay.Inventory.NetworkContract'
            Assertions = @('Main Hand intent carries only one physical identity', 'Inventory field is owner-only:', 'Inventory location is not a save field:')
        },
        @{
            Path = 'Source\KalmalaGameplay\Private\Tests\KalmalaInventoryGridTest.cpp'
            Automation = 'Kalmala.Gameplay.Inventory.SharedGrid'
            Assertions = @('Axe stays in slot three when equipped', 'New items use free space in owned-item order')
        },
        @{
            Path = 'Source\KalmalaUI\Private\Tests\KalmalaInventoryGridWidgetTest.cpp'
            Automation = 'Kalmala.UI.Inventory.NumberedHotbar'
            Assertions = @('Empty hotbar hides every cell', 'Hotbar omits empty cells and lower rows without renumbering')
        },
        @{
            Path = 'Source\KalmalaGameplay\Private\Tests\KalmalaInventoryEquipmentTest.cpp'
            Automation = 'Kalmala.Gameplay.Inventory.WornGearTransactions'
            Assertions = @('Wearing all six items vacates every original pack row', 'Returned grid row receives its own identity')
        },
        @{
            Path = 'Source\KalmalaUI\Private\Tests\KalmalaEquipmentIconTest.cpp'
            Automation = 'Kalmala.UI.CatalogueIcons.EquipmentCoverage'
            Assertions = @('CheckIcon(Item.ItemId);', 'for (const FKalmalaToolProfileData& Profile : Tools->Profiles) CheckIcon(Profile.ItemId);')
        },
        @{
            Path = 'Source\KalmalaUI\Private\Tests\KalmalaStatusHotbarTest.cpp'
            Automation = 'Kalmala.UI.CatalogueIcons.CompleteCoverage'
            Assertions = @('for (const auto& Item : Items->Items) Check(Item.ItemId);', 'imported texture loads')
        }
    )
    foreach ($coverageCase in $equipmentCoverageCases) {
        $casePath = Join-Path $projectRoot $coverageCase.Path
        if (-not (Test-Path -LiteralPath $casePath -PathType Leaf)) {
            throw "Equipment acceptance automation source is missing: $($coverageCase.Path)"
        }
        $caseSource = Get-Content -LiteralPath $casePath -Raw
        if (-not $caseSource.Contains($coverageCase.Automation)) {
            throw "Equipment acceptance automation is not registered: $($coverageCase.Automation)"
        }
        foreach ($assertion in $coverageCase.Assertions) {
            if (-not $caseSource.Contains($assertion)) {
                throw "Equipment acceptance assertion is missing from $($coverageCase.Automation): $assertion"
            }
        }
    }

    $inventoryComponent = Get-Content -LiteralPath (Join-Path $projectRoot 'Source\KalmalaGameplay\Private\KalmalaInventoryComponent.cpp') -Raw
    foreach ($fixtureAnchor in @(
        'KalmalaInventoryEquipmentPrivacyTest',
        'Inventory owner-only fixture: Ready=',
        'Inventory owner-only remote: Hidden='
    )) {
        if (-not $inventoryComponent.Contains($fixtureAnchor)) {
            throw "Live equipment-privacy fixture is missing: $fixtureAnchor"
        }
    }
    foreach ($runnerAnchor in @(
        '-KalmalaInventoryEquipmentPrivacyTest',
        'Inventory owner-only fixture: Ready=1 MainHand=1 Gear=1 Grid=1',
        'Inventory owner-only remote: Hidden=1 MainHand=1 Gear=1 Grid=1'
    )) {
        if (-not $inventoryScript.Contains($runnerAnchor)) {
            throw "Inventory peer runner does not require the equipment-privacy fixture: $runnerAnchor"
        }
    }

    $architecture = Get-Content -LiteralPath (Join-Path $projectRoot 'docs\02-technical-architecture.md') -Raw
    if ($architecture.Contains('UKalmalaInventorySubsystem') -or $architecture.Contains('hit-test-invisible pack panel')) {
        throw 'Technical architecture still describes the retired persistent inventory panel.'
    }

    & (Join-Path $repoPaths.TestsRoot 'Presentation\Verify-TutorialRouteFree.ps1')
    Write-Output 'PASS: legacy inventory HUD and bottom tutorial card are absent; on-demand Inventory and empty-state coverage remain.'
    Write-Output "PASS: $($equipmentCoverageCases.Count) equipment/hotbar/privacy/save/icon automation cases retain their key source assertions; live host/client privacy checks cover nonempty Main Hand, gear and grid fields."
}

Invoke-InventoryContractChecks
if ($ContractOnly) { return }

# Live host/client inventory verification begins here.
$project = Join-Path ($repoPaths.ProjectRoot) 'Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path $env:TEMP ('KalmalaInventory-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$hostShaderDir = Join-Path $output 'Host\ShaderWorkingDir'
$clientShaderDir = Join-Path $output 'Client\ShaderWorkingDir'
if ($Rendered) { New-Item -ItemType Directory -Path $hostShaderDir, $clientShaderDir -Force | Out-Null }
$common = "-game -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaInventoryTest -KalmalaInventoryEquipmentPrivacyTest -KalmalaNotificationBaselineAudit -KalmalaUIDeveloperTextScale=$TextScale -KalmalaUIDeveloperContrast=$Contrast"
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
    $equipmentFixtureSeenAt = $null
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A peer exited before verification.' }
        $serverText = if (Test-Path $serverLog) { [string](Get-Content -LiteralPath $serverLog -Raw) } else { '' }
        $clientText = if (Test-Path $clientLog) { [string](Get-Content -LiteralPath $clientLog -Raw) } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Inventory server: Passed=0|Harvest inventory: Passed=0|Inventory remote: Empty=0|Inventory owner: Rejected=0|Inventory owner-only fixture: Ready=0|Inventory owner-only remote: Hidden=0') { throw 'Inventory verification failed.' }
        if ($null -eq $equipmentFixtureSeenAt -and $serverText -match 'Inventory owner-only fixture: Ready=1 MainHand=1 Gear=1 Grid=1') {
            $equipmentFixtureSeenAt = Get-Date
        }
        $equipmentFixtureSettled = ($null -ne $equipmentFixtureSeenAt) -and
            (((Get-Date) - $equipmentFixtureSeenAt).TotalSeconds -ge 2)
        $ownerIndex = $clientText.IndexOf('Inventory owner: Rejected=1 Wood=7 Slots=1')
        if ([regex]::Matches($serverText, 'Inventory server: Passed=1 Wood=7 Slots=1').Count -eq 2 `
            -and [regex]::Matches($serverText, 'Harvest inventory: Passed=1 Materials=2 Range=1 Full=1 Malformed=1 Duplicate=1 SparseDelta=1').Count -eq 2 `
            -and $equipmentFixtureSettled `
            -and $serverText -match 'Inventory owner-only fixture: Ready=1 MainHand=1 Gear=1 Grid=1' `
            -and $clientText -match 'Inventory owner-only remote: Hidden=1 MainHand=1 Gear=1 Grid=1' `
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
            if ($peer.Text -notmatch 'Notification reconnect baseline: Silent=1 Rows=0 Sources=5' -or
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
