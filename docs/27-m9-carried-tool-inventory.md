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
and bounded costs against the item catalogue.

The paid Forge recipe uses 5 Lashed timber and 6 Fieldstone. It uses the
existing generic construction placement and schema-1 construction record;
Workbench and Forge levels are derived from their kit identity, with each base
station at level 1. No new save field stores a station level.

The owner crafting panel now offers Bronze Axe crafting and Iron Axe upgrading.
The client sends only the target tool ID. The server selects the nearest
visible same-world station of the required family within 250 cm, derives its
base level, validates an exact level match, checks the carried-tool prerequisite
and condition bounds, and builds both the paid inventory candidate and carried
tool candidate before committing either. Bronze Axe is added at level 1 with
full condition from a level-1 Workbench. Iron Axe replaces a carried level-1
Bronze Axe and requires a level-2 Forge. Forge attachments are a later task, so
the current level-1 Forge correctly rejects that upgrade until it is improved.
Rejected calls leave materials and carried tools unchanged.

Axes remain absent from starting inventory and tool level remains transient.
The existing owner-only CarriedTools replication carries crafted axe level and
condition; no new RPC state, save field, or schema was added. M7 material-paid
repair remains until the later free-repair task.

## M9 station transaction verification

Kalmala.Gameplay.M9.ToolProgressionCatalogue checks the axe entries, target
and prerequisite levels, station matches, Forge recipe, and item-catalogue-
valid costs. Kalmala.Gameplay.M9.ToolStationProgression checks paid Bronze
Axe creation and Iron Axe replacement, exact station family/level validation,
prerequisite checks, full-condition output, material consumption, and
no-mutation rejection. It also verifies that the tool progression RPC carries
only the target tool ID, without client-supplied station, level, cost, or result.

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
