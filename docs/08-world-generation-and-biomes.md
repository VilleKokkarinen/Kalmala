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

### Master map and environmental fields (revision 7; current default)

First generate a continuous Perlin **land/water master map with its own seed**, independent of the game seed. The 128 km square atlas uses the revisioned constant `FKalmalaMasterMap::MasterSeed = 0x4b616c6d616c6137`, a 3 km main wavelength and a second 1.29 km octave (75%/25% amplitudes). Positive noise is land; zero or negative noise is ocean. It contains no biome identities, authored islands, or gameplay placements.

The game seed and generator revision select an atlas crop centre and an arbitrary rotation. The crop retains the current **16 km radius / 32 km diameter** playable circle. The centre is selected from up to 128 seeded candidates: use the first with land signal above 0.10, or the strongest candidate if none qualifies. This favours inland terrain for the start without painting land into the atlas. The entire rotated circle fits inside the master map. World zero is the crop centre; rotation preserves distance and scale. No baked map image is required.

The existing four independently seeded game fields remain:

| Map | Controls |
| --- | --- |
| **Elevation** | Inland relief from the existing Perlin noise, constrained to the master's land/water sign and tapered continuously to the coast. |
| **Humidity** | Surface and ground moisture, wetland suitability, vegetation support. |
| **Temperature** | Climate, snow, frost, warmth pressure, cold-biome suitability. |
| **Flora** | Local vegetation density, undergrowth and clearings; it does not fragment broad biome identity. |

Changing the game seed changes the crop, rotation and environmental fields, but not the underlying master atlas. Master tuning and its seed are fixed by the generator revision, not client-controlled settings. Changing them requires another revision; no save schema is added.

### Land biome placement and origin distance

On land, the existing regional Perlin signals and environmental suitability select **Shimmering Lakes, Elderwood, Mossy Mire and Freezing Tundra**. **Meadows and Thunder Mountains are the default land biomes** wherever no special biome wins. Distance is measured from game-world XY `(0, 0)`, not the player start or master-map origin.

| Rule / biome | Distance from world zero |
| --- | --- |
| **Meadows or Ocean only** | **0–0.35 km inclusive** |
| **Meadows** | **At most 4 km** |
| **Shimmering Lakes** | **At least 0.35 km**, with the inclusive starter rule taking precedence |
| **Elderwood** | **At least 0.75 km** |
| **Mossy Mire** | **At least 2 km** |
| **Freezing Tundra** | **At least 4 km** |
| **Thunder Mountains** | Default elevated land outside the starter zone; default remaining land beyond 4 km |

Minimum-distance weights rise smoothly over 50 m on the eligible side, never leaking into excluded radii. The lowland Meadows fallback fades into Mountains over the last 50 m before 4 km, reaching zero at 4 km. Elevation also favours the mountain fallback nearer the centre, with no mountain influence at or inside 0.35 km. A winning special biome takes precedence over the fallback, including on high terrain. Remaining land beyond 4 km is Thunder Mountains even where source relief is low. These restrictions replace the earlier Gaussian distance preferences; they are not enemy levels or travel gates.

Shimmering Lakes still requires a moisture-qualified, enclosed lowland bowl. Its entire bowl/rim support must lie outside the starter radius, and its local influence fades toward master-map coasts. Rivers remain enabled and may create inland water within a land biome, including Meadows; inland water does not turn that land into Ocean. The later island overlay is disabled: islands must already exist in the master crop. The player-start resolver searches dry Meadows candidates within 320 m of zero.

Terrain, collision, sea water, inland water, minimap and expanded map consume the same generator. Regional weights, biome identity and terrain remain computed functions. Only a small crop transform is memoized per worker alongside the existing bounded hydrology spline cache; no authoritative biome raster is stored or replicated. Weather, wildlife, ruins, discoveries and harvest nodes are derived later.

```text
MasterSeed -> continuous land/water atlas
WorldSeed + GeneratorRevision -> crop centre and rotation -> base land/water mask
WorldSeed + GeneratorRevision -> Elevation, Humidity, Temperature, Flora
  -> master-constrained terrain + regional signals + distance eligibility
  -> special land biomes; Meadows/Mountains fallback; Ocean from master water
  -> seeded content, weather, survival and sparse player changes
```

## Finite worlds and compatibility

New worlds default to **revision 7**. Revisions 5–8 use the existing 16 km origin-centred radius. Revisions 1–6 retain their original layouts; revisions 1–4 also retain unlimited extent. Open an existing world with its original seed and explicit `-GeneratorRevision`. No existing save is migrated or silently reinterpreted, and no saved-data schema changes.

**Legacy revisions 5/6 only:** Meadows, Elderwood, Mire and Tundra have Gaussian distance preferences centred at 0, 4, 8 and 12 km with 4.8 km breadth and a nonzero floor. Outer uplands gain mountain foothills. Revisions 7/8 replace these soft preferences with the master map and hard eligibility limits above.

Production revisions 5 and 7 retain large rivers and omit small streams. `-KalmalaEnableStreams` (non-shipping, without an explicit revision) now selects **revision 8**, the master-map debug layout with streams. Explicit revision 6 retains the old radial debug layout with streams; revisions 1–4 also retain streams. Each debug revision is a separate replicated identity/save. An explicit `-GeneratorRevision` takes precedence over the flag.

Terrain and water triangles retain the existing radial clipping. Exterior patches, population and discoveries are rejected; decoration keeps a 10 m edge margin and new hearth/construction placement keeps 3 m clearance. Character Movement enforces the radius minus capsule clearance on authority and owner prediction while preserving tangential/vertical movement. No edge wall, actor budget, asset or authority change is introduced.

### Full-world debug map

![Seed 418 master atlas, rotated crop and final biome placement](master-map-preview.png)

The retained preview shows the actual revision-7 generator. Gold outlines the selected crop in the master atlas; the second panel is the rotated land/water base and the third applies biomes. Small lakes/channels can be subpixel at this 32 km view.

M opens centred on world zero, fitted with 4% margin so the entire circle is visible. The local console settings `kalmala.Map.RevealAll 1` and `kalmala.Map.FitWorldOnOpen 1` are enabled by default for current development. Set either to `0` to restore exploration fog or player-centred opening respectively. Reveal affects terrain/water presentation only: it never records exploration, reveals server population, changes co-op privacy or changes ping range. Personal exploration continues to accumulate normally. R still recentres on the player; drag and wheel remain available.

Zoom-dependent power-of-two tiles cover the full view within the 64-tile budget (33x33 pixels locally, 65x65 for the overview; at most 1.04 MiB of tile pixels), with at most two new worker jobs pending at once. Overview pixels directly sample the same regional generator; fine zoom uses the existing collision-lattice terrain/water raster. Tiny water features can be subpixel at whole-world scale. The full image fills progressively without activating distant terrain actors. The minimap and expanded map mask the exterior of revision-5-and-later worlds.

## Biome palette

### Legacy terrain-based classifier (generator revision 2)

Revision 2 remains available for existing worlds; new worlds now default to revision 7. Selection uses only the existing four normalized fields, in priority order: submerged terrain (`Elevation < 0.22`) is Ocean; peaks (`> 0.78`) are Thunder Mountains; cold uplands (`Elevation >= 0.55`, `Temperature < 0.35`) are Freezing Tundra. Mossy Mire requires low ground (`Elevation < 0.45`), high moisture (`Humidity > 0.72`), and temperate conditions (`Temperature >= 0.28`). Remaining wet lowlands (`Elevation < 0.55`, `Humidity > 0.63`) are Shimmering Lakes. Elderwood requires both dense growth (`Flora > 0.64`) and moisture (`Humidity >= 0.35`); remaining land is Meadows.

These are original terrain-suitability rules, with no distance-from-spawn progression or extra noise map. Classification returns one dominant biome; it does not calculate slope, blend weights, or connected water basins. Existing continuous fields still drive terrain and environmental variation, and the separate lake-basin query determines visible standing water.

Revision 1 retains its original classifier and field seeds. Sampled fields carry revision metadata so existing callers select the correct rules. Use `-GeneratorRevision=1` to reopen the old layout (`-Revision=1` for visualization). Revision 2 creates a different base world because revision also participates in field seeds. The serialized config default remains 1 for compatibility; new-world entry points now select revision 7. Revision-2 population saves use a seed/revision-specific slot, leaving the legacy revision-1 slot intact; no save schema is changed.

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

### 7. Coherent biome generation and hydrology (revision 3)

The regional generator retains four authoritative continuous source fields. Elevation uses a 100,000 cm noise wavelength with only 1.5% local relief amplitude; Humidity and Temperature use 150,000 cm wavelengths. Flora remains local at roughly 1,333 cm and never selects a regional biome or shapes terrain. These wavelengths describe noise sampling, not guaranteed biome diameters.

`FKalmalaRegionalTuning` centralizes the 60,000 cm macro scale, regional frequency, 120,000 cm warp wavelength, 8,000 cm warp displacement, ring overlap and edge waves, source frequencies, and hydrology dimensions. These are versioned developer constants, printed in visualization `Tuning.txt`, rather than mutable client settings. Changing production tuning requires another generator revision.

Each biome has seeded offsets and overlapping concentric core/ring signals around procedural centers, with continuous motion noise and angular sine edge variation. Signals are evaluated on demand, never stored as a biome map. Environmental suitability bounds Elderwood by moisture, Mire by humid temperate lowlands, and Tundra by cold uplands. Normalized weights blend terrain-shaping contributions; maximum weight selects a dominant label, ties follow the stable enum order. Submerged source terrain explicitly selects Ocean and mountain-height source terrain selects Thunder Mountains. Meadow retains a nonzero fallback weight. Physical coast/mountain transition weights are smooth even when the dominant label changes.

Shimmering Lakes requires a deterministic source-qualified basin, not just humidity. Seeded lowland centers on a coarse 40,000 cm lattice produce separated elliptical bowls with raised enclosing rims, 4,000–8,500 cm major radii, and source-relative water levels. A broad lake-region signal and center climate qualify each bowl. The bowl and its rim blend into surrounding terrain; lake weights cover the water and immediate banks. This replaces flood-cache decisions only for revision 3. Legacy lake flood-fill semantics remain intact for revisions 1 and 2.

Rivers and streams derive separate candidate sets from 18,000 cm coarse cells. Nearby candidates coalesce to the lowest stable seed ID within 5,000 cm. Each retained point connects to an eligible lower neighbor within 28,000 cm with deterministic score/tie order. Streams require both ends on low land (40–560 cm above sea, starts at least 80 cm). Sine-displaced spline samples share exact endpoints and use seeded amplitude, wavelength, and phase; samples are at most 250 cm apart along the underlying segment. River/stream influence widths are 600/220 cm. A 6,000 cm spatial index includes every intersecting segment envelope across cell edges. Only generated spline segments are cached, bounded to 256 cells for one identity; eviction, rebuild, and query order do not change the network.

Final height blends per-biome relief, then enclosed bowls and continuous channel carving. Inland water levels are clipped against those same shaped terrain triangles and interpolated at intersections, including river grades. Sea depth continues to use the shared final collision-triangle plane. The minimap interpolates the same inland water-depth differences on the same lattice. Basin rims take precedence over channel carving to retain enclosure. This is deterministic geometric hydrology; it does not simulate water volume, catchment discharge, erosion, currents, flooding, or inland swimming physics.

Revision 4 retains all revision-3 regional and hydrology rules, then adds occasional deterministic 6,500–10,000 cm-radius emergent islands from a 60,000 cm coarse lattice. Their centres, radii, summits, submerged aprons, and shore blend are derived solely from the immutable identity; no island map, actor, replicated placement, or save data exists. Revisions 1–3 remain byte-for-byte terrain-compatible. Revision 4 remains available explicitly; new game worlds and preview commandlets now select revision 7. Serialized config defaults, revision-1/2 source sampling, terrain formulas, classifiers, and legacy save identities remain unchanged. Use the original seed and its original `-GeneratorRevision` to reopen an existing layout. Revision-3/4 population deltas use the existing identity-specific save-slot convention and unchanged schema. The server owns world identity, population, interactions, exposure, and persistence; client terrain and water are reproducible presentation/prediction inputs only.

`RenderWorldGenerationVisualization -Revision=3 -Seed=418 -Extent=400000 -Size=256 -Output=<directory>` writes the four fields, dominant biomes, seven weight images (enum order), overlap strength, boundary overlay, indexed river/stream splines (red/green, basins blue), shaped height, and tuning values. The 400,000 cm square is a 4 km-wide diagnostic area. `Scripts/Verify-RegionalGeneration.ps1` runs large-area coherence, Flora independence, spline rebuild, GridCell continuity, shaped collision-depth and nonempty adjacent-water-edge tests, repeated/different-seed renders, and a conflicting-seed live peer fingerprint comparison.

**Done when:** those checks pass for seeds 418 and 419, repeated images are identical, and live peers reproduce the same 81-position fingerprint from the server identity. Coherence thresholds are boundary density below 0.18 and fewer than 250 components of one or two samples on a 161×161, 2,500 cm diagnostic lattice. These bounds detect regressions without claiming every seed or tiny sub-grid feature has been exhaustively verified.

Verified 2026-09-07: boundary densities 0.0923/0.0927, component counts 183/184, median component sizes 17/16 samples, and tiny-component counts 43/47 for seeds 418/419. Both adjacent-water fixtures matched 72 vertices. All repeated render images matched byte-for-byte and live peers agreed on fingerprint `7654679350939909151` for 81 positions.

![Revision-3 biome, hydrology, and shaped-height previews for seeds 418 and 419](phase7-generation-preview.png)

### 8. Ocean and long-distance travel

Revision 4 supplies occasional seed-derived emergent islands and generated-ocean swimming uses the same final terrain triangles as collision and coastlines. `Scripts/Verify-OceanTravel.ps1` drives a revision-4 host and conflicting-seed client through a sampled deep-ocean waypoint to the same existing island, while the server recycles only the bounded terrain-patch neighborhoods around authoritative pawns. At island arrival, each peer audits its locally replicated descriptors and requires a complete 3x3 neighborhood with unique patch coordinates; the runner rejects terrain movement-correction warnings. It proves normal movement prediction and replicated identity agree through land, ocean, and island arrival without a boat, route, island actor, client-selected target, or streaming-budget increase. Tune streaming only from profiling evidence; technical limits must not become gameplay zones.

**Done when:** players can travel between land and ocean without terrain gaps, duplicate content, or host/client disagreement.

## Immediate next step

Phase 1 was the original four-field bootstrap. Current maintenance follows the master-map contract above and the ordered backlog. Do not add external terrain or procedural-generation plugins without approval.
