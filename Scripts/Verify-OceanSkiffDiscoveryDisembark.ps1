param(
    [int]$Port = 18170,
    [string]$OutputDirectory = '',
    [string]$Project = '',
    [switch]$RenderedFrameTimeProfile,
    [int]$TimeoutSeconds = 120,
    [ValidateRange(0, 63)][int]$LogicalCoreAffinity = 0,
    [ValidateRange(-1, 3)][int]$ScalabilityQuality = -1,
    [ValidateRange(25, 100)][int]$ScreenPercentage = 100,
    [ValidateRange(0, 8192)][int]$TexturePoolMB = 0
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
$frameTimeDirectory = Join-Path $output 'FrameTimes'
New-Item -ItemType Directory -Path $hostShaderDir -Force | Out-Null
New-Item -ItemType Directory -Path $clientShaderDir -Force | Out-Null
$common = '-game -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaOceanDiscoveryDisembarkPeerTest -KalmalaWorldProfile'
if ($LogicalCoreAffinity -gt 0) { $common += " -processaffinity=$LogicalCoreAffinity" }
$renderSettings = [ordered]@{}
if ($ScalabilityQuality -ge 0) {
    if (!$RenderedFrameTimeProfile) { throw 'Rendering presets require -RenderedFrameTimeProfile.' }
    foreach ($group in 'ViewDistance', 'AntiAliasing', 'Shadow', 'GlobalIllumination', 'Reflection', 'PostProcess', 'Texture', 'Effects', 'Foliage', 'Shading', 'Landscape') {
        $renderSettings["sg.${group}Quality"] = $ScalabilityQuality
    }
    $renderSettings['r.DynamicRes.OperationMode'] = 0
    $renderSettings['r.ScreenPercentage'] = $ScreenPercentage
    $renderSettings['t.MaxFPS'] = 0
}
if ($TexturePoolMB -gt 0) {
    if (!$RenderedFrameTimeProfile) { throw 'Texture-pool presets require -RenderedFrameTimeProfile.' }
    $renderSettings['r.Streaming.UseFixedPoolSize'] = 1
    $renderSettings['r.Streaming.PoolSize'] = $TexturePoolMB
}
if ($renderSettings.Count -gt 0) {
    $commands = @($renderSettings.Keys | ForEach-Object { "$_ $($renderSettings[$_])" }) -join ','
    $common += " -ExecCmds=`"$commands`""
}
if ($RenderedFrameTimeProfile) {
    New-Item -ItemType Directory -Path $frameTimeDirectory -Force | Out-Null
    $common += " -RenderOffscreen -ResX=1280 -ResY=720 -novsync -csvGpuStats -KalmalaCaptureRenderedFrameTimes -KalmalaFrameTimeOutputDir=`"$frameTimeDirectory`""
}
else {
    $common += ' -nullrhi'
}

function Get-FrameTimeMetricSummary {
    param(
        [Parameter(Mandatory = $true)][string]$CsvPath,
        [Parameter(Mandatory = $true)][string]$Metric
    )

    $culture = [System.Globalization.CultureInfo]::InvariantCulture
    $samples = [System.Collections.Generic.List[double]]::new()
    # Engine CSVs can repeat unrelated GPU-stat names. Parse by column position
    # so those duplicate names cannot hide or prevent reading our four metrics.
    $header = (Get-Content -LiteralPath $CsvPath -TotalCount 1).Split(',')
    $metricIndices = @(for ($column = 0; $column -lt $header.Count; $column++) {
        if ($header[$column].Trim('"') -eq $Metric) { $column }
    })
    if ($metricIndices.Count -ne 1) { throw "Expected exactly one $Metric column in '$CsvPath'." }
    $eventIndices = @(for ($column = 0; $column -lt $header.Count; $column++) {
        if ($header[$column].Trim('"') -eq 'EVENTS') { $column }
    })
    if ($eventIndices.Count -ne 1) { throw "Expected exactly one EVENTS column in '$CsvPath'." }
    $columnNames = @(for ($column = 0; $column -lt $header.Count; $column++) { "Column$column" })
    foreach ($row in (Import-Csv -LiteralPath $CsvPath -Header $columnNames)) {
        # Footer metadata can contain numbers in timing-column positions.
        if ($row.PSObject.Properties[$columnNames[$eventIndices[0]]].Value -eq '[HasHeaderRowAtEnd]') { continue }
        # Original/repeated header rows are nonnumeric and skipped.
        $rawValue = $row.PSObject.Properties[$columnNames[$metricIndices[0]]].Value
        $value = 0.0
        if ($null -ne $rawValue -and [double]::TryParse([string]$rawValue, [System.Globalization.NumberStyles]::Float, $culture, [ref]$value) -and $value -gt 0.0) {
            [void]$samples.Add($value)
        }
    }

    if ($samples.Count -ne 300) {
        throw "Rendered M8 CSV '$CsvPath' contains $($samples.Count) positive $Metric samples; expected 300."
    }

    $sorted = @($samples.ToArray() | Sort-Object)
    $p50 = $sorted[[int][Math]::Ceiling(0.50 * $sorted.Count) - 1]
    $p95 = $sorted[[int][Math]::Ceiling(0.95 * $sorted.Count) - 1]
    $maximum = $sorted[-1]
    return [pscustomobject]@{
        Samples = $samples.Count
        P50Ms = $p50.ToString('F2', $culture)
        P95Ms = $p95.ToString('F2', $culture)
        MaxMs = $maximum.ToString('F2', $culture)
    }
}

$hostProcess = $null
$client = $null
try {
    $hostProcess = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$projectPath`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -ShaderWorkingDir=`"$hostShaderDir`" -abslog=`"$hostLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
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
        $frameTimeProfilesReady = $true
        if ($RenderedFrameTimeProfile) {
            $hostCapture = $hostText -match 'Ocean M8 rendered frame capture complete: Peer=ListenServer Resolution=1280x720 RenderMode=OffscreenRHI Frames=300 Csv=M8SkiffListenServer\.csv'
            $clientCapture = $clientText -match 'Ocean M8 rendered frame capture complete: Peer=Client Resolution=1280x720 RenderMode=OffscreenRHI Frames=300 Csv=M8SkiffClient\.csv'
            $hostCaptureFile = Test-Path -LiteralPath (Join-Path $frameTimeDirectory 'M8SkiffListenServer.csv')
            $clientCaptureFile = Test-Path -LiteralPath (Join-Path $frameTimeDirectory 'M8SkiffClient.csv')
            $frameTimeProfilesReady = $hostCapture -and $clientCapture -and $hostCaptureFile -and $clientCaptureFile
        }
        $ready = $serverAccepted -and $clientAccepted -and $clientWorld -and $profileMatch.Success -and $networkProfileMatch.Success -and $frameTimeProfilesReady
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
    if ($RenderedFrameTimeProfile) {
        foreach ($peerText in $hostText, $clientText) {
            $captureStart = $peerText.IndexOf('Ocean M8 rendered frame capture started:')
            foreach ($setting in $renderSettings.Keys) {
                $settingPattern = [regex]::Escape($setting) + '\s*=\s*"?(?<value>[0-9.]+)'
                $settingMatches = [regex]::Matches($peerText, $settingPattern)
                if ($settingMatches.Count -eq 0 -or $captureStart -lt 0 -or $settingMatches[$settingMatches.Count - 1].Index -ge $captureStart -or [double]$settingMatches[$settingMatches.Count - 1].Groups['value'].Value -ne $renderSettings[$setting]) {
                    throw "A rendered peer did not confirm $setting=$($renderSettings[$setting]); inspect logs."
                }
            }
        }
        if ($renderSettings.Count -gt 0) { Write-Output "Verified rendering preset on both peers: $commands" }
        $hostRhiMatch = [regex]::Match($hostText, 'LogRHI: Using Default RHI: (?<rhi>[^\r\n]+)')
        $clientRhiMatch = [regex]::Match($clientText, 'LogRHI: Using Default RHI: (?<rhi>[^\r\n]+)')
        if (!$hostRhiMatch.Success -or !$clientRhiMatch.Success -or $hostRhiMatch.Groups['rhi'].Value -match 'NullRHI' -or $clientRhiMatch.Groups['rhi'].Value -match 'NullRHI') {
            throw 'Rendered M8 captures did not report an active non-NullRHI render interface on both peers.'
        }
        $hostAdapterLine = Select-String -LiteralPath $hostLog -Pattern 'LogRHI:\s+Name:' | Select-Object -First 1
        $clientAdapterLine = Select-String -LiteralPath $clientLog -Pattern 'LogRHI:\s+Name:' | Select-Object -First 1
        $hostAdapter = if ($null -ne $hostAdapterLine) { $hostAdapterLine.Line -replace '.*LogRHI:\s+Name:\s*', '' } else { 'unreported' }
        $clientAdapter = if ($null -ne $clientAdapterLine) { $clientAdapterLine.Line -replace '.*LogRHI:\s+Name:\s*', '' } else { 'unreported' }
        $hostCsv = Join-Path $frameTimeDirectory 'M8SkiffListenServer.csv'
        $clientCsv = Join-Path $frameTimeDirectory 'M8SkiffClient.csv'
        foreach ($metric in 'FrameTime', 'GameThreadTime', 'RenderThreadTime', 'GPUTime') {
            $hostMetric = Get-FrameTimeMetricSummary -CsvPath $hostCsv -Metric $metric
            $clientMetric = Get-FrameTimeMetricSummary -CsvPath $clientCsv -Metric $metric
            Write-Output ('M8 rendered frame timing: Metric={0} Unit=ms Samples={1} ListenServerP50={2} ListenServerP95={3} ListenServerMax={4} ClientP50={5} ClientP95={6} ClientMax={7}' -f $metric, $hostMetric.Samples, $hostMetric.P50Ms, $hostMetric.P95Ms, $hostMetric.MaxMs, $clientMetric.P50Ms, $clientMetric.P95Ms, $clientMetric.MaxMs)
        }
        Write-Output "M8 rendered frame CSVs: $frameTimeDirectory"
        Write-Output ('Render mode: Resolution=1280x720 OffscreenRHI VSync=off GPUStats=on ListenServerRHI={0} ListenServerAdapter={1} ClientRHI={2} ClientAdapter={3}; each capture covers 300 frames after accepted discovery and safe disembark.' -f $hostRhiMatch.Groups['rhi'].Value, $hostAdapter, $clientRhiMatch.Groups['rhi'].Value, $clientAdapter)
    }
}
finally {
    foreach ($peer in @($client, $hostProcess)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
    Write-Output "Discovery/disembark logs: $output"
}
