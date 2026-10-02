# M10 constrained single-machine performance diagnostics

## Purpose and acceptance boundary

The product owner's 2026-10-02 request permits investigating lower-resource
profiles on the available i7-14700K / RTX 5090 Windows PC. The diagnostic
matrix below makes resource and quality settings repeatable without changing gameplay,
machine-wide power settings, drivers, or project configuration. It does not
approve minimum/recommended hardware, numerical release limits, or substitute
for measurements on those targets. The owner subsequently directed completion
of the M10 performance backlog item using this evidence with its recorded
limitations; see the owner closeout below.

## Named resource and quality presets

The owner's follow-up adds five selectable presets. These are diagnostic
settings rather than minimum/recommended hardware specifications or player
video-menu defaults. They combine CPU contention, rendering workload, and
texture-pool budgets; use the separate CPU-only profiles below to isolate CPU
effects. All keep a 1280×720 output viewport and disable VSync, the FPS cap,
and dynamic resolution. Screen percentage controls internal rendering scale.

| Profile | Shared logical-processor limit | Unreal quality level | Screen percentage | Texture pool MB |
| --- | ---: | --- | ---: | ---: |
| Potato | 2 | Low (0) | 50 | 256 |
| Low | 4 | Low (0) | 75 | 512 |
| Med | 8 | Medium (1) | 100 | 1024 |
| High | 16 | High (2) | 100 | 2048 |
| Ultra | Original caller affinity | Epic (3) | 100 | 4096 |

The quality level applies to ViewDistance, AntiAliasing, Shadow,
GlobalIllumination, Reflection, PostProcess, Texture, Effects, Foliage, Shading
and Landscape scalability groups. Texture-pool size is applied after quality
so the explicit budget takes precedence. `r.Streaming.UseFixedPoolSize=1`
enables runtime pool resizing. The verifier checks that both peer logs report
every requested setting before frame capture starts, alongside the scenario,
CSV and affinity checks. This confirms console-variable application, not visual quality or
continuous observation of effective GPU allocations during the capture.

List all presets without launching Unreal:

```powershell
& '.\Scripts\Verify-ConstrainedPerformance.ps1' -ListProfiles
```

Run the five presets sequentially with isolated evidence directories:

```powershell
$profileProject = Join-Path $env:TEMP 'k10s\Kalmala.uproject'
$profilePort = 19991
foreach ($profileName in 'Potato', 'Low', 'Med', 'High', 'Ultra') {
    & '.\Scripts\Verify-ConstrainedPerformance.ps1' -Profile $profileName -Project $profileProject -Port $profilePort -TimeoutSeconds 240
    $profilePort++
}
```

Names are case-insensitive. Low-resource presets need the first N logical
processors available in the caller's affinity; the runner rejects an
incompatible mask rather than silently widening it. Ultra means the Epic
quality preset, not Unreal's Cinematic level. The actual GPU remains an
RTX 5090; lower quality reduces its workload rather than emulating a slow GPU.
The texture pool is not a cap on total VRAM or system RAM.

## Original CPU-only matrix

| Profile | CPU access shared by listen host and remote client | Render workload |
| --- | --- | --- |
| Reference | Caller's original affinity (28 logical processors on this PC) | Existing 1280×720 offscreen hardware RHI, VSync off |
| Cpu8Threads | Logical processors 0–7 (caller must allow them) | Identical to reference |
| Cpu4Threads | Logical processors 0–3 (caller must allow them) | Identical to reference |

These are logical processors, not physical cores. The selected bits may
include SMT siblings and hybrid P/E cores. Both peers compete on the same
mask; this is more contention than two independent PCs with that processor
count. Clock speed, IPC, caches, memory bandwidth, GPU performance, physical
RAM/VRAM, storage latency and network hardware remain those of this PC.
These profiles cannot be named after a particular older CPU or GPU.

`Scripts/Verify-ConstrainedPerformance.ps1` wraps the existing rendered
discovery/disembark verifier. It restricts its own process before launching
Unreal, passes the engine's explicit `-processaffinity=N` option to prevent
Unreal startup from widening the mask, restores its original affinity in
`finally`, and monitors the actual
direct-child Unreal processes. Passing requires the existing two-peer
scenario, both 300-frame CSVs, and observations of exactly two Unreal peers
retaining the requested mask. The monitor records each observed PID/mask
combination; it is periodic sampling, not proof that affinity never changed
between samples. No existing unrelated process is throttled or stopped.

Build the current source in a disposable mirror as described in
`07-development-setup.md`; do not generate files in the source checkout.
Run profiles sequentially, using fresh output paths and unused ports:

```powershell
$projectMirror = Join-Path $env:TEMP 'k10s'
& '.\Scripts\Verify-ConstrainedPerformance.ps1' -Profile Reference -Project "$projectMirror\Kalmala.uproject" -Port 19981
& '.\Scripts\Verify-ConstrainedPerformance.ps1' -Profile Cpu8Threads -Project "$projectMirror\Kalmala.uproject" -Port 19982
& '.\Scripts\Verify-ConstrainedPerformance.ps1' -Profile Cpu4Threads -Project "$projectMirror\Kalmala.uproject" -Port 19983
```

Keep `profile.json`, `scenario.txt`, peer logs and the frame CSVs. JSON records
affinity observations and diagnostic pass/failure; PASS means fixture and
capture integrity, not compliance with an approved performance budget. The
profile elapsed time includes editor startup, connection, fixture and monitor
shutdown; it is not the game's startup metric. Run at least three repetitions
before drawing regression conclusions from timing differences.

## Other useful simulations and their limits

| Budget area | Useful constrained test | What it cannot establish |
| --- | --- | --- |
| Startup | Time a packaged launch under CPU restriction; separately test cold/warm caches | Slow-drive or older CPU startup from this warm editor fixture |
| Frame time | Compare identical scene/settings under verified CPU affinity; separately test quality/resolution workloads | RTX 5090 becoming a slower GPU; an FPS cap mostly adds waiting |
| Actors/streaming | Observe existing caps during extended land/ocean travel and late joins | Long-travel peaks from the short moored fixture |
| Memory | Record process/VRAM peaks and long-session growth; separately lower texture pool | Physical RAM or VRAM capacity, bandwidth, paging or allocation behavior of another device |
| Workers/raster | Repeat existing world-map/minimap diagnostics under CPU restriction | Older per-core latency or missing map metrics from this skiff capture |
| Replication | Add engine-local packet lag/loss in a separate authority/reconnect fixture | Real bandwidth capacity or internet behavior from loopback byte counters |
| Package/save | Measure actual archive and serialized records against approved byte caps | CPU throttling changing byte-size acceptance limits |

Unreal scalability settings control rendering cost and visual quality.
`r.Streaming.PoolSize` controls the texture pool in MB; it does not cap all GPU
memory (render targets, buffers and other allocations remain). These are useful
separate controls in the named presets above. The original CPU-only profiles
keep their rendering settings unchanged so CPU effects can be isolated.
Network impairment is likewise a separate test; never alter server authority
or client validation to accommodate simulated latency.

Sources: [Windows process affinity and child inheritance](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setprocessaffinitymask),
[Unreal scalability reference](https://dev.epicgames.com/documentation/unreal-engine/scalability-reference-for-unreal-engine),
and [texture streaming configuration](https://dev.epicgames.com/documentation/en-us/unreal-engine/texture-streaming-configuration-in-unreal-engine).

## Remaining closeout requirements

Future target-hardware certification needs approved device/profile coverage and numerical
limits for startup, frame-time percentiles/hitches, actors, process/VRAM peaks
and growth, streaming, worker/raster latency, per-peer traffic, archive size,
and save size. Existing structural/save caps in `20-m8-ocean-performance-budget.md`
remain unchanged. Hardware coverage, representative packaged scenes, sustained
travel, long-session measurements and map worker/raster profiles are still
required. Simulation evidence can supplement that matrix or form an explicitly
approved limited diagnostic gate. The owner explicitly accepted the limited
closeout described below; this does not establish target-hardware certification.

## Harness validation — 2026-10-02

The source-identical disposable `k10s` mirror's UE 5.8.2
`KalmalaEditor Win64 Development` build passed (up to date). Initial testing
demonstrated why peer monitoring is required: inherited affinity alone was
widened by Unreal startup, and the wrapper rejected the run. The engine's
explicit affinity option fixed this; both restricted peers retained their
requested masks. The option is implemented in the installed engine's
`Runtime/Launch/Private/Windows/LaunchWindows.cpp` and
`Runtime/Core/Private/Windows/WindowsPlatformProcess.cpp`.

A subsequent capture contained duplicate unrelated GPU-stat headers
(`ShadowCacheUsageMB` and `LightCount/UpdatedShadowMaps`). PowerShell's normal
`Import-Csv` rejected that file. The verifier now imports columns by position,
requires exactly one column for each measured metric, and retains the existing
300-positive-sample requirement. Focused synthetic checks accepted duplicate
unrelated stats and rejected duplicate `FrameTime` columns; both retained
eight-thread CSVs parsed successfully after the repair. No gameplay, authority,
save format, quality setting, or numerical release limit changed. These
profiling repairs support the frozen M10 performance closeout.

The final Reference, Cpu8Threads and Cpu4Threads runs passed the rendered
two-peer scenario and affinity monitor. Each peer supplied 300 positive
samples for all four timing metrics. The following are single-run p95
milliseconds (host/client), not acceptance limits:

| Profile | Frame | Game thread | Render thread | GPU |
| --- | --- | --- | --- | --- |
| Reference (mask 268435455) | 4.60 / 5.34 | 3.03 / 2.73 | 4.58 / 5.20 | 1.34 / 1.37 |
| Cpu8Threads (mask 255) | 5.77 / 5.47 | 3.05 / 3.09 | 5.49 / 5.55 | 1.32 / 1.41 |
| Cpu4Threads (mask 15) | 13.75 / 15.99 | 5.13 / 5.89 | 13.59 / 14.70 | 1.28 / 1.34 |

Frame maxima were 170.71/167.16, 138.82/190.25 and 170.55/256.79 ms
respectively. The short captures include setup hitches and do not prove
long-session smoothness. CPU restriction increased tail frame time while
GPU time remained broadly similar; repetition is needed to quantify that
effect reliably. This is contention on a fast CPU, not a lower-end GPU test.

| Profile | Initial generation ms | Actors / replicated | Host / client private MiB | Client→server / server→client bytes |
| --- | ---: | --- | --- | --- |
| Reference | 185.81 | 57 / 39 | 2928.96 / 2913.29 | 2574 / 8644 (1.00 s) |
| Cpu8Threads | 106.73 | 56 / 38 | 2945.77 / 2961.16 | 3155 / 7422 (1.00 s) |
| Cpu4Threads | 114.76 | 57 / 39 | 3005.77 / 2967.54 | 2086 / 6853 (1.01 s) |

All three observed nine active terrain patches, two population keys and a
2,215-byte population save. These fixture snapshots do not measure sustained
traffic, memory peaks/growth or extended streaming. Package size, packaged
startup, map worker/raster timing and save-cap automations were not rerun;
their latest evidence remains in `20-m8-ocean-performance-budget.md`.

Evidence directories under `%TEMP%`: `k10cpu-ref`, `k10cpu-8c`, `k10cpu-4`.
Rejected diagnostic attempts remain in `k10cpu-8` (affinity widened) and
`k10cpu-8b` (duplicate CSV names; retained captures subsequently parsed).

## Named preset verification — 2026-10-02

All five named presets passed the rendered host/client scenario, actual
affinity observations and requested console-variable confirmations before
capture. The UE 5.8.2 editor mirror build passed (up to date). Each peer
provided 300 positive samples for each of the four timing metrics.

| Profile | Observed shared CPU mask | Frame p95 host / client ms | GPU p95 host / client ms | Evidence directory under `%TEMP%` |
| --- | ---: | --- | --- | --- |
| Potato | 3 | 17.38 / 14.58 | 0.59 / 0.56 | k10tier-potato |
| Low | 15 | 9.65 / 10.64 | 0.73 / 0.75 | k10tier-low |
| Med | 255 | 4.16 / 5.45 | 0.98 / 1.09 | k10tier-medb |
| High | 65535 | 3.69 / 3.91 | 1.19 / 1.21 | k10tier-highb |
| Ultra | 268435455 | 4.55 / 5.50 | 1.24 / 1.50 | k10tier-ultrab |

These are single short captures, not hardware-tier rankings or approved
performance ceilings. The presets change both CPU resources and GPU workload.
Setup hitches remain in the samples (the Potato client's maximum frame was
1,259.76 ms). Nine terrain patches, two population keys and a 2,215-byte
population save were observed in every run; full snapshots remain in each
`scenario.txt`, with preset metadata and peer masks in `profile.json`.

The first Med run (`k10tier-med`) exposed a numeric CSV footer value being
counted as a 301st FrameTime sample. The parser now locates the EVENTS column
and skips `[HasHeaderRowAtEnd]` metadata before reading timing values, retaining
its exact 300-sample guard. A synthetic regression with duplicate unrelated
headers and a numeric footer passed; all eight retained Med metrics and all
24 legacy CPU-profile metrics then parsed with exactly 300 samples. The final
Med rerun passed end to end. Script syntax, all eight entries in `-ListProfiles`,
Potato pre-capture settings ordering, and git diff checks passed.

No gameplay code, authority, replication, save schema, product video defaults
or machine-wide settings changed. Physical hardware coverage, numerical limits,
and packaged/sustained scenarios remain limitations of these measurements.

## Product-owner closeout — 2026-10-02

After reviewing the five passing presets, the owner explicitly requested
marking the blocked task done. The backlog parent is now completed on that
direction, accepting the existing one-machine measurements and simulated
profile evidence with the limitations recorded here and in
`20-m8-ocean-performance-budget.md`. Earlier statements that the parent is
blocked or pending are superseded by this acceptance.

No numerical limits, physical low/mid-tier measurements or additional test
results were supplied or invented. Those coverage gaps are accepted limitations
of this closeout, not claims of certification. The release-candidate package
and normal-player co-op walkthrough are the next backlog task.
