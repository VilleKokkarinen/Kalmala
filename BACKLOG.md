# Development backlog

## Active queue

- [x] Targeted world-save and Escape-menu repair (2026-10-30; user-requested
  outside the project-wide review queue).
  - [x] Write a fresh population record on world entry so untouched worlds are
    discoverable by the existing local-world list.
  - [x] Add solo/host Save world and Save and log out actions; save current
    world components and player state before host/solo logout, retaining the
    session on save failure. Clients retain server-mediated player logout.
  - Lightweight presentation checks passed; Unreal build and live
    create/save/logout/reopen acceptance remain deferred. Review tasks unchanged.

- [ ] Project-wide code review — review every project-owned file and integration.
  - **Goal:** Produce a complete, traceable review of the current project against
    AGENTS.md and the brief, design, architecture, roadmap and decision log.
  - **Run instructions:** Complete the first unchecked, unblocked child in order,
    exactly one per run. Split large children into numbered file batches before
    reviewing them (target 10–20 minutes plus a lightweight check per batch).
    Update backlog/progress, commit only run-owned changes, then stop.
  - **Review-only boundary (CR01–CR19):** Do not change code, tests, scripts,
    configuration, catalogues or assets, including small fixes or verification
    repairs. Only BACKLOG.md, PROGRESS.md and the review evidence ledger/reports
    may be edited. Inspection tools must not rewrite project files.
  - **Finding handoff:** Before checking off each review child, add every
    confirmed actionable finding as an unchecked RF task under the separate
    Review findings remediation goal below; do not wait until CR18. Link its
    finding ID and evidence. Never implement a repair during a review run.
  - **Review defaults chosen for this plan:** Review and findings only. Record actionable defects with severity, file/line
    or asset reference, trigger, impact, evidence and suggested repair. Separate
    confirmed defects from questions. Queue repairs explicitly without treating
    this review as permission for restricted changes. A clean review also needs
    a recorded result; an unchecked or unread file is never implicitly covered.
  - **Coverage:** Include all project-owned tracked files, authored local files
    and relevant ignored source assets; exclude generated build/cache output.
    Inspect binary assets through their metadata, references and available
    previews/editor inspection; do not claim textual review of binary content.
    Record third-party/generated exclusions and ownership rationale explicitly.
  - [x] **CR01 — Establish the review baseline and coverage ledger.** Record Git
    revision and existing local changes; enumerate files by domain in a short
    `Temporary/Review/coverage.md` ledger. Give every file a pending/reviewed/
    excluded status and rationale, and create bounded batches for CR02–CR16.
    Store findings in `Temporary/Review/findings.md`; track exact reviewed
    revisions or hashes so later changes can be identified for re-review.
    Baseline `fc83040078cb02284d7bc0b4ce2f960d873b648a` fingerprints all 1,437
    tracked files and defines 272 bounded batches; no implementation review or
    findings are claimed in this baseline increment.
  - [x] **CR02 — Review project structure and Core.** Check module/target rules,
    project descriptor, shared types, ownership, dependencies and public APIs.
  - [ ] **CR03 — Review session and server code.** Check solo/listen/dedicated
    lifecycle, identity, joins, disconnects, reconnects and cleanup.
  - [ ] **CR04 — Review network trust boundaries.** Trace every client request
    to server validation, ownership, bounds, replay/rate handling and replicated
    privacy; cover item IDs, targets, quantities, damage and interactions.
  - [ ] **CR05 — Review player traversal and interaction.** Check input routing,
    movement, traces, range/line-of-sight gates and state/lifetime transitions.
  - [ ] **CR06 — Review inventory, equipment and progression.** Check capacities,
    transactions, crafting, upgrades, rewards and failure/rollback paths.
  - [ ] **CR07 — Review construction and camp systems.** Check placement,
    support, storage and direct-material Hammer payment; verify no portable
    construction items can enter grants, loot, recipes or restored inventory.
  - [ ] **CR08 — Review survival, combat and support magic.** Check damage,
    resources, status timing/stacking, death/recovery, authority and design limits.
  - [ ] **CR09 — Review creatures and population.** Check AI, navigation,
    abilities, spawning/despawning, caps, loot identity and restart behavior.
  - [ ] **CR10 — Review world generation and simulation.** Check determinism,
    seeds, terrain, biomes, water, streaming, weather, exposure and travel bounds.
  - [ ] **CR11 — Review persistence end to end.** Check ownership, serialization,
    malformed/incompatible rejection, atomicity, rollback and restart behavior;
    apply the unreleased no-migration policy and never rewrite user data.
  - [ ] **CR12 — Review UI and accessibility.** Check Home/gameplay transitions,
    input/focus/cursor ownership, settings apply/revert, HUD, menus, map, scaling,
    labels, empty/error states and exposure of private information.
  - [ ] **CR13 — Review catalogues and authored content.** Check every JSON and
    manifest reference, recipes/stats/attacks, icon mappings/provenance, maps,
    materials, meshes, textures, audio and Blueprint wiring against runtime use.
  - [ ] **CR14 — Review Editor tools and scripts.** Check commandlets, previews,
    import/export, validators, packaging helpers, argument/path handling,
    destructive operations, error reporting and Windows path-length limits.
  - [ ] **CR15 — Review configuration, plugins and automation definitions.**
    Check config defaults/overrides, authored plugin code and descriptors,
    dependency assumptions, secrets/logging and build/release definitions.
    Inspection does not authorize dependency installation or CI/release edits.
  - [ ] **CR16 — Review tests, documentation and all remaining files.** Check
    assertions against real contracts, missing regressions, stale commands,
    acceptance claims and provenance; reconcile every file outside CR02–CR15.
  - [ ] **CR17 — Review integration and performance risks.** Trace complete
    gather/craft/build/save/reload, combat/reward and menu/join/reconnect flows;
    inspect tick/allocation/streaming costs, race/lifetime issues and failure
    propagation. Cross-reference findings instead of duplicating them.
  - [ ] **CR18 — Close coverage and prioritize findings.** Reconcile the ledger
    against the current tree, re-review changed files and eliminate unexplained
    gaps. Give each confirmed finding a concrete unchecked repair task with
    acceptance criteria and dependencies under Review findings remediation, or
    an explicit user disposition. Confirm earlier children already queued their
    findings; CR18 reconciles the list rather than delaying the initial handoff.
  - [ ] **CR19 — Complete review acceptance.** Every in-scope file/batch has a
    recorded result, exclusions have reasons, all findings have dispositions,
    and a final report lists reviewed revision/local changes, coverage totals,
    evidence, unresolved defects and limits. Perform lightweight checks per
    batch; full implementation verification belongs to the remediation goal.
    Any inspection failure becomes a finding, never an in-review code repair.
    Review completion means coverage and triage complete; it does not imply
    unresolved defects are fixed or unperformed runtime checks passed. Leave
    this parent unchecked until these review acceptance criteria are satisfied.

- [ ] Review findings remediation — implement fixes recorded by CR01–CR19.
  - [ ] **RF01 — Prune stale character rain timers (P3; CR03-001b-02).**
    GameMode.cpp:435 adds weak-key timers; GameMode.h:610 retains them with no
    production pruning (only test-fixture removal). Repeated pawn replacement
    or reconnects grows the map until world teardown. After CR19, remove expired
    keys while preserving live rain accumulation; validate repeated destroyed
    characters/disconnects and unchanged Wet timing. Evidence and acceptance:
    `Temporary/Review/findings.md#rf01--prune-stale-per-character-rain-timers-p3`.
  - **Dependency:** Blocked until CR19 review acceptance is complete. Review
    completion queues work here and never starts a repair in the same run.
  - **Queue rules:** Review agents append concrete unchecked RF01, RF02, ...
    children as findings arise; do not invent defects or placeholder fixes.
    Each child includes finding/source CR ID, severity, affected files, trigger,
    evidence, proposed change, dependencies, acceptance criteria and focused
    validation. Order eligible repairs by severity and dependencies, retaining
    stable IDs. Record explicitly accepted/deferred findings with reasons.
  - **Implementation runs:** After review completion, complete the first
    unchecked, unblocked repair child, exactly one per run. Split oversized
    repairs before implementation; preserve repository permission boundaries.
    Perform applicable lightweight checks, update backlog/progress, commit
    only run-owned changes and stop. No review finding bypasses this queue.
  - **Final acceptance:** Once every queued repair is implemented or explicitly
    dispositioned, run the applicable milestone-final verification prescribed
    by docs/07-development-setup.md; record commands, results and limitations.
    Repair verification failures within this goal and rerun required checks.
    Keep this parent unchecked until required verification passes. If review
    finds no actionable defects, record that evidence and close this goal as
    having no repairs required, without claiming unperformed checks passed.

- [x] Repair cursor loss when returning from gameplay to Home (2026-10-30).
  - Pause-menu cleanup restores gameplay input only to its original controller
    while that controller is still current; never to Home after travel.
  - Lightweight checks passed; rebuilt round-trip confirmation remains SM15.

- [x] User-requested opening-screen and existing-menu visual refresh (2026-10-30).
  - Original static northern landscape, centered title/actions, framed existing
    player/world/join/save/credits/exit branches, and paired world/join choices.
  - Existing Options and session/save behavior retained. Lightweight checks pass;
    Unreal compilation and rendered acceptance remain deferred under SM15.

- [x] Repair main-menu startup cursor/input capture and leave visible menu cursors unlocked in windowed mode (2026-10-30).
  - Disable launch capture; initialize the menu controller in UI-only mode with
    a visible cursor and no mouse lock; keep Options clicks from hiding it.
  - Lightweight checks passed; rebuilt runtime confirmation remains under SM15.

- [x] Minimap notification queue.
  - [x] Research player-facing actions and implement a bounded owner-local queue
    beneath the minimap with icons for item gains, skill levels, discoveries,
    and support results; combat outcomes stay outside the queue.
  - [x] Use imported item/status textures for every notice instead of vector
    glyphs; hide an unavailable texture instead of drawing a vector fallback.
- [x] Four-view creature visual cleanup, one species per increment.
  - [x] Boar: replace detached spike legs with connected rounded limbs and
    clarify the snout, eyes, ears, and short bristle ridge.
  - [x] Badger: join the narrow head and legs to its low body and make the
    facial stripe visible.
  - [x] Deer: connect the raised neck, head, antlers, and legs to its body.
  - [x] Rat: replace the flat tail with a curved, tapered 3D tail.
  - [x] Moon Bear: connect its head and feet to the body with a heavier bear
    silhouette.
  - [x] Death Adder: lower and connect the head to the coiled body.
  - [x] Viper: lower and connect the head to the coiled body.
  - [x] Giant Snake: lower and connect the head to the coiled body.
  - [x] Wolf: connect the head, tail, and feet to the torso.
  - [x] Lynx: connect the head, tail, and feet to the torso.
  - [x] Polar Bear: give the head and short legs a connected, heavier shape.
  - [x] Horned Goat: connect the raised head and legs to the body.
  - [x] Reef Maw: replace the flat fins, tail, and jaw with rounded 3D forms.
  - [x] Mireling: replace flat arm, leg, and crown geometry with rounded forms.
  - [x] Mosquito: replace the triangular beak with a pointed 3D needle.
  - [x] Thunder Sentinel: align its paired limbs and chest vein across its body.
  - [x] Snakes: replace segmented Viper, Giant Snake, and Death Adder coils
    with continuous tapered body meshes.
  - [x] Polar Bear follow-up: extend upper legs into the belly and torso.
  - [x] Horned Goat follow-up: join upper legs to the torso.
  - [x] Deer follow-up: join upper legs to the belly and chest.

- [ ] Pre-game start menu — implement [docs/startmenu.md](docs/startmenu.md).
  - **Instructions for every run:**
    - Read the plan and current backlog/progress before changing files.
    - Complete the first unchecked, unblocked SM task in order; exactly one per run.
    - Split a task before implementation if it cannot fit one ordinary run.
    - Perform lightweight checks, update backlog/progress, commit only run-owned changes, then stop.
    - Keep normal startup unchanged until SM14. Add no new services, portable character saves or gameplay save schemas.
    - Leave the parent unchecked until SM15 acceptance succeeds.
  - [x] **SM01 — Expose existing world and session information.**
    - **Implement:** Map world enumeration, creation, travel, authentication and logout APIs to the plan. Add a read-only world-descriptor adapter.
    - **Complete when:** Narrow adapter checks confirm supported metadata is returned accurately and unsupported metadata is explicitly identified.
  - [x] **SM02 — Add the menu flow coordinator.**
    - **Implement:** Add isolated front-end states and a request token for each launch/connection attempt. Keep the default startup map unchanged.
    - **Complete when:** Duplicate submissions and callbacks from canceled or superseded requests cannot start travel or change the active screen.
  - [x] **SM03 — Isolate the menu from gameplay.**
    - **Implement:** Add a lightweight menu context. Prevent gameplay HUD and player-pawn creation in that context.
    - **Complete when:** The menu context has no gameplay HUD/pawn and the existing gameplay context still creates them normally.
  - [x] **SM04 — Build the Home screen.**
    - **Implement:** Use the existing theme; add Play, Options, Save information, Credits and Exit in that order, plus the build-version footer.
    - **Complete when:** Play receives initial focus and the layout fits both resolutions and text-scale cases specified in the plan.
  - [x] **SM05 — Connect the existing Options screen.**
    - **Implement:** Reuse the existing Options presenter from Home and gameplay. Retain one settings store and one input owner.
    - **Complete when:** Closing Options returns focus to its caller and settings retain their existing apply/revert behavior.
  - [x] **SM06 — Show the current player.**
    - **Implement:** Add the read-only current-player step, Continue/Back actions and authentication-failure explanation.
    - **Complete when:** Friendly labels reveal no authentication keys and cannot select or override saved-player ownership.
  - [x] **SM07 — Add world selection and details.**
    - **Implement:** Bind Your worlds and World details to validated descriptors. Add empty/unsupported states, planned sorting and read-only world rules.
    - **Complete when:** Selecting a world shows its accurate identity/status without launching it or rewriting saves.
  - [x] **SM08 — Connect New world.**
    - [x] Add strict decimal seed parsing for the full runtime `uint64` range; reject empty, non-decimal, trailing text, and overflow without wraparound.
    - [x] Classify listed ocean-save revisions against the current generator revision through the read-only adapter; retain unknown and conflicting states without claiming payload compatibility.
    - [x] Classify duplicate seed/revision outcomes; require confirmation only for an exact match and hold uncertain existing seeds for review.
    - [x] Add a themed confirmation panel that opens only for an exact listed seed/revision match; Cancel returns to the world list and Confirm revalidates the match.
    - [x] Connect New world seed entry to the confirmation; after confirmation open existing details without writing a save.
    - [x] Connect New world to a validated server-owned world lifecycle API.
    - **Implement:** Use the existing seed and world-lifecycle API. Validate the full runtime seed range and handle duplicate seed/revision identities.
    - **Complete when:** Invalid seeds are rejected and duplicate identities select the existing world only after confirmation, without overwriting it.
  - [x] **SM09 — Connect solo play.**
    - **Implement:** Route Play solo through the validated launch adapter and protect against repeated submissions.
    - **Complete when:** Server-owned setup receives the selected seed/revision and starts one unadvertised solo session.
  - [x] **SM10 — Connect co-op hosting.**
    - **Implement:** Use the existing listen-server backend with a four-player cap and explain that the session ends when the host leaves.
    - **Complete when:** Address/visibility information is truthful and password/discovery controls appear only when enforced by the backend.
  - [x] **SM11 — Connect joining by address.**
    - **Implement:** Add the plan's structured address parser and route accepted endpoints through validated client travel.
    - **Complete when:** Supported address/port forms work and malformed inputs, unsupported forms and travel-option injection are rejected.
  - [x] **SM12 — Handle loading, cancellation and failures.**
    - **Implement:** Add loading states, connection timeout/cancel, stale-callback protection and return to the originating panel on failure.
    - **Complete when:** The plan's timing rules hold; retry preserves input, canceled requests cannot travel and failed attempts do not mutate saves.
  - [x] **SM13 — Add the remaining Home branches.**
    - **Implement:** Add read-only Save information, existing project credits and Exit confirmation.
    - **Complete when:** Save status wording matches actual persistence support, Back restores focus and Exit confirmation initially focuses Cancel.
  - [ ] **SM14 — Enable normal startup and return from play.**
    - **Implement:** Wait for save completion, tear down the owned session and return to Home. Route normal packaged startup into Home.
    - **Complete when:** Explicit server/gameplay startup overrides still work and repeated play/Home round trips leave no duplicate HUD, delegates or input handlers.
    - [x] **SM14.1 — Author the dedicated Home map.**
      - **Implement:** Create an empty `/Game/Kalmala/Maps/Menu/L_Menu` map with `AKalmalaMenuGameMode`; keep packaged startup routing unchanged until return is connected.
      - **Complete when:** The saved map loads with the menu GameMode and no gameplay actors.
    - [x] **SM14.2 — Return from solo gameplay after a successful save.**
      - **Implement:** Add a gameplay return action that persists the local player's state on the authoritative GameMode before travelling to Home; re-enter Home flow after travel.
      - **Complete when:** Failed persistence keeps the player in gameplay, and successful persistence returns to one active Home screen.
    - [x] **SM14.3 — Return networked players through the session owner.**
      - **Implement:** Validate each client's return request and complete server persistence before the client leaves; when the listen host leaves, persist players and tear down the owned session.
      - **Complete when:** Clients arrive in local Home after accepted departure, and a host departure ends its listen session without bypassing logout saves.
    - [x] **SM14.3.1 — Return a network client after server persistence.**
      - **Implement:** Route the owning client's gameplay return request to the server; validate its controller and pawn, persist the existing player record, and send only that client to local Home after success.
      - **Complete when:** A failed save keeps the client in the session; an accepted save returns that client to one local Home flow.
    - [x] **SM14.3.2 — Return the listen host through session teardown.**
      - **Implement:** Persist every active player's existing record before the host closes its owned listen session; return remote clients and host to local Home while retaining logout-save behavior.
      - **Complete when:** The host no longer owns a listen session after return, and connected players arrive in local Home only after their saves complete.
    - [x] **SM14.4 — Clean up repeated gameplay/Home round trips.**
      - **Implement:** Release stale gameplay HUD, menu widgets, delegates, timers, request tokens and input bindings as travel changes contexts.
      - **Complete when:** Repeated solo and listen-host round trips recreate one Home presentation and one input/delegate owner per local player.
      - [x] **SM14.4.1 — Reset world-scoped weather presentation on Home entry.**
        - **Implement:** Clear transient regional fog and the local weather material response when the dedicated menu controller becomes active.
        - **Complete when:** The next gameplay world begins without the previous world's weather response or fog state.
      - [x] **SM14.4.2 — Reconcile gameplay HUD and input owners across controller changes.**
        - **Implement:** Release stale local gameplay widgets, delegates, timers and input bindings when gameplay controllers are replaced by menu controllers, then create one owner for the new gameplay controller.
        - **Complete when:** Each local gameplay HUD/input owner has one current controller and no presentation or binding remains attached to the prior controller.
      - [x] **SM14.4.3 — Rebuild Home presentation and retire stale menu callbacks after travel.**
        - **Implement:** Release old Home widgets, delegates and request watchers/tokens on outbound travel, then recreate one Home flow for the returning menu controller.
        - **Complete when:** Repeated solo and listen-host returns produce one Home presentation and callbacks from prior worlds cannot affect the new flow.
    - [x] **SM14.5 — Route normal packaged startup to Home.**
      - **Implement:** Use the dedicated menu map as the packaged default while preserving the editor startup map and explicit gameplay/server map and GameMode overrides.
      - **Complete when:** A packaged launch opens Home and explicit gameplay/server startup still opens the requested gameplay context.
  - [ ] **SM15 — Verify and finalize the complete menu.**
    - **Run:** The integrated acceptance in docs/startmenu.md and applicable builds, automation and packaged-session checks in docs/07-development-setup.md.
    - **Cover:** Fresh launch, world creation/selection, solo return/restart, listen host/client, dedicated join, failures/cancel, save safety, focus and text scaling.
    - **Repair and record:** Fix feature-related defects; record actual commands, results and reruns; finalize affected workflow documentation.
    - **Complete when:** Required acceptance succeeds. Only then check this task and the parent; report any genuine blocker without claiming completion.

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
