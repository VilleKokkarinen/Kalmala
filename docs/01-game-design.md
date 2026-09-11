# Game design

## Core loop

1. Prepare at camp: eat, repair and craft equipment, rest, and choose a direction or goal.
2. Travel through the open wilderness: gather, hunt, discover lore, and survive weather and hazards.
3. Engage with optional sites: use combat, construction, terrain, and elemental reactions when the player chooses.
4. Return with materials and, on major discoveries, new materials, gear and or magic scrolls.
5. Improve the sanctuary and range farther into stranger parts of the world.

## Vertical-slice player kit

- Movement: walk, sprint, jump, dodge, swim
- Vitality: health, stamina, warmth, hunger (simple and legible).
- Gear: basic clothing and a carry pack. Progression improves warmth, durability, and carry capacity.
- Weapons: Wooden club, sword, bow.
- Tools: axe, pickaxe, hammer, torch.
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

New generated worlds use an independently seeded land/water master map, cropped and rotated by the game seed at the existing 16 km radius. The centre through 350 m contains only Meadows/Ocean. Eligible land can develop Shimmering Lakes beyond 350 m, Elderwood from 750 m, Mossy Mire from 2 km and Freezing Tundra from 4 km according to procedural environmental suitability. Meadows is limited to 4 km. Meadows and Thunder Mountains fill remaining land; beyond 4 km the fallback is Thunder Mountains. These are origin-distance placement rules, not compulsory routes or enemy levels. The full generation and compatibility contract is in `08-world-generation-and-biomes.md`.

Start with a **bounded, server-owned interaction grid** around active gameplay areas. It is not a full world cellular simulation.

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
