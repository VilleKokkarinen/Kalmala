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

**Accept:** two players choose their own route through the world, learn and use every support effect, encounter all three creature archetypes, and return with a progression reward. No magic effect directly damages an enemy.

## M5 — Vertical-slice finish

Add original art/audio pass, tutorial beats, settings/accessibility, performance pass, balance, regression tests, packaging, and a dedicated-server playtest.

**Accept:** a new player can complete the documented 20–30 minute co-op loop without developer tools.

### World-generation Phase 5 — Companion minimap delivery plan

Deliver this UI feature as Phase 5 of the world-generation track, before biome expansion. It is a navigation aid, not a separate world simulation or a source of hidden gameplay information.

1. Add a `KalmalaUI` minimap view model that converts the locally available generated-world presentation and the owning player's replicated transform into map-space data. It must not query world actors directly or expose undiscovered server-owned population, loot, hazards, or other players beyond the normal game presentation contract.
2. Add a circular minimap widget anchored to the top-right HUD. Clip all terrain, water, and markers to the circle; keep the owning-player marker visible at the centre and rotate it to communicate facing direction.
3. Render a lightweight local representation of terrain, water, and known player-facing landmarks. Reuse the replicated world identity and deterministic terrain/biome sampling where appropriate; do not add a second biome map or replicate minimap textures.
4. Bind mouse-wheel input to the minimap zoom only while no modal UI owns the input. Expose tunable `MinZoom` and `MaxZoom` limits, clamp every wheel update to that range, and retain the selected local zoom for the active session.
5. Verify at multiple aspect ratios and UI scales that the circular mask remains top-right, the player marker remains legible, zooming clamps at both limits, and opening/closing other UI cannot trap movement or mouse-wheel input.
6. Run a host/client test confirming both players see a minimap derived from the same world identity while each sees only their own player-centred view; minimap interaction must neither mutate nor reveal server-authoritative gameplay state.
