# Alpha item coverage proposal

Research and repository audit: 2026-10-19. This is a planning comparison, not
an alpha release declaration or a change to the accepted vertical-slice scope.
The actionable category queue is in [BACKLOG.md](../BACKLOG.md). Existing
roadmap acceptance remains historical; new unchecked tasks do not reopen it.

## Comparison boundary and sources

Use Valheim to identify useful **roles and complete acquisition loops**, not
to copy names, artwork, lore, recipes, stats, biome order, or its whole catalogue.
The proposed alpha baseline is Kalmala's existing Stone/Bronze/Iron span and
current creature roster. It is a recommendation, not a newly approved release
scope. Broader systems below require a scope decision before implementation.

The A0–A10 checklist in [BACKLOG.md](../BACKLOG.md) gives short tasks with
sources and recipes. [Item details](alpha-item-details.md) retain the full
stats, upgrades, behavior, persistence and acceptance criteria by category.
Task status belongs only in the backlog. Keep its summary and the detailed
values consistent when reviewing/editing the proposals.
These new/changed entries are drafts for user review/editing, not accepted
game-design decisions or implementation authorization. Agents must follow the
reviewed entries and report conflicts instead of choosing alternative designs.
Existing-route rows retain their authored yields. Potato Seed acquisition stays
deferred; A7 specifically proposes Potato tuber propagation. A1 supplies the
processing station costs/timing/capacity previously left open. Any further
missing gameplay value must likewise be proposed for review before coding.

Latest recipe direction: Workbench **4 Sinew → 1 Bow String** (`BowString`).
Bow requirements use the dedicated material. Generic String in other proposals
still needs a separately reviewed source; the old sinew-to-String recipe is retired.

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
| Melee weapons and gathering tools | Stone/Bronze/Iron axes, knives, picks, spears and maces: 15 profiles | Existing slash/stab/crush roles are covered; prioritize reachable materials. Swords are a design target without a profile; a wooden club is also a target, but Stone Mace already fills early blunt combat. |
| Ranged and ammunition | One Bow profile and Wooden Arrow; crafting, draw/release and server projectiles | String lacks a normal source. Later add one improved bow and one metal arrow, with explicit ammunition dispatch and payment. |
| Utility tools | Building Hammer plus gathering tools above | No authored carried torch, cultivation tool, hoe or fishing rod. Torch is an existing design target; the other roles need new systems. |
| Armour and shields | Boar Hide Helmet, Tunic, Leggings, Boots, Cape; Wood Shield | No second armour tier or metal shield. Six functioning slots; Necklace/Ring are disabled placeholders. Shield currently supplies passive Armor, not Valheim-style blocking/parrying. |
| Crafting materials | Wood, Densewood, Lightwood, Stone, String, Bone Fragment, two hides, Amber, Frost Salt, Coal; Copper/Tin/Iron ores and Copper/Tin/Bronze/Iron bars | Wood/Densewood, hides, Amber and Frost Salt have source paths. No production recipes for ores/bars/Coal/String; no ordinary Lightwood or Bone Fragment reward found. Check actual generated availability before calling any loop complete. |
| Food | Two raw meats and seven prepared-food definitions; six cooking outputs | Hearth Broth has no recipe. Four root vegetables are ingredients. Food use is inconsistent with current catalogue IDs; repair before adding dish counts. |
| Crops and seeds | Carrot, Potato, Rutabaga, Onion plus four seed definitions | Farming is explicitly unimplemented. Definitions alone cannot supply a renewable garden. |
| Recovery and expedition consumables | No authored potion category | Healing/stamina preparations are candidate additions; hazard resistance needs supported effects and an approved hazard role. Do not require consumables for direct-damage magic. |
| Building/stations | 13 direct-material build recipes; 12 JSON metadata entries plus Campfire's runtime definition | Floor, wall, roof, chest, campfire, Workbench/Tool Rack, Forge/Anvil, Cooking Rack, Cauldron, Grinding Stone, Frying Pan. Missing processing stations and common shelter pieces are proposals, never inventory kits. |
| Travel, accessories and discoveries | Existing two-seat skiff; four support-scroll definitions/discovery effects | Cargo/hauling, jewellery and trophies are separate proposals. Keep support scrolls and discovery intact; no offensive staves, copied boss trophies or mandatory reference-game boss keys. |

JSON accounting: 46 ordinary non-buildable definitions (including materials,
food, crops, wearables, arrows and scrolls), 17 equipment profiles (15 tiered
profiles, Bow, Hammer), and 12 buildable metadata records. This is authored
coverage, not a count of obtainable items or the assembled runtime catalogue.

### Two integration gaps that affect the recommendation

- `KalmalaToolLifecycleContract.cpp::BuildServerSelection` still maps
  `mire-bog-iron` directly to `Iron`. The design document says Iron has no natural
  source. Record that discrepancy and verify generated reachability; do not
  describe Iron as definitely unobtainable or assume Iron Ore is already mined.
- `KalmalaPlayerStatusComponent.cpp::IsKnownFoodItem` accepts Hearth Broth and
  the legacy `RoastedFieldMeat`/`SmokedFieldMeat` IDs. The six current cooking
  outputs use different IDs. The existing single Steady Meal effect is not
  evidence of differentiated health/stamina meals. Reconcile catalogue-to-use
  dispatch and acquisition before planning extra food effects.

## Recommended priority and dependency order

**P0: make the existing set playable.** Audit each material and food from normal
world acquisition to use; close String, Lightwood, Bone Fragment and cooking
gaps. Resolve the ore/bar/Coal chain and Iron discrepancy. A smelter and charcoal
station would require an explicit system/scope decision, not just new JSON.

**P1: candidate alpha breadth after P0.** Torch, sword, one better bow, one metal
arrow, a second armour tier and shield, two forage foods and two recovery
preparations. Use existing materials where practical. Each addition needs its
own complete slice; quantity is not a release gate. Numerical balance, costs,
names and material sources are concrete drafts in the backlog and linked
details for manual review. They are not implementation-agent choices.

**P2: scope-dependent parity candidates.** The backlog specifically proposes
plot-based cultivation, lake-school fishing, six shelter/furniture pieces,
skiff cargo and bronze/gem accessories with distinct Armor, health, stamina,
sprint efficiency, regeneration, healing and support-spell-cost roles. A9
proposes ten gems with dedicated rare gemstone rocks,
Elderwood Bear, Mossy Mire Mireling and treasure drops. Star Opal, Storm Sapphire and Dusk Diamond are
special ingredients for named armour, weapon and jewellery recipes; all ten
have explicit uses, costs and effects in the linked details.
Gemstone rocks replace selected resource slots: each pickaxe hit risks a 10%
smash; successful extraction yields exactly one gem. Spawn rates and a proposed
three-hit extraction remain for review. Ordinary ore no longer has gem bonus drops.
New [A8a enemy roster](biome-enemies.md) precedes A9: fourteen proposed enemies
across the seven biomes, with common/hard roles and a separate acceptance gate.
Elderwood has an easier Woodland Badger; the strengthened Tundra Wolf moves
exclusively to Freezing Tundra alongside Snow Lynx and Tundra Bear.
Mossy Mire includes Mire Mosquito swarms alongside Bog Adder/Peat Husk:
repeated small bites, roll escape and avoidance of lit torches/other fires.
Moonstone moves to the hard Elderwood Bear; aquatic enemies remain scope-gated.
It defers terrain-editing
Hoe, cold-protection clothing, new trophies/scrolls, cart/carry packs and extra
brewing stations. These remain reviewable scope choices, not silent changes to
the accepted vertical slice. Backlog review tasks precede implementation.

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
