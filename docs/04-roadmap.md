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

## Next roadmap goals

These goals are ordered after the current M5 acceptance. They are planning
targets, not approval to change the platform, business model, online-service
commitment, visual identity, saved-data schemas, or vertical-slice scope.
Do not begin a later goal until the preceding goal has passed its exit gate.

### Goal 1 — Close the vertical slice

Finish the remaining M5 work in the order recorded by `BACKLOG.md`:

1. Complete the bounded survival, combat, creature, and support balance pass.
   Each increment must preserve server-selected outcomes, intent-only client
   requests, recoverable preparation choices, and the existing save and
   replication contracts.
2. Re-run the affected authority, persistence, reconnect, and host/client
   regressions after each balance change; keep the retained evidence and
   update the handoff limits.
3. Run release regression and packaging verification, including the current
   generator, rendered checks where available, Windows Development package
   smoke launch, and the documented dedicated-server attempt only if a
   server-capable UE 5.8 build exists.
4. Run the final M5 acceptance with a fresh player through the documented
   20–30 minute co-op loop using normal packaged-player actions only.

**Exit gate:** the packaged loop is repeatable without developer tools, peers
observe matching relevant state, optional routes remain optional, reconnect
and persistence evidence is retained, and all remaining limitations are
explicitly recorded. The Launcher-engine dedicated-server limitation remains
an honest blocker when no server-capable engine is installed.

### Goal 2 — Stabilize the accepted slice

After Goal 1 passes, convert every final-acceptance finding into a small,
replayable regression or a documented non-goal. Prioritize crash and hang
triage, reconnect and late-join reliability, save-identity safety, input and
accessibility regressions, and the existing actor, memory, worker, raster,
startup, and package budgets. Keep the generated world, population density,
save schemas, and online-service scope bounded while fixing defects.

**Exit gate:** the supported Windows Development package passes the complete
automated regression set twice from clean temporary user directories, the
documented performance budgets remain green, and no known defect can mutate
server-owned gameplay through client presentation or input paths.

### Goal 3 — Validate dedicated-server deployment when unblocked

Treat dedicated hosting as a conditional infrastructure goal, not a reason to
change the current product scope. When a server-capable UE 5.8 source build or
distribution is available, compile the retained `KalmalaServer` target and
run a bounded two-to-four-player playtest covering join, movement, interaction,
weather and camp recovery, creature encounters, support effects, late join,
reconnect, and sparse persistence. If the installed Launcher engine remains
the only available distribution, retain the documented blocker and continue
to support editor/listen-server development without inventing a replacement
service.

**Exit gate:** the dedicated server owns world, combat, support, discovery,
reward, and save decisions; clients provide intent only; late join and
reconnect preserve the same world identity; and the session remains inside
the accepted actor, memory, replication, and save budgets.

### Goal 4 — Expand the player-directed generated wilderness

Only after the vertical slice is accepted and stabilized, select one original
post-slice content slice from the existing generated-world direction. A slice
may deepen a biome, add an optional landmark or discovery, improve a wildlife
behavior loop, or extend ocean/island travel, but it must reuse the immutable
world identity and existing population, exposure, discovery, persistence, and
map contracts. It must not add authored routes, mandatory crossings, combat
tier zones, hidden-content queries, or client-selected rewards.

Deliver one integrated increment at a time: deterministic generation,
server-owned activation and persistence, local presentation, accessible
feedback, host/client agreement, and a measured performance budget. Prefer
environmental and preparation choices over linear enemy scaling, and require a
viable lower-risk approach wherever a new region or feature adds pressure.

**Exit gate:** the new slice reproduces for the same seed, varies meaningfully
for a different seed, blends continuously with existing terrain and biomes,
preserves co-op privacy and authority, survives reconnect/restart where its
contract requires it, and leaves the accepted 20–30 minute loop playable.

### Goal 5 — Revisit broader production commitments

Before starting work that changes the supported platform, business model,
online services, persistent-data schema, or post-slice product shape, record an
explicit decision covering the target platforms and session model, hosting or
service requirements, save migration policy, and the next content priority.
Until that decision exists, keep production work inside the current PC
solo/listen-server co-op scope, use original project-owned presentation, and
preserve the current server-authority and sparse-save contracts.

**Exit gate:** the decision is accepted by the project owner and reflected in
the relevant design, architecture, setup, and backlog documents before any
implementation begins.

### World-generation Phase 5 — Companion minimap delivery plan

Deliver this UI feature as Phase 5 of the world-generation track, before biome expansion. It is a navigation aid, not a separate world simulation or a source of hidden gameplay information.

1. Add a `KalmalaUI` minimap view model that converts the locally available generated-world presentation and the owning player's replicated transform into map-space data. It must not query world actors directly or expose undiscovered server-owned population, loot, hazards, or other players beyond the normal game presentation contract.
2. Add a circular minimap widget anchored to the top-right HUD. Clip all terrain, water, and markers to the circle; keep the owning-player marker visible at the centre and rotate it to communicate facing direction.
3. Render a lightweight local representation of terrain, water, and known player-facing landmarks. Reuse the replicated world identity and deterministic terrain/biome sampling where appropriate; do not add a second biome map or replicate minimap textures.
4. Bind mouse-wheel input to the minimap zoom only while no modal UI owns the input. Expose tunable `MinZoom` and `MaxZoom` limits, clamp every wheel update to that range, and retain the selected local zoom for the active session.
5. Verify at multiple aspect ratios and UI scales that the circular mask remains top-right, the player marker remains legible, zooming clamps at both limits, and opening/closing other UI cannot trap movement or mouse-wheel input.
6. Run a host/client test confirming both players see a minimap derived from the same world identity while each sees only their own player-centred view; minimap interaction must neither mutate nor reveal server-authoritative gameplay state.
