# M10 constrained single-machine performance diagnostics

## Purpose and acceptance boundary

The product owner's 2026-10-02 request permits investigating lower-resource
profiles on the available i7-14700K / RTX 5090 Windows PC. The diagnostic
matrix below makes CPU contention repeatable without changing gameplay,
machine-wide power settings, drivers, or project configuration. It does not
approve minimum/recommended hardware, numerical release limits, or substitute
for measurements on those targets. The M10 performance parent remains open.

## Implemented CPU matrix

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
separate experiments, not implemented presets in this CPU runner. Keep workload
and resource restrictions separate so changes can be attributed correctly.
Network impairment is likewise a separate test; never alter server authority
or client validation to accommodate simulated latency.

Sources: [Windows process affinity and child inheritance](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setprocessaffinitymask),
[Unreal scalability reference](https://dev.epicgames.com/documentation/unreal-engine/scalability-reference-for-unreal-engine),
and [texture streaming configuration](https://dev.epicgames.com/documentation/en-us/unreal-engine/texture-streaming-configuration-in-unreal-engine).

## Remaining closeout requirements

The owner still needs to approve target device/profile coverage and numerical
limits for startup, frame-time percentiles/hitches, actors, process/VRAM peaks
and growth, streaming, worker/raster latency, per-peer traffic, archive size,
and save size. Existing structural/save caps in `20-m8-ocean-performance-budget.md`
remain unchanged. Hardware coverage, representative packaged scenes, sustained
travel, long-session measurements and map worker/raster profiles are still
required. Simulation evidence can supplement that matrix or form an explicitly
approved limited diagnostic gate; it cannot silently close release acceptance.

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
