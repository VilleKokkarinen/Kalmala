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
terrain remains capped at 25 active patches. These are structural ceilings;
the live integrated voyage has not measured actual actor counts, per-process
memory growth, network bytes, or representative rendered frame times. The
crossing fixture remains blocked on steering as recorded in `PROGRESS.md`.
The serialized save cap is one completed slice of the open M8 actor, memory,
replication, save-size, and frame-time budget task.
