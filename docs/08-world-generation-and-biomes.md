# World generation and biome roadmap

## Direction

Kalmala is a seed-generated, player-directed open wilderness. The world contains no authored gameplay areas, fixed camp zones, prescribed routes, or required quest sequence. Players choose where to travel, build, gather, and take risks.

The world is built from continuous procedural maps, not a visible square grid. Streaming limits and server spatial partitions exist only to manage performance, spawning, and persistence; they must never shape biome boundaries or create gameplay zones.

Kalmala may learn from broad survival-world principles—shared seeds, player-made homes, environmental risk, and landmark-led discovery—but must remain wholly original in its world layout, content, names, visual language, and implementation.

## Goals

1. **A world worth wandering.** Terrain, weather, and natural features should invite curiosity without giving players a required route.
2. **Shelter changes travel.** Different environments alter how players prepare, build, and move through the world.
3. **A shared world.** The same seed and generator revision produce the same world for every player in a session.
4. **Optional discovery.** Resources, wildlife, hazards, and points of interest enrich exploration without becoming a checklist.
5. **Multiplayer-safe scale.** The server owns gameplay state while clients receive only the information needed to render and play nearby world content.

## World seed and generation

The server creates and saves two immutable values with every world:

- `WorldSeed` — unsigned 64-bit value that defines the world.
- `GeneratorRevision` — version of the generation rules used to create it.

Changing either value creates a different base world. Existing saves must never silently reinterpret terrain or object locations after a generator revision changes.

### Four continuous Perlin maps

The generator derives four independent Perlin-noise maps from the world seed. Each map uses its own deterministic sub-seed so that changing one map's tuning does not accidentally reshape the others.

| Map | Controls | Derived uses |
| --- | --- | --- |
| **Elevation** | Height of the terrain | Slope, drainage, lakes, shorelines, cliffs, and mountain forms. |
| **Humidity** | Availability of surface and ground moisture | Wet ground, lakeside conditions, marsh potential, and vegetation support. |
| **Temperature** | Local climate tendency | Snow, frost, rainfall type, warmth pressure, and cold-weather conditions. |
| **Flora** | Vegetation suitability and density | Meadows, forest density, ground cover, and plant population potential. |

At any world position, the generator samples all four maps, normalizes the results, and classifies the biome from their combined values. Biome transitions are continuous blends, not fixed-width borders. Wind, wetness, wildlife, ruins, scroll sites, and harvest nodes are generated later; they are not additional biome maps.

```text
WorldSeed + GeneratorRevision
  -> Perlin maps: Elevation, Humidity, Temperature, Flora
  -> sample at world position
  -> terrain and biome classification
  -> seeded world content
  -> weather, survival, and player-made changes
```

## Biome palette

Development order is not player progression. The seed decides which biomes are nearby; players decide whether and when to enter them.

| Biome | Character | Shelter and travel pressure | Discovery focus | Development order |
| --- | --- | --- | --- | --- |
| **Meadows** | Gentle hills, moderate humidity, open sightlines. | Introduces fire cover, simple timber shelter, and dry storage. | Deer routes, stones, birch stands, and calm camp locations. | First |
| **Shimmering Lakes** | Interlocking lakes, shore fog, and saturated low ground. | Favors bridges, boats, dry stores, and raised shelter. | Fishing waters, small islands, and lake-edge resources. | Second |
| **Elderwood** | Dense canopy, deep shade, and heavy growth. | Rewards marked trails, compact camps, and careful visibility management. | Ancient roots, wildlife dens, and overgrown stone sites. | Third |
| **Mossy Mire** | Wet ground, slow travel, and dry hummock paths. | Rewards raised floors, drainage, and waterproof fuel storage. | Bog iron, causeways, and Mireling scavenging sites. | Fourth |
| **Freezing Tundra** | Bitter wind, sparse cover, and rolling high ground. | Rewards insulated clothing, enclosed roofs, and windbreaks. | Ice-fed springs, exposed shrines, and weather-read routes. | Fifth |
| **Thunder Mountains** | Sheer ridges, thunder squalls, and exposed passes. | Rewards lightning-safe shelter, durable construction, and route planning. | Storm-carved overlooks, mineral seams, and deep cave systems. | Sixth |
| **Ocean** | Open water, currents, waves, and storms. | Rewards seaworthy construction, anchors, and coastal shelters. | Distant islands, sea caves, and rare shoreline materials. | Seventh |

Biomes should differ primarily through environment, travel, and shelter—not a linear increase in enemy strength. Every reachable biome needs a viable lower-risk approach and a riskier shortcut or reward opportunity.

## Seeded world content

Terrain and biomes establish the world; separate seeded systems populate it. No area is assigned a mandatory purpose.

- Wildlife, harvest nodes, and hazards use deterministic server-side spatial seeds and spawn budgets.
- Weather is a server-owned deterministic cycle derived from the immutable world identity and a cycle index; it is not a biome map and never creates weather zones.
- Landmarks and major discoveries are optional points of interest generated by their own seed rules.
- Decorative vegetation, rocks, and ambient detail are cosmetic where possible; gameplay-relevant content is server-owned.
- Player-made changes override generated content through persisted save data.

## Multiplayer, persistence, and performance

- The server generates and persists all gameplay-affecting placements, AI, harvest state, hazards, construction, and survival state.
- Clients may generate cosmetic detail locally, but never decide gameplay placement, loot, damage, or rewards.
- Save only sparse player/world deltas keyed by `WorldSeed`, `GeneratorRevision`, and a server spatial key; never serialize the entire generated base world.
- Server spatial partitions are implementation-defined and invisible to players. They may manage activation and budgets, but may not create square biome borders or authored gameplay areas.
- Use instancing or pooling for non-interactable vegetation and rocks. Promote only nearby interactive content to replicated actors.
- Profile generation time, memory, replicated actor count, save size, and late-join synchronization before increasing content density or streaming distance.

## Delivery roadmap

Build the world in visible, playable layers. Start the next phase only when the preceding phase is repeatable and stable in host/client play.

### 1. Seed and map proof

Generate the four Perlin maps and a developer-only visualization for terrain and biome classification.

**Done when:** the same seed always produces the same maps and biome layout, while a different seed visibly produces a different world.

### 2. First playable generated world

Turn the map output into traversable terrain with a seed-generated player start, Meadows, lakes, trees, and rocks. Do not add a fixed sanctuary, quest route, boss arena, or authored map area.

**Done when:** host and client can travel through the same generated terrain and see the same meaningful natural features.

### 3. Natural population

Add seeded wildlife, harvest nodes, hazards, and ambient detail. Keep gameplay-relevant spawning server-owned and budgeted.

**Done when:** the same seed produces matching gameplay content and consumed or defeated content remains consistent after reconnecting.

### 4. Weather, shelter, and survival

Connect rain, wind, wetness, warmth, fires, and shelter to the generated terrain. Environment should influence preparation and travel without blocking exploration behind a quest.

**Done when:** players naturally choose different routes, camps, and gear because of the terrain and weather they encounter.

### 5. Add biomes one at a time

Add Shimmering Lakes, Elderwood, Mossy Mire, Freezing Tundra, and Thunder Mountains individually. Each biome needs a clear environmental identity, original content, and a reason to build or travel differently.

**Done when:** every added biome is enjoyable on its own, blends naturally with its neighbours, and remains consistent for host and client.

### 6. Ocean and long-distance travel

Add ocean travel, islands, and the systems needed for long-distance movement after land biomes are stable. Tune streaming only from profiling evidence; technical limits must not become gameplay zones.

**Done when:** players can travel between land and ocean without terrain gaps, duplicate content, or host/client disagreement.

## Immediate next step

Implement Phase 1 only: generate and inspect the four seed-derived Perlin maps. Do not add external terrain or procedural-generation plugins without approval.
