# M9 carried-tool inventory contract

The first tool-progression increment replaces separate per-tool condition
properties with the character's bounded `CarriedTools` record array. Each
`FKalmalaToolState` carries a canonical tool ID, condition, and authored tool
level. Tool level belongs to the tool record and stays separate from the
player's skill progression ledger.

The server creates the initial three first-wave records at level 1 and full
condition when an authoritative character begins play. The inventory is
bounded to five records for the current three first-wave tools and two M9 axe
tiers. Uncrafted Bronze and Iron Axes remain absent from the starting list.

## M9 axe progression entries

`FKalmalaToolProgressionContract` defines the two server-authored axe paths.
The target tool level equals the required effective station level, following
the roadmap's station-match rule.

| Tool | Target level | Prerequisite | Station | Materials |
| --- | ---: | --- | --- | --- |
| Bronze Axe | 1 | None | Workbench level 1 | 4 Wood, 3 Stone, 2 Fibre |
| Iron Axe | 2 | Bronze Axe level 1 | Forge level 2 | 3 Lightwood, 2 PeatAmber, 4 Stone, 2 Fibre |

All costs use existing item-catalogue IDs. Densewood is excluded from the Iron
Axe recipe because Iron Axe is itself required to harvest it. The catalogue
validates tool identities, one-level progression, station/target matching,
and bounded costs against the item catalogue. These entries are definitions
only: no craft or upgrade transaction consumes the materials yet, and the
level-two Forge becomes usable after the later Forge attachment work. Axes
remain absent from starting inventory and tool level remains transient.

`CarriedTools` is replicated with `COND_OwnerOnly`. Harvest and repair code
reads and changes a record on the server; existing client requests continue to
send only tool or recipe intent. The detail array is transient and is not a
SaveGame field. Tool levels, workstation levels, upgrades, and free repair are
not enabled by this increment; the existing M7 material-paid repair behavior
remains until the later M9 repair task replaces it.

## Verification

After the forced editor build, run
`Kalmala.Gameplay.Tools.CarriedToolInventoryContract` with the existing
`Kalmala.Gameplay.Tools.LifecycleContract`,
`Kalmala.Gameplay.M9.AxeHarvestGates`,
`Kalmala.Gameplay.M9.SecondWaveHarvestAcceptance`, and
`Kalmala.Gameplay.Crafting.NetworkContract` tests. The carried-inventory
contract checks the bounded starting list, full initial condition, level-1
baseline, transient state, and owner-only replication condition.
`Kalmala.Gameplay.M9.ToolProgressionCatalogue` checks the axe entries, target
and prerequisite levels, station matches, and item-catalogue-valid costs.
`Scripts/Verify-InventoryReconnect.ps1` also checks two live client visits:
the owner receives tool condition and the remote peer receives no tool details.
