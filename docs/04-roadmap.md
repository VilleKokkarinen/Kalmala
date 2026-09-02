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
