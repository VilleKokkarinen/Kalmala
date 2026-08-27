# Game design

## Core loop

1. Prepare at camp: eat, repair, craft equipment, choose verses and supplies.
2. Travel into the wild: gather, hunt, discover lore, survive weather and hazards.
3. Overcome a site: use combat, construction, terrain, and elemental reactions.
4. Return with materials and song-fragments.
5. Improve the sanctuary and unlock safer or stranger expeditions.

## Vertical-slice player kit

- Movement: walk, sprint, jump, dodge, interact, swim.
- Vitality: health, stamina, warmth, hunger (simple and legible).
- Tools: hand axe, mining hammer, torch, simple bow.
- Crafting: campfire, workbench, basic storage, three build pieces (floor, wall, roof).
- Magic: one rune-song focus with three verses: ember, gust, and binding. Verses combine only where explicitly designed and tested.

## World simulation v0

Start with a **bounded, server-owned interaction grid** around active gameplay areas. It is not a full world cellular simulation.

| Property | Initial states | Example effect |
| --- | --- | --- |
| Material | wood, stone, water, metal, vegetation | determines valid interactions |
| Temperature | cold, normal, hot, burning | fire spreads only through flammable targets |
| Wetness | dry, damp, soaked | soaked wood resists ignition; players lose warmth |
| Ward state | unwarded, protected | changes supernatural hazard behavior |

Rules must be deterministic enough for server replication and cheap enough to profile. Visual effects are client-side representations of authoritative state.

## Combat

Combat favors commitment and readable timing over dense combos. Enemies pressure camp preparation and positioning:

- **Mireling:** quick melee scavenger attracted to unprotected food.
- **Iron Wisp:** ranged spirit disrupted by water, wind, or warding.
- **Root-Bear:** slow territorial brute that can damage unreinforced structures.

## Art and audio direction

- Strong silhouettes, carved wood and woven-textile motifs, birch bark, oxidized iron, cold lakes, firelight.
- Palette: winter blue, soot black, birch white, ember orange, lichen green; reserve saturated magical colors for rituals.
- Avoid direct replication of a particular illustrator’s compositions or motifs. Build a distinct, consistent symbol library.
- Soundscape: wind, water, wood, distant singing; original recordings and generated/source-cleared sounds only.

## Accessibility baseline

Subtitles, remappable input, scalable text, adjustable camera shake, color-independent interaction cues, and configurable friendly fire are required from the vertical slice onward.

