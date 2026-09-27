# M9 carried-tool inventory contract

The first tool-progression increment replaces separate per-tool condition
properties with the character's bounded `CarriedTools` record array. Each
`FKalmalaToolState` carries a canonical tool ID, condition, and authored tool
level. Tool level belongs to the tool record and stays separate from the
player's skill progression ledger.

The server creates the initial three first-wave records at level 1 and full
condition when an authoritative character begins play. The inventory is
bounded to five records for the current three first-wave tools and two M9 axe
tiers. Uncrafted Bronze and Iron Axes remain absent; later progression work
will define their levels, costs, and server-side acquisition transaction.

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
`Kalmala.Gameplay.Crafting.NetworkContract` tests. The new contract checks the
bounded starting list, full initial condition, level-1 baseline, transient
state, and owner-only replication condition. `Scripts/Verify-InventoryReconnect.ps1`
also checks two live client visits: the owner receives tool condition and the
remote peer receives no tool details.
