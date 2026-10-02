# M8 ocean travel budget profile

## Serialized travel-save record cap

Each schema-1 M8 ocean travel save record must serialize to at most 3,072
bytes through Unreal's `SaveGameToMemory` path. This applies separately to
the one world-scoped moored-vessel record and each authenticated player-scoped
seat record. `SetVesselState` and `SetPassengerState` roll back and reject a
candidate if serialization fails or exceeds the cap. A two-player world can
therefore contribute at most 9,216 serialized bytes across one vessel and two
seat records. The separate M7 sparse discovery ledger is outside this cap.

The 2026-09-25 UE 5.8.2 Development measurement used the stable primary vessel
identity and a `player-alpha` seat for the ordinary records, then the maximum
128-character vessel ID and maximum 128-character authenticated owner identity
for the maximum-valid cases:

| Record case | Serialized bytes | Cap |
| --- | ---: | ---: |
| World vessel, primary ID | 2,598 | 3,072 |
| Player seat, `player-alpha` | 2,821 | 3,072 |
| World vessel, maximum-length ID | 2,707 | 3,072 |
| Player seat, maximum owner and vessel IDs | 2,937 | 3,072 |

These are `SaveGameToMemory` payload lengths, which include Unreal serialization
overhead. They are not measurements of on-disk slot wrapper size or the full
M7 discovery ledger. Maximum-valid two-player travel payloads measured at
8,581 bytes total, below the 9,216-byte arithmetic ceiling.

## Two-player actor and process-memory snapshot

Run `Scripts/Verify-OceanSkiffDiscoveryDisembark.ps1` after the forced editor
build to measure a bounded M8 state with the seed-418 listen host and one
conflicting-seed client. The runner enables the existing `KalmalaWorldProfile`
report and waits until the discovery/disembark fixture passes before accepting
the profile. It records total and replicated world actors, active terrain
patches, initial generation time, and the engine's used/available physical
memory snapshot. It also samples each Unreal process's private bytes and
working set from PowerShell while both peers are still running.

The scenario has one moored primary skiff, two authenticated seats, two accepted
optional discovery claims, and an empty vessel after safe disembark. This
startup snapshot provides actor and per-process memory evidence for that
specific two-peer fixture. It is not a memory-growth or peak measurement and
does not cover underway travel, a late join, restart, longer sessions, or a
representative rendered frame-time target. Current structural limits remain
one vessel per session and at most 25 active terrain patches; numeric actor
and process-memory ceilings still require an accepted target.

The 2026-09-25 run reported:

| Measurement | Value |
| --- | ---: |
| World actors (including the M8 skiff and fixture) | 56 |
| Replicated world actors | 38 |
| Active terrain patches | 9 |
| Active population keys | 2 |
| Initial generation | 102.35 ms |
| System used / available physical memory | 1,799.97 / 19,039.00 MiB |
| Listen-server private bytes / working set | 1,677.39 / 1,799.97 MiB |
| Client private bytes / working set | 1,675.50 / 1,774.26 MiB |

The profile sampled one run immediately after the peers accepted their rewards
and disembarked from the still-live moored skiff. These values are evidence for
that machine and fixture, not numeric performance ceilings.

## Two-peer connection traffic snapshot

`Scripts/Verify-OceanSkiffDiscoveryDisembark.ps1` also records the remote
client's server-side `UNetConnection` byte and packet counter deltas. The
baseline is captured after both peers are ready and immediately before the
discovery/skiff fixture mutates state. The server samples again one second
after it verifies both rewards and safe disembarkation, allowing the accepted
actor state and owner feedback to replicate. The reported directions are from
the server's perspective: inbound is client-to-server and outbound is
server-to-client.

These are Unreal connection counters for one remote peer over the fixture
window. They include all connection traffic in that interval and do not
separate actor replication from RPC/control traffic or account for UDP/IP
headers. They exclude the listen host's in-process local player and the initial
connection handshake. This moored discovery/disembark snapshot does not measure
underway movement, sustained traffic, or a traffic ceiling; rendered frame
time is measured separately below.

The 2026-09-26 UE 5.8.2 Development/null-RHI run on seed 418 reported:

| Direction | Bytes in 1.00 s | Packets in 1.00 s |
| --- | ---: | ---: |
| Client to listen server | 3,842 | 67 |
| Listen server to client | 7,951 | 68 |

The same run passed the two-peer discovery/disembark acceptance and recorded
56 world actors, 38 replicated actors, 9 terrain patches, 177.49 ms initial
generation, and 2,215 serialized population-save bytes. The byte counts are a
single bounded moored-state sample, not a numeric budget target.

## Two-peer rendered frame-time snapshot

After a forced editor build, run
`Scripts/Verify-OceanSkiffDiscoveryDisembark.ps1 -RenderedFrameTimeProfile`.
This retains the seed-418 `L_Prototype` listen-host and conflicting-seed client
scenario. Each peer starts a 300-frame CSV capture only after it observes the
accepted optional discovery, exact owner reward, and empty moored skiff after
safe disembark. Captures are written to the run's temporary `FrameTimes`
directory, outside the project `Saved` tree. The runner requires 300 positive
samples for `FrameTime`, `GameThreadTime`, `RenderThreadTime`, and `GPUTime`
from both peer CSVs and rejects NullRHI.

The 2026-09-26 UE 5.8.2 Win64 Development run requested a 1280x720 offscreen
D3D12 RHI render, with VSync disabled and GPU CSV stats enabled. Both peer logs
reported D3D12 on an NVIDIA GeForce RTX 5090. The measured scene contained the
accepted seed-418 shellbank-shoal reward, two disembarked players, one empty
moored skiff, 56 world actors, 38 replicated actors, and 9 active terrain
patches. Values below are p50 / p95 milliseconds across 300 frames per peer;
they are a single diagnostic snapshot, not approved performance ceilings.

| Metric | Listen host p50 / p95 (ms) | Remote client p50 / p95 (ms) |
| --- | ---: | ---: |
| Total frame (`FrameTime`) | 2.52 / 4.67 | 4.07 / 7.09 |
| Game thread | 1.80 / 2.56 | 2.68 / 3.97 |
| Render thread | 2.49 / 4.83 | 4.04 / 6.47 |
| GPU | 0.97 / 1.15 | 1.14 / 1.45 |

The capture ran offscreen through the hardware RHI and did not present to a
monitor; it excludes display and VSync presentation costs. The first captured
frames include setup hitches (maximum total-frame samples were 165.81 ms on
the host and 187.06 ms on the client), so the percentiles describe the steady
portion better than the maxima. This is one short moored-state sample, not an
underway, sustained, peak, packaged, or representative-hardware matrix result.
No numeric actor, memory, replication, or frame-time ceilings have been
approved.

## Verification

After the forced `KalmalaEditor Win64 Development` build, run
`Kalmala.World.OceanTravel.PersistenceContract` and
`Kalmala.World.M7.PersistenceContract` with an isolated `-UserDir`, unique
`-abslog`, `-DDC-ForceMemoryCache`, and
`-TestExit="Automation Test Queue Empty"`. The travel contract checks normal
and maximum-valid identities and rejects setters that exceed the byte cap.
The 2026-09-25 run passed both automations; the travel test logged the table's
four measurements.

## Remaining M8 profile scope

The skiff's existing contract remains one vessel per session with movement
replication capped at 10 updates per second. Ocean discovery activation is
bounded to 18 nearby cells across the two-player prototype, and generated
terrain remains capped at 25 active patches. These are structural ceilings.
The two-peer startup snapshot records actor count, per-process memory, a
bounded per-client connection-traffic delta, and the rendered frame-time
snapshot above, but sustained/peak use remains unmeasured. The integrated
2.4 km host/client crossing now passes its steering and terrain-patch checks;
see `docs/19-m8-ocean-travel-contract.md`. The fixture records
`OriginShift=inactive`, and underway sustained/peak budgets remain unmeasured.
Numeric performance targets have not been approved, so the recorded snapshots
do not establish that the game meets a performance ceiling.

## M10 one-machine diagnostic refresh — 2026-10-02

These measurements refresh the existing development diagnostics on one
Windows machine; they do not close M10's representative hardware/profile
matrix. The offscreen D3D12 runs reported an NVIDIA GeForce RTX 5090 with
32,607 MiB of video memory and driver 591.86. UnrealBuildTool reported 20
physical and 28 logical cores and 31.76 GB physical memory. A follow-up local
inventory identified the processor as an Intel Core i7-14700K. No low- or
mid-tier device was available to this run.

The forced UE 5.8.2 editor build in a disposable mirror was up to date. A
Windows Development `BuildCookRun` build then compiled both editor and game
targets, cooked all 521 packages, and archived with `-pak -iostore` in 87.07
seconds. The archive contained 48 files and measured 925.69 MiB total: 851.14
MiB under `Windows/Kalmala` and 74.55 MiB of engine runtime dependencies. Its
longest absolute path was 145 characters. The fresh-profile null-renderer
smoke reached `Game Engine Initialized` and loaded `L_Prototype` in 3.65
seconds; the nested game process stayed alive for the required 20 seconds.
This is a Development archive measurement, not the release package or a
package-size ceiling.

The current seed-418 two-peer world-profile run reported 101.24 ms initial
generation, 1,759.27 MiB used and 18,385.62 MiB available physical memory,
47 actors / 29 replicated actors, 9 active terrain patches, 1 active
population key, and 2,215 serialized population-save bytes. The moored
discovery/skiff fixture reported 103.54 ms initial generation, 56 actors / 38
replicated actors, 9 patches, 2 population keys, and the same 2,215-byte
population save. Its memory snapshot reported 2,542.62 MiB used and 16,593.34
MiB available; host private bytes / working set were 3,008.58 / 2,542.70 MiB,
and client values were 3,029.99 / 2,566.82 MiB. These are startup or fixture
snapshots, not peak or long-session measurements.

The 1.00-second server-side connection window in that fixture counted 2,404
client-to-server bytes in 40 packets and 10,994 server-to-client bytes in 52
packets. This is a single moored-state traffic sample, not a bandwidth target.
Two valid 300-frame captures at 1280×720, with VSync disabled after the
scenario reached its accepted moored state, produced these p50/p95 ranges in
milliseconds:

| Metric | Listen host p50 / p95 | Remote client p50 / p95 |
| --- | ---: | ---: |
| Total frame | 2.44–3.36 / 4.43–5.48 | 3.62–3.90 / 5.53–5.68 |
| Game thread | 2.15–2.84 / 4.19–4.51 | 2.94–3.05 / 4.94–5.04 |
| Render thread | 2.48–3.37 / 3.83–5.30 | 3.65–3.86 / 5.32–5.39 |
| GPU | 0.97–1.14 / 1.25–1.42 | 1.15–1.22 / 1.42–1.59 |

The maximum total-frame sample ranged from 106.26–126.12 ms on the host and
116.90–138.85 ms on the client; these short captures include setup hitches.
The map worker profile at 1280×720 recorded host/client open times of 327.135
/ 62.658 ms, worker totals of 20.001 / 27.895 ms, worker maxima of 10.171 /
15.380 ms, and 33,800 cached CPU bytes on each peer. That sample had two ready
tiles. `Kalmala.UI.Minimap.GenerationPerformance` passed with mean refreshes
of 44.304, 58.459, and 101.790 ms for 2.5, 5, and 10 km raster radii.

The focused `Kalmala.World.OceanTravel.PersistenceContract` and
`Kalmala.World.M7.PersistenceContract` automations passed again. The former
rechecks the existing 3,072-byte record caps and 9,216-byte maximum-valid
two-player total; the two-peer world profile separately confirmed a 2,215-byte
population save.

The frame-profile runner's two-argument `String.IndexOf` overload failed in
its Windows PowerShell 5.1 and PowerShell 7.6 invocations before it could
report the peer results. Its three literal marker searches now use the
single-string overload; the same complete live host/client scenario then
passed and captured both 300-frame CSVs. This is a profiling-harness fix only;
gameplay, network authority, replication, and save contracts are unchanged.

The M10 performance closeout remains open. The repository has no accepted
numeric ceiling for frame time, actor count, memory, replication traffic, or
archive size, and no representative low/mid/high hardware matrix is recorded.
The measurements above cover one high-end GPU, one seed, and a short moored
profile; they do not establish sustained travel, long-session growth, peak
resource use, or target-hardware compliance. Closeout needs an approved
hardware/profile matrix and acceptance limits, then measurements on those
targets. The M10 backlog item remains unchecked until that matrix passes.

## Constrained CPU diagnostics — 2026-10-02

The product owner's follow-up requested simulated lower-resource profiles.
[`34-m10-constrained-performance.md`](34-m10-constrained-performance.md)
documents the new reference/8-thread/4-thread matrix and its measured evidence.
The runner uses Unreal's explicit process affinity and monitors both actual
peers, with identical rendered workload across profiles. It also tolerates
duplicate unrelated GPU-stat CSV headers while rejecting ambiguous measured
metrics. These simulations provide CPU-contention evidence on the same
i7-14700K / RTX 5090; GPU, physical RAM/VRAM and storage remain unchanged.
The representative-hardware and approved-numerical-budget blocker remains.

### Product-owner closeout — 2026-10-02

Following verification of Potato/Low/Med/High/Ultra, the owner explicitly
directed marking the performance backlog task done. This supersedes the pending
closeout/blocker statements above. Completion accepts the available one-machine
and simulated-profile evidence with its documented limitations; physical
low/mid-tier coverage, approved numerical ceilings, and extended/packaged
measurements remain deferred. No additional measurements or numerical-budget
compliance are implied. See `34-m10-constrained-performance.md`.
