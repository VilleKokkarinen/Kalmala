# M5 onboarding and tutorial contract

This document defines optional, local onboarding prompts for a fresh player.
Prompts teach the existing Kalmala loop without turning the generated wilderness
into a route, quest chain, or mandatory camp. The contract is deliberately
presentation-only; it does not add a gameplay action, server state, save field,
replicated property, or online service.

## Prompt rules

- Prompts are short, dismissible, and contextual. They may appear once per
  local first-run profile, but completing or dismissing one never changes a
  server-owned value.
- A prompt may trigger from a normal local input, a readable replicated state,
  or an already visible actor. It must not query hidden population, undiscovered
  rewards, private pins, target IDs, or future route information.
- Prompts describe choices rather than objectives. They may say what an input
  does and what a player can try; they must not select a destination, camp site,
  creature, reward, recipe outcome, or support target.
- Every prompt has a text equivalent, a non-colour icon/shape cue, and the
  keyboard/controller input label. A player can dismiss, revisit, or ignore all
  prompts without losing normal movement or menu access.
- Prompt timing is local presentation timing. It cannot pause the server, grant
  an interaction, alter stamina or Wet, confirm a hit, complete a discovery, or
  advance a reward.

## Route-free beat matrix

| Beat | Local trigger | Player-facing prompt | Required boundary |
| --- | --- | --- | --- |
| Arrive | Fresh local pawn becomes controllable | “Move with WASD or the left stick. Look around, jump with Space, and hold Shift to sprint.” | Does not name a direction, coordinate, route, or destination. |
| Interact | The player first receives normal interaction focus on a visible actor | “Face a nearby usable object and press E, or the controller interact button.” | The prompt reflects only the existing local focus; the server still validates the request and outcome. |
| Gather | The player has a visible, usable harvest node in normal range | “Gather what you need from the wilderness. Your pack shows what was accepted.” | Does not reveal distant nodes, quantities, depleted state, or a preferred resource. |
| Prepare | The player opens the existing camp crafting UI | “Choose what to make, then place it where the terrain and your materials allow.” | No fixed camp, construction transform, payment result, or shelter success is promised locally. |
| Weather | The local HUD already shows an active Wet or exposure-related cue | “Weather changes comfort and travel. Shelter, cover, and a lit hearth are options.” | Uses existing replicated presentation only; it never invents a local weather value or a guaranteed recovery. |
| Explore | The player leaves the immediate start area through normal movement | “Pick a heading and see what the generated land offers. The map is for orientation.” | Never points to an authored corridor, biome, encounter, discovery, or required return point. |
| Optional encounter | A relevant creature is already visible or the player has initiated the normal attack intent | “You can engage or move on. Attacks are committed by the server.” | Does not reveal hidden creatures, target selection, damage, cooldown, loot, or defeat outcome. |
| Discovery | A visible server-owned discovery is already interactable | “This discovery is optional. Interact normally if you want to investigate.” | Does not show undiscovered IDs, reward contents, private entitlement, or duplicate-claim state. |
| Support magic | The entitled player has already learned an effect and opens its existing local UI | “Support effects help an eligible ally or situation; choose a valid target when the UI allows.” | Learned effects, targets, durations, stamina, cooldowns, and execution remain server-owned; no effect deals direct damage. |
| Return | The player chooses to head back or remains near a visible camp | “You can return, shelter, use the hearth, store materials, or keep exploring.” | Return is a choice, not a quest completion, timer, route requirement, or persistence grant. |

The first three beats teach controls and interaction. The remaining beats are
opportunistic: a player may encounter them in any order, never encounter some
of them, or dismiss them all. A fresh-player walkthrough may therefore record
which prompts were applicable rather than requiring every prompt to appear.

## Presentation and accessibility acceptance

The runtime presenter is acceptable when a fresh local session can demonstrate
the following without developer commands or fixed fixture coordinates:

1. Arrival teaches movement, jump, and sprint using the actual bound labels.
2. A visible normal interaction can teach interaction and gathering without
   promising acceptance before the server response.
3. Camp, weather, exploration, optional encounter, discovery, support, and
   return prompts appear only when their local visible context exists.
4. Every prompt can be understood from text and shape/icon cues without colour
   or audio, and keyboard and controller labels remain readable.
5. Dismissing or ignoring prompts leaves movement, interaction, combat intent,
   support UI, map access, and reconnect behaviour unchanged.
6. A two-player observation shows no prompt-driven replication, reward leak,
   client-selected outcome, or private discovery/learned-effect disclosure.

The runtime presenter is a `ULocalPlayerSubsystem`. It shows one short prompt
at a time after possession, from a local visible-focus trace, the local map
position relative to the initial pawn position, the existing crafting shell,
or the owning pawn's readable replicated Wet and learned-effect state. Visible
harvest nodes, discoveries, wildlife, usable actors, and campfires are tested
only after the local view trace hits them. The trace never enumerates active or
hidden world content. Prompt history lasts for the local-player session only;
it is not written to gameplay or local save data. Each card shows readable text,
a high-contrast compass shape, and the current keyboard/controller labels.
F1 / the controller's B button dismisses the current card; F2 / right-stick
click revisits the last card. Both bindings observe input without consuming
movement or interaction. Prompts expire after 18 seconds or disappear when
their context ends.

`Scripts/Verify-OnboardingContract.ps1` validates the specification; it is not
a runtime presentation test. `Scripts/Verify-LocalInputContract.ps1` checks the
actual keyboard/controller movement, interaction, and prompt bindings. The
fresh-player context walkthrough, route-free trigger/privacy review, and
packaged two-player evidence remain part of the acceptance work.

## Authority, privacy, and persistence

Prompts are local UI state. The server continues to own world identity,
terrain, population, interactions, weather, exposure, construction, inventory,
combat targeting and damage, discoveries, learned effects, rewards, and sparse
world/player saves. A client may observe only the same relevant replicated
actors and values it could already observe during normal play. No prompt sends
a new RPC or carries an item ID, target, quantity, damage, transform, reward,
weather, status, or effect payload.

Prompt dismissal and history are session-local. A future local settings option
may persist a prompt preference; it must never be included in the gameplay save schema
or replicated to another player.
