# World generation and biome prototype roadmap

## Status and reference boundary

This is a **working proposal**, not an accepted product decision or a commitment to build an open world for the vertical slice.

Kalmala may borrow broad survival-world design lessons from contemporary games: a memorable safe home region, escalating environmental pressure, landmark-led discovery, and a world seed that makes co-op sessions shareable. It must not reproduce another game's source code, biome names, terrain layout, creatures, structures, progression order, or visual language. Kalmala remains a Nordic-Finnic-inspired but wholly original world.

The vertical slice continues to use one camp location, one biome, and a small handcrafted expedition. Procedural generation first supports **bounded expedition spaces**, then expands only when profiling and multiplayer tests justify it.

## Design goals

1. **Readable geography.** Players should identify shelter, water, wind, and route risk at a glance.
2. **Shelter-driven progression.** A biome changes which shelter, clothing, food, and construction decisions matter; it is not merely a higher-level enemy zone.
3. **Shared, repeatable discovery.** A seed and generator revision produce the same authoritative layout for every player in a session.
4. **Landmarks over empty distance.** Every expedition must contain a useful choice: a resource, safe resting point, story site, hazard, or return route.
5. **Bounded simulation.** Weather, elemental interactions, AI, and replication run only in active cells around players.

## Generation model

The server owns a `WorldSeed` (unsigned 64-bit), `GeneratorRevision`, and an `ExpeditionId`. The complete world-save contract records all three before any player modification is saved.

Generation is a deterministic, layered pipeline:

```text
Seed + generator revision
  -> region layout (routes, basin, shore, high ground)
  -> terrain fields (height, slope, drainage)
  -> climate fields (cold, wetness, wind exposure, fertility)
  -> biome and local-variant classification
  -> landmark reservation and traversal validation
  -> resources, wildlife, hazards, and cosmetic dressing
```

### Initial parameters

| Parameter | Prototype value | Purpose |
| --- | ---: | --- |
| Expedition footprint | 1.0 km x 1.0 km | Keeps M0–M4 traversal and replication measurable. |
| Authoritative generation cell | 128 m square | Unit for spawn ownership, persistence, and activation. |
| Active simulation radius | 2 cells per player | Limits world simulation to nearby areas. |
| Terrain height range | -12 m to +96 m | Supports lakes, low forest, ridges, and sheltered sites. |
| Landmark spacing | 180–320 m | Keeps discovery frequent without becoming a checklist. |
| Safe return path | at least 1 per expedition | Guarantees a non-combat route back to camp. |
| Biome transition width | 48–96 m | Avoids abrupt climate and material changes. |
| Determinism tolerance | exact server result | Gameplay placement must not depend on client hardware or frame rate. |

`Elevation`, `Moisture`, `Cold`, `WindExposure`, `Fertility`, and `Oldness` are normalized scalar fields. The classifier selects a biome from weighted ranges, then selects a local variant. `Oldness` is Kalmala's original world-history field: it affects ruin density, anomalous flora, and protective-scroll sites, but never substitutes for a player-facing map marker.

## Proposed biome palette

These are Kalmala concepts, deliberately distinct from any reference game's biome list. Their names, content, and progression are provisional.

| Biome | Environmental rule | Shelter/material pressure | Discovery role | Vertical-slice status |
| --- | --- | --- | --- | --- |
| **Lakewood Vale** | Light rain and lake fog raise wetness; ridges are windier than shore paths. | Teaches fire cover, simple wood structures, and dry storage. | First camp, deer routes, small stone-and-birch sites. | Build first. |
| **Ironmoss Fen** | Saturated ground slows travel and cools exposed players; dry hummocks form routes. | Rewards raised floors, drainage, and waterproof fuel storage. | Bog-iron deposits, causeway ruins, Mireling scavenging sites. | Second prototype. |
| **Kelo Crown** | Cold wind and sparse cover drain warmth; fallen silver-pine trunks create natural bridges. | Rewards insulated clothing, enclosed roofs, and windbreak placement. | High overlooks, ice-fed springs, protective-scroll shrines. | Third prototype. |
| **Stormglass Shore** | Crosswinds and spray alternate rapidly with temporary lee pockets. | Rewards anchors, low profiles, and stone reinforcement. | Shipwreck-like original structures, wave-carved caves, rare coastal materials. | Post-slice exploration. |
| **Emberpeat Hollow** | Subsurface heat, warm ground, and smoke pockets create conflicting local temperatures. | Rewards ventilation and heat-aware camp placement, not stronger damage output. | Charred root halls, warm-water pools, unusual forging resources. | Post-slice exploration. |

The first three biomes form a **shelter progression**, not a linear combat ladder: wetness and drainage, then cold and wind. Each must have at least one viable low-risk route and one high-risk shortcut.

## Landmark and encounter contract

Each active 128 m cell may reserve one primary purpose: `Resource`, `Shelter`, `Story`, `Hazard`, `Wildlife`, or `Transit`. Reservation happens before decorative spawn placement, preventing a landmark from being blocked by foliage or terrain dressing.

For every generated expedition, validate these minimums before it becomes playable:

- camp-to-goal route exists without requiring a jump, swim, or damage exploit;
- one sheltered rest option appears before the highest environmental-pressure segment;
- at least two route choices differ in exposure, distance, or resource opportunity;
- a fixed scroll/major-reward site has a stable server ID and is reachable;
- wildlife, harvest nodes, and hazards use deterministic cell seeds and server-side spawn budgets.

## Multiplayer, persistence, and performance rules

- The server generates and persists gameplay-affecting placements. Clients receive replicated actors/state; they may generate only non-authoritative cosmetic dressing.
- Save modified cells as sparse deltas keyed by `GeneratorRevision + WorldSeed + ExpeditionId + CellCoord`; never serialize the entire generated base map.
- A generator revision change requires either a migration path or a new expedition instance. Never silently reinterpret saved cell coordinates.
- Use pooled or instanced rendering for non-interactable vegetation and rocks. Promote only nearby/interactive objects to replicated actors.
- Profile generation time, memory, replicated actor count, and late-join synchronization separately before enlarging the footprint or cell radius.

## Delivery roadmap

### WG0 — Data contract and deterministic test harness

Define `FWorldGenerationKey`, `FGenerationCellCoord`, a revisioned `UWorldGenerationProfile` Data Asset, and a command-line determinism test that compares two server-side layouts for the same key.

**Accept:** identical key produces identical cell classifications, landmark IDs, and traversal graph in an automated test.

### WG1 — Lakewood Vale expedition

Generate one 1 km x 1 km bounded Lakewood Vale expedition using simple terrain, water mask, forest density, and a handcrafted camp anchor. Keep the final goal/expedition site authored while terrain and minor routes vary.

**Accept:** host and client load matching terrain classification and landmarks; each generated seed has a valid camp-to-goal path and sheltered rest point.

### WG2 — Local climate and shelter probes

Connect wind, rain, wetness, temperature, and shelter to the active cell model. Expose debug overlays for scalar fields and route validation, but do not ship them in the player HUD.

**Accept:** moving between shore, forest, and ridge produces the documented environmental differences with server-authoritative outcomes.

### WG3 — Landmark reservation and encounter budgets

Add deterministic reservations, resource/wildlife budgets, and stable IDs for scroll sites. Validate that gameplay locations are not obstructed and that reconnecting does not respawn consumed rewards.

**Accept:** two players reconnect to an expedition and observe the same consumed reward, harvest, and landmark state.

### WG4 — Ironmoss Fen contrast test

Add Ironmoss Fen as the second profile using wet ground, hummock routing, and drainage-focused construction pressure. Do not increase enemy tiers until the environmental loop is fun and legible.

**Accept:** playtests choose different routes and construction responses than Lakewood Vale for measurable environmental reasons.

### WG5 — Kelo Crown and multi-biome seams

Add Kelo Crown plus 48–96 m blended transitions. Test seed compatibility, save deltas across seams, and host/client late join.

**Accept:** all three profiles stream without terrain gaps, duplicate landmarks, or cell-state disagreement.

### WG6 — Scale decision

Decide whether to keep authored expedition islands, generate a connected regional map, or use a hybrid. This requires profiling evidence and a separate product decision; it is not part of the vertical-slice commitment.

## Next prototype task

When M0–M2 permits world-generation work, begin with WG0 only: create the revisioned generation key/profile data contract and a deterministic commandlet test. Do not add terrain plugins, external procedural-generation frameworks, or a full open-world streamer without approval.
