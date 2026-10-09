# Roadmap

Each milestone must leave the project playable. Do not begin a later milestone while the earlier one fails its acceptance criteria.

## M0 — Bootstrap

Create the UE5 C++ project, source-control setup, module skeleton, default map, basic input, CI/build notes, and developer onboarding.

**Accept:** editor opens, a packaged development build launches, and a dedicated-server target can compile (or its exact setup blocker is documented).

## M1 — Networked traversal and interaction

Implement replicated character movement, camera, interaction trace, interactable interface, and a two-player test map.

**Accept:** host/client can join, move, and reliably interact with the same object; invalid client interaction is rejected server-side.

## M2 — Survival camp loop

Add server-owned inventory, harvesting, campfire warmth, basic crafting, placement preview, and three construction pieces.

**Accept:** two players can gather, craft, build, save/load a camp, and observe matching state after reconnect.

## M3 — Elemental world prototype

Build the bounded interaction grid, fire/wetness/temperature rules, and client visual feedback.

**Accept:** a player who stands in rain receives the **Wet** status effect. Moving to a camp and standing near a lit campfire removes the effect, with clear client-side visual feedback for both state changes.

## M4 — Combat and support magic

Add combat attributes, damage execution, the Mireling, boar, and deer creature archetypes, open-world points of interest, and scroll-learned support magic. Implement Mending, Hearth Shield, Bear's Vigor, and Deer Call; place scroll discoveries across the biome and make one a boss reward.

**Accept:** focused server-authority, persistence, and peer regressions cover the
three creature archetypes, optional discoveries, and all four support effects.
No magic effect directly damages an enemy. A rendered two-player acceptance
scenario is not required by the revised roadmap scope.

## M5 — Vertical-slice finish

The world-generation track places **Phase 7 — Coherent biome generation and hydrology** before **Phase 8 — Ocean and long-distance travel**. Its current user-directed revision-7 rework samples a separately seeded land/water master atlas through a game-seeded crop and rotation, then applies the specified hard origin-distance biome limits with Meadows/Mountains fallback. It retains existing regional signals, enclosed lakes, large rivers and the 16 km radius. Revision 8 provides opt-in debug streams; revisions 1–6 remain compatible. Detailed contracts and acceptance checks are in `docs/08-world-generation-and-biomes.md` and `BACKLOG.md`. This does not expand the vertical-slice creature, platform, or online-service scope.

Add original art/audio pass, tutorial beats, settings/accessibility, performance pass, balance, regression tests, packaging, and a dedicated-server playtest.

**Accept:** a new player can complete the documented 20–30 minute co-op loop
without developer tools. If the active validation environment cannot expose a
targetable native Windows game surface after the documented retry threshold,
the M5 queue may instead close through the explicit external-surface skip in
`BACKLOG.md`. A skip is not player-visible acceptance: it defers normal
packaged co-op, input, audio, reconnect, and persistence observation to M6.

## M6 — Production hardening and supported-session validation

Start only after M5's vertical-slice acceptance passes or its documented
external-surface skip closes the M5 queue. Turn the accepted M5 loop and any
explicitly deferred player-visible checks into a reliable release candidate
without adding new gameplay content, changing saved-data schemas, expanding
online services, or changing the current PC solo/listen-server co-op scope.

M6 has four ordered goals:

1. **Close acceptance findings.** Fix issues found during the final fresh-player
   20–30 minute packaged co-op run, or first restore that run when M5 used the
   external-surface skip. Convert each finding into a focused regression or an
   explicit documented non-goal. Prioritize crash and hang triage, reconnect
   and late-join reliability, save-identity safety, input, accessibility, and
   readable non-audio feedback.
2. **Re-run the complete release suite.** From clean temporary user
   directories, run the automated, rendered, current-generator, authority,
   persistence, reconnect, and performance checks twice. Preserve the logs,
   screenshots, package metadata, and known-limit record.
3. **Validate the Windows Development package.** Produce the accepted package,
   smoke-launch it, and verify that a fresh player can complete the documented
   loop using normal player actions only. Keep actor, memory, worker, raster,
   startup, replication, and save budgets within their recorded limits.
4. **Attempt dedicated-server validation only when unblocked.** If a
   server-capable UE 5.8 source build or distribution becomes available,
   compile the retained `KalmalaServer` target and run a bounded two-to-four-
   player playtest covering join, movement, interaction, weather and camp
   recovery, creatures, support effects, late join, reconnect, and sparse
   persistence. If the installed Launcher engine remains the only available
   distribution, retain the documented blocker and do not invent a replacement
   service.

**M6 multiplayer boundary:** the server continues to own world generation,
combat, support, discovery, rewards, persistence, and all accepted outcomes.
Clients provide intent only, and the hardening work must not add client-
selected targets, damage, timing, rewards, hidden-content queries, or save
values.

**M6 accept:** the supported Windows Development package passes the complete
release suite twice from clean temporary user directories; a fresh player can
complete the 20–30 minute co-op loop without developer tools; relevant peer
state, reconnect, and persistence remain correct; documented performance
budgets remain green; and the only remaining limitations are explicitly
recorded. Dedicated-server acceptance is conditional on the documented UE5.8
engine capability.

## M7 — Content update: survival progression and readable gameplay UI

Start only after M6's release-candidate gates pass. Use the existing inventory,
harvest-node, camp-crafting, construction, status, and local HUD foundations to
add the survival fundamentals still missing from the vertical slice. The
comparison target is the survival-game category, not a source of names, art,
lore, recipes, balance, or copied interface design; all Kalmala content remains
original and route-free.

M7 has nine ordered goals:

1. **Add server-owned skill progression.** Define a small allowlisted set of
   original skills for gathering, woodcutting, mining, crafting, cooking, and
   survival. Award bounded experience only from accepted server actions, derive
   levels and unlocks on the server, and replicate only the owning player's
   detailed progression plus the presentation state relevant to other peers.
   Clients cannot submit experience, level, unlock, multiplier, or reward
   values.
2. **Establish biome-specific material and creature identity.** Give each
   completed biome a bounded original material family and a creature niche.
   Materials may come from biome trees, ore patches, harvest plants, creature
   drops, elite or boss rewards, loot chests, hidden treasures, or shipwreck
   discoveries, but every source must be a server-selected catalogue entry with
   a stable spatial or encounter identity. Start with a small authored set:
   one reliable gathering source, one creature, and one rarer discovery source
   per first-wave biome rather than an unbounded loot table. Creatures must add
   readable behaviour or ecological pressure, not only act as resource bags;
   bosses and rare caches remain optional discoveries and never become a
   designed route, mandatory gate, or copied fantasy reference.

   The first identity slice uses the server-owned
   `FKalmalaBiomeContentContract` catalogue for Meadows, Shimmering Lakes,
   Elderwood, Mossy Mire, Freezing Tundra, and Thunder Mountains. Each entry
   has one stable gathering-source ID, one creature-niche ID, and one optional
   rare-discovery-source ID. Generated harvest and wildlife descriptors carry
   only the matching server-selected ID; their existing spatial/seed sparse
   identities remain unchanged. Ocean receives no land-content entry, and the
   existing point-of-interest descriptor uses the optional rare-source ID.
   Valid gathering sources now materialize an original, collision-free
   procedural harvest presentation on the relevant server-owned actor. The
   presentation is selected from the replicated source ID on every peer; it
   does not add a tool, reward, depletion, or save field, and the existing
   legacy reward path remains unchanged until the tool lifecycle slice.
   Valid point-of-interest rare sources now materialize a small original,
   collision-free procedural discovery presentation selected from the
   replicated presentation identity. The source remains optional and
   route-free; discovery claiming, scroll rewards, and persistence remain
   unchanged.
   Each niche now also selects a bounded server-owned flee response and sparse
   original presentation accent from that ID; the existing archetype, damage,
   reward, and sparse identity paths remain unchanged. Tool validation,
   material rewards, and creature-specific loot remain later M7 work. Rare
   sources and boss scrolls remain optional discoveries:
   they create no route, mandatory gate, or client-selected location.
3. **Complete tool-based gathering and tool lifecycle.** Turn suitable
   generated trees, rocks, ore patches, and harvest resources into original
   server-validated woodcutting, mining, and gathering interactions. Tools have
   bounded server-owned durability; accepted work spends durability, zero
   condition blocks the action, and repair at an accepted repair station or
   workbench consumes validated repair materials. The server selects the
   resource from the authoritative trace and range, validates tool, durability,
   skill, and node state, applies bounded health or use state, awards
   catalogue-validated materials, and persists only the necessary sparse
   depletion facts. Clients provide intent and tool/action selection only; they
   cannot choose a node, yield, damage, durability, repair result, or reward.
4. **Add optional food, preparation, and nutrition choices.** Add original
   edible items and recipes that can be gathered, prepared, and consumed through
   the existing inventory/crafting transactions. Add a first camp-processing
   set such as a cooking rack over a fire, a heat-safe kettle or cauldron
   analogue, and a drying or smoking frame; each station must have explicit
   fuel, heat, access, batch, and failure rules. Eating grants finite, readable
    stat modifiers such as stamina capacity, recovery, movement comfort, or
    exposure resilience. Effects must define server-owned stacking and
    replacement behavior, expire on authoritative time, and reject invalid or
    duplicate consumption. Food should create
   preparation choices without making starvation or a mandatory food route a
   hard travel gate in the first M7 slice.
5. **Broaden the crafting and camp progression.** Extend the existing recipe
   catalogue with a small first tier of tools, gathering implements, repair
   materials, cooking and preservation recipes, storage/camp improvements, and
   skill-gated recipes. Reuse atomic inventory exchanges, station validation,
   stack ceilings, batch limits, and owner-only result feedback. Each recipe
   must have explicit ingredient, station, unlock, failure, repair, and
   accessibility text rather than relying on colour. Processing stations must
   be useful preparation choices, not parallel inventory or fire authorities.
   The first progression hook assigns each accepted prepared-food recipe one
   fixed 10-point Cooking award after its inventory exchange succeeds; batch
   size does not multiply experience, and rejected requests award none. The
   award remains transient until a versioned progression save contract exists.
   Smoke-frame recipes are the first optional Cooking level-2 unlock; the
   crafting panel shows the requirement and current level, and the server
   checks the owner's progression before attempting station or pack access.
   The first tool replacement recipe rebuilds a Field Hatchet only at zero
   condition through a visible workbench and atomic two-Wood/two-Stone/one-
   Fibre payment; it restores the existing owner-only condition and awards a
   fixed 10 Crafting experience only after payment succeeds.
   The next first-tier replacement rebuilds a Stone Pick only at zero
   condition, for two Wood, three Stone, and one Fibre at a visible same-world
   workbench; accepted payment restores its existing owner-only condition and
   awards the same fixed transient Crafting experience.
   A Reed Knife replacement is also available only at zero condition, for one
   Wood, one Stone, and two Fibre at a visible same-world workbench; accepted
   payment restores its existing owner-only condition and awards the same
   fixed transient Crafting experience.
6. **Add biome-specific hazards and active-weather pressure.** Extend the
   server-owned environmental presentation and exposure rules with readable
   fog, rain, storms, heat, cold, and a clearly marked highly-active-weather
   state. Hazards may alter visibility, wetness, warmth, stamina recovery,
   movement comfort, fire safety, or creature pressure, but must remain bounded,
   reversible, and counterable by shelter, fire, food, tools, or timing rather
   than becoming biome damage walls or mandatory travel gates. The server
   selects weather and hazard intensity from the authoritative world state;
   clients never submit weather, exposure, hazard, or mitigation outcomes. The
   first bounded outcome reduces stamina recovery by at most 20% below 40
   warmth and returns to normal as shelter or a lit fire restores warmth. A second bounded outcome shortens the existing exposed-rain Wet trigger from 10 to 7.5 uninterrupted seconds only during Highly Active weather; roofs reset exposure and lit-fire recovery remains available. A third bounded outcome increases exposed campfire fuel wetting by up to 25% in severe rain/wind storms, with accepted roofs and windbreaks suppressing their server-sampled weather inputs.
   The replicated activity tier is now shown to each local player with a distinct
   circle, diamond, or triangle marker and explicit tier text below the minimap;
   local text-scale and contrast preferences apply without changing gameplay.
7. **Build the survival HUD and GUI pass.** Add a persistent local status strip
   with an original icon or shape marker, name, category, readable remaining
   timer, intensity or stack count, source, and recovery hint for `Wet`, food
   modifiers, exposure hazards, and other active effects. Add a compact weather
   activity indicator that distinguishes calm, active, and highly active states
   without colour alone. Extend inventory, crafting, station, and equipment
   views with food details, active modifiers, skill progress, tool condition,
   repair cost, recipe unlock state, ingredient counts, station requirements,
   creature/material provenance, and clear unavailable reasons. Status timers
   must read replicated server state rather than count down independently, and
   local UI changes must not mutate gameplay or send new client-authored
   outcomes. Keyboard/controller focus, text scale, contrast, and
   text-plus-marker feedback remain required.
8. **Verify biome discovery and recovery choices.** A fresh player should be
   able to gather a first material, craft or obtain a tool, cut wood or mine
   ore, repair the tool, prepare food at camp, choose a temporary benefit,
   recognize an active-weather hazard, and return with a creature or hidden
   discovery while weather, inventory, and status feedback remain
   understandable. Verify the cross-system outcomes with deterministic,
   fresh-profile host/client fixtures and local presentation automations; this
   M7 gate does not require native computer use or an unscripted single-session
   walkthrough. The M6 player-visible session remains preferred and its
   limitation must stay documented; item 9 defines the headless M7 regression
   path. Verify same-seed host/client resource and creature
   agreement, rejected client mutations, owner-only progression and inventory
   privacy, rare-loot visibility rules, status presentation, reconnect
   behavior, and bounded actor, memory, replication, and save costs.
9. **Retain the M6 release-candidate regression.** After the content pass,
   re-run traversal, camp, combat, support, weather, construction, storage,
   persistence, and minimap checks. If no targetable native game window is
   available, use the documented headless host/client acceptance suite after
   these regression slices and clean-profile package smoke pass. This verifies
   process-level behavior; it does not claim that physical input or the native
   20–30-minute player walkthrough passed.

**M7 persistence gate:** skill experience, learned recipes, tool condition,
food effects, food inventory, biome resource depletion, defeated creature
states, opened chests, claimed treasures, shipwreck loot, or boss rewards that
must survive reconnect or restart require a versioned save contract,
identity/world matching, migration policy, and focused round-trip/rejection
tests before implementation. The approved first gate is the independent
`UKalmalaM7PersistenceSaveGame` schema 1 contract: it records the immutable
world seed, current generator revision 7, explicit world/player scope, and at
most 256 server-selected sparse resource, creature, or discovery identities.
Current schema 1 loads only with an exact valid identity match; unversioned
schema 0 requires an explicit migration, and future schemas are rejected.
Malformed, duplicate, over-cap, or path-like sparse IDs fail without mutation.
The gate is currently exercised through memory serialization only and does not
extend the existing population, construction, storage, discovery, or local UI
save schemas. Until later contracts are approved, development fixtures may use
transient server-owned state but must not silently extend those schemas.

**M7 multiplayer boundary:** the server owns skill awards, resource identity,
creature behaviour and defeat, tool validation, durability, repair outcomes,
node depletion, recipe unlocks, station processing, ingredient costs, food
effects, stat changes, weather and hazard intensity, timers, loot selection,
and rewards. Clients receive only the state needed for their own controls and
presentation or ordinary relevant world feedback; private inventory,
progression, and detailed loot remain owner-scoped.

The first M7 progression increment defines a transient, server-owned ledger for
Gathering, Woodcutting, Mining, Crafting, Cooking, and Survival. An accepted
server action may award at most 25 experience, total experience is capped at
1,000 across levels 1-10, and unlock tiers are derived at levels 2, 5, and 10.
`UKalmalaSkillProgressionComponent` attaches that ledger to each replicated
player character. Detailed skill state is replicated only to the owning player;
relevant peers receive only a derived highest-level/unlock presentation badge.
There is no client RPC or setter for experience, level, unlock, multiplier, or
reward values, and persistence remains behind the approved M7 save gate.

**M7 accept:** the integrated biome gathering, creature, crafting, repair,
food-processing, progression, hazard, and HUD loop is playable without
developer commands; all outcomes remain authoritative and recoverable; rare
discoveries are optional and original; status icons, timers, intensity, and
recovery guidance are readable without colour; matching peers observe the
permitted state; rejected requests leave inventory, skills, tools, resources,
effects, hazards, loot, and saves unchanged; and the M6 host/client regression
remains green through the supported native session or documented headless
acceptance path. Headless completion leaves physical input and the player-
visible packaged walkthrough explicitly unverified.


**## M8 — Ocean and long-distance travel**

Start only after M7 acceptance passes and the existing Phase 7 coherent biome generation and hydrology contract remains green. Implement the deferred **Phase 8 — Ocean and long-distance travel** track without replacing the current land-generation, authority, persistence, or PC solo/listen-server foundations.

M8 has six ordered goals:

1. **Authoritative ocean traversal.** Add an original player-controlled watercraft or equivalent long-distance travel system with server-owned movement state, boarding, disembarking, occupancy, damage or disable state where applicable, and bounded replication.

2. **Coast and launch readability.** Ensure generated coastlines expose usable launch/landing opportunities without requiring handcrafted routes. Water access, shallow hazards, collision, and invalid embark locations must be readable without developer tools.

3. **Ocean weather and navigation pressure.** Extend the existing weather/hazard model to open water with bounded wind, rain, visibility, wave, exposure, or stamina pressure that remains reversible and readable. Weather must complicate travel without becoming an unavoidable damage wall.

4. **Optional ocean discoveries.** Add a small original catalogue of shipwreck, shoal, islet, or other sea discoveries with stable server-selected identities. Rewards remain optional, sparse, and route-free.

5. **Travel-safe persistence and reconnect.** Define the minimum versioned state required to restore accepted vessel, passenger, discovery, and sparse ocean facts without duplicating players, cargo, rewards, or world identities.

6. **Long-distance peer validation.** Exercise embark, travel, weather pressure, discovery, disembark, late join, reconnect, and world-origin/streaming transitions across representative long-distance journeys while holding actor, memory, replication, save, and frame-time budgets.

**M8 persistence gate:** vessel or travel state may persist only through a versioned identity-safe contract with explicit world/player scope, bounded cargo/state, migration policy, and round-trip/rejection tests. Ocean discoveries use stable sparse identities and cannot be claimed twice through reconnect or load.

**M8 multiplayer boundary:** the server owns vessel state, accepted movement outcomes, occupancy, ocean weather, discovery identity, rewards, damage/disable outcomes, and persisted travel facts. Clients provide steering, interaction, and action intent only.

**M8 accept:** two players can launch, travel a meaningful ocean distance, survive or mitigate active ocean weather, find an optional sea discovery, safely disembark elsewhere through a validated exit (a qualifying deep-water exit is accepted; generated dry shore is not required under the 2026-09-26 scope decision), reconnect without duplicated or lost accepted state, and remain within documented performance and replication budgets.

**## M9 — Expanded biome content and encounter depth**

Start only after M8 acceptance passes. Deepen the existing biomes and progression systems with a bounded second content wave while preserving the route-free world, optional discoveries, original content requirement, and server-owned outcome model.

M9 has five ordered goals:

1. **Second-wave biome identity.** Expand each supported biome with a small additional set of original gathering sources and loot.
   - **Example new material sources and harvests:**
     - Meadows — birch tree (the generated tree presentation exists; a harvest interaction would be new).
       - Lightwood from the trunk, requires an bronze axe or better to harvest.    
     - Elderwood — ironheart tree (new tree and harvest source).
       - Densewood from the trunk, requires an iron axe or better to harvest.
     - Mossy Mire — peat bank (new harvest source).
       - Peat amber from hardened seams in the bank.
     - Freezing Tundra — salt deposits (new harvest source).
       - Frost salt, new mining resource
   - Treat these as candidate directions for a curated catalogue, not a locked list. Every approved gameplay entry needs a canonical ID, server-derived source or placement rule, bounded activation budget, and existing-item reward or recipe mapping where applicable. Ocean content must use M8 travel interactions; placement, depletion, hazards, claims, and rewards remain server-owned and deterministic. Do not add open-ended drop tables or client-authored content data.

2. **Camp and equipment progression.** Add level upgrades for tools and matching workstation progression. Use a carried, wielded Construction Hammer to open the build menu and place constructions directly from server-validated raw-material costs, then extend camp utility through the existing inventory, crafting, construction, station, and accessibility contracts.
   - The menu starts with floors, walls, and roofs, built directly from Wood and Fibre without producing kit items. Keep their existing internal construction identities and save records stable. Migrate the remaining camp kits through the same menu and server transaction in later bounded increments.
   - **Tool level and workstation matching:**
     - Give each carried tool an authored level path. Tool level is distinct from the player's skill level and remains server-owned, with detailed tool state visible only to its owner.
     - Extend the existing owner-only tool-condition records into the carried-tool inventory shown to that owner; the current first-wave tools are condition fields rather than pack stacks. The server, not the client, supplies the eligible tool list to repair and upgrade actions.
     - Crafting or upgrading a tool to level N requires the server-selected, same-world Workbench or Forge to have effective level N. The level match is checked at the accepted craft or upgrade; the tool remains usable away from that station afterward.
     - Use the existing server-owned skill ledger and its level-5 unlock band for tier-two recipes. The server validates the current tool level, requested next level, skill, station, materials, condition, and any required output slot as one transaction; clients do not choose the result or author either level.
   - **Buildable workstation upgrades:**
     - The existing Joiner's Workbench has no level today; M9 gives it a level-1 base. The Forge and Grinding Stone are new buildable objects. A Forge starts at level 1, and a nearby, server-accepted, station-compatible buildable attachment adds +1 to a Workbench's or Forge's effective level; the server derives the result from placed objects in the same world and the existing validated station-use range.
     - Example new attachment objects are a Workbench tool rack or vise and a Forge anvil. Each must be a real paid construction object with a catalogue identity, placement validation, and readable recipe and station feedback.
     - Bound effective levels to the authored progression tiers. A missing, distant, wrong-station, unbuilt, or invalidated attachment leaves station level and tool state unchanged.
   - **Free tool repair:**
     - Repair costs no materials or currency and restores damaged tools, including zero-condition tools, to their defined maximum condition. Repair itself awards no crafting experience.
     - At a Workbench or Forge, an owner-local GUI action repairs the selected damaged carried tool; the server reads its current condition and validates the station and owner state.
     - Add a Grinding Stone as a buildable camp utility. Its in-world `Repair All` action repairs every eligible damaged tool in the server-owned carried-tool list after a same-world range check, without accepting a client-supplied tool list or condition value.
     - Play a short, tool-appropriate sharpening animation during the Grinding Stone action, such as sharpening the carried axe or sword. The animation is presentation only; the server owns acceptance and resulting condition.
     - This free-repair rule supersedes M7's material-paid repair path and zero-condition replacement recipes. Broken tools use the same free repair action; material costs remain for tool-level upgrades and workstation attachments.
   - **Other optional camp and equipment examples:** a storm-rated shelter piece or an insulated travel wrap. Extend existing server-owned construction, status, and inventory paths.
   - Show tool level, skill requirement, matching station level, material cost for upgrades, repair availability, and rejection reasons in readable text as well as colour or icons. Keep upgrades optional and recoverable, and award progression only after an accepted server transaction.
   - Keep detailed tool and skill state owner-scoped. Do not persist new tool levels or workstation attachment progression until the M9 persistence goal defines and verifies the versioned migration contract; extend existing tool-condition and construction authority rather than adding parallel subsystems.

3. **Exploration rewards without quest routing.** Add optional clue, landmark, treasure, boss, or environmental-discovery structures that reward observation and travel without turning the world into a prescribed quest chain. The first two land-discovery candidates and their reward boundaries are defined in `29-m9-exploration-rewards.md`; server placement and claim behavior are implemented, with durable claims still gated by the M9 persistence contract.

4. **Persistence schema expansion.** Version and migrate any newly persistent progression, creature, discovery, storage, equipment, or encounter state before enabling it in normal saves. Preserve exact identity/world matching and bounded sparse records. The accepted M9 tool, station, land-discovery migration boundary is defined in `docs/30-m9-persistence-migration.md`; schema-2 writes remain gated on its round-trip and rejection coverage.

5. **Cross-system regression.** Verify the second content wave against land/ocean travel, weather, survival HUD, crafting, combat, support effects, construction, reconnect, persistence, and the retained supported-session loop.

**M9 content boundary:** expansion stays within the established PC solo/listen-server co-op product scope unless a separate roadmap revision explicitly changes platforms or online services. New systems should extend existing contracts rather than introduce parallel inventory, combat, weather, progression, or persistence authorities.

**M9 multiplayer boundary:** all encounter selection, creature state, crafting/progression outcomes, construction material payment and placement, loot, hazards, rewards, and persistent facts remain server-owned. Clients submit a canonical buildable identity only; the server derives costs, terrain transform, validation and result. Private inventory and progression remain owner-scoped; peers receive only relevant world and presentation state.

**M9 accept:** the second content wave is playable across the supported biomes and ocean travel path without developer commands; new encounters and progression remain optional and readable; rejected client mutations leave state unchanged; versioned saves round-trip and migrate as documented; and the M8 travel loop plus M6 supported-session loop remain green.

**## M10 — Release completion and launch validation**

Start only after M9 acceptance passes. Freeze feature scope and turn the complete supported game into the launch candidate through content lock, compatibility validation, performance hardening, accessibility review, packaging, and release evidence rather than adding another gameplay layer.

M10 has six ordered goals:

1. **Feature and content freeze.** Close or defer remaining backlog items, lock accepted gameplay and save schemas, and require any post-freeze change to include a focused regression and explicit release rationale. The accepted baseline and change rule are recorded in `docs/31-m10-scope-freeze.md`.

2. **Full clean-profile validation.** Run the complete automated, rendered, authority, persistence, reconnect, world-generation, land-travel, ocean-travel, combat, support, construction, crafting, progression, weather, HUD, accessibility, and performance suite from clean temporary user directories.

3. **Save compatibility and recovery.** Validate current-version saves, all approved migrations, corrupt/malformed rejection, world/player identity mismatch handling, backup/recovery behaviour where supported, and no partial mutation on failed loads.

4. **Performance and scalability closeout.** Reconfirm startup, frame-time, actor, memory, streaming, worker, raster, replication, package-size, and save budgets across the documented representative hardware/profile matrix. Record any remaining limitations explicitly.

5. **Supported package and multiplayer playtest.** Produce the release candidate package and run the documented fresh-player co-op loop plus extended progression and long-distance travel using normal player actions only. Attempt the retained dedicated-server validation only when the documented engine capability is available.

6. **Release evidence and known limits.** Archive build metadata, test results, logs, screenshots, migration notes, accessibility checks, known limitations, and reproducible release steps so the candidate can be rebuilt and audited.

**M10 release boundary:** no new gameplay system, platform, online service, save schema, or authority model is introduced after feature freeze unless the roadmap is explicitly reopened. Release work may fix defects, tune bounded values, improve presentation/accessibility, or optimize implementation while preserving accepted contracts.

**M10 multiplayer boundary:** the established server-authority model remains unchanged through release. Launch hardening must not add client-selected targets, outcomes, rewards, hidden-content queries, save values, timing authority, or private-state leakage.

**M10 accept:** the release candidate passes the complete documented suite from clean profiles, approved saves load or migrate correctly, the full co-op progression from first spawn through biome exploration and long-distance ocean travel is playable without developer tools, performance and accessibility budgets remain within recorded limits, rejected requests leave authoritative state unchanged, and all remaining limitations are explicitly documented.

## M11 — User experience and visual UI upgrades

Start only after M10 acceptance closes under its recorded owner-approved scope and limitations. Improve the existing game's user experience, HUD, and graphical UI presentation while preserving gameplay behavior and the established visual identity. This milestone authorizes presentation improvements only; it does not reopen gameplay or content scope.

M11 has twenty-three ordered goals:

1. **Configurable theme and shared components.** Establish one easily editable, documented UI theme file as the source of styling across the game. Define named styling options for colours, font families/assets, text sizes and weights, outlines/borders, thickness, corners, button and panel backgrounds, image references, padding, spacing, slot/icon sizes, and animation duration/easing. Cover normal, hover, pressed, selected, focused, and disabled states, plus semantic text roles and high-contrast variants. Use reusable themed components for most UI: buttons, text/labels, panels/backgrounds, borders, tabs, slot cells, icon/timer entries, and detail views; existing HUD and menus consume these components/styles rather than copying values into each widget. Preserve view-specific layouts and gameplay bindings. Document theme keys, defaults, asset paths, edit/load workflow, and safe fallback for missing or invalid values. Local text scale and contrast preferences must apply consistently over theme defaults. Verify representative theme changes propagate across HUD, inventory, building, map, and options without editing individual widgets. Introduce no dependency or gameplay/save schema.
2. **Top-right status container.** Replace the lower-left active-status presentation with an owner-local, top-right status hotbar beside or below the existing minimap, without overlapping it. The hotbar is an invisible layout parent: no background, border, or empty-slot chrome, and nothing visible when empty. It contains active player-status and weather icons in a stable order, with spacing and wrapping that remain readable at supported viewport sizes and UI scales. It is a display container, not an action/equipment bar, and must not capture ordinary gameplay or minimap input.
3. **Status and weather icons with timers.** Give existing effects such as Wet and Hot, and weather such as Storm, distinct original icons in that container, each with a readable name and its respective remaining timer. Move the active-effect/weather list out of the left panel rather than duplicating it; keep inventory and other unrelated left-panel information intact. Read the owning player's existing replicated status/exposure/support state and the existing server weather interval. Use server-published remaining time or expiry/start/duration with synchronized server time; never invent a client-authoritative duration. A condition without an authoritative expiry must be labelled as ongoing, with its existing recovery guidance, rather than given a fabricated countdown. Hot is a presentation of the existing heat condition, not a new buff/debuff. Preserve access to existing category, intensity/stack, source, and recovery details through accessible text or a detail view, without requiring hover or colour alone.
4. **Complete item and buildable icon coverage.** Give every current inventory item and every current build-menu entry an original, recognizable icon mapped to its canonical catalogue identity, including carried tools shown by the inventory. Audit the complete current catalogues rather than a sample, share an icon where the same identity appears in both menus, and verify that no supported entry lacks its assigned icon. Icons are presentation assets; they do not change item IDs, recipes, quantities, availability, or construction identities. Retain readable names and accessible detail text.
5. **Inventory and build-menu backgrounds and slot grids.** Visually update both menus with original background images and a grid-like structure of individually framed slots. Inventory slots show the item icon and existing quantity/condition/selection information where applicable; build slots show the buildable icon and preserve material costs, requirements, availability, and rejection details. Give empty, selected, focused, and unavailable cells clear visual treatment without relying on colour alone. Grids are a presentation of existing entries, not a new inventory capacity, stacking rule, persisted slot index, drag/drop mechanic, or build transaction. Preserve current actions and keyboard/controller navigation; allow layout reflow or scrolling at supported viewport sizes and text scales.
6. **M-key world-map menu background and visual update.** Add an original background image to the expanded map menu opened with M and improve its framing, typography, controls, and information hierarchy. Keep the terrain/water map, fog, player marker, pins, and existing interaction hints readable above the decorative background. Preserve M-toggle/Escape behavior, pan, zoom, recenter, personal pins, modal input ownership, privacy, and map persistence; decorative imagery must not imply undiscovered world content or create a second map simulation. All three menus (inventory, build, and M-key map) must have background images consistent with the established visual identity, with sufficient contrast for foreground text and icons.
7. **Option-menu backgrounds.** Add original background images to the main Escape options/settings shell and all option screens/tabs (Video, Audio, Controls, and Settings), using the shared theme and panel components. Keep labels, buttons, selected tabs, values, focus outlines, and high-contrast/text-scale variants readable; preserve current option behavior and modal input ownership. Background image references and presentation styling belong in the theme file.
8. **Fast main-options opening animation.** Give the main Escape options menu a slight, fast downward slide from the top into its final position on opening. Animate the whole panel's presentation rather than scrolling its content or changing layout/input contracts. Keep duration, travel distance, and easing configurable in the theme; default to a short, restrained transition. Acquire modal input/focus immediately, keep hit testing aligned with the moving panel, and handle closing/reopening or resizing mid-animation without stuck input or off-screen placement. Include an instant/reduced-motion presentation path and verify repeated opening plus keyboard/controller access. The animation is local only and must not delay accepted actions or change gameplay timing.
9. **Consistent graphical UI polish.** Improve the existing HUD and crafting, construction, equipment, and settings views in small increments: typography, spacing, alignment, icons, hierarchy, state feedback, and optional restrained transitions. Retain current actions, information, keyboard/controller focus, modal input ownership, text scaling, contrast options, and original art direction. Each increment must identify its affected view and concrete readability or usability improvement before implementation.
10. **Item detail panel.** Selecting or focusing an inventory slot shows a larger assigned icon, readable name/description, existing weight and condition where supported, and currently available actions. Reuse a shared detail component and preserve owner-only information; do not invent missing item statistics or new actions. Details must work through keyboard/controller focus as well as pointer selection, and update safely when an item is consumed or removed.
11. **Search, filters, sorting, and category grouping.** Add local search and relevant category filters/sorting to inventory, crafting, and build menus. Where useful, group entries under named categories; the build menu should group existing pieces into categories such as structural pieces, stations, and camp utilities. Derive groups from the current catalogue through presentation metadata without changing canonical identities or availability. Keep an All view, readable empty/no-results feedback, stable selection/focus across filtering, and keyboard/controller access. Sorting/grouping does not rearrange persisted inventory state, unlock entries, or reveal hidden rewards/content.
12. **Clear crafting requirements.** Show ingredient icons with owned/required counts alongside station, tool/skill/unlock requirements already supported by each recipe. Name every unavailable reason in readable text and update after accepted inventory/station/progression changes. Read existing owner-visible state for previews; the server still independently validates each transaction, and UI availability must not promise acceptance or invent a requirement.
13. **Compact notifications.** Present accepted item gains, discoveries, and skill increases in brief original icon-plus-text notifications. Use existing owner-visible accepted results, not speculative client intent or hidden peer state. Bound and coalesce the queue to prevent HUD clutter, avoid duplicate messages on refresh/reconnect, and preserve existing persistent information. Placement must avoid status icons/minimap, interaction prompts, and modal controls; lifetime and transitions come from the theme and respect reduced motion.
14. **Consistent hover and focus feedback.** Apply shared subtle highlights and short transitions to buttons, tabs, slots, and other selectable UI. Make hover, keyboard/controller focus, selection, and disabled states distinguishable with shape/outline/text as well as colour. Use theme-defined styling/timing, retain focus on refresh, and keep essential state cues visible in the instant/reduced-motion path.
15. **Readable interaction prompts.** Near the crosshair, show the existing target's readable name, supported action, and the player's current remapped keyboard/controller binding. Use the existing interaction candidate and owner-visible state; preserve server trace/range/action validation and avoid new hidden-target searches. Handle no target, unavailable actions, rebinding, and modal menus without stale prompts or input leakage; local prompt visibility never authorizes an interaction.
16. **Map legend and marker filters.** Add a readable legend for existing map symbols and local category visibility filters for markers already available to that player. Keep fog/privacy rules, personal pin data, and accepted co-op visibility intact. Filtering only changes drawing, must not discover/query hidden content or delete pins, and must keep selection/legend counts truthful and keyboard/controller accessible.
17. **UI scale and reduced-motion settings.** Expose an adjustable interface scale and a reduced-motion/disable-decorative-animation option through the existing local settings path. Distinguish whole-interface scale from the existing text-size preference and apply both without clipped controls. Reduced motion bypasses decorative slide/fade/highlight motion while preserving essential state feedback, immediate focus, and menu actions. Theme defaults remain configurable; player accessibility choices override them. Document bounded ranges/defaults and verify HUD/menu reflow, supported viewport sizes, contrast, navigation, restart persistence through existing local settings, and no gameplay/save-schema change.
18. **Recipe and build-result previews.** Show a larger original image/icon of the selected crafted result or buildable beside its description and existing requirements. Reuse the shared detail/preview components and canonical icon mappings, support keyboard/controller selection, and keep unavailable results clearly labelled. Decorative previews must not spawn gameplay actors, change placement/crafting behavior, or imply unimplemented item properties.
19. **Favorites and usage-ranked markers.** Let the player locally bookmark recipes and build pieces and access a Favorites category. Make marker treatment configurable through the shared theme: a small star at the slot's bottom-right, a border, or both; distinguish manually bookmarked entries from automatic usage ranks. Independently rank successful building, cooking, and other crafting usage: gold for the most-built piece, most-cooked recipe, and most-crafted item in their respective lists, silver for second, bronze for third. A cooking action belongs to the cooking list rather than also inflating other crafting usage. Also surface the most recently successfully crafted item, built piece, and cooked recipe as Recent shortcuts in Favorites and mark their ordinary menu slots with a distinct theme-configurable treatment, such as a small clock/corner badge and Recent text. Track one latest entry per action category from accepted owner-visible events; a new successful action replaces only that category's previous Recent marker, while failed requests, menu selection, and replayed refresh/reconnect events do not update it. Let Recent coexist with manual favorites and gold/silver/bronze rank markers without overlap or lost meaning; define badge placement, outline/colour, and optional emphasis in the theme and preserve static readable cues under reduced motion. Do not require a recent entry to be manually bookmarked or change usage counts merely to display it. Count each accepted action once, not attempted/failed requests or repeated replication/refresh/reconnect; use a deterministic tie order, and award no rank to unused entries. Use owner-visible accepted-result signals only and keep records bounded by current canonical entries; badges do not grant bonuses, unlocks, or affect costs. Provide accessible Favorite/Rank 1/2/3 labels and theme-defined contrast so rank is not colour-only. Document the counting, tie, local preference/history lifecycle, and treatment of removed catalogue entries before implementation; cross-restart history must not silently alter gameplay or map save schemas.
20. **Remember menu position.** On reopening inventory, crafting, build, map, or options menus, restore the last applicable tab/category, selected entry, and scroll position for that local player during the session. Keep remembered state separate per menu, clamp scroll positions on resize/scale changes, and fall back safely when an entry is consumed, removed, filtered out, or no longer available. Restore focus without performing an item/build/craft action. Preserve the map's existing pan/zoom behavior, modal rules, and local saves; cross-restart persistence is not required by this goal.
21. **Status transition cues.** Briefly highlight an icon when an existing effect starts, is authoritatively refreshed, or ends. Detect actual transitions from replicated/server-timed state rather than replaying cues every countdown update; an ending cue may briefly retain a clearly ended icon without implying the effect is active. Use shared theme timing and restrained animation, coalesce rapid updates, avoid duplicate cues on reconnect, and keep an immediate static cue in reduced-motion mode. Cues never apply, refresh, prolong, or remove an authoritative effect.
22. **Inline equipped-versus-selected stat comparison.** In existing item/forge/crafting detail text, show each supported selected-item stat and its signed difference from the currently held/equipped comparable item: selected value (selected minus current). Use green for increases and red for decreases in damage bonuses such as crush/slash, neutral for equal values, plus signed numbers and accessible comparison text independent of colour. For example, if the held Iron Sword has crush 0 and slash 40, selecting an Iron Mace with crush 60 and slash 0 shows `Iron Mace`, `crush 60 (+60)`, and `slash 0 (-40)`, with the slash decrease red and crush increase green. These names/numbers illustrate formatting, not new catalogue entries or balance values. Read real supported catalogue/owner-equipment stats; use the existing stat semantics for whether higher or lower is favourable, and label missing or incompatible comparisons unavailable rather than inventing a zero. Refresh when selection/equipment changes, preserve existing craft validation, and add no separate comparison card or new combat/stat system.
23. **Presentation and gameplay regression.** Verify status appearance, refresh, expiry/removal, weather transitions, simultaneous effects, empty layout, reconnect, and separate host/client owners. Inspect rendered top-right placement with the minimap, complete item/buildable icon coverage, inventory/build/map and all option-menu backgrounds, inventory/build slot grids and states, shared-theme propagation/fallbacks, the fast slide-down opening and interrupted/reduced-motion paths, item details, search/filter/sort/category grouping, crafting counts/reasons, bounded notifications, hover/focus feedback, remapped interaction prompts, map legend/marker filters, UI scale and reduced motion, recipe/build previews, favorite/rank/Recent markers, coexistence, category replacement, and accepted-use counting, remembered menu state, status transition cues, inline equipped/selected stat deltas, and M-key map interactions at supported resolutions/aspect ratios and text scales, including high contrast and keyboard/controller detail access. Run the relevant existing UI, authority, weather, status, inventory, construction, map, input, and reconnect checks from `docs/07-development-setup.md`; reconfirm affected UI performance budgets and retain screenshots and known limitations.

**M11 boundary:** no new gameplay effect, balance change, timer duration, weather rule, item, recipe, progression rule, collision, save schema, platform, online service, or visual-identity change. Cosmetic presentation must not alter accepted actions or outcomes.

**M11 multiplayer boundary:** UI remains local and read-only over state already available to its owner. All effect application, refresh, removal, weather selection, duration, damage, modifiers, rewards, and persistence remain server-owned. Add no gameplay mutation RPC, hidden-content query, or private peer-state exposure.

**M11 accept:** active status and weather icons with truthful timers appear in an invisible top-right parent without minimap overlap or a duplicate left-panel list; empty state has no visible hotbar chrome; every current inventory item and build-menu entry has its assigned original icon; inventory and build menus use readable slot grids; inventory, build, M-key map, and all option menus have background images; the main options menu opens with a fast, restrained slide down from the top; a documented configurable theme drives styling across mostly shared UI components, respects local accessibility preferences, and propagates edits across views; item details, menu search/filter/sort/category grouping (including building), crafting requirements, compact notifications, shared hover/focus cues, interaction prompts, map legend/marker filters, UI scale, reduced-motion options, result previews, configurable favorites, gold/silver/bronze usage markers, and distinct most-recent crafting/building/recipe markers, remembered menu position, status transition cues, and inline stat comparisons work over existing visible state; status and menu details remain accessible across supported layout/accessibility settings; rendered host/client evidence and relevant regressions pass; gameplay and persistence contracts are unchanged.

## M12 — Inventory menu and cleaner gameplay HUD

Start after M11 acceptance. Continue UX/UI improvements through existing shared
themes, catalogue widgets, local input, and owner-visible state.

Execution sizing is defined by the direct unchecked children in `BACKLOG.md`,
not these cumulative feature goals. Normal increments target about 20–30 minutes
including implementation, lightweight checks, progress/backlog updates and commit.
Small layout/removal changes combine implementation and narrow checks; menu,
art/import and catalogue work use bounded integrated increments. Generated art
batches contain at most four objects; copy batches at most eight; texture mapping
batches at most sixteen. Pin membership in manifests and add ordered bounded
children when needed for full coverage. Keep old action entry points until their
replacement works. Full build/rendered/regression execution stays in the mandatory
milestone-final run, which can exceed 30 minutes and must finish or report a true
blocker under `AGENTS.md`; elapsed estimates never count as implementation or
verification success.

M12 has ten ordered UI goals:

1. **Dedicated inventory menu.** Add an Inventory menu with the same presentation
   quality and shared layout/components as the crafting menu. Both `Tab` and `I`
   open/toggle the same local inventory menu by default. Provide the existing
   pack slot grid, item counts, carried tools/equipment, selected-item details,
   search/filter/sort, and remembered local browsing state as applicable. Keep
   crafting/building available through its existing menu. Respect configurable
   bindings, text-entry and focus navigation, modal priority, Escape-to-close,
   and restoration of movement/look input when the inventory closes. Use only
   inventory/equipment information already visible to the owning player.
2. **Remove the persistent left-side view.** Remove the entire tall left-side
   panel shown in the user's 2026-10-07 screenshot from the normal gameplay
   screen, including its support glyph row, combat/support instructions and
   diagnostic text, pack slots, and carried-tool list. Inventory inspection
   belongs in the new menu. Preserve existing combat/support actions and their
   bindings, essential player feedback through appropriate existing HUD/menu
   surfaces, interaction prompts, notifications, and the top-right status/weather
   presentation. Removing a widget must not disable a gameplay system.
3. **Halve minimap edge padding.** Reduce the circular minimap's right and top
   margins from the current 24 UI units to 12 UI units each. Preserve its size,
   circular clipping, zoom, facing marker, and separation from status/weather
   indicators. Apply the same proportional change under interface/DPI scaling;
   this goal concerns the top-right minimap rather than the M-key full map.

4. **Remove the bottom tutorial card.** Remove the bottom-centred onboarding/help
   card shown in the follow-up screenshot (for example, “Arrive · optional” and
   its movement/look instructions) from the normal gameplay screen. Retain
   essential action results, interaction target names, and status/notifications
   through their existing appropriate presentation; no replacement help banner.
5. **Input bindings belong only in Options.** Remove all text that identifies
   input keys/buttons or explains control combinations from every other view:
   gameplay HUD, tutorial cards, inventory, build/crafting/station menus, map,
   interaction prompts, tooltips, and keyboard/controller help legends. Only the
   Options controls/binding configuration displays those bindings. Keep concise
   action labels such as Build, Repair, Cook, and Open chest, accessible focus
   navigation, and remapping behavior; prompts describe the available action
   without key/button-name labels. Update binding changes only where configured.
6. **Write recipes for players.** Replace verbose developer-facing recipe copy
   with a short name, icon, useful description, ingredient counts, output quantity
   when relevant, actual station/tool/heat requirements, and one clear actionable
   unavailable reason when needed. Remove repeated names/costs/reasons, server
   validation and transaction explanations, request/session/internal-ID language,
   instructional selection legends, irrelevant skill lists, and “none/no lock”
   boilerplate. Keep real placement/fuel constraints understandable and truthful;
   technical contracts belong in documentation and developer diagnostics.
7. **Construction-only build menu and dedicated context menus.** Each entry
   below defines the object or entry point and its menu contents or direct action.
   World-object menus open only after a validated interaction with that object.

   - **Carried Construction Hammer -> Build menu.** Keep the existing build-menu
     entry action. Contents: buildable structures, stations and attachments;
     categories/search, piece icon and short description; material counts and
     relevant placement prerequisites; selection, local preview, and build/place
     actions. Retain no-station construction recipes so the player can establish
     a first camp/workbench. Place already crafted buildables through this menu;
     producing an item that requires a station belongs to that station's menu.
     Exclude pack inspection, food use/cooking, tool crafting/upgrades/repair,
     hearth refuelling/lighting, chest transfers, and general skill/diagnostic lists.
   - **Owning player's pack (Tab/I; no world object) -> Inventory menu.** Contents:
     pack grid and quantities, search/filter/sort, selected-item icon/description,
     carried tools and their level/condition, existing equipment inspection/actions,
     and an Eat/use action for supported carried food with its existing effect
     and availability state. No station requirement is added for eating. No
     construction, station crafting, repair, or chest contents belong here.
   - **Workbench -> Workbench menu.** Separate Craft and Repair
     sections. Craft contains only current recipes/tool operations supported by
     this station, including Bronze Axe creation and Grinding Stone production:
     selected result, ingredients owned/required, relevant station level and
     other real prerequisites, and Craft. Repair contains the owner's carried
     tools, current/max condition, selected-tool free Repair, and one unmet reason
     if unavailable. Show the station's effective level and relevant attachment
     state; do not put the full build catalogue, cooking, storage, or eating here.
   - **Forge -> Forge menu.** Separate Craft, Upgrade, and Repair sections. Craft
     includes Forge-compatible production such as the Frying Pan. Upgrade includes
     supported tool upgrades such as Bronze Axe -> Iron Axe, showing current and
     resulting tool, required station/skill level, material counts, concise stat
     comparison, and Upgrade. Repair contains carried-tool condition and the
     existing free selected-tool Repair. Show effective Forge level/attachment
     state. Exclude unrelated building, cooking, inventory browsing, and storage.
   - **Grinding Stone -> Direct Repair All interaction; no menu.** Use the
     existing Interact action (default E) on the stone to repair all damaged
     carried tools through the existing free server-validated repair-all path.
     Do not open a panel, tool selector, or confirmation dialog. Keep gameplay
     input active and show only concise success/unavailable feedback. A valid
     interaction performs the action once; rejected or repeated/replayed requests
     must not cause extra mutations. The player-facing prompt says Repair all
     without displaying the key binding outside Options.
   - **Cooking Rack -> Rack cooking menu.** Contents: Cooked boar meat and Cooked
     deer meat; ingredient counts, selected food/result description, supported
     quantity limits, nearby hearth heat availability, and Cook. No other station's
     recipes, tool operations, construction, storage, or eating controls.
   - **Hearth Cauldron -> Cauldron cooking menu.** Contents: Meat stew and Root
     vegetable soup, with the same recipe/count/quantity/heat/Cook presentation.
     Show only recipes supported by this cauldron; no unrelated actions.
   - **Frying Pan -> Pan cooking menu.** Contents: Roasted root vegetables and
     Deer and rutabaga roast, with the same recipe/count/quantity/heat/Cook
     presentation. This uses an already placed pan; forging a pan belongs in
     the Forge menu and placing it belongs in Build.
   - **Campfire -> Direct Add fuel interaction; no menu.** Use the
     existing Interact action (default E) on the hearth to consume exactly one
     available eligible fuel item from the interacting player's inventory and
     add its existing fuel duration. The server selects the item using the
     current raw-fuel priority: Wood, then Lightwood, then Densewood, then Coal.
     Do not open a fuel picker, fire panel, or confirmation dialog, or perform
     an additional lighting toggle from this interaction. Preserve the existing
     60 seconds per item, fuel cap, weather/ignition rules and use-range checks.
     No fuel, a full hearth, or a rejected interaction consumes nothing. Keep
     gameplay input active and show concise result/unavailable feedback; the
     prompt says Add fuel without displaying a key binding outside Options.
   - **Chest -> Storage menu.** Contents: this interacted chest's item stacks,
     the owner's pack stacks needed for transfer, selected-item name/icon/count,
     existing Deposit/Withdraw actions and capacity/unavailable feedback. Preserve
     existing transfer quantities and owner-only access/refresh behavior. No
     construction, station crafting, repair, cooking, upgrades, or eating.
   - **Workbench Tool Rack / Forge Anvil -> No separate service menu.** These
     existing passive attachments raise the corresponding station's effective
     level; their state is shown in its Workbench/Forge menu. Their construction
     placement belongs in Build. Do not invent attachment services or upgrades.

   **Shared menu rules:** show only operations actually supported by the current
   catalogue/gameplay contracts; the examples above do not add recipes or items.
   Use shared themes, concise action labels, accessible focus, and remembered
   selection where applicable. Keep input-binding text exclusively in Options.
   Grinding Stone and Hearth interactions open no menu and do not capture modal
   input. Verify one Repair All or one fuel debit per accepted interaction and no
   mutation on rejection. Open one context menu at a time; close/leave restores
   local input and clears
   stale object data. Revalidate object identity, distance, sight line, station
   level, heat, costs, tool state, and privacy through existing server paths at
   action time. Moving away, destruction, reconnect, or a newly unmet requirement
   must never leave an actionable stale context. Keep existing gameplay/save
   contracts and do not add world objects or new station gates. The listed
   destinations must exist before their old build-menu sections are removed.

8. **Generated 64x64 catalogue image icons.** Replace the current item/result
   line glyphs with original generated raster thumbnails of the actual objects,
   using the attached reference only for the general idea of readable object
   thumbnails in a selection grid. Keep Kalmala's own objects, materials and
   visual identity; do not copy the reference game's icons, assets or menu art.

   - **Coverage:** every current inventory item/material/food, carried tool or
     equipment identity, recipe output, crafting/upgrade selection and building
     selection needs an explicit image assignment. Enumerate live runtime
     catalogues and tool/upgrade/build contracts rather than relying on a fixed
     historical count. Distinct visible objects must remain distinguishable.
   - **Image specification:** deliver a generated image for each distinct object
     as an exactly 64x64-pixel RGBA PNG with transparent background. Use a centred,
     fully visible object, consistent angle/lighting, useful colour/material
     detail and a clear silhouette at native size. No baked-in text, numbers,
     key labels, slot background, selection border, or Favorite/Rank/Recent badge.
     Generate higher-resolution originals when needed, then crop/downsample and
     inspect the final 64x64 image; keep a manifest of identity and source/output.
   - **Recipe and selection mapping:** show the image of the produced item or
     buildable piece. Reuse that object's canonical image across inventory,
     crafting, building, upgrade results, ingredient rows, storage and item/result
     details; a recipe producing the same object does not need contradictory art.
     An upgrade uses its target object's image. Retain the existing larger
     selected-result preview layout using the same assignment; preserve source
     art if needed for a clean larger preview. No runtime image-generation call.
   - **Integration:** import the PNGs as project-owned UI textures with suitable
     alpha/filtering settings and route them through the shared icon component
     and canonical runtime IDs/aliases. Keep counts, names, unavailable states,
     focus/selection and activity markers as separate accessible UI overlays.
     Provide an honest fallback for unknown/unmapped IDs; fallback use for a
     current supported identity fails coverage acceptance.
   - **Acceptance:** audit complete mappings, exact output dimensions, alpha,
     missing/duplicate/wrong-object assignments and short Windows paths. Inspect
     every final icon at 64x64 on light/dark menu backgrounds and in host/client
     inventory, crafting, build, station and storage views; verify scaled/larger
     previews, contrast, recognizable silhouettes and non-overlapping markers.
     Retain the icon manifest, source/final images and rendered evidence. Add no
     items, recipes, stat authority, gameplay balance or save fields.

9. **Clear, consistent item names and descriptions.** Review every current item,
   material, food, tool/equipment, buildable and recipe-result display name and
   description. Preserve the user's existing intentional manual improvements;
   fix remaining inconsistent or poor copy rather than replacing everything
   with generic generated prose.

   - **Names:** use one recognizable player-facing name for the same object in
     inventory, ingredients, recipes, building, station menus, storage, prompts
     and previews. Specifically, item/recipe `Workbench` displays Workbench,
     replacing Joiner's bench. Use normal readable spacing/capitalization for
     compound identifiers; do not expose internal Kit suffixes or force literal
     technical IDs into labels. Stable IDs, aliases and saved identities stay
     unchanged (for example, internal WorkbenchKit remains an internal identity).
   - **Campfire is construction, not an inventory item:** remove HearthRing from
     the normal inventory-item catalogue/presentation. The existing Campfire
     recipe belongs only in Build, is displayed as Campfire, and places a
     Campfire directly from its current material costs and placement rules.
     It never crafts a HearthRing/Campfire inventory stack or pickup item.
     Represent its output as construction metadata rather than requiring a
     normal inventory item; update catalogue loading/validation, UI selection
     and icon coverage accordingly. Its generated image belongs to the Campfire
     build selection and placed object, not an inventory slot. Use Campfire in
     player-facing names/prompts; retain necessary internal CampfireKit and
     legacy HearthRing aliases for existing construction/save compatibility.
     Preserve current fuel/ignition costs, direct refuelling and server authority.
     Verify valid placement creates one Campfire and no item stack, while failed
     placement spends nothing; existing saved Campfires still restore correctly.
   - **Descriptions:** write short natural sentences about what the object is
     and its real use or character. Retain intentional flavour where it fits;
     remove developer/AI-style explanations, implementation vocabulary, redundant
     names, outdated uses and vague filler. Do not invent effects, gathering
     sources, recipes, unlocks, mechanics or statistics. Put costs, condition,
     quantities and requirements in their existing structured detail fields.
   - **Audit and propagation:** capture the current name/description and proposed
     replacement per canonical identity, identify manual edits to retain, and
     trace every text source (JSON, runtime catalogue/aliases, hardcoded UI,
     recipe/result/tool/interaction labels). Resolve conflicting labels at their
     shared source and keep recipe/output naming aligned. Use current content
     and gameplay contracts to check each claimed use; do not change schemas.
   - **Acceptance:** review the complete before/after copy list, confirm no
     supported entry is missed and retained manual improvements are preserved,
     then verify consistent names/descriptions across host/client menus and
     object prompts. Check wrapping, search/filter/sort, accessible labels,
     icon association and current unavailable states. Update affected text
     expectations and documentation without changing transaction/gameplay rules.

10. **Compact active-status icons at the minimap's upper left.** Move the local
    status/weather group from below the minimap to immediately beside its upper
    left. Match the minimap's top alignment and 12-UI-unit top margin from goal 3;
    anchor the group's right edge to the minimap's left edge with a small
    theme-configurable gap (default 12 UI units). Derive this from the minimap's
    actual scaled layout rather than a separate hardcoded vertical offset.

    - **Active conditions only:** show an icon only for a currently active special
      state already available to the owner: for example Wet, an active meal or
      support effect, qualifying Hot/Cold exposure, or a current Storm. Hide
      normal/Calm weather and generic Active/High activity weather labels when
      they do not represent an active Storm. Use existing authoritative status,
      exposure and storm qualification rules; do not add gameplay thresholds,
      penalties, effects or durations. Hot/Cold remain their own exposure icons;
      do not add a second weather icon merely to repeat them.
    - **Truly hidden empty/inactive state:** when there are no qualifying active
      conditions, collapse the entire group with no background, border, placeholder
      or reserved slots. Remove each expired/cleared icon immediately and close
      its gap; decorative end cues must not retain an inactive icon. Reappearance
      follows current owner state, without reconnect/countdown replay or stale
      weather icons. Existing detail views may still explain the actual state.
    - **64x64 icon with player-effect timer underneath:** each supported status
      or weather condition uses an original 64x64 image icon, displayed at 64x64
      UI units at default scale and following normal interface scaling. Centre
      the remaining duration directly below the icon only for a finite player
      status effect (for example Wet, an active meal or timed support effect).
      Format it as minutes:seconds, such as 2:00, using the effect's existing
      authoritative expiry/duration. No visible name label above, below or beside
      the icon. Keep nonvisual accessible names and on-demand effect details.
      Weather/Storm and untimed Hot/Cold exposure show the icon alone: no server
      weather-cycle timer, duration, ongoing label, or empty timer placeholder.
      Do not expose server weather intervals in player-facing detail views either;
      developer diagnostics may retain them. The reference defines layout only
      and does not authorize adding its example effects or copying its artwork.
    - **Tighter spacing:** remove wide fixed cells and unused horizontal padding.
      Size each entry to its 64x64 icon and optional timer underneath; use compact
      theme-configurable inter-entry gaps (default 4 UI units) and minimal inner
      padding. Keep multiple active entries together with stable order. Grow
      leftward from the minimap and wrap downward within available viewport space
      when needed; never overlap the minimap or clip icons/timers. Maintain
      readable timers and nonvisual accessible names; do not add visible name text.
    - **Acceptance:** retain host/client captures for zero, one and many active
      conditions; normal-to-Storm/back, Hot/Cold recovery and status expiry;
      reconnect and separate owners; UI/text scaling, high contrast, reduced
      motion, and 4:3/16:9/ultrawide layouts. Verify shared top padding, compact
      gaps, 64x64 icons with no visible names, correct player-effect timers underneath,
      absent weather/untimed-condition durations, immediate inactive collapse and
      no minimap overlap.
      This changes layout/visibility only, with no server or save-state changes.

**M12 boundary:** presentation and local input changes only. Reuse existing item,
equipment, crafting, support, and inventory contracts; add no gameplay content,
balance changes, authoritative mutation paths, new save schemas, hidden-content
queries, peer-private state exposure, or platform/online-service changes. Existing
inventory actions continue through their current server-validated paths.

**M12 accept:** Tab and I reliably open the same usable inventory menu on host
and client; item/tool information and existing permitted actions remain truthful
and accessible; menu close restores input without replaying an action; the
pictured persistent left panel and bottom tutorial card are absent during normal
play; input-binding text appears only in Options; recipes use concise player-facing
copy; the build menu contains construction only and displaced functions are
accessible in their dedicated interaction/inventory contexts; minimap top/right
padding is half its former value without clipping or status overlap; every current
catalogue object/result has its original generated 64x64 image icon, consistently
mapped across views with readable separate overlays and complete coverage; item
names/descriptions are concise, consistent and truthful with intentional manual
improvements retained and Workbench named Workbench; Campfire exists only as a
building recipe/placed construction, never a HearthRing inventory item; compact
status/weather icons sit at the minimap's upper left and show only active special
conditions as 64x64 icons without visible names, with timers underneath only
finite player effects, no weather/untimed-condition durations, and a fully hidden
empty state. Verify
empty/populated inventories, selection and input changes, coexistence with
crafting/settings/full map, separate owners, reconnect, keyboard/controller
navigation, text entry, 4:3/16:9/ultrawide layouts, interface/text scaling, high
contrast, and reduced motion. Retain rendered host/client captures. Perform
lightweight checks during child increments and the prescribed affected build,
UI/input/inventory/construction/crafting/repair/cooking/storage/authority/reconnect
regressions and full milestone-final
verification from `docs/07-development-setup.md` before declaring M12 complete.

**M12 final acceptance — 2026-10-09:** Complete in the retained
`codex/m12-hud-feedback-rebuild` implementation. Final repairs, build and
116-test queue, owner Inventory/service captures, twelve combined-HUD cases,
diagnostic performance profiles and owner-confirmed keyboard/controller
acceptance are recorded in [45 M12 final acceptance](45-m12-acceptance.md).
