# Autonomous development backlog

This file is the execution queue for Codex automations. Keep items small, ordered, and independently verifiable. Only start work in the earliest milestone whose acceptance criteria are not yet met.

## M0 — Bootstrap

- [x] Verify that `KalmalaEditor Win64 Development` builds with the command in `docs/07-development-setup.md`.
- [x] Open the project in Unreal Editor and create the prototype map at `/Game/Kalmala/Maps/Prototype/L_Prototype`; document the result.
- [x] Configure and verify a packaged development build launches after the prototype map exists.
- [x] Confirm whether a server-capable Unreal 5.8 build is available; otherwise document the dedicated-server build blocker.

## M1 — Networked traversal and interaction

Do not begin until M0 acceptance criteria in `docs/04-roadmap.md` are met.

- [x] Implement the smallest server-authoritative replicated character and camera setup needed for the two-player prototype map.
- [x] Add a server-validated interaction trace and an interactable interface.
- [x] Add a two-player test map flow and verify invalid client interactions are rejected.

## World generation track

Start this track only after M1 passes. `docs/08-world-generation-and-biomes.md` is the authoritative contract: the world is seed-generated from continuous `Elevation`, `Humidity`, `Temperature`, and `Flora` maps; do not introduce authored gameplay regions, fixed routes, or additional biome maps.

### Phase 1 — Seed and map proof

- [x] Define the immutable server-owned `WorldSeed` and `GeneratorRevision` world-generation contract.
- [x] Implement deterministic sub-seed derivation for `Elevation`, `Humidity`, `Temperature`, and `Flora`.
- [x] Implement continuous Perlin sampling and normalization for all four maps at a world position.
- [x] Implement deterministic biome classification and continuous transition blending from the four sampled values.
- [x] Add a developer-only visualization of the four fields and final biome classification.
  - [x] Add the deterministic `RenderWorldGenerationVisualization` editor commandlet and committed field/biome previews.
- [x] Verify same-seed reproducibility and visible different-seed variation in host/client play.
  - [x] Replicate the server-selected world identity through `GameState` and verify a conflicting-seed client receives it.
  - [x] Verify field and biome previews reproduce for the same seed and vary for a different seed.

### Phase 2 — First playable generated world

- [x] Generate traversable terrain from the seed, including a seed-generated player start, Meadows, lakes, trees, and rocks.
  - [x] Resolve and spawn a deterministic Meadow-preferred player start on the server.
  - [x] Add one continuous Elevation-derived terrain height and normal contract for spawns, rendering, and collision.
  - [x] Generate a 24×24-cell continuous terrain mesh with server-authoritative collision and matching client prediction collision.
  - [x] Replace the earlier coarse collision tiles with collision from the continuous mesh itself.
  - [x] Render collision-free seed-derived sea-level surface water over submerged terrain cells and verify world-scale coverage.
  - [x] Add local deterministic, non-interactable Meadow rock instances.
  - [x] Add local deterministic, non-interactable Meadow tree trunk and canopy instances.
  - [x] Activate an initial server-owned 3×3 terrain-patch neighborhood around the generated start.
  - [x] Add bounded, deduplicated server-side patch activation around connected players.
  - [x] Add distinct Shimmering Lakes water and shoreline treatment beyond sea-level coverage.
  - [x] Replace temporary engine primitive meshes/materials with original terrain, rock, tree, and water assets.
    - [x] Create and apply project-owned generated terrain, water, and lake-shore materials.
    - [x] Create and apply project-owned rock and tree meshes/materials.
- [x] Verify host and client traverse matching generated terrain and observe the same meaningful natural features.
  - [x] Verify a conflicting-seed client receives the server identity and builds all nine initial terrain surfaces without movement-base warnings.
  - [x] Run an actual two-player traversal test across matching terrain, water, rocks, and trees.

### Phase 3 — Natural population

- [ ] Add deterministic server-side spatial seeds and spawn budgets for wildlife, harvest nodes, and hazards.
- [ ] Persist consumed or defeated gameplay content as sparse world deltas keyed by seed, generator revision, and server spatial key.
- [ ] Verify the same seed produces matching gameplay content and that state remains consistent after reconnecting.

### Phase 4 — Weather, shelter, and survival

- [ ] Connect generated terrain to rain, wind, wetness, warmth, fires, and shelter without a prescribed route.
- [ ] Verify terrain and weather create meaningful player choices in routes, camps, and preparation.

### Phase 5 — Biome expansion

- [ ] Add Shimmering Lakes with its environmental, shelter, and discovery identity.
- [ ] Add Elderwood with its environmental, shelter, and discovery identity.
- [ ] Add Mossy Mire with its environmental, shelter, and discovery identity.
- [ ] Add Freezing Tundra with its environmental, shelter, and discovery identity.
- [ ] Add Thunder Mountains with its environmental, shelter, and discovery identity.
- [ ] Verify each added biome is independently enjoyable, blends naturally, and is host/client-consistent.

### Phase 6 — Ocean and long-distance travel

- [ ] Add ocean terrain, islands, and the systems required for long-distance movement.
- [ ] Profile generation time, memory, replicated actor count, save size, and late-join synchronization before increasing density or streaming distance.
- [ ] Verify land-to-ocean travel has no terrain gaps, duplicate content, or host/client disagreement.

## Later gameplay milestones

Use `docs/04-roadmap.md` as the source of truth. Add decomposed M2–M5 tasks here only after their preceding milestone acceptance criteria pass.
