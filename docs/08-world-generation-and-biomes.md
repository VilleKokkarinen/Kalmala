# World generation and biome roadmap

## Direction

Kalmala is a seed-generated, player-directed open wilderness. The world contains no authored gameplay areas, fixed camp zones, prescribed routes, or required quest sequence. Players choose where to travel, build, gather, and take risks.

The world is built from continuous procedural maps, not a visible square grid. Streaming limits and server spatial partitions exist only to manage performance, spawning, and persistence; they must never shape biome boundaries or create gameplay zones.

Kalmala may learn from broad survival-world principlesâ€”shared seeds, player-made homes, environmental risk, and landmark-led discoveryâ€”but must remain wholly original in its world layout, content, names, visual language, and implementation.

## Goals

1. **A world worth wandering.** Terrain, weather, and natural features should invite curiosity without giving players a required route.
2. **Shelter changes travel.** Different environments alter how players prepare, build, and move through the world.
3. **A shared world.** The same seed produce the same world for every player in a session.
4. **Optional discovery.** Resources, wildlife, hazards, and points of interest enrich exploration without becoming a checklist.
5. **Multiplayer-safe scale.** The server owns gameplay state while clients receive only the information needed to render and play nearby world content.

## World seed and generation

The server owns one immutable `WorldSeed` (unsigned 64-bit). Every session runs the current generator. Generation compatibility versions and their command-line selectors are removed. Changing production tuning can change existing seed layouts during development; old version-specific save slots are not loaded or deleted. Current sparse saves and local maps use the seed as their world identity.

### Master map and environmental fields

First generate a continuous Perlin **land/water master map with its own seed**, independent of the game seed. The 128 km square atlas uses the constant `FKalmalaMasterMap::MasterSeed = 0x4b616c6d616c6137`, a 3 km main wavelength and a second 1.29 km octave (75%/25% amplitudes). Positive noise is land; zero or negative noise is ocean. It contains no biome identities, authored islands, or gameplay placements.

The game seed select an atlas crop centre and an arbitrary rotation. The crop retains the current **16 km radius / 32 km diameter** playable circle. The centre is selected from up to 128 seeded candidates: use the first with land signal above 0.10, or the strongest candidate if none qualifies. This favours inland terrain for the start without painting land into the atlas. The entire rotated circle fits inside the master map. World zero is the crop centre; rotation preserves distance and scale. No baked map image is required.

The existing four independently seeded game fields remain:

| Map | Controls |
| --- | --- |
| **Elevation** | Inland relief from the existing Perlin noise, constrained to the master's land/water sign and tapered continuously to the coast. |
| **Humidity** | Surface and ground moisture, wetland suitability, vegetation support. |
| **Temperature** | Climate, snow, frost, warmth pressure, cold-biome suitability. |
| **Flora** | Local vegetation density, undergrowth and clearings; it does not fragment broad biome identity. |

Changing the game seed changes the crop, rotation and environmental fields, but not the underlying master atlas. Master tuning and its seed are compiled developer settings, never client-controlled. All peers must run the same build.

### Land biome placement and origin distance

On land, the existing regional Perlin signals and environmental suitability select **Shimmering Lakes, Elderwood, Mossy Mire and Freezing Tundra**. **Meadows and Thunder Mountains are the default land biomes** wherever no special biome wins. Distance is measured from game-world XY `(0, 0)`, not the player start or master-map origin.

| Rule / biome | Distance from world zero |
| --- | --- |
| **Meadows or Ocean only** | **0â€“0.35 km inclusive** |
| **Meadows** | **At most 4 km** |
| **Shimmering Lakes** | **0.35–3 km**, with the inclusive starter rule taking precedence |
| **Elderwood** | **At least 0.75 km** |
| **Mossy Mire** | **3–16 km** |
| **Freezing Tundra** | **At least 4 km** |
| **Thunder Mountains** | Default elevated land outside the starter zone; default remaining land beyond 4 km |

Minimum-distance weights rise smoothly over 50 m on the eligible side, never leaking into excluded radii. The lowland Meadows fallback fades into Mountains over the last 50 m before 4 km, reaching zero at 4 km. Elevation also favours the mountain fallback nearer the centre, with no mountain influence at or inside 0.35 km. A winning special biome takes precedence over the fallback, including on high terrain. Remaining land beyond 4 km is Thunder Mountains even where source relief is low. These restrictions replace the earlier Gaussian distance preferences; they are not enemy levels or travel gates.

Shimmering Lakes and Mossy Mire share the same regional noise, humidity, elevation and temperature suitability, terrain relief, and analytic basin parameters. Their only generation difference is the distance range: Lakes 0.35–3 km, Mire 3–16 km. Both ends ramp over 50 m inside the allowed range; their weight is zero at the exact endpoints. Biome labels do not require standing water. Qualified bowls shape terrain and water within either wetland without overriding biome weights or distance limits.

Shared suitability defaults: humidity rises from 0.48 to full strength at 0.65, elevation falls from full strength at 0.43 to zero at 0.58, and temperature rises from 0.18 to full strength at 0.32. `FKalmalaRegionalTuning` and the preview parameter file expose these same six settings. Rivers remain enabled; small streams are disabled. Inland water can occur in a land biome without making it Ocean. Islands come from the master crop. The player-start resolver searches dry Meadows within 320 m of zero.

Terrain, collision, sea water, inland water, minimap and expanded map consume the same generator. Regional weights, biome identity and terrain remain computed functions. Only a small crop transform is memoized per worker alongside the existing bounded hydrology spline cache; no authoritative biome raster is stored or replicated. Weather, wildlife, ruins, discoveries and harvest nodes are derived later.

```text
MasterSeed -> continuous land/water atlas
WorldSeed -> crop centre and rotation -> base land/water mask
WorldSeed -> Elevation, Humidity, Temperature, Flora
  -> master-constrained terrain + regional signals + distance eligibility
  -> special land biomes; Meadows/Mountains fallback; Ocean from master water
  -> seeded content, weather, survival and sparse player changes
```

## Finite world

The playable radius is always 16 km. There is one generation implementation and no alternate stream-debug world identity.

Terrain and water triangles retain the existing radial clipping. Exterior patches, population and discoveries are rejected; decoration keeps a 10 m edge margin and new hearth/construction placement keeps 3 m clearance. Character Movement enforces the radius minus capsule clearance on authority and owner prediction while preserving tangential/vertical movement. No edge wall, actor budget, asset or authority change is introduced.

### Full-world debug map

![Seed 418 master atlas, rotated crop and final biome placement](master-map-preview.png)

The retained image is a historical layout illustration; regenerate the PNGs below for current tuning. Gold outlines the selected crop in the master atlas; the second panel is the rotated land/water base and the third applies biomes. Small lakes/channels can be subpixel at this 32 km view.

M opens centred on world zero, fitted with 4% margin so the entire circle is visible. The local console settings `kalmala.Map.RevealAll 1` and `kalmala.Map.FitWorldOnOpen 1` are enabled by default for current development. Set either to `0` to restore exploration fog or player-centred opening respectively. Reveal affects terrain/water presentation only: it never records exploration, reveals server population, changes co-op privacy or changes ping range. Personal exploration continues to accumulate normally. R still recentres on the player; drag and wheel remain available.

Zoom-dependent power-of-two tiles cover the full view within the 64-tile budget (33x33 pixels locally, 65x65 for the overview; at most 1.04 MiB of tile pixels), with at most two new worker jobs pending at once. Overview pixels directly sample the same regional generator; fine zoom uses the existing collision-lattice terrain/water raster. Tiny water features can be subpixel at whole-world scale. The full image fills progressively without activating distant terrain actors. The minimap and expanded map mask the exterior of the world.

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

Biomes should differ primarily through environment, travel, and shelterâ€”not a linear increase in enemy strength. Every reachable biome needs a viable lower-risk approach and a riskier shortcut or reward opportunity.

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
- Save only sparse player/world deltas keyed by `WorldSeed` and a server spatial key; never serialize the entire generated base world.
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

### 5. Companion minimap

Add a circular, top-right minimap that keeps the owning player at its centre, shows facing direction, and uses mouse-wheel zoom clamped between tunable minimum and maximum levels. It is a local UI view of available seed-derived terrain, water, and player-facing landmarks; it must not reveal hidden server-owned gameplay content, create a second biome map, or direct travel.

**Done when:** the map remains circular and legible at supported UI scales and aspect ratios, zoom clamps at both bounds, and host/client peers each see only their own player-centred local view without any gameplay-state mutation.

### 6. Add biomes one at a time

Add Shimmering Lakes, Elderwood, Mossy Mire, Freezing Tundra, and Thunder Mountains individually. Each biome needs a clear environmental identity, original content, and a reason to build or travel differently.

The shared biome-expansion contract uses the existing world identity, four-field classifier, invisible server spatial keys, and Phase 4 exposure inputs. It defines deterministic terrain-feature intent, bounded per-kind population multipliers, normalized exposure modifiers, and stable terrain-aligned discovery candidates. A candidate is not content: only its completed biome slice may have the server materialize it, replicate it when relevant, and persist a sparse state delta. A server-only feature inspection may report a nearby classifier seam and candidate identifier for development; it must not reveal, reserve, or route players toward a discovery.

Shimmering Lakes is complete: continuous patch rendering supplies interlocking collision-free water and shore treatment, while its server slice increases wet-shore pressure through the existing exposure loop and resolves one dry, water-adjacent optional harvest discovery from the active server spatial key. The discovery uses the normal validated harvest and sparse depletion path. It adds no boat, swimming physics, route, bridge, island crossing requirement, or authored camp location.

Elderwood is complete: local terrain presentation derives larger field-driven canopy and non-colliding root buttresses, with continuous Flora leaving lower-density natural clearings. Its server slice applies the Elderwood population and exposure profile only at Elderwood-classified keys and pawn positions, and resolves at most one gently sloped, lower-flora optional harvest discovery per active key. The compact canopy reduces wind and increases natural cover relative to open ground through the existing exposure loop. It adds no trail, route, reserved camp, authored clearing, or client-selected discovery.

Thunder Mountains is complete: continuous elevation forms high, steep-but-traversable ridges without designed passes. Its server slice applies the bounded mountain population and stronger-wind profile only at Mountain-classified keys and pawn positions, and resolves at most one high-ridge storm-carved overlook harvest discovery per active key. Existing server weather and player-built roof/windbreak shelter provide storm and lightning-safe-enclosure preparation. It adds no precision gate, authored route, reserved shelter, client-selected discovery, or new persistence contract.

**Done when:** every added biome is enjoyable on its own, blends naturally with its neighbours, and remains consistent for host and client.

### 7. Coherent biome generation and hydrology

`FKalmalaRegionalTuning` centralizes source frequencies, region scale/warp, biome eligibility and hydrology dimensions. Regional signals are sampled on demand. Normalized biome weights blend terrain relief; maximum weight selects the label with stable enum tie ordering. Flora affects local vegetation only. The master coast determines Ocean.

Shared wetland bowls use a 400 m placement lattice, 40–85 m major radii, elliptical supports, enclosing rims and source-relative water levels. Rivers use deterministic downhill connections and spatially indexed splines. Only derived spline segments are cached, bounded to 256 cells; query order and cache eviction do not change results. Final terrain blends biome relief, bowls and channel carving. Inland water clips against the same terrain triangles used for collision, depth queries and local maps. This is geometric hydrology without flow-volume or erosion simulation.

Run `Scripts/Verify-RegionalGeneration.ps1` and the master-map tests for regional coherence, distance boundaries, shared sampler agreement, deterministic rebuilds, water continuity and host/client identity agreement. Old preview captures and earlier progress entries are historical evidence, not compatibility contracts.

### 8. Ocean and long-distance travel

Islands derive from the master land/water crop. Generated-ocean swimming uses the final collision triangles. `Scripts/Verify-OceanTravel.ps1` queries a seed-derived island and exercises server-authoritative predicted movement with bounded terrain streaming. Tune streaming from profiling evidence; technical limits must not become gameplay zones.

**Done when:** players can travel between land and ocean without terrain gaps, duplicate content, or host/client disagreement.

## Immediate next step

Phase 1 was the original four-field bootstrap. Current maintenance follows the master-map contract above and the ordered backlog. Do not add external terrain or procedural-generation plugins without approval.

### Fast preview experiments

`Scripts/Export-WorldMaps.ps1 -Watch` keeps a headless PNG exporter warm. Save `Scripts/WorldMapPreview.params` to inspect master seed/wavelength, land threshold, regional scale/warp and biome distance experiments without loading gameplay or recompiling. It exports the independent master atlas, rotated crop and exact sampled biome classification to `.cache/WorldMaps/`. See the fast PNG export section in `07-development-setup.md` for controls and validation.

Preview overrides are temporary commandlet-only settings; they are not production tuning, replicated identity, or saved-world data. Defaults reproduce the current compiled generator. The fast shared classification path skips only hydrology and terrain-height work that cannot change biome identity; wetland suitability and distance eligibility remain active. Committing a parameter experiment does not change gameplay. Promote chosen settings by updating production constants and rebuilding every peer.
