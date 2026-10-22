# Game design

## Bow shot contract — updated 2026-10-22

Hammer left-click builds the selected piece; right-click opens the piece menu.
Bow left-click draws while held and releases an arrow on release at any charge.
`DrawDuration` is authored per bow: 1.5 seconds for Bow, 2.0 seconds for
Light Bow, and 3.0 seconds for Horn Bow. The crosshair circle
shrinks from 50 to 4 logical UI units over full draw. Earlier release reduces
arrow velocity and accuracy; Wooden Arrow Pierce damage also follows draw,
while Stone and Bronze Arrows deal their authored fixed 25 and 50 Pierce. The
Horn Bow adds its 100 Pierce weapon bonus once to the selected arrow's damage;
its upgrade adds condition without increasing that bonus.
Full draw clamps. Each
successful persisted shot consumes one selected arrow and one bow condition.
Selection may prioritize an exact inventory stack; otherwise the server selects
the first supported arrow in top-row-first inventory order. The crosshair shows
the selected arrow type and current Pierce damage; a lost/exhausted preference
falls back to that inventory order.
Gravity-driven server projectiles stop at
visibility blockers and can damage wildlife. Light Bow is a separate carried
profile with its own Workbench cost, condition, weight and draw duration; it
shares the server shot path and adds no bow damage bonus. Horn Bow shares that
path and combines its authored weapon bonus with arrow damage. Current shots
use Wooden, Stone, and Bronze Arrows. There is no PvP, arrow recovery or Archery progression; a future
server skill multiplier can shorten effective draw duration through the existing
calculation hook.

## Free melee swings — 2026-10-17

Any usable equipped melee weapon can swing without a nearby target, except
the Building Hammer (construction only) and the Bow, Light Bow, and Horn Bow,
which use draw/release shooting. Empty hands can punch freely. Melee attacks use
the shared 0.18-second windup and 0.36-second recovery with the arm swing animation. Misses
cost no condition and grant nothing. Validated hits use authored weapon damage,
or the existing basic 25 Crush fallback for items without an authored attack.
Depleted items still require repair. Hammer construction remains available, and tree harvesting still
requires the appropriate axe tier. Inventory materials and wearables do not
become new Main Hand equipment through this change.

## Thunder Maul — 2026-10-22

Thunder Maul is a two-handed Forge mace with 100 Crush and 30 Lightning damage.
The Lightning is applied instantly as part of the same accepted server hit.
Wearable physical Armor reduces only its physical damage; target general Armor
then reduces the combined remaining physical and Lightning subtotal once.
The item has no Lightning resistance, chain effect, damage over time, stun or
environmental effect. Its Forge upgrade adds 5 Crush and 50 maximum condition,
leaving Lightning at 30.

## Reef Harpoon — 2026-10-22

Reef Harpoon is a one-handed, 4 kg Forge spear with 150 starting condition and
60 Pierce damage. It uses the shared 220 cm melee reach, windup, and recovery;
the Basic attack is a server-confirmed close strike and is not thrown. The
Forge recipe consumes three Reef Teeth, eight Iron, and four Lightwood. Its
upgrade consumes one Reef Tooth, four Iron, and two Lightwood, adding 5 Pierce
and 30 maximum condition. Reef Harpoon adds no skiff damage.

## Bow crafting and two-handed equipment — 2026-10-15

Bow String (`BowString`) stacks to 20; Wooden Arrows (`WoodenArrow`) and Stone
Arrows (`StoneArrow`) stack to 100 at 0.05 kg each. A credited deer defeat has
one server-owned 50% roll: success grants 1 Sinew,
and failure grants 0.
At a level-one Workbench, craft 1 Bow String from 4 Sinew, a Bow from 5 Wood
and 1 Bow String, a Light Bow from 25 Lightwood + 1 Bow String + 5 Deer Hide,
or 20 Wooden Arrows from 5 normal Wood. Light Bow weighs 1.5 kg, starts at 140
condition, draws fully in 2 seconds, and gains 30 condition per Workbench
upgrade for 3 Lightwood + 1 Bow String. A Workbench also crafts 20 Stone Arrows
from 20 Wood + 5 Stone, and 20 Bronze Arrows from 20 Wood + 1 Bronze bar.
Bronze Arrows carry a fixed 50 Pierce attack record and use the server bow shot
path. The owner may prioritize an exact stack; otherwise the first arrow in
row-major inventory order is selected. The crosshair shows its type and damage.
Bow String is reserved for bows; ammunition is catalogued in `items/arrows.json`.
All bow profiles use both hands. Equipping a bow clears the previous held state and returns
a worn shield to a free pack cell as one validated transaction. Tools retain their
original cells; a full pack rejects shield return unchanged. Equipping a shield
clears the held bow. Clearing a bow alone never requires a free cell.
This user-requested increment covers items,
crafting and wielding; the 2026-10-18 bow increment adds drawing, shooting and arrow consumption.

## Core loop

1. Prepare at camp: eat, repair and craft equipment, rest, and choose a direction or goal.
2. Travel through the open wilderness: gather, hunt, discover lore, and survive weather and hazards.
3. Engage with optional sites: use combat, construction, terrain, and elemental reactions when the player chooses.
4. Return with materials and, on major discoveries, new materials, gear and or magic scrolls.
5. Improve the sanctuary and range farther into stranger parts of the world.

## Player-kit design targets

- Movement: walk, sprint, jump, dodge, swim. Dodge is a grounded server-owned
  forward roll: 25 stamina before active status modifiers, 620 cm/s for 0.35 s,
  with a 1 s cooldown and no invulnerability. Its current defaults are Left Alt
  and Gamepad Face Button Right.
- Vitality: health, stamina, warmth, hunger (simple and legible).
- Gear: basic clothing and a carry pack. Progression improves warmth, durability, and carry capacity.
- Weapons: knife, sword, bow.
- Tools: axe, pickaxe, hammer, torch.
- The Basic Torch is crafted from 2 Wood and 1 Amber, occupies one carried-tool
  slot, and equips in Main Hand. It burns only while held, casts 6 m of local
  light for up to 300 seconds, and refuels from inventory with 1 Wood.
  Uncovered rain extinguishes the flame until it is re-equipped in dry
  conditions. Fuel is server-owned and saved with the carried tool.
- Crafting: campfire, workbench, basic storage, three build pieces (floor, wall, roof).
- Magic: scroll-taught support effects. The initial set is:
**Mending** (heal an ally)
**Hearth Shield** (temporary protective shield)
**Bear's Vigor** (temporary stamina/strength boost)
**Deer Call** (attract nearby deer).
Magic uses no runes, and does not use combinable verses.

### Magic discovery and use

- Scrolls are fixed rewards attached to biome landmarks, rare world discoveries, or boss completion—not crafted consumables or random shop stock.
- Reading a scroll permanently unlocks its effect for the player profile; the server validates discovery, unlock, and activation.
- Effects may use a simple cooldown and stamina cost, tuned later. They must never be required to deal damage or replace normal tools, food, shelter, or combat.
- Deer Call only changes nearby wildlife behavior within a bounded area; it cannot create animals or bypass harvest/loot rules.

## World simulation v0

New generated worlds use an independently seeded land/water master map, cropped and rotated by the game seed at the existing 16 km radius. The centre through 350 m contains only Meadows/Ocean. Eligible land can develop Shimmering Lakes beyond 350 m, Elderwood from 750 m, Mossy Mire from 3 km and Freezing Tundra from 4 km according to procedural environmental suitability. Meadows is limited to 4 km. Meadows and Thunder Mountains fill remaining land; beyond 4 km the fallback is Thunder Mountains. These are origin-distance placement rules, not compulsory routes or enemy levels. The full generation and compatibility contract is in `02-technical-architecture.md`.

M3 starts with a **bounded, server-owned interaction grid** around active gameplay areas. It is not a full world cellular simulation: only cells near a possessed player or lit hearth exist at runtime, and only authoritative nearby changes are presented to a client. Its exact coordinate, activation, persistence, and client-intent rules are in the interaction-grid contract in `02-technical-architecture.md`.

| Property | Initial states | Example effect |
| --- | --- | --- |
| Material | wood, stone, water, metal, vegetation | determines valid interactions |
| Temperature | cold, normal, hot, burning | fire spreads only through flammable targets |
| Wetness | dry, damp, soaked | soaked wood resists ignition; players lose warmth |
| Shelter | exposed, covered | rain and wind affect players, fires, and stored materials differently |

Rules must be deterministic enough for server replication and cheap enough to profile. Visual effects are client-side representations of authoritative state.

## Combat

Combat favors commitment and readable timing over dense combos. Most wildlife is part of the survival loop rather than a constant combat encounter: animals can be observed, avoided, called, hunted, or provoke a defensive response.

- **Mireling:** quick melee scavenger attracted to campsites. It is the vertical slice’s supernatural creature and pressures players to protect their camp supplies.
- **Boar:** territorial wild animal that charges when threatened or when players approach its resting area. It is a close-range danger and a source of meat, hide, and crafting materials.
- **Deer:** wary, herd-based wildlife that flees from noise and nearby combat. It can be hunted for meat and hide; Deer Call can draw nearby deer into a bounded area without creating new animals.

## Art and audio direction

- Strong silhouettes, carved wood and woven-textile motifs, birch bark, oxidized iron, cold lakes, firelight.
- Palette: winter blue, soot black, birch white, ember orange, lichen green; use restrained luminous accents for protective and restorative magic.
- Avoid direct replication of a particular illustrator’s compositions or motifs. Build a distinct, consistent symbol library.
- Soundscape: wind, water, wood, distant singing; original recordings and generated/source-cleared sounds only.

## Accessibility baseline

Subtitles, remappable input, scalable text, adjustable camera shake, color-independent interaction cues, and configurable friendly fire are required from the vertical slice onward.

## M3 wetness and rain override (2026-09-14)

This section supersedes earlier references to continuous player wetness, warmth-derived wetness penalties, fuel wetness extinguishing, or rain damage below. `Wet` is only a server-owned player debuff with a reusable parameter definition: default maximum duration 120 seconds, unroofed-rain trigger 10 uninterrupted seconds, movement multiplier 0.92, and stamina-use multiplier 1.15. Standing in server-confirmed water applies it immediately. Rain applies it only when the server finds no accepted roof above the pawn. Reapplication cannot exceed 120 seconds; a nearby lit campfire with nonzero authoritative heat removes it. Surface moisture remains a grid material/fire input and is never a second player wetness system.

Roofs are rain-immune. Unroofed floors, walls, workbenches, and storage take slow server-owned rain wear, clamped at 50% health; an accepted overhead roof prevents that wear. A fuelled campfire is normal in dry weather. Rain without a roof makes it `Smouldering`, with zero heat; roof protection automatically returns it to `Lit`. Clients submit no wetness, duration, roof, health, weather, fire, or relight state.


## Implemented gameplay and current limits

See [Game features](game-features.md) for a short inventory of additions. The player kit above describes design targets; it is not proof that every listed weapon, equipment slot, hunger system, or action is implemented.

The implemented survival loop includes gathering, paid camp construction, weather/shelter recovery, six cooking recipes, free item repair, Workbench/Forge axe progression, optional creature encounters and discoveries, a two-seat ocean skiff, local maps, and owner-private inventory/progression feedback. Four support effects remain non-damaging and scroll-learned. Exact transactions, privacy, persistence, and current overrides are consolidated in [Technical architecture](02-technical-architecture.md#consolidated-system-contracts).

The current player inventory is one 10×4 grid with its first row assigned to hotbar numbers 1–0. Six wearable slots have owner-private, session-only server equip/unequip transactions, and the Inventory popup displays equipped names with owner-facing equip, replace, and return actions. Wearable defense is one integer Armor value. Sum equipped, non-depleted Armor and apply it once to combined Slash, Crush, Pierce and Stab damage, clamped to zero. Lightning bypasses wearable Armor but remains subject to general Armor; Fire, Water, Holy and Poison bypass both. Each accepted wildlife hit spends one condition on every equipped wearable, including hits fully absorbed by defenses or support shields; depleted gear supplies no defense. Main Hand references the selected item in its original cell; its blue highlight follows the exact equipped copy. Pressing its number again unequips it without moving it. Encumbrance is not implemented. Ordinary materials, meals, cell positions, skill XP, weather, active effects, and equipped gear are not generally persisted. Saved tool state, discovered entitlements, construction, storage, and approved sparse world/player facts have separate validated owners.

As of 2026-10-15, the six wearable tiles display owner-only equipped item names and let the owner equip, replace, or return wearables through server-validated GUID transactions. Equipped wearable defenses now contribute to server-resolved wildlife damage. Main Hand equips, switches or clears a physical weapon/tool without moving its numbered grid cell. Equipped names include remaining/maximum condition; depleted wearables provide no defense.

Root vegetables and their seeds are catalogue definitions; farming is not implemented. Coal has no natural gathering source; placed Charcoal Kilns turn Wood into Coal. Generated Elderwood copper-rock and dry Shimmering Lakes tin-shore-gravel nodes yield three matching ore each to Stone Picks or better. Generated Mossy Mire bog-iron nodes yield three Iron Ore per exhausted node to Bronze Picks or better; the Smelter converts one Iron Ore and one Coal into one Iron bar. Stone Picks mine through Bronze-grade resources, while Bronze and Iron Picks currently mine through Iron. The fresh-start Stone access gap and the user-directed Lightwood Iron Axe acquisition and upgrade costs are recorded in `alpha-items.md`; Meadow aspen trees now provide four Lightwood on final bundle pickup with Stone Axe or better; the deterministic forest layout retains the birch Wood route. Storm-rated shelter, insulated clothing/equipment rules, and vessel cargo/hull durability remain outside the accepted scope.

M0–M13 have recorded acceptance with limitations; M13's integrated-main scripted/offscreen gate passed on 2026-10-11. M14's separate isolated acceptance does not integrate its source into main. Settings candidates in the architecture and roadmap are requested scope, not implemented features; consult the live backlog before describing them as available.

## Typed weapon damage and entity defenses

Weapons define additive damage in physical Slash, Stab, Crush and Pierce channels and elemental Fire, Water, Holy, Lightning and Poison channels. Each weapon attack style adds its bonuses to the weapon level bonuses. Every production gameplay entity has a general flat Armor amount plus flat reductions for each channel. Negative per-type resistance defines a flat weakness (for example Slash −5 adds 5 to a hit containing slash). Inactive channels add no damage, even when their resistance is negative. Each active channel is reduced and clamped independently. Wearable Armor reduces the physical subtotal once; general Armor then reduces the combined remaining physical and Lightning damage once. Fire, Water, Holy and Poison bypass both armor values. Default defenses are zero pending balance authoring. This damage model does not introduce offensive magic, new attacks or destructibility.

## Attack-driven resource trees (2026-10-13)

Wood-resource nodes (meadows-birch-bark, elderwood-resinwood, meadows-birch-trunk, elderwood-ironheart-trunk) now use the equipped axe Basic attack instead of interaction harvesting. Standing tree health is 100; the accepted felling hit starts a server-timed 1.2-second Falling phase with fresh trunk health of 60. Falling rejects further hits and pickup. A server timer then exposes the settled Trunk phase; breaking that trunk exposes a wood bundle for interaction pickup, yielding Wood for Meadow trees (Stone Axe or better) and Densewood for Elderwood trees (Iron Axe), with the existing source yield quantities. A hit cannot carry excess damage into the next stage. The same replicated source actor represents each stage. The replicated fall start uses synchronized server time for clamped elapsed-progress reconstruction, including newly relevant peers; clients cannot settle or damage the tree. The original procedural trunk and canopy hinge towards the tree's local forward direction using that synchronized progress. At settlement the canopy is removed and the full-size tapered timber rests horizontally at the original root, with exposed cut faces. The mesh alone moves; the authoritative actor transform and persistent identity stay fixed. Tick runs only during Falling. Settled logs disable the root sphere, use the timber mesh for targeting, and validate server axe reach against the timber surface rather than the identity root. Breaking the log places the original two-piece wood bundle at its midpoint on the root plane. The bundle mesh supplies visibility collision and server physical pickup range uses that same reconstructed anchor; the identity-root sphere stays disabled. Rendered acceptance remains pending.

The server selects and rechecks the blocking target after windup, applies validated equipped-axe damage through tree resistances, enforces MaterialTier and carried condition, and persists one durability point per accepted hit before tree mutation. Trees do not use harvesting skill gates or client tool/action values. A low melee sweep reaches fallen trunks. Pickup validates the complete grant and player persistence before committing existing sparse depletion callbacks; rejected/full-pack/duplicate pickups retain the drop without reward. Tree health/stage replicate to relevant peers, including late joins. Health, trunks and uncollected drops are session state and reset on actor regeneration/restart; no save schema was changed. Existing second-wave source depletion retains its session policy.

Generated Meadow/Elderwood trees now participate in this same axe-driven lifecycle as patch-owned harvest actors, with stable seed/patch/candidate identities and matching cosmetic suppression. Successful pickup records sparse depletion; eviction and restart restore uncollected candidates as standing trees. Focused stage, exact-ID peer delivery/pickup, isolated patch lifecycle and real-slot process-restart checks have passed; see the [current forest acceptance summary](07-development-setup.md#generated-forest-current-acceptance-2026-10-17) for commands and coverage limits. Settled-log targeting/range and midpoint bundle placement are implemented with focused regression coverage; the defined final tree-feature gates and evidence audit passed, including live collected-ID streaming/reactivation and real-slot process restart. Rendered usability, Falling-phase late join, adverse networking, live peer process restart and full-project/package verification remain outside this acceptance.

Crafting/acquisition and item upgrades have no player-level, skill-level or skill-unlock eligibility gate. Their server transactions and owner requirement/blocker text use the station context/level, materials, capacity and current item/predecessor level only. Skill XP awards from accepted ordinary crafting/cooking remain rewards, not prerequisites. Equipment progression no longer reads the character skill component; its legacy pure-function skill-ledger argument is ignored for caller compatibility. Station-family and existing level matching, sight/range, payment and authority checks remain. The station progression regression now includes an uninitialized skill ledger alongside low-level and missing ledgers; Forge presentation verification requires station-only wording. Build/automation/UI runtime verification remains deferred.
