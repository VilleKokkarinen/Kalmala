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
   - **Other optional camp and equipment examples:** a raised weatherproof chest, a covered smokehouse using existing hearth heat checks, a storm-rated shelter piece, an insulated travel wrap, or a drying line for supplies. Extend their existing server-owned storage, processing, construction, status, and inventory paths.
   - Show tool level, skill requirement, matching station level, material cost for upgrades, repair availability, and rejection reasons in readable text as well as colour or icons. Keep upgrades optional and recoverable, and award progression only after an accepted server transaction.
   - Keep detailed tool and skill state owner-scoped. Do not persist new tool levels or workstation attachment progression until the M9 persistence goal defines and verifies the versioned migration contract; extend existing tool-condition and construction authority rather than adding parallel subsystems.

3. **Exploration rewards without quest routing.** Add optional clue, landmark, treasure, boss, or environmental-discovery structures that reward observation and travel without turning the world into a prescribed quest chain. The first two land-discovery candidates and their reward boundaries are defined in `29-m9-exploration-rewards.md`; server placement and claim behavior are implemented, with durable claims still gated by the M9 persistence contract.

4. **Persistence schema expansion.** Version and migrate any newly persistent progression, creature, discovery, storage, equipment, or encounter state before enabling it in normal saves. Preserve exact identity/world matching and bounded sparse records. The accepted M9 tool, station, land-discovery, and Drying Line migration boundary is defined in `docs/30-m9-persistence-migration.md`; schema-2 writes remain gated on its round-trip and rejection coverage.

5. **Cross-system regression.** Verify the second content wave against land/ocean travel, weather, survival HUD, crafting, combat, support effects, construction, reconnect, persistence, and the retained supported-session loop.

**M9 content boundary:** expansion stays within the established PC solo/listen-server co-op product scope unless a separate roadmap revision explicitly changes platforms or online services. New systems should extend existing contracts rather than introduce parallel inventory, combat, weather, progression, or persistence authorities.

**M9 multiplayer boundary:** all encounter selection, creature state, crafting/progression outcomes, construction material payment and placement, loot, hazards, rewards, and persistent facts remain server-owned. Clients submit a canonical buildable identity only; the server derives costs, terrain transform, validation and result. Private inventory and progression remain owner-scoped; peers receive only relevant world and presentation state.

**M9 accept:** the second content wave is playable across the supported biomes and ocean travel path without developer commands; new encounters and progression remain optional and readable; rejected client mutations leave state unchanged; versioned saves round-trip and migrate as documented; and the M8 travel loop plus M6 supported-session loop remain green.

**## M10 — Release completion and launch validation**

Start only after M9 acceptance passes. Freeze feature scope and turn the complete supported game into the launch candidate through content lock, compatibility validation, performance hardening, accessibility review, packaging, and release evidence rather than adding another gameplay layer.

M10 has six ordered goals:

1. **Feature and content freeze.** Close or defer remaining backlog items, lock accepted gameplay and save schemas, and require any post-freeze change to include a focused regression and explicit release rationale.

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

M11 has four ordered goals:

1. **Top-right status container.** Replace the lower-left active-status presentation with an owner-local, top-right status hotbar beside or below the existing minimap, without overlapping it. The hotbar is an invisible layout parent: no background, border, or empty-slot chrome, and nothing visible when empty. It contains active player-status and weather icons in a stable order, with spacing and wrapping that remain readable at supported viewport sizes and UI scales. It is a display container, not an action/equipment bar, and must not capture ordinary gameplay or minimap input.
2. **Status and weather icons with timers.** Give existing effects such as Wet and Hot, and weather such as Storm, distinct original icons in that container, each with a readable name and its respective remaining timer. Move the active-effect/weather list out of the left panel rather than duplicating it; keep inventory and other unrelated left-panel information intact. Read the owning player's existing replicated status/exposure/support state and the existing server weather interval. Use server-published remaining time or expiry/start/duration with synchronized server time; never invent a client-authoritative duration. A condition without an authoritative expiry must be labelled as ongoing, with its existing recovery guidance, rather than given a fabricated countdown. Hot is a presentation of the existing heat condition, not a new buff/debuff. Preserve access to existing category, intensity/stack, source, and recovery details through accessible text or a detail view, without requiring hover or colour alone.
3. **Consistent graphical UI polish.** Improve the existing HUD and inventory, crafting, construction, equipment, map, and settings views in small increments: typography, spacing, alignment, icons, hierarchy, state feedback, and optional restrained transitions. Retain current actions, information, keyboard/controller focus, modal input ownership, text scaling, contrast options, and original art direction. Each increment must identify its affected view and concrete readability or usability improvement before implementation.
4. **Presentation and gameplay regression.** Verify status appearance, refresh, expiry/removal, weather transitions, simultaneous effects, empty layout, reconnect, and separate host/client owners. Inspect rendered top-right placement with the minimap at supported resolutions/aspect ratios and text scales, including high contrast and keyboard/controller detail access. Run the relevant existing UI, authority, weather, status, input, and reconnect checks from `docs/07-development-setup.md`; reconfirm affected UI performance budgets and retain screenshots and known limitations.

**M11 boundary:** no new gameplay effect, balance change, timer duration, weather rule, item, recipe, progression rule, collision, save schema, platform, online service, or visual-identity change. Cosmetic presentation must not alter accepted actions or outcomes.

**M11 multiplayer boundary:** UI remains local and read-only over state already available to its owner. All effect application, refresh, removal, weather selection, duration, damage, modifiers, rewards, and persistence remain server-owned. Add no gameplay mutation RPC, hidden-content query, or private peer-state exposure.

**M11 accept:** active status and weather icons with truthful timers appear in an invisible top-right parent without minimap overlap or a duplicate left-panel list; empty state has no visible hotbar chrome; status details remain accessible; the affected UI is readable across supported layout/accessibility settings; rendered host/client evidence and relevant regressions pass; gameplay and persistence contracts are unchanged.
