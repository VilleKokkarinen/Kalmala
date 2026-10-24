# A9 implementation and acceptance handoff

Implementation and combined I5 verification are complete on `codex/a9-runtime`, including master's accepted A8/I4 gate (`fdda0f39`). The final queue passed all 187 tests; Editor/Game builds, catalogue/icon contracts, applicable peers/restart and current gear renders passed. Normal integration into local master is the final handoff step. Exact commands, repairs and evidence are recorded in `PROGRESS.md`.

## Approved source decisions

On 2026-10-23 the user approved the documented replacement/reward rates and three accepted pick hits with 10% smash resolved before extraction on every hit. They approved population save schema 2, preserving/rejecting incompatible schema-1 files without migration. They approved treasure at existing seeded discovery points in Elderwood, Lakes and Mountains and the missing bounded Frost Salt deposit generator (one possible safe, separated deposit per 60 m key, retaining frostmoss).

The five rock sources and three treasure sources are implemented/enabled. Moonstone is enabled through `MoonstoneSourceAccepted` after integrated I4 acceptance: only validated generated Elderwood Moon Bears can select the fixed 1/50 reward, persisted with the credited owner's ordinary bear payload. Dusk Diamond is enabled through `DuskDiamondSourceAccepted` for validated generated Mossy Mire Mirelings at the approved 1/100 chance. Both use owner-bound persisted payloads; combined I5 verification passed. The independent verification increment is executed as recorded below; it does not close I4 or combined I5.

## Existing scroll-route audit

| Scroll/effect | Existing seeded discovery eligibility | Existing use |
| --- | --- | --- |
| Mending | Meadows, Elderwood; even Lakes descriptor seed | Server traces an injured living ally; incoming recovery respects effective maximum |
| Hearth Shield | Thunder Mountains; odd Lakes descriptor seed | Server-owned finite absorption with existing cooldown/expiry |
| Bear's Vigor | Freezing Tundra | Existing temporary strength and +40 stamina; sums with equipped flat stamina |
| Deer Call | Mossy Mire | Server validates nearby eligible deer and applies existing influence |

`WorldPopulationLayout::GetDiscoveryDefinition` retains these locations. `GameMode::ClaimDiscovery` validates the current descriptor and range, persists the discovery and allowlisted canonical effect before learning it, and rejects repeat claims. `SupportMagicComponent::LearnEffectFromServer` supplies the corresponding ordinary scroll item when missing; that runtime stack is reconstructed from the persisted entitlement rather than saved as a separate item. Player restoration relearns saved canonical effects. The optional existing Mireling boss route still selects one of the same four definitions by world seed modulo four and uses the existing player-scoped boss entitlement. No source, new scroll, trophy or boss key was added by the audit.

This audit is source inspection, not a new live discovery/restart pass. Applicable verification includes `Kalmala.Gameplay.Discovery`, support/status/inventory automation and the existing discovery peer/restart procedures in 07-development-setup.md.

## Historical independent verification and then-remaining gates

The isolated mirror is `C:/Users/Ville/AppData/Local/Temp/a9v`; general evidence is `C:/Users/Ville/AppData/Local/Temp/a9e`. Editor and Game builds passed. Final full queue `auto4.log/r4` passed all 185 tests (179 clean, six with warnings), zero failures/not-run, exit 0. All 148 catalogue icons, nine status icons and 17 creature profiles passed validation. All 1,083 build/runner inputs matched the source by SHA-256; generated/output path audit found a maximum of 210 and no MAX_PATH violations. The installed Launcher engine's documented dedicated-server limitation remains applicable; no unsupported Server target was attempted.

New focused cases are `Kalmala.World.GemstoneContract`, `Kalmala.World.GemstoneGeneration`, `Kalmala.Gameplay.Inventory.AccessorySlots`, `Kalmala.Gameplay.Inventory.A9GearRecipes`, `Kalmala.Gameplay.Status.AccessoryStats`, `Kalmala.Gameplay.Inventory.OpalHelm`, and `Kalmala.Gameplay.Combat.StormcutSword`. All passed in the final full queue. Initial failures exposed new Stormcut/recipe test expectations and an older biome identity assertion that omitted approved replacements; repairs and rerun history are recorded in PROGRESS.md. Opal's real nearby-Forge/Anvil route passed upgrade/payment, station rejection, damage/wear and free repair. Current station attachments reach Forge two; higher authored item tiers retain that existing prerequisite.

`Verify-A9Peer.ps1 -Port 19309 -OutputDirectory C:/Users/Ville/AppData/Local/Temp/a9p6` passed with one listen host and two actual owning remote clients, a canonical seeded Quartz descriptor and a canonical seeded Garnet discovery descriptor. It exercises ordinary owning attack/interaction RPCs, saved partial progress, full-pack rejection, competing claims, owner-only accessory stats, relevant-peer node state and fresh-process claimed-ledger restoration. The fixture provisions equipment/materials, holds players at interaction height while distant terrain streams, and removes duplicate actors for its identities; it does not establish fresh-character progression without grants. Both remote logs contain treasure requests. Shared crafting (`19311`) and discovery (`19313`, `a9d1`) peer runners also passed.

Final equipped-inventory renders `gear720b.png` and `gear150b.png` were inspected at 1280x720/100%/contrast0 and 1024x768/150%/contrast1. The Opal backplate height, accessibility styling and overflowing labels were repaired before those captures. This is commandlet-render evidence, not physical input certification.

The initial static failures were checker defects: Markdown line wrapping, obsolete slider/contrast and label/item-ID expectations, named-switch argument binding and inherited native exit status. Checker-only repairs now pass all five M5 contracts and presentation ownership (seven assets/32 source seams). Negative missing-prose/source fixtures still fail, and the aggregate propagates real child failures; the private source fixture is restored byte-for-byte. Results are `m5-final2.log`, `ownership-final.log` and `m5-stale-exit.log` in the evidence root. No product behavior changed to satisfy these checks.

The inventory peer runner still failed its missing client notification-baseline log (`KalmalaInventory-e6aba8b04c2a4faa9d98ea44e5c6bb9b`), corresponding to the known I4 baseline disposition. That runtime runner is not a pass; retain its disposition and the combined I5 gate.

Keep I4-dependent enemy enablement and the combined I5 gate open until their actual evidence exists. Ordinary material inventory and equipped gear retain their existing session-only limitations; node/treasure claim persistence does not establish general inventory restoration. No A9 cargo implementation is implied: current BACKLOG A9 scope omits the details document's older blocked cargo proposal. Record its factual existing vessel-persistence gap if broader acceptance requires it.

Committed I4 snapshot `09ea50e4` was subsequently integrated into the isolated A9 branch, preserving both handoffs and fixture additions. Combined Editor/Game builds passed; `ia1.log/ir1` passed all 185 tests (178 clean, seven with warnings). The repeated A9 peer/restart case passed at port 19319 in `C:/Users/Ville/AppData/Local/Temp/a9ip1`. This is a tested committed snapshot, not accepted I4 or a merge of A9 into main; the active main worker remains independent.

After that queue, the additional `Kalmala.World.EnemyGemPayload` case compiled and passed separately (`payload-test.log/pr1`). It checks Dusk chance/exclusions, credited-owner and allowed-gem rejection rules, serialization of both enemy-gem payloads, and persisted removal without losing another owner's pending gem. Production activation remains gated. The final combined namespace now includes this additional case and still requires its integrated rerun before I5 closes.

Committed A8b progression snapshot `a33711ec` was subsequently integrated into the isolated A9 branch on 2026-10-24. It adds the main worker's Wolf Pelt Boots, Wolf Stew, Lynx Mantle, Polar Bear Coat, Horn Bow, Thunder Maul and Reef Harpoon acceptance handoffs and focused progression assertions. Both progress histories are retained. All five no-build documentation contracts pass on the merged source; this integration did not run a new Unreal build or automation queue. Main remains independently active, I4 remains open, enemy gem sources remain disabled, and final combined runtime verification and the merge into main remain pending.


## Retained A9 gameplay contract

Master removed the general alpha proposal document during documentation cleanup.
The active A9 contract is retained here so its approved values and exclusions
remain reviewable; this does not reopen excluded cargo scope.

## A9

Cargo, ten gems, special gear and discovery rewards (P2; scope approved 2026-10-19).
Enemy expansion A8a precedes this category. See the
[biome roster](biome-enemies.md); Moonstone requires its accepted Moon Bear.

**A9 review (2026-10-23):** the user approved the documented gem source rates, mutually exclusive resource replacements, three-hit extraction, tool-tier requirements and 10% per-hit smash rule. This authorizes implementation; source prerequisites and I4/I5 acceptance remain open. Isolated parallel A9 implementation is explicitly authorized while another worker finishes A8/I4.

**User-directed A9 data/icon batch (2026-10-22):** for the requested catalogue
and icon work, use the item stats, accessory effects and Forge costs in the
proposal tables below. Record accessory slot/effect fields as catalogue
metadata; equipping and applying those effects remain runtime work. Do not
enable gem acquisition sources as part of this data/icon batch.

After Review A9 and the I4 gate, handle each of the ten raw gems and each
resulting gear item as item-scoped stages: first author its complete catalogue
JSON and register the file in `Content/Data/GameCatalogues.json`; next add and
register its recipe when it is crafted;
then generate the original icon, retain provenance, prepare/import it, add the
runtime mapping and icon validation; finally add or enable its acquisition
source/drop-table entry when it needs one. Raw gems have no crafting recipe;
crafted gear is acquired from its recipe and has no drop-table entry. Treat
existing or proposed source references as inactive until the item's JSON and
icon are complete and its source is accepted. Each stage is a separate
commit-sized task that may be completed in its own short-path worktree. The
Skiff Cargo Chest remains a direct-material buildable, not an inventory item.

- Review travel scope: propose only Skiff Cargo Chest (`SkiffCargoChest`), defer cart/carry-pack progression, encumbrance and hull durability. Hammer placement costs 6 Lightwood + 2 non-bow binding input (ID/cost to review) + 1 Bronze, on one fixed cargo socket per existing skiff; no skiff inventory item or new boat tier.
- Implement the approved cargo chest: 12 shared stack cells, interact within 2 m while skiff speed is at most 100 cm/s, same GUID/quantity/capacity rules as land storage. Save cargo against stable vessel ID; reject detachment/removal with contents, retain contents across validated vessel reload, no spill/duplication on reconnect. In-flight transfer rejects unchanged (blocked on existing vessel-persistence support; report any persistence gap).
- Add exactly ten gems from the table below, each as a separate integrated increment. These are concrete source/rate/recipe/stat proposals for review, not a new alpha release commitment. Seven are ordinary rare gems; three special gems are required by named special gear. No empty-purpose collectibles or universal gem pool.
- All raw gems stack 20, proposed weight 0.1 kg, with no combat/equip/upgrade behavior. Each needs a registered material definition and its own original generated icon. Normal resource nodes keep their existing yields; dedicated gemstone rocks replace a small share of spawn slots rather than adding gems to ore rewards. Wildlife/treasure rules remain separate.
- Treasure means server-owned generated world loot containers, never player storage. Stable treasure loot and sparse resource/enemy reward claims are explicit prerequisites; do not invent new creatures or silently treat an unsupported source as implemented.

| Gem / item ID | Exclusive eligible source | Proposed rarity / successful yield | Required use |
| --- | --- | --- | --- |
| Smoky Quartz (`SmokyQuartz`) | Dedicated Quartz rock replacing Copper slots in Elderwood or Tin slots in Shimmering Lakes | 5% of eligible spawn slots; 1 gem if extracted | Quartz Necklace |
| Moonstone (`Moonstone`) | Credited Moon Bear defeats only; A8a prerequisite | 2% (1/50) | Moonstone Ring |
| Garnet (`Garnet`) | World treasure chests in Elderwood or Shimmering Lakes | 10% (1/10) | Garnet Necklace |
| Green Jade (`GreenJade`) | Dedicated Jade rock replacing Tin slots in Shimmering Lakes only | 2% of eligible spawn slots; 1 gem if extracted | Jade Ring |
| Amethyst (`Amethyst`) | Dedicated Amethyst rock replacing Iron Ore slots in Mossy Mire only | 3% of eligible spawn slots; 1 gem if extracted | Amethyst Necklace |
| Aquamarine (`Aquamarine`) | World treasure chests in Shimmering Lakes only | 5% (1/20) | Aquamarine Ring |
| Tourmaline (`Tourmaline`) | Dedicated Tourmaline rock replacing slate slots in Thunder Mountains only | 2% of eligible spawn slots; 1 gem if extracted | Tourmaline Necklace |
| **Special: Star Opal** (`StarOpal`) | Dedicated Star Opal rock replacing Frost Salt mineral-deposit slots in Freezing Tundra; not frostmoss | 1% of eligible spawn slots; 1 gem if extracted | Opal Helm |
| **Special: Storm Sapphire** (`StormSapphire`) | World treasure chests in Thunder Mountains only | 2% (1/50) | Stormcut Sword |
| **Special: Dusk Diamond** (`DuskDiamond`) | Credited Mireling defeats in Mossy Mire only | 1% (1/100) | Dusk Ring |

**Gemstone-rock generation:** choose the rock type during seeded resource
generation, not after exhausting an ore vein. Retain the existing candidate
budget, suitability and stable identity. Use one mutually exclusive selection
per candidate: Copper 5% Quartz/95% Copper; Tin 5% Quartz/2% Jade/93% Tin;
Iron Ore 3% Amethyst/97% Iron Ore; slate 2% Tourmaline/98% slate; Frost Salt
deposits 1% Star Opal/99% Frost Salt. These replacement rates were approved by the user on 2026-10-23. No extra gemstone spawn on top of the ore
candidate, no ore co-drop and no hidden gem roll from ordinary ore nodes.
Give each rock an original visible mineral inclusion and its own target name.

**Mining rule selected by the user:** every accepted pickaxe hit has a 1/10
(10%) chance to smash the gem. Resolve that roll before extraction, including
the final hit. A smash ends the rock with zero gem reward. If extraction
finishes without a smash, expose exactly one matching gem, with no additional
drop-chance roll. No gem shards, secondary ore reward or retry after a smash.

**Hit tuning approved 2026-10-23:** extraction takes 3 accepted hits per rock;
pick level/damage cannot skip hits or change the 10% risk. This gives 72.9%
overall success (0.9 cubed), not 90%. Quartz/Jade/Tourmaline/Star Opal require
Stone Pick or better; Amethyst requires Bronze Pick or better, matching its
replaced Iron Ore source. Spend one pick condition per accepted hit, including
a smash. Misses, duplicate requests and invalid/broken/wrong-family/out-of-range
tools cannot roll, progress or spend condition. No bare-hand/axe extraction.

The server owns type, hit count, smash results and reward. Persist partial
hit progress, terminal outcome and loose gem/claim state under the stable node
identity; streaming, reconnect or restart cannot reset progress, reroll hits,
revive smashed gems or duplicate extraction. Commit the hit result before
publishing it. A full inventory leaves a successfully extracted gem available
for normal pickup. Show cracking progress and an explicit "Gem shattered"
result on failure. This requires dedicated attack-driven mining and persistence
support; it is not assumed to exist in current interaction-based mining.

Enemy/treasure gems still roll once per credited defeat/chest generation.
Persist claims; opening, retries or reload cannot reroll. Lake treasure may
roll Garnet/Aquamarine independently. Moonstone now belongs only to A8a's
hard Moon Bear, replacing the boar proposal. Check enemy kind and
authoritative Elderwood spawn identity. Resolve once per credited defeat
alongside its listed ordinary loot; no retry/reload reroll. Wolves,
boars, deer and Mirelings cannot drop Moonstone. Dusk Diamond remains exclusive
to Mossy Mire Mirelings. No universal source fallback.
Existing Amber is not counted among the ten gems; no gem conversion or random
crafting outcomes. Existing non-gem resource generation is unchanged except
for the explicitly selected replacement slots.

- Review Necklace/Ring slot support. Equip one item in each named slot using validated GUID/replace/return and full-pack rules; copies cannot stack bonuses. The varied effect table below supersedes the uniform necklace-Armor/ring-stamina proposal. Quartz Necklace and Moonstone Ring retain the earlier +10 Armor/+20 maximum stamina roles; other bonuses are concrete reviewable proposals.
- Add Quartz Necklace (`QuartzNecklace`) with the reviewed craft/equip/stat/icon path (blocked on gem acquisition and accessory support).
- Add Moonstone Ring (`MoonstoneRing`) with the reviewed craft/equip/stat/icon path, replacing the Quartz Ring proposal (blocked on gem acquisition and accessory support).
- Add Garnet Necklace (`GarnetNecklace`) for maximum health (blocked on treasure loot and accessory support).
- Add Jade Ring (`JadeRing`), Amethyst Necklace (`AmethystNecklace`), Aquamarine Ring (`AquamarineRing`) and Tourmaline Necklace (`TourmalineNecklace`), one complete item per increment, using the distinct endurance/support/recovery roles below.
- Add Opal Helm (`OpalHelm`), Stormcut Sword (`StormcutSword`) and Dusk Ring (`DuskRing`) as separate special-gear entries below. They require their specific special gem; no ordinary-gem substitute.
- Audit the four existing Mending/Hearth Shield/Bear's Vigor/Deer Call scrolls against their current authored discovery-to-unlock/use paths. Preserve existing locations/entitlements; propose no new scroll, trophy, creature or boss key for this alpha set. Record missing current routes as defects against the existing contract, not permission to invent a reward source.

| Accessory | Proposed Forge 1 cost for 1 | Server-owned effect while equipped |
| --- | --- | --- |
| Quartz Necklace | 3 Bronze + 1 Smoky Quartz | +10 integer Armor, added once to the existing physical-defense total |
| Moonstone Ring | 2 Bronze + 1 Moonstone | +20 maximum stamina; no direct stamina refill |
| Garnet Necklace | 3 Bronze + 1 Garnet | +20 maximum health |
| Jade Ring | 2 Bronze + 1 Green Jade | `StaminaCostMultiplier = -0.10` for every stamina-consuming action |
| Amethyst Necklace | 3 Bronze + 1 Amethyst | `SupportSpellStaminaCostMultiplier = -0.15` for support spells |
| Aquamarine Ring | 2 Bronze + 1 Aquamarine | `StaminaRegenerationMultiplier = +0.15` |
| Tourmaline Necklace | 3 Bronze + 1 Tourmaline | `HealingReceivedMultiplier = +0.10` for Mending/draught healing |
| Special Dusk Ring | 5 Bronze + 1 Dusk Diamond | +10 maximum health, +10 maximum stamina and +5 integer Armor |

All accessories stack 1 and weigh 1 kg each. No durability, upgrades, repair or
attacks. Accessory Armor remains active while equipped without wear;
use the existing physical-only Armor calculation, not elemental resistance.
Maximum-stat bonuses never increase current health/stamina on equip;
unequip/replacement clamps current values to the new maxima. Repeated equip
cannot heal, refill stamina or accumulate bonuses. Recompute derived stats on
equipment changes; normal regeneration uses the applicable maximum.

Accessory multiplier fields are additive decimal adjustments: zero has no
effect, `+0.10` adds ten percent and `-0.10` subtracts ten percent. Sum equipped
accessory adjustments and active Wet/Steady Meal adjustments, including mixed
positive and negative values, then apply the bounded factor
`clamp(1 + sum, 0.5, 2.0)` once to each authoritative stamina cost. Jade's
global delta applies to every stamina cost routed through the shared
authoritative calculation. Its current callers are sprint, dodge and support
spells; future stamina-consuming actions inherit the adjustment when routed
through that path. The
Amethyst support-spell adjustment is added on top only for Mending, Hearth
Shield, Bear's Vigor and Deer Call; it does not create a cost when the authored
cost is zero or change cooldown, duration, potency or unlocks. Aquamarine's
regeneration delta applies only to active stamina recovery, without changing
recovery delay or conditions. Tourmaline's healing delta applies once to actual
incoming Mending/draught recovery, clamped to maximum health. No passive
healing, food bonus, shield bonus, outgoing-heal multiplier or resurrection is
added. Evaluate modifiers when the action/tick resolves; unequip removes them
from subsequent actions/ticks.

Combine flat maximum-stat/Armor contributions additively. Percentage effects
use additive deltas and are summed once per applicable server-owned base
calculation; never apply one effect twice through equipment and status paths.
Keep calculations finite/bounded and use the existing stat precision without
early integer rounding. Accessory Armor and stamina-cost adjustments are now
supported by the owner-bound equipment path. Maximum health/stamina are recalculated on accepted equipment changes without refilling; Vigor combines with accessory stamina. Regeneration and incoming Mending/draught healing read the current equipped additive deltas once at resolution, with factors clamped to 0.5–2.0.

Acceptance must cover each effect's allowed and excluded actions, injured/full
health, exhausted/full stamina, blocked regeneration, partial healing ticks,
Wet/Steady Meal combinations and repeated equip/replace/unequip. Verify that
removing an item immediately removes its future modifiers, equipment modifiers
do not leak into another owner's derived stats, and rejected equipment requests
change nothing.

Gear remains owner-private and session-only under the existing equipped-gear
persistence limit; no general gear-save system. Add explicit accessory/derived-
stat handling rather than assuming the current durable-armour schema supports
it. Alternative rings/necklaces compete for the same one-per-slot positions;
their bonuses cannot stack within a slot. All ten gems and every resulting
equipment item need original generated icons and applicable manifest-registered
catalogue/recipe/stat records. Authored base spell costs stay unchanged; only
the equipped Amethyst modifier changes the resolved stamina cost.

**Special weapon/armour proposals:** both crafted at Forge level 1, output 1,
stack 1; each gem is consumed during crafting, not socketed or recoverable.

| Item | Crafting materials | Level-one profile | Upgrade base / per-level reward |
| --- | --- | --- | --- |
| Opal Helm | 20 Iron + 5 Deer Hide + 1 Star Opal | Head; 12 Armor, 150 condition, 5 kg | 5 Iron + 2 Deer Hide; +1 Armor, +10 condition |
| Stormcut Sword | 20 Iron + 2 Densewood + 2 Deer Hide + 1 Storm Sapphire | One hand; 70 Slash, 220 condition, 2.5 kg | 5 Iron + 1 Densewood; +4 Slash, +30 condition |

Use existing level cap/cost scaling, Forge upgrades and free station repair.
Helm follows normal durable wearable hit-wear and physical Armor rules; its
name does not grant cold immunity or introduce insulation. Sword uses existing
Basic melee timing/reach, zero extra attack-style damage, one condition per
accepted hit, no gathering or elemental/offensive-magic effect. Author all
required profiles, attack membership, costs/rewards, held/worn presentation and
icons. Missing catalogue-family/derived-stat support is implementation work,
not permission to replace these items with unsupported generic definitions.
