param(
    [ValidateSet('Reference', 'Cpu8Threads', 'Cpu4Threads')]
    [string]$Profile = 'Reference',
    [Parameter(Mandatory = $true)][string]$Project,
    [int]$Port = 19981,
    [string]$OutputDirectory = '',
    [ValidateRange(120, 240)][int]$TimeoutSeconds = 180
)

# Diagnostic CPU contention profiles, not emulation of a particular CPU/GPU.
# Keep the rendering workload identical so the CPU comparisons are useful.
$ErrorActionPreference = 'Stop'
if ([IntPtr]::Size -ne 8 -or [Environment]::ProcessorCount -gt 63) {
    throw 'This runner requires 64-bit Windows with at most 63 logical processors in one processor group.'
}
$projectPath = (Resolve-Path -LiteralPath $Project).Path
if ([IO.Path]::GetFileName($projectPath) -ne 'Kalmala.uproject') { throw 'Project must point to Kalmala.uproject.' }
$runner = Join-Path $PSScriptRoot 'Verify-OceanSkiffDiscoveryDisembark.ps1'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaCpuProfile-' + [guid]::NewGuid().ToString('N')) }
$output = [IO.Path]::GetFullPath($output)
if (Test-Path -LiteralPath $output) { throw 'Use a new output directory for each profile run.' }
New-Item -ItemType Directory -Path $output | Out-Null
$stopMonitor = Join-Path $output 'monitor.stop'
$self = Get-Process -Id $PID
$originalMask = $self.ProcessorAffinity.ToInt64()
$threadCount = switch ($Profile) { 'Cpu8Threads' { 8 }; 'Cpu4Threads' { 4 }; default { 0 } }
$selectedMask = $originalMask
if ($threadCount -gt 0) {
    # Unreal's processaffinity option selects the lowest N logical processors.
    $selectedMask = ([long]1 -shl $threadCount) - 1
    if (($originalMask -band $selectedMask) -ne $selectedMask) {
        throw "Caller affinity must allow the first $threadCount logical processors."
    }
}

# The monitor starts before affinity is restricted, and never modifies peers.
# Windows children inherit the calling process's affinity; verify that the
# actual Unreal host and client retain it, rather than assuming inheritance.
# Unreal also needs its explicit processaffinity option to disable its normal
# thread-affinity setup, which can otherwise widen the inherited process mask.
$monitor = Start-Job -ArgumentList $PID, $stopMonitor -ScriptBlock {
    param($ownerPid, $stopFile)
    $ErrorActionPreference = 'Stop'
    $seen = @{}
    $deadline = (Get-Date).AddMinutes(7)
    while (!(Test-Path -LiteralPath $stopFile) -and (Get-Date) -lt $deadline) {
        $peers = Get-CimInstance Win32_Process -Filter "ParentProcessId=$ownerPid AND Name='UnrealEditor.exe'"
        foreach ($peer in $peers) {
            try {
                $process = Get-Process -Id $peer.ProcessId -ErrorAction Stop
                $mask = $process.ProcessorAffinity.ToInt64()
                $key = "$($peer.ProcessId):$mask"
                if (!$seen.ContainsKey($key)) {
                    $seen[$key] = $true
                    [pscustomobject]@{ Pid = [int]$peer.ProcessId; AffinityMask = $mask; ObservedUtc = [DateTime]::UtcNow.ToString('o') }
                }
            }
            catch [Microsoft.PowerShell.Commands.ProcessCommandException] {
                # A peer may have exited between the inventory and the read.
            }
        }
        Start-Sleep -Milliseconds 250
    }
}
$started = [DateTime]::UtcNow
$passed = $false
$failure = $null
$observations = @()
try {
    $self.ProcessorAffinity = [IntPtr]$selectedMask
    if ($self.ProcessorAffinity.ToInt64() -ne $selectedMask) { throw 'Caller affinity did not apply.' }
    Write-Output "Diagnostic profile: $Profile; shared logical-processor mask=$selectedMask; GPU and rendering settings unchanged."
    & $runner -Project $projectPath -Port $Port -OutputDirectory (Join-Path $output 'Peers') -RenderedFrameTimeProfile -TimeoutSeconds $TimeoutSeconds -LogicalCoreAffinity $threadCount |
        Tee-Object -FilePath (Join-Path $output 'scenario.txt')
    $passed = $true
}
catch {
    $failure = $_.Exception.Message
}
finally {
    $self.ProcessorAffinity = [IntPtr]$originalMask
    New-Item -ItemType File -Path $stopMonitor | Out-Null
    $null = Wait-Job -Job $monitor -Timeout 15
    if ($monitor.State -ne 'Completed') {
        $passed = $false
        if (!$failure) { $failure = 'Affinity monitor did not complete successfully.' }
        Stop-Job -Job $monitor
    }
    $observations = @(Receive-Job -Job $monitor -ErrorAction Continue | Select-Object Pid, AffinityMask, ObservedUtc)
    Remove-Job -Job $monitor
    $peerIds = @($observations | Select-Object -ExpandProperty Pid -Unique)
    if ($peerIds.Count -ne 2 -or @($observations | Where-Object { $_.AffinityMask -ne $selectedMask }).Count -ne 0) {
        $passed = $false
        if (!$failure) { $failure = 'Expected two Unreal peers retaining the selected affinity; inspect profile.json.' }
    }
    [ordered]@{
        Profile = $Profile
        EvidenceClass = 'Single-machine CPU contention diagnostic; not target-hardware acceptance'
        Project = $projectPath
        StartedUtc = $started.ToString('o')
        ElapsedSeconds = [Math]::Round(([DateTime]::UtcNow - $started).TotalSeconds, 2)
        SharedLogicalProcessors = $threadCount
        ReferenceUsesCallerAffinity = ($threadCount -eq 0)
        OriginalAffinityMask = $originalMask
        SelectedAffinityMask = $selectedMask
        ObservedPeers = $observations
        ScenarioPassed = $passed
        Failure = $failure
        Limitations = 'Host and client share the mask; logical processors may be SMT siblings or hybrid cores. Per-core speed, GPU, VRAM, RAM and storage are unchanged. No numerical acceptance limits applied.'
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $output 'profile.json') -Encoding UTF8
    Write-Output "CPU profile evidence: $output"
}
if (!$passed) { throw $failure }
Write-Output "PASS: $Profile scenario and observed host/client affinity."
