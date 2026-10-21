# Alpha item coverage proposal

Research and repository audit: 2026-10-19. This is a planning comparison for
the user-approved P0/P1/P2 alpha scope, not a release declaration. The
actionable category queue is in [BACKLOG.md](../BACKLOG.md). Existing roadmap
acceptance remains historical; new tasks do not reopen it.

## Comparison boundary and sources

Use Valheim to identify useful **roles and complete acquisition loops**, not
to copy names, artwork, lore, recipes, stats, biome order, or its whole catalogue.
The approved alpha plan builds on Kalmala's existing Stone/Bronze/Iron span and
extends the creature roster. The user included all queued P0/P1/P2 systems on
2026-10-19; explicit exclusions below remain deferred.

The A0–A10 checklist in [BACKLOG.md](../BACKLOG.md) gives short tasks with
sources and recipes. [Item details](alpha-item-details.md) retain the full
stats, upgrades, behavior, persistence and acceptance criteria by category.
Task status belongs only in the backlog. Keep its summary and the detailed
values consistent when reviewing/editing the proposals.
The user approved the queued scope on 2026-10-19 and resolved the documented
cross-file conflicts. Category-specific tasks still review their detailed
contracts before implementation; agents must follow those entries and report
conflicts instead of choosing alternative designs.
Existing-route rows retain their authored yields. Potato Seed acquisition stays
deferred; A7 specifically proposes Potato tuber propagation. A1 supplies the
processing station costs/timing/capacity previously left open. Any further
missing gameplay value must likewise be proposed for review before coding.

Reviewed bow-string recipe: Workbench **4 Sinew → 1 Bow String** (`BowString`),
reserved for bows. Sinew is credited from deer at a server-rolled 50% chance
for exactly one. The `WoodenArrow` item now lives in the sibling `items/arrows.json`
category. The existing A0 source table is canonical for vegetable biomes
and yields; A3 uses Light Bow; Dusk Diamond uses a 1% Mireling drop and the
Dusk Ring uses its detailed A9 Bronze cost.

Sources checked online on 2026-10-19:

- [Weapons](https://valheim.fandom.com/wiki/Weapons): melee families, shields,
  ranged weapons/ammunition, and magic. Swords and heavier two-handed choices
  expose missing roles; ranged magic is incompatible with Kalmala's pillars.
- [Armour](https://valheim.fandom.com/wiki/Armor): multiple material tiers,
  including bronze and iron chest armour. Kalmala has one hide set today.
- [Item IDs](https://valheim.fandom.com/wiki/Item_IDs) and
  [early axes/tool navigation](https://valheim.fandom.com/wiki/Early_axes):
  hoe, cultivator, fishing equipment and lighting illustrate utility roles.
- [Food](https://valheim.fandom.com/wiki/Food) and
  [materials](https://valheim.fandom.com/wiki/Materials): foraged ingredients,
  cultivated food, prepared dishes, and raw/refined crafting inputs.
- [Mead](https://valheim.fandom.com/wiki/Mead),
  [minor healing mead](https://valheim.fandom.com/wiki/Minor_healing_mead), and
  [fermenter](https://valheim.fandom.com/wiki/Fermenter): recovery/preparation
  consumables have production and cooldown systems behind them.
- [Wood](https://valheim.fandom.com/wiki/Wood): wood-to-coal production and
  ore processing connect gathering to gear rather than leaving ingredients idle.

Access limitation: these community-wiki facts were available through indexed
search excerpts; direct article retrieval failed. Independent wiki endpoints
were also inaccessible. This is category research, not a verified exhaustive
or version-pinned Valheim item list. No precise Valheim totals or tuning are
used. All alpha priorities and proposed Kalmala additions are our inference.

## Current Kalmala inventory

Audited the registered JSON files in `Content/Data/GameCatalogues.json`, their
recipes and upgrade files, the loader, resource rewards, wildlife rewards, and
food status dispatch. Definitions do not establish obtainability or usability.

| Category | Authored current coverage | Gap / recommended direction |
| --- | --- | --- |
| Melee weapons and gathering tools | Stone/Bronze/Iron axes, knives, picks, spears and maces: 15 profiles | A2 retains Bronze Sword and Iron Sword as one-hand Basic melee additions, plus an optional Wood-only Wooden Club. The club remains below the starter Stone Mace's authored Crush damage and adds no gathering role. Use Deer Hide for the sword binding cost; Bow String stays bow-only. |
| Ranged and ammunition | Bow and Light Bow profiles plus Wooden, Stone and Bronze Arrows; crafting, draw/release and server projectiles | Bow String is reachable through deer Sinew and Workbench crafting. A3 routes all three arrow types through selected-stack preference or row-major inventory fallback, server-paid shots, and crosshair type/damage display. |
| Utility tools | Building Hammer plus gathering tools above | No authored carried torch, cultivation tool, hoe or fishing rod. Torch is an existing design target; the other roles need new systems. |
| Armour and shields | Boar Hide Helmet, Tunic, Leggings, Boots, Cape; Bronze Helmet and Tunic; Wood Shield | Bronze Helmet and Tunic are Forge-made metal pieces; Bronze Leggings and Shield remain queued. Six functioning slots; Necklace/Ring are disabled placeholders. Shield currently supplies passive Armor, not Valheim-style blocking/parrying. |
| Crafting materials | Wood, Densewood, Lightwood, Stone, Sinew, Bow String, Bone Fragment, two hides, Amber, Frost Salt, Coal; Copper/Tin/Iron ores and Copper/Tin/Bronze/Iron bars | Deer Sinew feeds the bow-only Bow String recipe; credited boar/deer defeats now grant Bone Fragment for the cape recipe and upgrade. A1 now includes Wood → Coal at the Kiln, Copper/Tin/Iron Ore + Coal → matching bars at the Smelter, and Copper + Tin → Bronze at the Forge. Bog Iron Iron Ore acquisition remains pending; verify generated availability before calling the whole metal loop complete. |
| Food | Two raw meats and seven prepared-food definitions; seven cooking outputs | Hearth Broth now has its reviewed Cauldron recipe. All seven prepared foods use the existing Steady Meal effect. Four root vegetables are ingredients; planting remains deferred to A7. |
| Crops and seeds | Carrot, Potato, Rutabaga, Onion plus four seed definitions | A0 supplies 2 Carrots + 1 Carrot Seed and 2 Potatoes per Meadow patch, 2 Rutabagas + 1 Rutabaga Seed per dry Shimmering Lakes bank patch, and 2 Onions + 1 Onion Seed per dry Elderwood patch. Potato Seed acquisition and all seed use remain deferred to A7. Farming is otherwise unimplemented. |
| Recovery and expedition consumables | No authored potion category | Healing/stamina preparations are candidate additions; hazard resistance needs supported effects and an approved hazard role. Do not require consumables for direct-damage magic. |
| Building/stations | 15 direct-material build recipes; 14 JSON metadata entries plus Campfire's runtime definition | Floor, wall, roof, chest, campfire, Workbench/Tool Rack, Forge/Anvil, Cooking Rack, Cauldron, Grinding Stone, Frying Pan, Charcoal Kiln and Smelter. Kiln and Smelter processing state is saved with 30-active-second jobs; common shelter pieces remain proposals, never inventory kits. |
| Travel, accessories and discoveries | Existing two-seat skiff; four support-scroll definitions/discovery effects | Cargo/hauling, jewellery and trophies are separate proposals. Keep support scrolls and discovery intact; no offensive staves, copied boss trophies or mandatory reference-game boss keys. |

JSON accounting: 48 ordinary non-buildable definitions (including materials,
food, crops, wearables, arrows and scrolls), 17 equipment profiles (15 tiered
profiles, Bow, Hammer), and 14 buildable metadata records. This is authored
coverage, not a count of obtainable items or the assembled runtime catalogue.

### A0 source and starter-progression audit (2026-10-19)

- Fresh characters receive six level-one carried tools: Construction Hammer,
  Stone Axe, Stone Pick, Stone Knife, Stone Spear, and Stone Mace. The hammer is
  active; character startup does not grant material stacks. The protected center
  through 350 m is Meadows/Ocean. Meadow birch sources provide Wood. In addition
  to the existing Thunder Mountains slate vein and optional Three-Run Rillstone
  reward, the user-directed source update adds a deterministic dry Stone outcrop
  opportunity to every land biome, including Meadows. It adds at most one per
  biome without an existing slate vein in a 6 km spatial key when a safe,
  separated candidate is found. A particular key is not guaranteed to contain
  one. Meadow's
  `meadows-hidden-stone` landmark still records discovery but grants no
  material. This broadens Stone availability beyond the mountains without
  creating a starter material grant or promising an exact source at the spawn
  point. The approved A0 detail row's "existing starter acquisition" remains
  unsupported by current startup/source code.
- The generated biome contract maps Mossy Mire harvest candidates to
  `mire-bog-iron`. `BuildServerSelection` maps that source to 1 `Iron` per node
  with a Bronze-tier mining minimum. The current Forge `BronzePick` acquisition
  costs 4 Wood + 4 Stone. The user-directed A1 targets are Stone Pick for 10
  Wood + 5 Stone, Bronze Pick for 10 Wood + 10 Bronze, and Iron Pick for 10
  Wood + 10 Iron. Thus the planned 3-IronOre-per-node replacement remains
  Bronze-Pick-gated and now requires Bronze production. Keep the current direct
  Iron reward and recipe until that A1 route is usable. The design sentence
  claiming Iron has no natural source was stale. `IronOre` is catalogued but
  has no current source or refining recipe; A1 implementation remains pending.
- The user-directed Forge `IronAxe` acquisition now costs 4 Iron + 3 Lightwood,
  resolving the cycle with the only current Densewood source: Elderwood
  ironheart trees gated by the Iron Axe tier. Its level-one upgrade also costs
  4 Iron + 3 Lightwood. Iron Knife, Spear, and Mace acquisitions still require
  Densewood, which becomes harvestable after crafting the axe; Iron Pick costs
  4 Iron, 2 Wood, and 2 Stone. Meadow Aspen trees now provide four Lightwood on final bundle pickup with a Stone Axe or better.
- Other current normal routes include Wood from generated Meadow birch/bark,
  Lakes reeds, and Elderwood resinwood; meat/hides from server-confirmed
  boar/deer rewards; Amber from Mossy Mire; Frost Salt from Tundra; and the
  implemented credited-deer Sinew roll feeding the Workbench Bow String recipe,
  one Bone Fragment from each credited boar/deer defeat, and natural A0 sources
  for Carrot, Potato, Rutabaga, and Onion. Their recipes are reachable without
  farming; seed use remains deferred to A7. Current acquisition gaps remain for
  Coal and IronOre; three Copper Ore per eligible Elderwood node and three Tin
  Ore per eligible dry Lakes shoreline node are now sourced to Stone Pick or
  better. Production for Copper/Tin/Bronze/Iron and Coal remains pending A1
  implementation.
- The current spawn layout derives harvest identities from the classified biome
  and material rewards/tool gates from server contracts. The initial source
  audit was static; the A0 final gate now passes deterministic placement and
  source-transaction automation for representative Meadow, Lakes shoreline,
  and Elderwood samples. A broad generated-seed sweep and fresh-player
  walkthrough remain unverified.

### Other integration gap that affects the recommendation

- `KalmalaPlayerStatusComponent.cpp::IsKnownFoodItem` accepts all seven
  current cooking-output IDs; retired food IDs are rejected.
  Cooking output payment and one-serving consumption use the existing
  server-owned transaction, and all seven prepared foods share the single
  Steady Meal effect. This does not add differentiated health/stamina meals.

## Recommended priority and dependency order

**P0: make the existing set playable.** Audit each material and food from normal
world acquisition to use; close Lightwood and cooking gaps. Resolve the
ore/bar/Coal chain and Iron discrepancy. The user approved
the A1 smelter and charcoal-station scope; A1 review still confirms their
processing contract.

**P1: alpha breadth after P0.** Torch, sword, one better bow, one metal
arrow, a second armour tier and shield, two forage foods and two recovery
preparations. Use existing materials where practical. Each addition needs its
own complete slice; quantity is not a release gate. Numerical balance, costs,
names and material sources remain concrete task-level proposals in the backlog
and linked details; category reviews still apply before implementation.

**P2: approved new systems.** The backlog specifically proposes
plot-based cultivation, lake-school fishing, six shelter/furniture pieces,
skiff cargo and bronze/gem accessories with distinct Armor, health, stamina,
sprint efficiency, regeneration, healing and support-spell-cost roles. A9
proposes ten gems with dedicated rare gemstone rocks,
Moon Bear, Mossy Mire Mireling and treasure drops. Star Opal, Storm Sapphire and Dusk Diamond are
special ingredients for named armour, weapon and jewellery recipes; all ten
have explicit uses, costs and effects in the linked details.
Gemstone rocks replace selected resource slots: each pickaxe hit risks a 10%
smash; successful extraction yields exactly one gem. Spawn rates and a proposed
three-hit extraction remain task-level rules for review. Ordinary ore no longer
has gem bonus drops.
New [A8a enemy roster](biome-enemies.md) precedes A9: fifteen proposed enemies
across seven land biomes and the Ocean, with common/hard roles and a separate
acceptance gate.
Elderwood has an easier Badger; the strengthened Wolf moves
exclusively to Freezing Tundra alongside Lynx and Polar Bear.
Mossy Mire includes Mosquito swarms alongside Death Adder/Peat Husk:
repeated small bites, roll escape and avoidance of lit torches/other fires.
Moonstone moves to the hard Moon Bear. The approved A8a/A8b scope includes
Ocean Reef Maw, Reef Tooth and Reef Harpoon; verify the combined enemy/item
milestone after A8b and before A9. It explicitly defers terrain-editing Hoe,
cold-protection clothing, new trophies/scrolls, cart/carry packs and extra
brewing stations. These remain outside the approved A0–A9 queue; category
review tasks still precede implementation.

**Deferred beyond this proposal:** more metals beyond Iron, every-biome gear
sets, crossbows/bolts, polearms/heavy two-handed/fist families, bombs, numerous
boat tiers, teleportation, livestock, brewing trees/feasts, currencies/traders,
and ornamental item volume. These are reference gaps, not alpha requirements.
Offensive magic/runes and portable construction objects remain excluded by
established Kalmala rules. Do not revive the retired Reed Fibre identity.

## Completion and acceptance contract

Every selected item must have a normal non-debug acquisition path, a reason to
use it, original visuals, and applicable catalogue/recipe/equipment/upgrade/reward/
attack records registered in the manifest. Explicitly record non-applicable
records. New IDs may require extending compiled identity/tool-family dispatch;
JSON alone is insufficient. Generate and retain an original icon, prepare/import
it, update runtime mappings, and validate it through `docs/material-icons.md`
and the relevant provenance workflow. No placeholder-icon completion.

Placed objects belong to Buildables and need hammer placement, direct material
payment and placement support; never grant a portable construction item.
Server validation owns gathering, production, item GUIDs, quantities, targets,
consumption, damage, cooldowns and equipment state. Check full-pack rejection,
duplicate/invalid requests and peer privacy. Define persistence/restart behavior
against current development contracts without adding save migrations.

Normal increments use lightweight targeted checks. At the final implementation
increment of an approved milestone, run its required full verification under
`docs/07-development-setup.md`, including relevant catalogue/icon, build,
automation, host/client and persistence checks. Fresh-start progression must
reach all selected items without debug grants. This research did not run builds,
gameplay, automation, icon generation or runtime progression validation.
