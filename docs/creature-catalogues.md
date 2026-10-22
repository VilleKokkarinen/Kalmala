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
table's health, armor, and base attack values directly. Server-side player-hit
validation accepts up to 300 total wildlife damage, matching the largest
authored A8a attack. Rat (10 health, zero armor, 5 Pierce) now has deterministic
Meadows selection, procedural presentation, an authority-validated 100 cm bite,
and credited Bone Fragment/Rat Tail rewards. Full-pack Rat rewards stay in
world-population save escrow until both items fit. Badger (100 health, 10 armor,
20 Slash) now uses the reviewed 40% Elderwood common share, a generated low-poly
silhouette, and a server-owned warning/claw cycle. Its baseline Bone Fragment
reward is saved against the credited attacker's authenticated identity and
delivered atomically when their pack can accept it; unclaimed rewards retry
after reconnect or restart. Badger Fur and Badger Meat remain disabled while
their A8b source-item tasks are incomplete. The server-side stat lookup covers
all 17 reviewed A8a profiles; other planned species remain disabled.
Listing a drop does not enable it or certify its A8b/A9 item/icon/source work.
Rat's runtime peer/restart acceptance remains part of combined I4. The existing
world-population save keeps pending reward rows beside its existing harvested
and defeated IDs; the additive row does not change those stable identities.

Run `python Scripts/Validate-CreatureCatalogues.py` to check the manifest,
17 unique IDs, allowed fields, stats, typed attacks, drop references, inline
poison, biome IDs and exclusive gem sources. Unreal/runtime acceptance remains
part of the combined I4 milestone-final gate in `07-development-setup.md`.
