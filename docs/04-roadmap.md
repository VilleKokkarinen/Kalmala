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

**Accept:** a new player can complete the documented 20–30 minute co-op loop without developer tools.

## M6 — Production hardening and supported-session validation

Start only after M5's vertical-slice acceptance passes. Turn the accepted M5
loop into a reliable release candidate without adding new gameplay content,
changing saved-data schemas, expanding online services, or changing the
current PC solo/listen-server co-op scope.

M6 has four ordered goals:

1. **Close acceptance findings.** Fix issues found during the final fresh-player
   20–30 minute packaged co-op run. Convert each finding into a focused
   regression or an explicit documented non-goal. Prioritize crash and hang
   triage, reconnect and late-join reliability, save-identity safety, input,
   accessibility, and readable non-audio feedback.
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

M7 has six ordered goals:

1. **Add server-owned skill progression.** Define a small allowlisted set of
   original skills for gathering, woodcutting, mining, crafting, and survival.
   Award bounded experience only from accepted server actions, derive levels
   and unlocks on the server, and replicate only the owning player's detailed
   progression plus the presentation state relevant to other peers. Clients
   cannot submit experience, level, unlock, multiplier, or reward values.
2. **Add optional food and nutrition choices.** Add original edible items and
   recipes that can be gathered, prepared, and consumed through the existing
   inventory/crafting transactions. Eating grants finite, readable stat
   modifiers such as stamina capacity, recovery, movement comfort, or exposure
   resilience. Effects must stack, replace, expire, and reject invalid or
   duplicate consumption through server-owned rules. Food should create
   preparation choices without making starvation or a mandatory food route a
   hard travel gate in the first M7 slice.
3. **Complete tool-based material gathering.** Turn suitable generated trees,
   rocks, and harvest resources into original server-validated woodcutting,
   mining, and gathering interactions. The server selects the resource from
   the authoritative trace and range, validates the tool and skill requirement,
   applies bounded node health or use state, awards catalogue-validated
   materials, and persists only the necessary sparse depletion facts. Clients
   provide intent and tool/action selection only; they cannot choose a node,
   yield, damage, durability, or reward.
4. **Broaden the crafting progression.** Extend the existing recipe catalogue
   with a small first tier of tools, gathering implements, food preparation,
   storage/camp improvements, and skill-gated recipes. Reuse atomic inventory
   exchanges, station validation, stack ceilings, batch limits, and owner-only
   result feedback. Each recipe must have explicit ingredient, station,
   unlock, failure, and accessibility text rather than relying on colour.
5. **Build the survival HUD and GUI pass.** Add a persistent local status strip
   with an original icon, name, readable remaining timer, and non-colour state
   for `Wet` and the new food/stat effects. Extend the inventory and crafting
   views with food details, active modifiers, skill progress, tool condition,
   recipe unlock state, ingredient counts, and clear unavailable reasons.
   Status timers must read replicated server state rather than count down
   independently, and local UI changes must not mutate gameplay or send new
   client-authored outcomes. Keyboard/controller focus, text scale, contrast,
   and text-plus-marker feedback remain required.
6. **Verify one integrated content loop.** A fresh player should be able to
   gather a first material, craft or obtain a tool, cut wood or mine stone,
   prepare food, choose a temporary benefit, and return to camp while weather,
   creature pressure, inventory, and status feedback remain understandable.
   Verify same-seed host/client resource agreement, rejected client mutations,
   owner-only progression and inventory privacy, relevant status presentation,
   reconnect behavior, and bounded actor, memory, replication, and save costs.

**M7 persistence gate:** skill experience, learned recipes, tool condition,
food effects, or food inventory that must survive reconnect or restart require a
versioned save contract, identity/world matching, migration policy, and focused
round-trip/rejection tests before implementation. Until that contract is
approved, development fixtures may use transient server-owned state but must
not silently extend the existing saved-data schemas.

**M7 multiplayer boundary:** the server owns skill awards, resource identity,
tool validation, node depletion, recipe unlocks, ingredient costs, food effects,
stat changes, timers, and rewards. Clients receive only the state needed for
their own controls and presentation or ordinary relevant world feedback.

**M7 accept:** the integrated gathering, crafting, food, progression, and HUD
loop is playable without developer commands; all outcomes remain authoritative
and recoverable; status icons and timers are readable without colour; matching
peers observe the permitted state; rejected requests leave inventory, skills,
resources, effects, and saves unchanged; and the M6 release-candidate loop
remains playable.

### World-generation Phase 5 — Companion minimap delivery plan

Deliver this UI feature as Phase 5 of the world-generation track, before biome expansion. It is a navigation aid, not a separate world simulation or a source of hidden gameplay information.

1. Add a `KalmalaUI` minimap view model that converts the locally available generated-world presentation and the owning player's replicated transform into map-space data. It must not query world actors directly or expose undiscovered server-owned population, loot, hazards, or other players beyond the normal game presentation contract.
2. Add a circular minimap widget anchored to the top-right HUD. Clip all terrain, water, and markers to the circle; keep the owning-player marker visible at the centre and rotate it to communicate facing direction.
3. Render a lightweight local representation of terrain, water, and known player-facing landmarks. Reuse the replicated world identity and deterministic terrain/biome sampling where appropriate; do not add a second biome map or replicate minimap textures.
4. Bind mouse-wheel input to the minimap zoom only while no modal UI owns the input. Expose tunable `MinZoom` and `MaxZoom` limits, clamp every wheel update to that range, and retain the selected local zoom for the active session.
5. Verify at multiple aspect ratios and UI scales that the circular mask remains top-right, the player marker remains legible, zooming clamps at both limits, and opening/closing other UI cannot trap movement or mouse-wheel input.
6. Run a host/client test confirming both players see a minimap derived from the same world identity while each sees only their own player-centred view; minimap interaction must neither mutate nor reveal server-authoritative gameplay state.
