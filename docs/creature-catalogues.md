# Creature catalogue data

`Content/Data/GameCatalogues.json` registers `creatures.json` through
`creatureFiles`. The file contains one `creatures` array with 17 flat records,
including exactly one Wolf.

Each record contains `CreatureId`, `DisplayName`, `Health`, `Armor`, `Biomes`,
`Attacks` and `Drops`. Body length is included where specified by the design. There are no implementation-status
flags, runtime snapshots, planned-profile wrappers, locomotion labels, AI prose
or engineering notes in the JSON.

The stats are the authored targets from `biome-enemies.md`, including the
boar/deer/Mireling retunes. They do not describe the old runtime defaults.
Damage uses explicit typed channels before defenses; omitted channels are zero.
Timings use seconds and distances use centimetres. Every poison application
uses the same attack field: `Poison: {DamagePerSecond, DurationSeconds}`.
Tick scheduling and refresh/stacking rules belong to the shared engine status
system; species do not define their own clocks. Drops contain only `ItemId`, `Quantity` and `Chance` (0-1);
quantity is awarded when the roll succeeds.

Defensive trigger ranges, swarm behavior, AI movement, spawning/pack rules,
poison refresh/overlap policy,
reward safety and outstanding design decisions remain in `biome-enemies.md`
and the backlog. They belong to the later engine implementation. There is no
separate rules JSON. Existing Mireling boss-scroll handling stays in code.

This is authoring data. The schema-15 C++ item/tool loader does not consume the
creature category yet. The existing Boar, Deer, and Mireling actors apply the
table's health, armor, and base Crush damage values directly. Server-side
player-hit validation accepts up to 300 total wildlife damage, matching the
largest authored A8a attack; remaining A8a runtime stat profiles and attack
behavior are open. Listing a drop does not enable it or certify its A8b/A9
item/icon/source work.
This cleanup changes no gameplay, RPCs, replication or save format.

Run `python Scripts/Validate-CreatureCatalogues.py` to check the manifest,
17 unique IDs, allowed fields, stats, typed attacks, drop references, inline
poison, biome IDs and exclusive gem sources. Unreal/runtime acceptance remains
part of the combined I4 milestone-final gate in `07-development-setup.md`.
