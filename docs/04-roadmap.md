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
   exposure resilience. Effects must stack, replace, expire, and reject invalid
   or duplicate consumption through server-owned rules. Food should create
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
6. **Add biome-specific hazards and active-weather pressure.** Extend the
   server-owned environmental presentation and exposure rules with readable
   fog, rain, storms, heat, cold, and a clearly marked highly-active-weather
   state. Hazards may alter visibility, wetness, warmth, stamina recovery,
   movement comfort, fire safety, or creature pressure, but must remain bounded,
   reversible, and counterable by shelter, fire, food, tools, or timing rather
   than becoming biome damage walls or mandatory travel gates. The server
   selects weather and hazard intensity from the authoritative world state;
   clients never submit weather, exposure, hazard, or mitigation outcomes.
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
   understandable. Verify same-seed host/client resource and creature
   agreement, rejected client mutations, owner-only progression and inventory
   privacy, rare-loot visibility rules, status presentation, reconnect
   behavior, and bounded actor, memory, replication, and save costs.
9. **Retain the M6 release-candidate loop.** Re-run the supported-session
   acceptance after the content pass; the new systems must not regress the
   existing traversal, camp, combat, support, weather, construction, storage,
   persistence, or minimap contracts.

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

**M7 accept:** the integrated biome gathering, creature, crafting, repair,
food-processing, progression, hazard, and HUD loop is playable without
developer commands; all outcomes remain authoritative and recoverable; rare
discoveries are optional and original; status icons, timers, intensity, and
recovery guidance are readable without colour; matching peers observe the
permitted state; rejected requests leave inventory, skills, tools, resources,
effects, hazards, loot, and saves unchanged; and the M6 release-candidate loop
remains playable.
