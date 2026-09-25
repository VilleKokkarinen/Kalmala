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
The two-peer startup snapshot now records actual actor count and per-process
memory, but sustained/peak use, network bytes, and rendered frame time remain
unmeasured. The crossing fixture remains blocked on steering as recorded in
`PROGRESS.md`. The serialized save cap and two-peer actor/memory snapshot are
completed slices of the open M8 budget task.
