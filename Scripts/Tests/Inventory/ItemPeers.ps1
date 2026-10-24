[CmdletBinding(DefaultParameterSetName = 'Run')]
param(
    [Parameter(Mandatory, ParameterSetName = 'Run')]
    [ValidateSet('CraftEquipment', 'WeaponProgression', 'Meal', 'Antidote', 'PoisonedArrows')]
    [string]$Scenario,
    [Parameter(Mandatory, ParameterSetName = 'List')][switch]$ListScenarios,
    [ValidateRange(1024, 65535)][int]$Port = 17892,
    [string]$OutputDirectory = '',
    [string]$Project = '',
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
)
$ErrorActionPreference = 'Stop'
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')

# One representative per behavior. Fixture flags/logs retain their internal names.
$scenarios = [ordered]@{
    'CraftEquipment' = @{
        Description = 'Craft, exact payment, GUID equip, armor/wear and owner privacy'
        Flag = '-KalmalaA8bVestPeerTest'
        Label = 'A8b Vest peer'
        Server = @('A8b Vest peer server: Passed=1 RemoteOwner=1 Craft=1 Payment=1 Guid=1 Equip=1', 'A8b Vest peer listen host: Passed=1 OwnVest=0')
        Owner = 'A8b Vest peer owner: Passed=1 Authority=0 Craft=1 Payment=1 Guid=1 Equip=1 ArmorDamage=1 Condition=1'
        Observer = 'A8b Vest peer observer: Passed=1 Authority=0 TargetPrivate=1'
    }
    'WeaponProgression' = @{
        Description = 'Forge tool progression, Main Hand equip and accepted melee damage'
        Flag = '-KalmalaA8bFangSpearPeerTest'
        Label = 'A8b Fang Spear peer'
        Server = @('A8b Fang Spear peer server: Passed=1 Craft=1 Payment=1 Equip=1 Stab=100 Damage=100 Durability=200', 'A8b Fang Spear peer listen host: Passed=1 HostItems=Stable')
        Owner = 'A8b Fang Spear peer owner: Passed=1 Authority=0 Craft=1 Payment=1 MainHand=1 Stab=100 Damage=100 Durability=200'
        Observer = 'A8b Fang Spear peer observer: Passed=1 Authority=0 PublicHealth=1 OwnerPrivate=1'
    }
    'Meal' = @{
        Description = 'Heated Cauldron cooking, meal effect and duplicate-use rejection'
        Flag = '-KalmalaA8bPeatHeartStewPeerTest'
        Label = 'A8b Peat Heart Stew peer'
        Server = @('A8b Peat Heart Stew peer server: Passed=1 Craft=1 Payment=1 Output=2 Eat=1 Meal=1 NoStack=1', 'A8b Peat Heart Stew peer listen host: Passed=1 HostItems=Stable Meal=0')
        Owner = 'A8b Peat Heart Stew peer owner: Passed=1 Authority=0 Craft=1 Payment=1 Output=2 Eat=1 Meal=1 NoStack=1'
        Observer = 'A8b Peat Heart Stew peer observer: Passed=1 Authority=0 TargetPrivate=1 MealPrivate=1'
    }
    'Antidote' = @{
        Description = 'Poison treatment, resistance and cooldown rejection'
        Flag = '-KalmalaA8bAntidotePeerTest'
        Label = 'A8b Antidote peer'
        Server = @('A8b Antidote peer server: Passed=1 Craft=2 Payment=1 Output=2 ClearWeak=1 ReduceMajor=1 Resist=1 CooldownReject=1 DosePreserved=1', 'A8b Antidote peer listen host: Passed=1 HostItems=Stable')
        Owner = 'A8b Antidote peer owner: Passed=1 Authority=0 Resistance=1 CooldownReject=1 DosePreserved=1'
        Observer = 'A8b Antidote peer observer: Passed=1 Authority=0 PublicPoisonDamage=1 TargetPrivate=1'
    }
    'PoisonedArrows' = @{
        Description = 'Crafted ammunition, accepted projectile hit and poison tick'
        Flag = '-KalmalaA8bFangArrowsPeerTest'
        Label = 'A8b Fang Arrows peer'
        Server = @('A8b Fang Arrows peer server: Passed=1 Craft=1 Payment=1 Output=20 Shot=1 Pierce=50 Poison=5 Tick=1', 'A8b Fang Arrows peer listen host: Passed=1 OwnMaterials=0 OwnArrows=0')
        Owner = 'A8b Fang Arrows peer owner: Passed=1 Authority=0 Craft=1 Payment=1 Output=20 Shot=1 Ammo=19'
        Observer = 'A8b Fang Arrows peer observer: Passed=1 Authority=0 PublicPoisonDamage=1 TargetPrivate=1'
    }
}
if ($ListScenarios) {
    $scenarios.GetEnumerator() | ForEach-Object {
        [pscustomobject]@{ Scenario = $_.Key; Coverage = $_.Value.Description }
    }
    return
}
$case = $scenarios[$Scenario]
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot
$projectPath = if ($Project) { $Project } else { Join-Path $repoPaths.ProjectRoot 'Kalmala.uproject' }
if (!(Test-Path -LiteralPath $projectPath -PathType Leaf)) { throw "Project not found: $projectPath" }
if (!(Test-Path -LiteralPath $Editor -PathType Leaf)) { throw "Editor not found: $Editor" }
$output = if ($OutputDirectory) { [IO.Path]::GetFullPath($OutputDirectory) } else {
    Join-Path $env:TEMP ('ItemPeers-' + [guid]::NewGuid().ToString('N'))
}
if ($output.Length -gt 180) { throw "Item peer output root is too long: $output" }
# Never reuse saves or old pass markers from a previous run.
if (Test-Path -LiteralPath $output) { throw "Output directory must be new: $output" }
$hostDir = Join-Path $output 'Host'
$ownerDir = Join-Path $output 'Owner'
$observerDir = Join-Path $output 'Observer'
$serverLog = Join-Path $output 'server.log'
$ownerLog = Join-Path $output 'owner.log'
$observerLog = Join-Path $output 'observer.log'
New-Item -ItemType Directory -Path $hostDir, $ownerDir, $observerDir -Force | Out-Null
$common = "-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush $($case.Flag) -ExecCmds=`"t.MaxFPS 60`""
$server = $null
$owner = $null
$observer = $null
function Read-PeerLog([string]$Path) {
    if (Test-Path -LiteralPath $Path) { Get-Content -LiteralPath $Path -Raw } else { '' }
}
function Assert-PeersRunning($Peers) {
    foreach ($peer in $Peers) {
        if ($peer.HasExited) { throw "$Scenario peer exited before acceptance: $($peer.ExitCode). Logs: $output" }
    }
}
try {
    $server = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -UserDir=`"$hostDir`" -abslog=`"$serverLog`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        Assert-PeersRunning @($server)
        $ready = (Read-PeerLog $serverLog) -match 'GameNetDriver.*listening on port'
        if (!$ready) { Start-Sleep -Milliseconds 500 }
    } while (!$ready -and (Get-Date) -lt $deadline)
    if (!$ready) { throw "$Scenario listen server readiness timed out." }

    $owner = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" 127.0.0.1:$Port -WorldSeed=999 $common -UserDir=`"$ownerDir`" -abslog=`"$ownerLog`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        Assert-PeersRunning @($server, $owner)
        $joined = [regex]::Matches((Read-PeerLog $serverLog), 'LogNet: Join succeeded:').Count -ge 1
        if (!$joined) { Start-Sleep -Milliseconds 500 }
    } while (!$joined -and (Get-Date) -lt $deadline)
    if (!$joined) { throw "$Scenario owner did not join before observer launch." }

    $observer = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" 127.0.0.1:$Port -WorldSeed=1000 $common -UserDir=`"$observerDir`" -abslog=`"$observerLog`""
    $deadline = (Get-Date).AddSeconds(240)
    do {
        Assert-PeersRunning @($server, $owner, $observer)
        $serverText = Read-PeerLog $serverLog
        $ownerText = Read-PeerLog $ownerLog
        $observerText = Read-PeerLog $observerLog
        $allText = "$serverText`n$ownerText`n$observerText"
        $failure = [regex]::Escape($case.Label) + ' (FAILED:|(server|owner|listen host|observer): Passed=0)'
        if ($allText -match "Fatal error:|Assertion failed:|Ensure condition failed:|$failure") {
            throw "$Scenario peer verification failed; inspect retained logs: $output"
        }
        $passed = $serverText.Contains($case.Server[0]) -and $serverText.Contains($case.Server[1]) `
            -and $ownerText.Contains($case.Owner) -and $observerText.Contains($case.Observer)
        $identity = $ownerText.Contains('Client received world-generation identity: Seed=418') `
            -and $observerText.Contains('Client received world-generation identity: Seed=418')
        if (!($passed -and $identity)) { Start-Sleep -Milliseconds 500 }
    } while (!($passed -and $identity) -and (Get-Date) -lt $deadline)
    if (!($passed -and $identity)) { throw "$Scenario acceptance or world identity timed out. Logs: $output" }
    Write-Output "PASS: $Scenario — $($case.Description)."
}
finally {
    foreach ($peer in @($observer, $owner, $server)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Scenario logs: $output"
}
