# Development backlog

## Active queue

- [ ] Four-view creature visual cleanup, one species per increment.
  - [x] Boar: replace detached spike legs with connected rounded limbs and
    clarify the snout, eyes, ears, and short bristle ridge.
  - [x] Badger: join the narrow head and legs to its low body and make the
    facial stripe visible.
  - [x] Moon Bear: connect its head and feet to the body with a heavier bear
    silhouette.
  - [x] Death Adder: lower and connect the head to the coiled body.
  - [x] Viper: lower and connect the head to the coiled body.
  - [x] Giant Snake: lower and connect the head to the coiled body.
  - [x] Wolf: connect the head, tail, and feet to the torso.
  - [ ] Lynx: connect the head, tail, and feet to the torso.
  - [ ] Polar Bear: give the head and short legs a connected, heavier shape.
  - [ ] Horned Goat: connect the raised head and legs to the body.

Completed goal records are retained under Completed post-roadmap work.

## Completed roadmap

| Milestone | Completed scope / acceptance |
| --- | --- |
| M0 — Bootstrap | Project setup, prototype map, packaged build and dedicated-server availability recorded. |
| M1 — Networked traversal and interaction | Replicated movement and server-validated interaction. |
| World generation, Phases 1–9 | Deterministic generation, survival, biome expansion, ocean travel and expanded maps. |
| M2 — Survival camp loop | Gathering, crafting, construction and camp persistence. |
| M3 — Elemental world prototype | Server-owned weather, wetness, exposure and recovery. |
| M4 — Combat and support magic | Combat, creature encounters, discoveries and support effects. |
| M5 — Vertical-slice finish | Complete under recorded acceptance scope. |
| M6 — Production hardening | Complete under recorded supported-session scope and limits. |
| M7 — Survival progression and gameplay UI | Complete. |
| M8 — Ocean and long-distance travel | Complete; dry-shore acceptance waived. |
| M9 — Expanded biome content | Complete under recorded scope. |
| M10 — Release completion and launch validation | Complete under recorded scope and limits. |
| M11 — User experience and visual UI | Final acceptance passed 2026-10-06. |
| M12 — Inventory menu and cleaner HUD | Final acceptance passed 2026-10-09. |
| M13 — Revalidated UI on main | Integrated-main acceptance passed 2026-10-11: 122/122 tests, rendered peers, input/privacy/reconnect and Development/Shipping smoke. [Evidence](docs/evidence/m13/README.md). |
| M14 — Expanded world map | Integrated-main acceptance passed 2026-10-12: editor build, 123/123 tests, live map runners, rendered/performance matrix. [Evidence](docs/evidence/m14/main-integration.md). |

## Completed post-roadmap work

### Four-angle creature preview sheets — 2026-10-28

- **Selected task:** Render each live creature from side, front, back, and a
  preset three-quarter angle in one combined PNG.
- **Work completed:** The creature commandlet now projects its live procedural
  mesh from four orthographic camera directions and composes a 2×2 sheet. The
  original side camera is preserved; front and back are level, and the fixed
  front three-quarter camera is 45 degrees. Script results report sheet and
  per-view dimensions.
- **Checks:** PowerShell parser and `-List` check passed; source and output
  dimensions were inspected, along with `git diff --check`. No Editor build or
  preview render was performed.
- **Status:** Implemented; all 17 four-view sheets were rebuilt and visually
  inspected. Species-specific cleanup is tracked in the active queue.

### Separate preview outputs and repair PeatHusk/Sentinel silhouettes — 2026-10-28

- **Selected task:** Put menu and creature batches in separate folders and fix
  the PeatHusk and Thunder Sentinel meshes shown as disconnected triangles in
  the user's renders.
- **Work completed:** Changed menu and creature render defaults and the
  combined launcher to use `png\Menus` and `png\Creatures`. Rebuilt the two
  identified silhouettes from overlapping ellipsoid masses and articulated
  limbs, retaining small mineral/moss details and their existing attack poses.
- **Checks:** Inspected the user's PeatHusk and Thunder Sentinel PNGs and the
  successful 17-creature render log, confirming the two models were built only
  from tetrahedra. PowerShell parser and `-List` checks plus `git diff --check`
  passed. No Editor build or render rerun was performed.
- **Status:** Source and output routing are updated; the new silhouettes still
  need a rebuilt preview run and visual confirmation.

### Creature preview rasterizer repair — 2026-10-28

- **Selected task:** Fix the user's combined preview run stopping with creature
  render code 7 after a uniform scene capture.
- **Work completed:** Replaced the unreliable offscreen scene-capture path with
  an orthographic software rasterizer that draws each live procedural mesh's
  triangles, vertex colors, and normals with a depth buffer. Added a generated
  Mireling body, which previously had no mesh and was the first archetype to
  trip the blank-frame guard.
- **Checks:** Inspected the failed run log, Unreal's SceneCapture behavior, the
  procedural section buffers, and the Mireling presentation path; `git diff
  --check` passed. No Editor build or render rerun was performed.
- **Status:** Code 7's blank SceneCapture path is removed; a rebuilt cache and
  rendered appearance still need user-run confirmation.

### Double-click combined preview renderer — 2026-10-28

- **Selected task:** Add a launcher that builds Editor modules once and runs
  both the menu and live-creature renderers without requiring terminal use.
- **Work completed:** Added a `.cmd` double-click entrypoint and its PowerShell
  coordinator. Both renderers now mirror authored local plugin files and use
  the same native-source freshness stamp, allowing the menu build to feed the
  creature render from one shared cache.
- **Checks:** PowerShell parser and both renderers' `-List` checks passed;
  shared cache/digest inputs and `git diff --check` were inspected. No Editor
  build or render batch was run.
- **Status:** Launcher and shared-cache flow are implemented; rendered outputs
  were generated in a user run, but all 17 creature captures were uniform
  near-black; see the follow-up below.

### Creature preview blank-capture repair — 2026-10-28

- **Selected task:** Make the offscreen creature commandlet render its scene
  instead of saving the render target's clear color.
- **Work completed:** Initialize the standalone Slate RHI renderer, matching the
  working menu commandlet, before creating the capture world. Reject uniform
  captured frames so blank images fail visibly.
- **Checks:** Inspected the generated Wolf PNG (uniform RGB 9,13,18), the 17
  creature success/vertex log entries, and Unreal 5.8's SceneCapture source;
  `git diff --check` passed. No Editor build or render rerun was performed.
- **Status:** Source repair is in place; generated appearance remains unverified
  until the Editor cache is rebuilt and the renderer is run again.

### Creature renderer plugin-cache repair — 2026-10-28

- **Selected task:** Fix the creature preview cache build failure caused by its
  missing project-local `VisualStudioTools` plugin.
- **Work completed:** Added local plugin mirroring to the creature renderer,
  excluding generated build/save/cache folders, and included authored plugin
  files in the source freshness stamp so plugin changes request a rebuild.
- **Checks:** Inspected the project plugin declaration and local plugin
  descriptor/source; PowerShell parser, `-List`, and `git diff --check` checks
  passed; the authored plugin file set includes the required descriptor and
  excludes generated folders. No Editor build or creature render has been
  rerun.
- **Status:** The cache-copy fix is implemented. A later user-run build found
  the plugin and compiled modules, then failed linking the commandlet because
  `KalmalaEditor` did not directly depend on `ProceduralMeshComponent`; see the
  follow-up below.

### Creature preview procedural-mesh link dependency — 2026-10-28

- **Selected task:** Resolve the commandlet link errors for
  `Z_Construct_UClass_UProceduralMeshComponent` and
  `UProceduralMeshComponent::GetProcMeshSection`.
- **Work completed:** Added `ProceduralMeshComponent` as a direct private
  dependency of `KalmalaEditor`, which owns the commandlet that calls those APIs.
- **Checks:** Inspected the failing commandlet calls and existing module
  dependency declarations; `git diff --check` passed. No build was run during
  this increment.
- **Status:** Source dependency is fixed; cache rebuild and creature rendering
  remain unverified.

### Live-creature render script — 2026-10-28

- **Selected task:** Add a `Render-Menu.ps1`-style batch preview for every live
  wildlife archetype.
- **Work completed:** Added `Render-Creatures.ps1` with all 17 archetype names,
  cache mirroring/build stamps, output selection, progress timeout, and PNG
  validation. Added an offscreen Editor commandlet that initializes each
  archetype in an isolated preview world, frames it with an orthographic side
  camera, captures a lit PNG, and logs vertex count and capture duration.
- **Checks:** PowerShell parser validation and `-List` succeeded for all 17
  names; inspected the commandlet's Unreal API usage and ran `git diff --check`.
  No Editor build or render run has been performed.
- **Status:** Source implementation is complete; image appearance remains
  unverified.

### Wildlife model silhouette repair — 2026-10-28

- **Selected task:** Replace triangle-built animal body masses that read as
  unrelated wedges with recognizable rounded anatomy.
- **Work completed:** Added low-poly ellipsoid surfaces with smooth vertex
  normals for animal bodies and heads, and replaced overlapping tetrahedra on
  boar, deer, rat, badger, bears, goat, wolf, lynx, snakes, reef maw, and
  mosquito bodies. Retained angular geometry for species markings, limbs,
  horns, antlers, fins, facial details, and attack cues. Updated the surface
  checker to support mixed mesh shapes and documented the model contract.
- **Checks:** Inspected generated surface winding, ellipsoid dimensions, and
  remaining anatomy details; `git diff --check` passed. No build, tests, or
  automation were run.
- **Status:** Source implementation is complete; rendered appearance has not
  been reviewed in the game.

### Meadows relief retune — 2026-10-27

- **Selected task:** Apply a Valheim-informed Meadows height adjustment after
  the user reported that the merged biome still looked flat.
- **Work completed:** Increased compiled defaults from 4 / 0.6 / 0.1 m to
  8 / 1.2 / 0.2 m for broad hills, rolling relief and floor detail; changed
  floor scale from 8 m to 12 m. Updated the focused default assertion and the
  current terrain contract. The prior settings remain in the historical
  BH25 profile record.
- **Checks:** Manually checked the defaults against the existing preview bounds
  and one-factor sweep ranges; `git diff --check` passed. No build, test suite,
  profile sweep or rendered review was run.
- **Status:** Source implementation is complete. Fresh combined-profile and
  rendered terrain acceptance remain pending.

### Meadows 30 m height follow-up — 2026-10-28

- **Selected task:** Raise Meadow relief headroom to reflect the user's
  observation of Valheim Meadows reaching roughly 30 m.
- **Work completed:** Set the broad Meadow amplitude to 20 m, move shoulder
  compression to a 30 m threshold, and expand accepted preview bounds to cover
  both values. Updated the preview parameter file and current tuning docs.
- **Checks:** Source and preview bounds were inspected; `git diff --check`
  passed. No build, automation, combined-profile sweep or rendered review was
  run.
- **Status:** Defaults and preview contracts are updated. Actual absolute
  height range and rendered terrain readability remain unverified.

### Valheim-informed forest and mountain follow-up — 2026-10-28

- **Selected task:** Compare the other Kalmala biome height functions with
  Valheim's corresponding terrain formulas and adjust the close analogs.
- **Work completed:** Raised shared Elderwood/Tundra broad relief from 4 m to
  20 m at the existing 1 km scale, matching Valheim Black Forest's nominal
  ±20 m detail contribution. Raised Thunder Mountain highland gain from 0.4
  to 1.0, matching the full one-for-one amplification of source excess above
  its threshold. Expanded preview bounds and the existing profile matrix to
  cover the adjusted settings. Kept the intentionally compressed Mire,
  mixed Lakes and master-constrained Ocean controls at their measured values.
- **Checks:** Compared the formulas and control units, inspected all current
  preview bounds and regression expectations, and ran `git diff --check`. No
  build, automation, profile sweep or rendered review was run.
- **Status:** Source settings and research notes are updated. Fresh profiles
  and rendered terrain acceptance remain pending for Elderwood/Tundra,
  Thunder Mountains, and Lakes after the Meadow change.

### Elderwood/Tundra height headroom — 2026-10-28

- **Selected task:** Give Elderwood and Freezing Tundra the ability to rise
  above Meadows' 60 m shoulder ceiling.
- **Work completed:** Raised their shared 1 km broad-relief amplitude from
  20 m to 60 m. Expanded preview bounds and the existing forest profile
  matrix to cover the new default; updated the height comparison and tuning
  notes.
- **Checks:** Inspected Meadow's shoulder asymptote, Elderwood/Tundra source
  height composition and all matching preview limits; ran `git diff --check`.
  No build, automation, profile sweep or rendered review was run.
- **Status:** The biome candidates can exceed the Meadow ceiling when positive
  broad relief combines with positive source height. Seeded profile and render
  confirmation remain pending.

### Inventory drag and drop — 2026-10-28

- **Selected task:** Make inventory items draggable between grid cells and onto
  compatible equipment slots.
- **Work completed:** Replaced the mouse-release-only grid gesture with a
  native drag operation and visible item icon. Grid drops request the same
  server-validated instance swap; drops on compatible wearable slots or Main
  Hand send the existing exact-GUID equip request. Updated the inventory
  interaction contract.
- **Checks:** Reviewed the widget input and server request paths and ran
  `git diff --check`. No Unreal build, automation, or in-game interaction
  review was run.
- **Status:** Source implementation is complete. Interactive acceptance of
  grid moves and equipment drops remains pending.

## Scope and history

The [roadmap](docs/04-roadmap.md) and [feature contracts](docs/game-features.md)
remain authoritative for scope; the [verification index](docs/07-development-setup.md#consolidated-verification-index)
records project checks. Detailed implementation history and retired checklists
remain in [PROGRESS.md](PROGRESS.md) and Git history. No acceptance status or
product scope changes as part of this cleanup.
