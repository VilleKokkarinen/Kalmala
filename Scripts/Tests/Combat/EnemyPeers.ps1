[CmdletBinding(DefaultParameterSetName = 'Run')]
param(
    [Parameter(Mandatory, ParameterSetName = 'Run')]
    [ValidateSet('SharedCombat', 'CombatFeedback', 'BoarCharge', 'DeerHerd', 'WolfPack', 'LynxLeap', 'PolarBearCharge', 'HornedGoatPush', 'MosquitoCounterplay', 'PeatHuskSweep', 'ThunderSentinelSlam', 'ReefMawBite')]
    [string]$Scenario,
    [Parameter(Mandatory, ParameterSetName = 'List')][switch]$ListScenarios,
    [ValidateRange(1024, 65535)][int]$Port = 18149,
    [string]$Project = '',
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [string]$OutputDirectory = '',
    [long]$WorldSeed = -1
)
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot
$ErrorActionPreference = 'Stop'

# One shared lifecycle case; other cases select distinct mechanics. Existing
# native fixture flags stay stable so this runner works with the current build.
$cases = [ordered]@{
    SharedCombat = @{
        Flag = 'KalmalaRatPeerTest'; Seed = 418; Clients = 2
        Description = 'Server damage, invalid requests, owner-only escrow/retry, replicated defeat and exact-ID restart'
        Check = {
            param($serverText, $clientText, $observerText, $State)
            $ready = $serverText -match 'Rat verification server: Passed=1 SeedReproduced=1 BoundedActivation=1 InvalidRejected=1 ActionSerial=1 Health=0\.0 PlayerHealth=[0-9]+\.0 Defeated=1 Saved=1 FullPackEscrow=1 RetryDelivered=1 OwnerOnly=1 BoneFragment=1 RatTail=1 RemoteBoneFragment=0 RemoteRatTail=0 OtherPeerBoneFragments=0 OtherPeerRatTails=0 OtherPeers=(?:[2-9]|[1-9][0-9]+)'
            $ready = $ready -and $clientText -match 'Rat verification client rejected invalid owned attack without target data\.'
            $ready = $ready -and $observerText -match 'Rat verification client rejected invalid owned attack without target data\.'
            $ready = $ready -and $clientText -match 'Rat verification client observed shared action serial=1\.'
            $ready = $ready -and $observerText -match 'Rat verification client observed shared action serial=1\.'
            $ready = $ready -and $clientText -match 'Rat verification client observed relevant wildlife defeat\.'
            $ready = $ready -and $observerText -match 'Rat verification client observed relevant wildlife defeat\.'
            $ready = $ready -and $clientText -match 'Rat verification client observed replicated player health=[0-9]+\.0\.'
            $ready = $ready -and $observerText -match 'Rat verification client observed replicated player health=[0-9]+\.0\.'
            return $ready
        }
    }
    CombatFeedback = @{
        Flag = 'KalmalaCombatPeerTest'; Seed = 418; Clients = 1
        Description = 'Local hit/defeat audio, text-only unavailable feedback and remote cue privacy'
        Check = {
            param($serverText, $clientText, $observerText, $State)
            $serverHitCue = $serverText -match 'Ambient audio combat context: Local=1 Feedback=Hit Serial=[1-9][0-9]* CueSubmitted=1 Asset=CombatResultCue'
            $serverDefeatCue = $serverText -match 'Ambient audio combat context: Local=1 Feedback=Defeat Serial=[1-9][0-9]* CueSubmitted=1 Asset=CombatResultCue'
            $clientUnavailableWithoutCue = $clientText -match 'Ambient audio combat context: Local=1 Feedback=Unavailable Serial=[1-9][0-9]* CueSubmitted=0 Asset=None'
            $clientNoRemoteCombatCue = $clientText -notmatch 'Ambient audio combat context: Local=1 Feedback=(Hit|Defeat) Serial=[1-9][0-9]* CueSubmitted=1 Asset=CombatResultCue'
            $passed = $serverText -match 'Combat verification server: Passed=1 ' -and $serverHitCue -and $serverDefeatCue -and $clientUnavailableWithoutCue -and $clientNoRemoteCombatCue
            return $passed
        }
    }
    BoarCharge = @{
        Flag = 'KalmalaBoarPeerTest'; Seed = 418; Clients = 2
        Description = 'Generated boar charge and replicated server damage'
        Check = {
            param($serverText, $clientText, $observerText, $State)
            $hit = [regex]::Match($serverText, 'Boar verification server: Passed=1 [^\r\n]*PlayerHealth=(\d+(?:\.\d+)?)')
            return $hit.Success -and [double]$hit.Groups[1].Value -lt 100.0 -and
                $clientText -match 'Boar verification client observed replicated player health=\d+\.0\.' -and
                $observerText -match 'Boar verification client observed replicated player health=\d+\.0\.'
        }
    }
    DeerHerd = @{
        Flag = 'KalmalaDeerPeerTest'; Seed = 418; Clients = 2
        Description = 'Deterministic herd alert after a server hit'
        Check = {
            param($serverText, $clientText, $observerText, $State)
            return $serverText -match 'Deer verification server: Passed=1 [^\r\n]*HerdAlert=1 ' -and
                $clientText -match 'Deer verification client observed shared action serial=4\.' -and
                $observerText -match 'Deer verification client observed shared action serial=4\.'
        }
    }
    WolfPack = @{
        Flag = 'KalmalaWolfPeerTest'; Seed = 445; Clients = 2
        Description = 'Three-member pack formation and replicated lunge'
        Check = {
            param($serverText, $clientText, $observerText, $State)
            $fixtureMatch = [regex]::Match($serverText, 'Wolf verification fixture armed: Seed=\d+ PackMembers=3 MemberSpawnId=(\S+) Observers=2\.')
            $serverMatch = [regex]::Match($serverText, 'Wolf verification server: Passed=1 Seed=\d+ PackMembers=3 Telegraph=1 Lunge=1 Recovery=1 BiteHits=(\d+) HostDamage=(\d+(?:\.\d+)?) PackSpawnId=(\S+) MemberSpawnId=(\S+)')
            $clientCue = $fixtureMatch.Success -and $clientText -match [regex]::Escape("Wolf verification client observed replicated telegraph: SpawnId=$($fixtureMatch.Groups[1].Value).")
            $observerCue = $fixtureMatch.Success -and $observerText -match [regex]::Escape("Wolf verification client observed replicated telegraph: SpawnId=$($fixtureMatch.Groups[1].Value).")
            $passed = $serverMatch.Success -and [int]$serverMatch.Groups[1].Value -gt 0
            $passed = $passed -and [double]$serverMatch.Groups[2].Value -gt 0.0 -and $fixtureMatch.Success
            $passed = $passed -and $serverMatch.Groups[4].Value -eq $fixtureMatch.Groups[1].Value -and $clientCue -and $observerCue
            return $passed
        }
    }
    LynxLeap = @{
        Flag = 'KalmalaLynxPeerTest'; Seed = 445; Clients = 2
        Description = 'Telegraphed leap within 350 cm'
        Check = {
            param($serverText, $clientText, $observerText, $State)
            $fixtureMatch = [regex]::Match($serverText, 'Lynx verification fixture armed: Seed=\d+ SpawnId=(\S+) HostDistanceCm=(\d+(?:\.\d+)?) Observers=2\.')
            $serverMatch = [regex]::Match($serverText, 'Lynx verification server: Passed=1 Seed=\d+ Telegraph=1 Leap=1 Recovery=1 HostDamage=(\d+(?:\.\d+)?) SpawnId=(\S+)')
            $clientCue = $fixtureMatch.Success -and $clientText -match [regex]::Escape("Lynx verification client observed replicated telegraph: SpawnId=$($fixtureMatch.Groups[1].Value).")
            $observerCue = $fixtureMatch.Success -and $observerText -match [regex]::Escape("Lynx verification client observed replicated telegraph: SpawnId=$($fixtureMatch.Groups[1].Value).")
            $passed = $serverMatch.Success -and [double]$serverMatch.Groups[1].Value -gt 0.0
            $passed = $passed -and $fixtureMatch.Success -and [double]$fixtureMatch.Groups[2].Value -le 350.0
            $passed = $passed -and $serverMatch.Groups[2].Value -eq $fixtureMatch.Groups[1].Value -and $clientCue -and $observerCue
            return $passed
        }
    }
    PolarBearCharge = @{
        Flag = 'KalmalaPolarBearPeerTest'; Seed = 445; Clients = 2
        Description = 'Start exclusion, charge, retreat and recovery'
        Check = {
            param($serverText, $clientText, $observerText, $State)
            $fixtureMatch = [regex]::Match($serverText, 'Polar Bear verification fixture armed: Seed=\d+ SpawnId=(\S+) StartDistanceCm=(\d+(?:\.\d+)?) HostDistanceCm=(\d+(?:\.\d+)?) Observers=2\.')
            $serverMatch = [regex]::Match($serverText, 'Polar Bear verification server: Passed=1 Seed=\d+ Telegraph=1 Charge=1 Retreat=1 Recovery=1 HostDamage=(\d+(?:\.\d+)?) SpawnId=(\S+)')
            $clientCue = $fixtureMatch.Success -and $clientText -match [regex]::Escape("Polar Bear verification client observed replicated telegraph: SpawnId=$($fixtureMatch.Groups[1].Value).")
            $observerCue = $fixtureMatch.Success -and $observerText -match [regex]::Escape("Polar Bear verification client observed replicated telegraph: SpawnId=$($fixtureMatch.Groups[1].Value).")
            $passed = $serverMatch.Success -and [double]$serverMatch.Groups[1].Value -gt 0.0
            $passed = $passed -and $fixtureMatch.Success -and [double]$fixtureMatch.Groups[2].Value -ge 15000.0 -and [double]$fixtureMatch.Groups[3].Value -le 220.0
            $passed = $passed -and $serverMatch.Groups[2].Value -eq $fixtureMatch.Groups[1].Value -and $clientCue -and $observerCue
            return $passed
        }
    }
    HornedGoatPush = @{
        Flag = 'KalmalaHornedGoatPeerTest'; Seed = 445; Clients = 2
        Description = 'Horn hit, bounded knockback and stagger'
        Check = {
            param($serverText, $clientText, $observerText, $State)
            $fixtureMatch = [regex]::Match($serverText, 'Horned Goat verification fixture armed: Seed=\d+ SpawnId=(\S+) HostDistanceCm=(\d+(?:\.\d+)?) Observers=2\.')
            $serverMatch = [regex]::Match($serverText, 'Horned Goat verification server: Passed=1 Seed=\d+ Telegraph=1 Recovery=1 HostDamage=(\d+(?:\.\d+)?) HostPushCm=(\d+(?:\.\d+)?) StaggerSeconds=(\d+(?:\.\d+)?) SpawnId=(\S+)')
            $clientCue = $fixtureMatch.Success -and $clientText -match [regex]::Escape("Horned Goat verification client observed replicated telegraph: SpawnId=$($fixtureMatch.Groups[1].Value).")
            $observerCue = $fixtureMatch.Success -and $observerText -match [regex]::Escape("Horned Goat verification client observed replicated telegraph: SpawnId=$($fixtureMatch.Groups[1].Value).")
            $passed = $fixtureMatch.Success -and [double]$fixtureMatch.Groups[2].Value -le 180.0
            $passed = $passed -and $serverMatch.Success -and [double]$serverMatch.Groups[1].Value -gt 0.0
            $passed = $passed -and [double]$serverMatch.Groups[2].Value -ge 50.0 -and [double]$serverMatch.Groups[2].Value -le 250.5
            $passed = $passed -and [double]$serverMatch.Groups[3].Value -ge 0.65 -and [double]$serverMatch.Groups[3].Value -le 0.95
            $passed = $passed -and $serverMatch.Groups[4].Value -eq $fixtureMatch.Groups[1].Value -and $clientCue -and $observerCue
            return $passed
        }
    }
    MosquitoCounterplay = @{
        Flag = 'KalmalaMosquitoPeerTest'; Seed = 445; Clients = 2
        Description = 'Owner-only poison, dodge cooldown and torch/campfire counterplay'
        Check = {
            param($serverText, $clientText, $observerText, $State)
            $serverPassed = $serverText -match 'Mosquito verification server: Passed=1 Seed=\d+ Bites=\d+ Damage=\d+\.\d+ WeakPoisonSeconds=\d+\.\d+ OtherPeers=2 DodgeAccepted=1 DodgeRejected=1 RollExpired=1 HeldTorchRepelled=1 HeldTorchOffNoProtection=1 CampfireUnlitNoProtection=1 CampfireLitRepelled=1 SpawnId=\d+/-?\d+/-?\d+/\d+'
            if ($serverPassed -and $null -eq $State.ServerPassedAt) { $State.ServerPassedAt = Get-Date }
            $clientHealth = [regex]::Matches($clientText, 'Mosquito verification client observed replicated player health=(\d+(?:\.\d+)?)\.') |
                Where-Object { [double]$_.Groups[1].Value -lt 99.9 } | Select-Object -First 1
            $observerHealth = [regex]::Matches($observerText, 'Mosquito verification client observed replicated player health=(\d+(?:\.\d+)?)\.') |
                Where-Object { [double]$_.Groups[1].Value -lt 99.9 } | Select-Object -First 1
            $clientPoison = $clientText -match 'Mosquito verification client observed replicated weak poison remaining=\d+\.\d+'
            $observerPoison = $observerText -match 'Mosquito verification client observed replicated weak poison remaining=\d+\.\d+'
            $poisonOwnerPrivacyVerified = $false
            if ($null -ne $State.ServerPassedAt)
            {
                $poisonOwnerPrivacyVerified = ((Get-Date) - $State.ServerPassedAt).TotalSeconds -ge 2.0
            }
            $passed = $serverPassed -and $null -ne $clientHealth -and $null -ne $observerHealth
            $passed = $passed -and ($clientPoison -xor $observerPoison) -and $poisonOwnerPrivacyVerified
            return $passed
        }
    }
    PeatHuskSweep = @{
        Flag = 'KalmalaPeatHuskPeerTest'; Seed = 445; Clients = 2
        Description = 'Hard-enemy exclusion and replicated sweep'
        Check = {
            param($serverText, $clientText, $observerText, $State)
            $serverMatch = [regex]::Match($serverText, 'Peat Husk verification server: Passed=1 Seed=\d+ StartDistanceCm=(\d+(?:\.\d+)?) Telegraph=1 Recovery=1 SweepHits=\d+ HostDamage=(\d+(?:\.\d+)?) Observers=2 SpawnId=-?\d+/-?\d+/-?\d+/\d+')
            $clientCue = $clientText -match 'Peat Husk verification client observed replicated telegraph\.'
            $observerCue = $observerText -match 'Peat Husk verification client observed replicated telegraph\.'
            $passed = $serverMatch.Success -and [double]$serverMatch.Groups[1].Value -ge 15000.0
            $passed = $passed -and [double]$serverMatch.Groups[2].Value -gt 0.0 -and $clientCue -and $observerCue
            return $passed
        }
    }
    ThunderSentinelSlam = @{
        Flag = 'KalmalaThunderSentinelPeerTest'; Seed = 445; Clients = 2
        Description = 'Step into reach, authored slam damage and replicated cues'
        Check = {
            param($serverText, $clientText, $observerText, $State)
            $fixtureMatch = [regex]::Match($serverText, 'Thunder Sentinel verification fixture armed: Seed=\d+ SpawnId=(\S+) StartDistanceCm=(\d+(?:\.\d+)?) HostDistanceCm=(\d+(?:\.\d+)?) Observers=2\.')
            $serverMatch = [regex]::Match($serverText, 'Thunder Sentinel verification server: Passed=1 Seed=\d+ Telegraph=1 Slam=1 Recovery=1 HostDamage=(\d+(?:\.\d+)?) StepCm=(\d+(?:\.\d+)?) SpawnId=(\S+)')
            $clientTelegraph = $fixtureMatch.Success -and $clientText -match [regex]::Escape("Thunder Sentinel verification client observed replicated telegraph: SpawnId=$($fixtureMatch.Groups[1].Value).")
            $observerTelegraph = $fixtureMatch.Success -and $observerText -match [regex]::Escape("Thunder Sentinel verification client observed replicated telegraph: SpawnId=$($fixtureMatch.Groups[1].Value).")
            $clientSlam = $fixtureMatch.Success -and $clientText -match [regex]::Escape("Thunder Sentinel verification client observed replicated slam: SpawnId=$($fixtureMatch.Groups[1].Value).")
            $observerSlam = $fixtureMatch.Success -and $observerText -match [regex]::Escape("Thunder Sentinel verification client observed replicated slam: SpawnId=$($fixtureMatch.Groups[1].Value).")
            $passed = $fixtureMatch.Success -and [double]$fixtureMatch.Groups[2].Value -ge 15000.0
            $passed = $passed -and [double]$fixtureMatch.Groups[3].Value -gt 300.0 -and [double]$fixtureMatch.Groups[3].Value -le 420.0
            $passed = $passed -and $serverMatch.Success -and [double]$serverMatch.Groups[1].Value -ge 299.0
            $passed = $passed -and [double]$serverMatch.Groups[1].Value -le 301.0
            $passed = $passed -and [double]$serverMatch.Groups[2].Value -ge 80.0 -and [double]$serverMatch.Groups[2].Value -le 120.5
            $passed = $passed -and $serverMatch.Groups[3].Value -eq $fixtureMatch.Groups[1].Value
            $passed = $passed -and $clientTelegraph -and $observerTelegraph -and $clientSlam -and $observerSlam
            return $passed
        }
    }
    ReefMawBite = @{
        Flag = 'KalmalaReefMawPeerTest'; Seed = 445; Clients = 2
        Description = 'Deep-ocean swimming bite, reach, damage and replicated cues'
        Check = {
            param($serverText, $clientText, $observerText, $State)
            $fixtureMatch = [regex]::Match($serverText, 'Reef Maw verification fixture armed: Seed=\d+ SpawnId=(\S+) StartDistanceCm=(\d+(?:\.\d+)?) SpawnDepthCm=(\d+(?:\.\d+)?) HostDistanceCm=(\d+(?:\.\d+)?) Observers=2\.')
            $serverMatch = [regex]::Match($serverText, 'Reef Maw verification server: Passed=1 Seed=\d+ Telegraph=1 Bite=1 Recovery=1 HostDamage=(\d+(?:\.\d+)?) BiteDistanceCm=(\d+(?:\.\d+)?) SpawnId=(\S+)')
            $clientTelegraph = $fixtureMatch.Success -and $clientText -match [regex]::Escape("Reef Maw verification client observed replicated telegraph: SpawnId=$($fixtureMatch.Groups[1].Value).")
            $observerTelegraph = $fixtureMatch.Success -and $observerText -match [regex]::Escape("Reef Maw verification client observed replicated telegraph: SpawnId=$($fixtureMatch.Groups[1].Value).")
            $clientBite = $fixtureMatch.Success -and $clientText -match [regex]::Escape("Reef Maw verification client observed replicated bite: SpawnId=$($fixtureMatch.Groups[1].Value).")
            $observerBite = $fixtureMatch.Success -and $observerText -match [regex]::Escape("Reef Maw verification client observed replicated bite: SpawnId=$($fixtureMatch.Groups[1].Value).")
            $passed = $fixtureMatch.Success -and [double]$fixtureMatch.Groups[2].Value -ge 15000.0
            $passed = $passed -and [double]$fixtureMatch.Groups[3].Value -ge 150.0
            $passed = $passed -and [double]$fixtureMatch.Groups[4].Value -gt 180.0 -and [double]$fixtureMatch.Groups[4].Value -le 500.0
            $passed = $passed -and $serverMatch.Success -and [double]$serverMatch.Groups[1].Value -ge 49.0
            $passed = $passed -and [double]$serverMatch.Groups[1].Value -le 51.0
            $passed = $passed -and [double]$serverMatch.Groups[2].Value -gt 0.0 -and [double]$serverMatch.Groups[2].Value -le 180.0
            $passed = $passed -and $serverMatch.Groups[3].Value -eq $fixtureMatch.Groups[1].Value
            $passed = $passed -and $clientTelegraph -and $observerTelegraph -and $clientBite -and $observerBite
            return $passed
        }
    }
}

function Test-EnemyEvidence($Case, [string]$ServerText, [string]$ClientText, [string]$ObserverText, $State, [long]$Seed) {
    $allText = $ServerText + "`n" + $ClientText + "`n" + $ObserverText
    if ($allText -match 'Fatal error:|Assertion failed:|Ensure condition failed:|verification FAILED:|verification server: Passed=0') {
        throw 'Enemy fixture failed; inspect the retained peer logs.'
    }
    $identity = 'Client received world-generation identity: Seed=' + $Seed + '(?!\d)'
    if ($ClientText -notmatch $identity -or ($Case.Clients -eq 2 -and $ObserverText -notmatch $identity)) { return $false }
    return [bool](& $Case.Check $ServerText $ClientText $ObserverText $State)
}

if ($ListScenarios) {
    $cases.GetEnumerator() | ForEach-Object { [pscustomobject]@{ Scenario = $_.Key; Purpose = $_.Value.Description } }
    return
}

$case = $cases[$Scenario]
$seed = if ($WorldSeed -eq -1) { $case.Seed } else { $WorldSeed }
$projectPath = if ($Project) { [IO.Path]::GetFullPath($Project) } else { Join-Path $repoPaths.ProjectRoot 'Kalmala.uproject' }
foreach ($path in @($projectPath, $Editor)) {
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required file not found: $path" }
    if ($path.Length -ge 260 -or $path.Contains('"')) { throw "Invalid or overlong file path: $path" }
}
$output = if ($OutputDirectory) { [IO.Path]::GetFullPath($OutputDirectory) } else { Join-Path $env:TEMP ('km-enemy-' + [guid]::NewGuid().ToString('N')) }
if ($output.Length -gt 130 -or $output.Contains('"')) { throw 'Use an output root of at most 130 characters without quotes.' }
if (Test-Path -LiteralPath $output) { throw 'OutputDirectory must be fresh; existing logs/saves cannot certify a new run.' }
$listener = [Net.Sockets.TcpListener]::new([Net.IPAddress]::Any, $Port)
try { $listener.Start() } finally { $listener.Stop() }
New-Item -ItemType Directory -Path $output | Out-Null
$peers = @()
$state = @{ ServerPassedAt = $null }
$common = "-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -$($case.Flag)"

function Read-PeerLog([string]$Role) {
    $path = Join-Path $output "$Role.log"
    if (Test-Path -LiteralPath $path) { return [string](Get-Content -LiteralPath $path -Raw) }
    return ''
}
function Start-EnemyPeer([string]$Role, [string]$Address, [long]$PeerSeed, [string]$Flags) {
    $user = Join-Path $output $Role
    $shader = Join-Path $user 'ShaderWorkingDir'
    New-Item -ItemType Directory -Path $shader -Force | Out-Null
    $log = Join-Path $output "$Role.log"
    $process = Start-Process $Editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" $Address -WorldSeed=$PeerSeed $Flags -ShaderWorkingDir=`"$shader`" -UserDir=`"$user`" -abslog=`"$log`""
    $script:peers += $process
    return $process
}
function Stop-EnemyPeers {
    foreach ($peer in $script:peers) {
        if (!$peer.HasExited) { Stop-Process -Id $peer.Id; $peer.WaitForExit(10000) | Out-Null }
    }
    $script:peers = @()
}
function Wait-EnemyState([scriptblock]$Condition, [int]$Seconds, [string]$Description) {
    $deadline = (Get-Date).AddSeconds($Seconds)
    do {
        if (@($script:peers | Where-Object HasExited).Count) { throw "A peer exited during $Description." }
        if (& $Condition) { return }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    throw "$Description timed out. Logs: $output"
}
try {
    $server = Start-EnemyPeer 'server' '/Game/Kalmala/Maps/Prototype/L_Prototype?listen' $seed "$common -port=$Port"
    Wait-EnemyState { (Read-PeerLog 'server') -match 'GameNetDriver.*listening on port' } 120 'listen startup'
    $client = Start-EnemyPeer 'client' "127.0.0.1:$Port" 999 $common
    if ($case.Clients -eq 2) {
        # Stable owner/observer ordering matters for pack, charge and poison fixtures.
        Wait-EnemyState { (Read-PeerLog 'server') -match 'LogNet: Join succeeded:' } 60 'first remote join'
        $observer = Start-EnemyPeer 'observer' "127.0.0.1:$Port" 1000 $common
    }
    Wait-EnemyState {
        Test-EnemyEvidence $case (Read-PeerLog 'server') (Read-PeerLog 'client') (Read-PeerLog 'observer') $state $seed
    } 240 "$Scenario acceptance"

    if ($Scenario -eq 'SharedCombat') {
        $spawn = [regex]::Match((Read-PeerLog 'server'), 'Rat verification server: Passed=1[^\r\n]* SpawnId=(\d+/-?\d+/-?\d+/\d+)')
        if (!$spawn.Success) { throw 'Shared combat result is missing its exact defeated spawn ID.' }
        $spawnId = $spawn.Groups[1].Value
        $parts = $spawnId.Split('/')
        Stop-EnemyPeers
        $restartFlags = "-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaReconnectVerification=WildlifeVerify -KalmalaReconnectKeyX=$($parts[1]) -KalmalaReconnectKeyY=$($parts[2]) -KalmalaReconnectSpawnId=$spawnId -port=$Port"
        # Restore the original server profile, not a new empty save directory.
        Copy-Item -LiteralPath (Join-Path $output 'server.log') -Destination (Join-Path $output 'acceptance.log')
        $restart = Start-EnemyPeer 'server' '/Game/Kalmala/Maps/Prototype/L_Prototype?listen' $seed $restartFlags
        if (!$restart.WaitForExit(120000)) { throw 'Shared combat restart timed out.' }
        $restartText = Read-PeerLog 'server'
        $restored = [regex]::Escape("Reconnect verification passed: defeated generated wildlife spawn $spawnId remained absent after listen-server restart.")
        if ($restart.ExitCode -ne 0 -or $restartText -match 'Fatal error:|Assertion failed:|Ensure condition failed:' -or
            $restartText -notmatch $restored) {
            throw 'Shared combat exact-ID defeat restoration failed.'
        }
    }
    Write-Output "PASS: $Scenario — $($case.Description)."
}
finally {
    Stop-EnemyPeers
    Write-Output "Scenario logs: $output"
}
