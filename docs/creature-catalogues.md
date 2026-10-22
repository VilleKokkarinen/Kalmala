# Creature catalogue data

`Content/Data/GameCatalogues.json` registers `creatures.json` through
`creatureFiles`. The file contains one `creatures` array with 17 flat records,
including one Wolf and one Lynx.

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
silhouette, and a server-owned warning/claw cycle. Its Bone Fragment, Badger
Fur, and Badger Meat reward bundle is saved against the credited attacker's
authenticated identity and delivered atomically when their pack can accept it;
unclaimed rewards retry after reconnect or restart. Moon Bear (250 health, 20
armor, 75 Crush) occupies the seeded 10% Elderwood hard-enemy share, has a
server-owned 220 cm overhead paw strike and queues three Bone Fragments for its
credited attacker in the existing population-save escrow. The separate A9
Moonstone source is not enabled here. The server-side stat lookup covers all 17
reviewed A8a profiles. Viper (100 health, 10 armor, 50 Stab) occupies the seeded
40% Shimmering Lakes share, is filtered to eligible dry shoreline, and uses the
server-owned narrow lunge to apply the existing five-second weak-poison status
after an accepted bite. Its one-Bone-Fragment baseline and guaranteed Viper
Fang rewards share a durable authenticated-owner claim in the same
population-save escrow, and the server commits them together. Fang Arrows remain
a separate A8b output task. Giant Snake
(250 health, 30 armor, 100 Stab) fills the remaining seeded 10%
Shimmering Lakes slot on dry shoreline and uses a server-owned, one-second
telegraphed lunge with a 1.2-second recovery. Each credited defeat queues two
Bone Fragments and two guaranteed Giant Scales in the same authenticated,
owner-bound population-save claim; the server commits the full bundle together.
It does not apply poison. The 150 m generated-start exclusion applies to this
hard enemy.
Horned Goat (200 health, 50 armor, 50 Crush) now occupies four seeded
Thunder Mountains slots (40%), limited to dry traversable terrain. Its original
procedural presentation shows a half-second charge warning; the server accepts
one 50-Crush horn hit within 180 cm, then pushes a living target up to 250 cm
and staggers it for 0.75 seconds, with collision clipping the displacement.
Credited defeats queue two Bone Fragments for the authenticated owner in the
existing population-save escrow. Goat Horn remains disabled pending A8b.
Thunder Sentinel fills the rare 10% Thunder Mountains share and uses the dry,
traversable mountain gate plus the shared 150 m generated-start exclusion. Its
original stone silhouette raises an arm for a one-second warning, steps up to
120 cm, and performs one server-validated downward slam within 300 cm
horizontally, followed by a 1.2-second recovery. Credited defeats queue three
Stone for the authenticated owner through the population-save escrow. Thunder
Core remains disabled pending A8b. Reef Maw uses ten percent of eligible
deep-Ocean wildlife candidates, requires at least 150 cm sampled water depth,
visibly approaches swimming players, and performs one server-validated 50-Stab
bite within 180 cm after a one-second warning. It recovers for 1.2 seconds and
queues three Bone Fragments for its authenticated credited owner. Reef Tooth
remains disabled until its A8b item task completes; the Maw never targets skiff
hulls or attached occupants.
Mireling (75 health, 15 armor, 20 Crush) now occupies half of Mossy Mire's
seeded candidate slots and is accepted only on dry Mossy Mire ground; its
existing melee and optional boss-scroll reward remain unchanged. Death Adder
(125 health, 20 armor, 75 Stab) uses 20% of the Mire slots, a thicker original
coiled snake model, and the server-owned close bite. Its accepted bite applies
the owner-visible five-damage-per-second major poison for ten seconds; weak and
major effects share a tick clock and the strongest dose wins. Credited defeats
queue one baseline Bone Fragment and one guaranteed Venom Sac for the
authenticated owner in the existing population-save escrow; the server commits
the complete bundle together. Antidote remains a separate A8b output task.
Mosquito swarm now uses a 20% common share, and Peat Husk occupies the final
10% hard slot. Its one actor carries six original cosmetic insect
silhouettes; server flight checks dry Mossy Mire ground and blocking geometry,
one swarm reserves each target, and accepted 5-Stab contacts every 0.2 seconds
refresh the existing ten-second weak poison. Server-accepted dodges and visible
active flames repel it; this adds no mosquito loot source. The planned 1% Dusk
Diamond source remains disabled for A9.
Peat Husk uses its reviewed 300-health, 50-armor, 150-Crush profile and the
existing original peat-and-wood procedural silhouette. The server keeps it on
dry Mossy Mire ground, excludes its hard spawn from within 150 m of the player
start, and controls a slow close sweep with a one-second warning, 260 cm reach,
120-degree forward arc, and 1.2-second recovery. Each credited defeat queues
one baseline Amber and makes a server-side 25% roll for one Peat Heart. The
outcome is encoded in the authenticated owner's durable population-save claim,
so retry and reload preserve it; awarded Amber and Peat Heart are committed
together when inventory capacity permits.
Freezing Tundra Wolf (250 health, 40 armor, 100 Slash) occupies the seeded 20%
Wolf slots. The server places each pack in a deterministic, seed-rotated
three-member triangle with a 240 cm radius, requiring every member to stay in
dry, traversable Tundra. The per-key Tundra actor cap fits one pack and counts
each surviving member. Wolves pursue within a 25 m spawn leash, telegraph for
0.5 seconds, visibly lunge up to 180 cm, recheck visibility and the reviewed
180 cm bite reach at impact, then recover for 0.8 seconds. Each defeated
member queues two Bone Fragments and one guaranteed Wolf Pelt for its
authenticated attacker under that member's stable identity. The complete
reward commits together when inventory capacity permits. Wolf Fang and Wolf
Meat remain disabled until their A8b source tasks are complete.
Lynx (150 health, 30 armor, 75 Slash) occupies the next seeded 20% of Tundra
candidate slots and stays on dry, traversable Tundra. The server-owned pounce
shows a half-second warning, closes toward the target, and applies damage only
within the user-reviewed 350 cm impact boundary and clear line of sight. A hit
recovers for 0.8 seconds; a miss takes 1.2 seconds. Credited defeats queue two
Bone Fragments under the authenticated owner's identity in population-save
escrow. Lynx Pelt remains disabled until its A8b source task is complete.
Polar Bear (750 health, 60 armor, 200 Crush) fills the final seeded 10% hard
slot, remains on dry traversable Tundra and stays outside the 150 m generated
start exclusion. Its replicated server-owned warning, committed charge, and
retreat use a 25 m spawn leash and apply at most one hit within 220 cm before
the 1.2-second hard recovery. Credited defeats queue three Bone Fragments under
the authenticated owner's identity in population-save escrow. Polar Bear Pelt
remains disabled until its A8b source task is complete. Live habitat, hard
spawn exclusion, hit rejection, full-inventory retry, reconnect/restart, and
relevant-peer acceptance remain part of combined I4.
Listing a drop does not enable it or certify its A8b/A9 item/icon/source work.
Rat's runtime peer/restart acceptance remains part of combined I4. The existing
world-population save keeps pending reward rows beside its existing harvested
and defeated IDs; the additive row does not change those stable identities.

Run `python Scripts/Validate-CreatureCatalogues.py` to check the manifest,
17 unique IDs, allowed fields, stats, typed attacks, drop references, inline
poison, biome IDs and exclusive gem sources. Unreal/runtime acceptance remains
part of the combined I4 milestone-final gate in `07-development-setup.md`.
