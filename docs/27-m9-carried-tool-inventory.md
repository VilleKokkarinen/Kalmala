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

## Paid station attachments

Two paid construction kits add one effective level to a matching nearby
station: the Workbench tool rack costs 2 Lashed timber and 2 Reed fibre; the
Forge anvil costs 3 Lashed timber and 4 Fieldstone. Each recipe requires its
matching visible same-world Workbench or Forge within the normal 250 cm
station-use range. Placement is server-derived from the pawn, rechecks the
matching usable station, and requires the attachment to land within 125 cm of
that station. One attachment upgrades a station from level 1 to level 2; a
second attachment at the same station is rejected. Effective level is capped
at the authored level-2 tier.

The placed attachment actor and level bonus are session-only until the M9
versioned save contract passes migration and round-trip checks. Attachments do
not enter the existing construction save, and no tool or construction save
schema changed. The server limits live attachments to 32, checks placement and
kit costs before committing, and derives level from initialized same-world
construction actors. Clients submit only the attachment kit ID for placement
and the tool ID for an upgrade; they cannot select the station or author a
distance, level, or outcome.

The owner crafting panel now offers Bronze Axe crafting and Iron Axe upgrading.
The client sends only the target tool ID. The server selects the nearest
visible same-world station of the required family within 250 cm, derives its
base level, validates an exact level match, checks the carried-tool prerequisite
and condition bounds, and builds both the paid inventory candidate and carried
tool candidate before committing either. Bronze Axe is added at level 1 with
full condition from a level-1 Workbench. Iron Axe replaces a carried level-1
Bronze Axe and requires a level-2 Forge. The level-1 Forge rejects that
upgrade until a nearby paid anvil is placed and accepted by the server.
Rejected calls leave materials and carried tools unchanged.

Axes remain absent from starting inventory and tool level remains transient.
The existing owner-only CarriedTools replication carries crafted axe level and
condition; no new RPC state, save field, or schema was added. M7 material-paid
repair remains until the later free-repair task.

## M9 station transaction verification

Kalmala.Gameplay.M9.ToolProgressionCatalogue checks the axe and attachment
entries, compatible station families, bounded level derivation, placement
range and duplicate rejection, paid recipes, and item-catalogue-valid costs.
Kalmala.Gameplay.M9.ToolStationProgression checks paid Bronze
Axe creation and Iron Axe replacement, exact station family/level validation,
prerequisite checks, full-condition output, material consumption, and
no-mutation rejection. It also verifies that the tool progression RPC carries
only the target tool ID, without client-supplied station, level, cost, or result.

`CarriedTools` is replicated with `COND_OwnerOnly`. Harvest and repair code
reads and changes a record on the server; existing client requests continue to
send only tool or recipe intent. The detail array is transient and is not a
SaveGame field. Tool and workstation levels are derived from transient
server-owned records and placed attachment actors. Free selected-tool repair
is available at a Workbench or Forge; Grinding Stone Repair All remains open.

## M9 free selected-tool repair

The owner crafting panel has a separate repair action for each of the three
starting tools and both carried axe tiers. Each request sends only the chosen
tool ID. The server finds that tool in the owner's carried records, validates
a visible same-world Workbench or Forge within 250 cm, and restores a damaged
or zero-condition record to its authored maximum. The action spends no pack
materials and awards no Crafting experience. Unknown, absent, full-condition,
or out-of-range tools leave condition and inventory unchanged. Tool level is
preserved, and the existing `CarriedTools` array remains owner-only and
transient.

This M9 rule retires M7's material-paid repair and zero-condition replacement
recipes.

## M9 Grinding Stone Repair All

The paid `GrindingStoneKit` costs 2 Lashed timber and 4 Fieldstone and is
crafted at a visible same-world Workbench within 250 cm. It uses the existing
construction placement checks and schema-1 construction record; the Grinding
Stone adds no save fields or repair inventory. The owner interacts with the
placed stone to request Repair All. The server revalidates that exact accepted
stone as same-world, visible, and within 250 cm before reading the owner's
server-owned `CarriedTools` array.

Repair All builds and validates a complete candidate for the bounded list
before publishing it. Every damaged or zero-condition known tool returns to
its authored maximum; already-full tools remain unchanged, and identity, list
slot, and tool level are preserved. Unknown, duplicate, oversized, or malformed
tool state rejects the whole action without partial repair. The client sends
no tool list, tool ID, or condition value. Repair costs no materials and gives
no Crafting experience. The result and detailed tool condition remain
owner-only.

After the server accepts Repair All, it sends a parameterless cosmetic
multicast to the character's procedural player model. Each peer locally plays
a 1.2-second, three-stroke sharpening pose: one arm braces while the other
draws across the stone, then both arms blend back to their current gait pose.
The procedural parts remain collision-free. The multicast carries no tool,
inventory, condition, cost, or result data; the pose changes no gameplay or
saved state. `Kalmala.Gameplay.Tools.SharpeningPresentation` checks the bounded
pose duration, alternating stroke, smooth rest-pose endpoints, and rejection
of invalid elapsed time. Rendered in-world readability remains a separate
visual review.

`Kalmala.Gameplay.M9.GrindingStoneRepairAll` checks the paid recipe and
existing save/placement kit contracts, all five carried-tool definitions,
broken and damaged repair, unchanged full tools and levels, and atomic rejection
for invalid authority, missing stone validation, and malformed lists.

## Verification

After the forced editor build, run
`Kalmala.Gameplay.Tools.CarriedToolInventoryContract` with the existing
`Kalmala.Gameplay.Tools.LifecycleContract`,
`Kalmala.Gameplay.M9.AxeHarvestGates`,
`Kalmala.Gameplay.M9.SecondWaveHarvestAcceptance`, and
`Kalmala.Gameplay.Crafting.NetworkContract` tests. The carried-inventory
contract checks the bounded starting list, full initial condition, level-1
baseline, transient state, and owner-only replication condition.
`Kalmala.Gameplay.M9.ToolProgressionCatalogue` checks axe and attachment
catalogues, station matches, level derivation, attachment placement bounds,
paid recipes, and item-catalogue-valid costs.
`Scripts/Verify-InventoryReconnect.ps1` also checks two live client visits:
the owner receives tool condition and the remote peer receives no tool details.
