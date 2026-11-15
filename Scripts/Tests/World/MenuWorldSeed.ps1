[CmdletBinding()]
param(
    [int]$Port = 18159,
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [string]$OutputDirectory
)
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot

$ErrorActionPreference = 'Stop'
$project = Join-Path $repoPaths.ProjectRoot 'Kalmala.uproject'
$menuHeaderPath = Join-Path $repoPaths.ProjectRoot 'Source\KalmalaGameplay\Public\KalmalaMenuContext.h'
$menuSourcePath = Join-Path $repoPaths.ProjectRoot 'Source\KalmalaGameplay\Private\KalmalaMenuContext.cpp'
$menuHeader = Get-Content -LiteralPath $menuHeaderPath -Raw
$menuSource = Get-Content -LiteralPath $menuSourcePath -Raw
if ($menuHeader -notmatch 'UFUNCTION\(Server, Reliable\)\s+void\s+ServerRequestWorldSeedSelection') {
    throw 'The reliable server seed-selection request is missing.'
}
if ($menuSource -notmatch 'IsWorldSeedSelectionRequesterAllowed\(HasAuthority\(\),\s*IsLocalController\(\),\s*GetWorld\(\) != nullptr\)' -or
    $menuSource -notmatch 'TrySelectWorldSeedFromServer\(\s*Seed,\s*bConfirmedExisting,') {
    throw 'World-seed selection no longer requires the authoritative local controller.'
}
$launch = [regex]::Match($menuSource,
    '(?s)bool AKalmalaMenuGameMode::TryStartGameplayFromServer\([\s\S]*?\n\}')
$revalidate = [regex]::Match($launch.Value, 'TrySelectWorldSeedFromServer\(\s*Seed,\s*bConfirmedExisting,')
$openLevel = $launch.Value.IndexOf('UGameplayStatics::OpenLevel', [StringComparison]::Ordinal)
if (!$launch.Success -or !$revalidate.Success -or $openLevel -lt $revalidate.Index) {
    throw 'Gameplay launch must revalidate the exact seed and confirmation before travel.'
}

foreach ($path in @($project, $Editor)) {
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required file not found: $path" }
    if ($path.Length -ge 260 -or $path.Contains('"')) { throw "Invalid or overlong file path: $path" }
}
$output = if ($OutputDirectory) { [IO.Path]::GetFullPath($OutputDirectory) }
    else { Join-Path $env:TEMP ('km-menu-seed-' + [guid]::NewGuid().ToString('N')) }
if ($output.Length -gt 130 -or $output.Contains('"')) { throw 'Use an output root of at most 130 characters without quotes.' }
if (Test-Path -LiteralPath $output) { throw 'OutputDirectory must be fresh; existing logs/saves cannot certify a new run.' }

New-Item -ItemType Directory -Path $output | Out-Null
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaWorldSeedSelectionPeerTest'
$peers = @()
function Read-PeerLog([string]$Role) {
    $path = Join-Path $output "$Role.log"
    if (Test-Path -LiteralPath $path) { return [string](Get-Content -LiteralPath $path -Raw) }
    return ''
}
function Start-MenuPeer([string]$Role, [string]$Address, [string]$ExtraFlags) {
    $user = Join-Path $output $Role
    $shader = Join-Path $user 'ShaderWorkingDir'
    New-Item -ItemType Directory -Path $shader -Force | Out-Null
    $log = Join-Path $output "$Role.log"
    $flags = "$common $ExtraFlags"
    $process = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" $Address $flags -ShaderWorkingDir=`"$shader`" -UserDir=`"$user`" -abslog=`"$log`""
    $script:peers += $process
    return $process
}
function Stop-MenuPeers {
    foreach ($peer in $script:peers) {
        if (!$peer.HasExited) { Stop-Process -Id $peer.Id; $peer.WaitForExit(10000) | Out-Null }
    }
    $script:peers = @()
}
function Wait-MenuState([scriptblock]$Condition, [int]$Seconds, [string]$Description) {
    $deadline = (Get-Date).AddSeconds($Seconds)
    do {
        if (@($script:peers | Where-Object HasExited).Count) { throw "A peer exited during $Description." }
        if (& $Condition) { return }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    throw "$Description timed out. Logs: $output"
}

$server = $null
$client = $null
try {
    $server = Start-MenuPeer 'server' '/Game/Kalmala/Maps/Menu/L_Menu?game=/Script/KalmalaGameplay.KalmalaMenuGameMode?listen' "-port=$Port"
    Wait-MenuState { (Read-PeerLog 'server') -match 'GameNetDriver.*listening on port' } 120 'menu listen startup'
    Wait-MenuState { (Read-PeerLog 'server') -match 'World seed selection peer fixture: Local=Passed Enumerations=2 Pending=1 Seed=86753091234 Existing=1' } 60 'local host selection fixture'

    $client = Start-MenuPeer 'client' "127.0.0.1:$Port" '-KalmalaWorldSeedRemoteTest'
    Wait-MenuState {
        $serverText = Read-PeerLog 'server'
        $clientText = Read-PeerLog 'client'
        $requests = [regex]::Matches($serverText,
            'World seed selection peer server: Token=(\d+) Generic=(\d) EnumerationBefore=(\d+) EnumerationAfter=(\d+) GateEnumerations=(-?\d+) Pending=(\d) Seed=(\d+) Existing=(\d)')
        if ($requests.Count -ne 2 -or $clientText -notmatch 'World seed selection peer client: Passed=1 Replies=2 Generic=1') { return $false }
        foreach ($request in $requests) {
            if ($request.Groups[2].Value -ne '1' -or $request.Groups[3].Value -ne $request.Groups[4].Value -or
                $request.Groups[5].Value -ne '2' -or $request.Groups[6].Value -ne '1' -or
                $request.Groups[7].Value -ne '86753091234' -or $request.Groups[8].Value -ne '1') { return $false }
        }
        $tokens = @($requests | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique)
        return $tokens.Count -eq 2 -and $tokens -contains '5201' -and $tokens -contains '5202'
    } 120 'remote seed-selection RPC rejection and pending-state preservation'

    $allText = (Read-PeerLog 'server') + "`n" + (Read-PeerLog 'client')
    if ($allText -match 'Fatal error:|Assertion failed:|Ensure condition failed:|World seed selection peer fixture: Local=Failed|World seed selection peer client: Passed=0') {
        throw 'World-seed peer verification failed; inspect retained logs.'
    }
    Write-Output 'PASS: local new/reuse selection, remote RPC generic rejection, no remote save enumeration, unchanged pending selection, and launch revalidation.'
}
finally {
    Stop-MenuPeers
    Write-Output "Scenario logs and isolated save fixture: $output"
}
