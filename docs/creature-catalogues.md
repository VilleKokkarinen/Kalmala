# Creature catalogue data

`Content/Data/GameCatalogues.json` registers one `creatureFiles` entry:
`creatures.json`. Its `creatures` array contains all 17 species: the three
current creatures and fourteen planned A8a additions.

Shared spawn, poison-refresh and reward policies remain in `biome-enemies.md`.
There is no separate rules JSON: the engine never consumed it, and duplicating
those design notes here is unnecessary. Per-species poison damage, duration
and tick timing are stored directly on the applicable attack as `Poison`.

Each species has one unique `CreatureId`, a `DisplayName`, an
`ImplementationStatus`, and a `PlannedProfile`. Existing species also have a
`RuntimeBaseline` snapshot from `KalmalaWildlifeSpawn` and the default zero
defenses in `KalmalaResistanceComponent`. The snapshot distinguishes current
100 HP, zero Armor, 15 Crush boar / 10 Crush Mireling defaults from the planned
retunes. Runtime code remains the source of truth for those current values.
Mireling boss-scroll eligibility is retained as a rule rather than represented
as a guaranteed inventory drop.

Profiles contain `Health`, `Armor`, `Biomes`, `Behavior`, typed `Attacks` and
`Drops`. Attack damage is the authored hit before defenses; omitted damage
channels are zero. Timings use seconds, distances use centimetres, and drop
`Chance` uses 0-1 probability. `Quantity` is awarded on a successful roll.
Drop `ImplementationStatus` describes whether its source grant currently exists,
not whether an item with that name happens to be defined. All new enemies and
their drops remain planned. Planned A8b/A9 item IDs are intentional future
references, never permission to grant unknown items or enable unfinished loot.

There is exactly one Wolf entry. Pack spawning is deferred to engine work per
the user's clarification; no pack sizes or duplicate Wolf species are authored.
Horned Goat push/stagger tuning and unmatched body-size reach still require
review. Reef Maw needs aquatic implementation and targets swimmers only.
Common/hard attack timings retain the detailed proposal; this data
does not certify task-level review or runtime acceptance.

This is an additive **authoring extension** to manifest schema 15. The existing
C++ item/tool loader ignores this creature list; no creature runtime loader, AI,
spawn retune, attack activation, loot grant, RPC, replication or save change is
introduced. A8a engine integration remains open and must validate these fields
on the server before consuming them. Keeping the existing manifest version
preserves the current loader; a future runtime creature contract may version
its own supported fields deliberately. No enemy implementation or milestone
checkbox is completed by data authoring alone.

Run the lightweight data check from the project root:

```powershell
python Scripts/Validate-CreatureCatalogues.py
```

It checks manifest paths and JSON, complete species coverage, duplicate IDs,
stats and typed damage, known/current versus intentional future drop references,
inline poison, exclusive gem sources and biome identities. It does
not replace the Unreal, multiplayer and milestone-final checks in
`07-development-setup.md`.
