param(
    [int]$Port = 18170,
    [string]$OutputDirectory = '',
    [string]$Project = '',
    [int]$TimeoutSeconds = 120
)

$ErrorActionPreference = 'Stop'
$projectPath = if ($Project) { $Project } else { Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject' }
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaOceanSkiffDiscoveryDisembark-' + [guid]::NewGuid().ToString('N')) }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$hostLog = Join-Path $output 'host.log'
$clientLog = Join-Path $output 'client.log'
$hostShaderDir = Join-Path $output 'Host\ShaderWorkingDir'
$clientShaderDir = Join-Path $output 'Client\ShaderWorkingDir'
New-Item -ItemType Directory -Path $hostShaderDir -Force | Out-Null
New-Item -ItemType Directory -Path $clientShaderDir -Force | Out-Null
$common = '-game -nullrhi -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaOceanDiscoveryDisembarkPeerTest -KalmalaWorldProfile'
$hostProcess = $null
$client = $null
try {
    $hostProcess = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -ShaderWorkingDir=`"$hostShaderDir`" -abslog=`"$hostLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($hostProcess.HasExited) { throw 'Ocean discovery/disembark listen server exited during startup.' }
        if ((Test-Path -LiteralPath $hostLog) -and (Select-String -LiteralPath $hostLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Ocean discovery/disembark listen server readiness timed out.' }

    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" 127.0.0.1:$Port -WorldSeed=999 $common -ShaderWorkingDir=`"$clientShaderDir`" -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $ready = $false
    do {
        if ($hostProcess.HasExited -or $client.HasExited) { throw 'An ocean discovery/disembark peer exited before verification completed.' }
        $hostText = if (Test-Path -LiteralPath $hostLog) { Get-Content -LiteralPath $hostLog -Raw } else { '' }
        $clientText = if (Test-Path -LiteralPath $clientLog) { Get-Content -LiteralPath $clientLog -Raw } else { '' }
        if (($hostText + $clientText) -match 'Ocean discovery-stop peer verification FAILED:|Fatal error:|Assertion failed:|Ensure condition failed:') {
            throw 'Ocean discovery/disembark peer verification failed; inspect retained logs.'
        }

        $serverAccepted = $hostText -match 'Ocean discovery-stop server passed: Seed=418 Players=2 Discovery=.+ Reward=(?:Wood|Fibre|Stone):[12] Claims=2 Mode=Moored Disembarked=2'
        $clientAccepted = $clientText -match 'Ocean discovery-stop peer replica passed: Authority=0 Seat=Helm Discovery=.+ DiscoveryFeedback=LandmarkFound Reward=(?:Wood|Fibre|Stone):[12] Disembarked=1 EmptySeats=1 Mode=Moored'
        $clientWorld = $clientText -match 'Client received world-generation identity: Seed=418'
        $profileMatch = [regex]::Match($hostText, 'World profile: InitialGenerationMs=(?<generation>[0-9.]+) UsedPhysicalMB=(?<used>[0-9.]+) AvailablePhysicalMB=(?<available>[0-9.]+) Actors=(?<actors>[0-9]+) ReplicatedActors=(?<replicated>[0-9]+) TerrainPatches=(?<patches>[0-9]+) PopulationKeys=(?<population>[0-9]+) SaveBytes=(?<save>[0-9]+) SaveSerialized=(?<serialized>[01]) LateJoinPlayers=(?<players>[0-9]+)\.')
        $networkProfileMatch = [regex]::Match($hostText, 'Ocean M8 peer connection profile: Peer=Client WindowSeconds=(?<window>[0-9.]+) InBytes=(?<inBytes>[0-9]+) OutBytes=(?<outBytes>[0-9]+) InPackets=(?<inPackets>[0-9]+) OutPackets=(?<outPackets>[0-9]+)')
        $serverOutcomeIndex = $hostText.IndexOf('Ocean discovery-stop server passed:', [System.StringComparison]::Ordinal)
        $profileIndex = $hostText.IndexOf('World profile:', [System.StringComparison]::Ordinal)
        if ($profileMatch.Success -and $serverOutcomeIndex -ge 0 -and $profileIndex -lt $serverOutcomeIndex) {
            throw 'The M8 world profile was captured before the two-peer skiff scenario completed.'
        }
        $networkProfileIndex = $hostText.IndexOf('Ocean M8 peer connection profile:', [System.StringComparison]::Ordinal)
        if ($networkProfileMatch.Success -and $serverOutcomeIndex -ge 0 -and $networkProfileIndex -lt $serverOutcomeIndex) {
            throw 'The M8 peer connection profile was captured before the two-peer skiff scenario completed.'
        }
        $ready = $serverAccepted -and $clientAccepted -and $clientWorld -and $profileMatch.Success -and $networkProfileMatch.Success
        if ($ready) { break }
        Start-Sleep -Seconds 1
    } while ((Get-Date) -lt $deadline)

    if (!$ready) { throw "Ocean discovery/disembark peer verification timed out after $TimeoutSeconds seconds; inspect retained logs." }
    $hostMemory = Get-Process -Id $hostProcess.Id -ErrorAction Stop
    $clientMemory = Get-Process -Id $client.Id -ErrorAction Stop
    $profile = $profileMatch.Groups
    $invariantCulture = [System.Globalization.CultureInfo]::InvariantCulture
    if ($profile['players'].Value -ne '2' -or $profile['serialized'].Value -ne '1') {
        throw 'The M8 profile did not capture exactly two players with a serialized population baseline.'
    }
    Write-Output 'PASS: both peers retained one server-selected sea-discovery reward and replicated the safely moored, empty skiff after disembark.'
    Write-Output ('M8 actor/memory profile: Seed=418 Players={0} Actors={1} ReplicatedActors={2} TerrainPatches={3} PopulationKeys={4} InitialGenerationMs={5} SystemUsedPhysicalMB={6} SystemAvailablePhysicalMB={7} PopulationSaveBytes={8}' -f $profile['players'].Value, $profile['actors'].Value, $profile['replicated'].Value, $profile['patches'].Value, $profile['population'].Value, $profile['generation'].Value, $profile['used'].Value, $profile['available'].Value, $profile['save'].Value)
    $networkProfile = $networkProfileMatch.Groups
    Write-Output ('M8 peer connection profile: WindowSeconds={0} ClientToServerBytes={1} ServerToClientBytes={2} ClientToServerPackets={3} ServerToClientPackets={4}' -f $networkProfile['window'].Value, $networkProfile['inBytes'].Value, $networkProfile['outBytes'].Value, $networkProfile['inPackets'].Value, $networkProfile['outPackets'].Value)
    $hostPrivateMiB = ($hostMemory.PrivateMemorySize64 / 1MB).ToString('F2', $invariantCulture)
    $hostWorkingSetMiB = ($hostMemory.WorkingSet64 / 1MB).ToString('F2', $invariantCulture)
    $clientPrivateMiB = ($clientMemory.PrivateMemorySize64 / 1MB).ToString('F2', $invariantCulture)
    $clientWorkingSetMiB = ($clientMemory.WorkingSet64 / 1MB).ToString('F2', $invariantCulture)
    Write-Output ('M8 process memory snapshot: ListenServerPrivateMiB={0} ListenServerWorkingSetMiB={1} ClientPrivateMiB={2} ClientWorkingSetMiB={3}' -f $hostPrivateMiB, $hostWorkingSetMiB, $clientPrivateMiB, $clientWorkingSetMiB)
}
finally {
    foreach ($peer in @($client, $hostProcess)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Discovery/disembark logs: $output"
}
