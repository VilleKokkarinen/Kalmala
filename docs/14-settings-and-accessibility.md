# M5 local settings and accessibility contract

This document defines the local settings and accessibility surface for the
existing Kalmala settings shell. It covers presentation and input preferences
only; it does not add gameplay tuning, a replicated option, a server setting,
or a new save schema.

## Existing shell and option groups

The existing local menu opens and closes with **Escape**, owns modal input while
open, and exposes the Options and Quit actions. Options already contains the
Video, Audio, Controls, and Settings tabs. Video currently applies local
resolution, V-Sync, window mode, and render-distance quality through Unreal's
local `GameUserSettings` path. The remaining groups are the ordered M5 surface
to implement:

- **Audio:** local master, music, ambient, and interaction/combat feedback
  levels plus a mute path. Audio changes must have a readable text confirmation
  and must not be required to understand gameplay state.
- **Controls:** local keyboard/controller bindings for the existing movement,
  look, jump, sprint, interact, attack, map, recenter, settings, and craft
  actions. The Controls tab now exposes bounded keyboard/controller choices,
  shows each current label, applies a change to only the owning local
  `UPlayerInput`, and provides a restore-defaults action without changing what
  the server validates.
- **Settings:** local text scale and contrast choices, together with the
  colour-independent feedback preference used by Wet, hearth, construction,
  combat, discovery, and support presentation.

Exact control ranges and device-specific labels remain implementation details;
they must stay bounded, reversible, and compatible with the current input
bindings. This increment makes no platform, visual-identity, or audio-content
decision.

The Audio tab provides a local master-volume cycle in 25% steps and a
mute/restore button. Ambient, music, and interaction/combat feedback each have
a focusable category button that cycles through 0%, 25%, 50%, 75%, and 100%.
Every button reports its current level or action in text. These values save in
the local `GameUserSettings` config. The master value applies through the
engine's primary output-volume multiplier; the ambient value scales the local
wind, rain, water, fire, and biome loops, and the interaction/combat value
scales owner-local movement, status, crafting/gathering, discovery, combat, and
support one-shots. The music value is ready for a future music playback path;
there is no music track in the current runtime.

The Controls tab stores only allowlisted local choices in the existing
`GameUserSettings` configuration. Movement axes retain positive/negative pairs
when a keyboard layout is changed; action and look bindings replace only the
selected keyboard or controller device family. Applying or restoring a choice
rebuilds the local player's input map immediately, and a fresh controller
instance rehydrates the same local choices. The project baseline in
`Config/DefaultInput.ini` is never rewritten. Escape remains available for the
modal close path even when the alternate Settings action key is changed.

The Settings tab now provides local text-scale choices of 100%, 125%, and 150%
and Standard or High contrast. Text choices immediately rebuild the modal with
scaled, auto-wrapped labels and buttons inside its larger bounded panel;
Controls remains scrollable at the largest size. Contrast updates the local
backdrop, panel, button surfaces, text, and focusable state controls together,
while every state continues to expose an explicit text value. Both choices
persist in the existing local `GameUserSettings` configuration and affect no
gameplay widget, replicated property, or server request.

## Existing input baseline

The current normal-player baseline in `Config/DefaultInput.ini` is the source
for the Controls tab's initial labels. It must remain available while local
remapping is added:

| Action or axis | Keyboard/mouse baseline | Controller baseline |
| --- | --- | --- |
| MoveForward | W/S and Up/Down | Left stick Y |
| MoveRight | A/D and Left/Right | Left stick X |
| Turn / LookUp | Mouse X / Mouse Y | Right stick X / Right stick Y |
| MinimapZoom | Mouse wheel | — |
| Interact | E | Face button bottom |
| Attack | Left mouse button | Right shoulder |
| Jump | Space | Face button left |
| Sprint | Left Shift / Right Shift | Left stick click |
| SettingsMenu | Escape / O | — |
| WorldMap / WorldMapRecenter | M / R | — |
| CraftMenu | B | Special left |
| Support selection | 1–4 | D-pad directions |
| Support activation | Q | Face button top |

The baseline check proves that these existing names and inputs are present; it
does not make them remappable. A runtime remapping presenter must display the
actual current binding, retain a keyboard/controller path to cancel or reset,
and send the same existing intent rather than a new gameplay payload.

## Accessibility requirements

- Every setting is reachable with keyboard focus and a controller, with visible
  focus, stable tab/order navigation, and an Escape path back to the prior menu.
- Text scale changes must keep labels, values, help text, and action buttons
  readable without clipping the existing modal layout. The option itself must
  not require reading a colour or hearing a cue.
- Contrast changes must affect local UI surfaces, text, focus, and state
  indicators together. State must still be distinguished by text, shape,
  pattern, or icon when colour is unavailable.
- Audio controls must have text equivalents for their current value and a
  mute/restore path. Silence must not remove the only indication of damage,
  Wet, hearth, construction, discovery, or support state.
- The master-volume and mute/restore controls remain focusable buttons, expose
  their current value and action in text, and retain the menu's Escape return.
- Cancel, apply, and reset actions must communicate whether a local change was
  retained. Invalid or out-of-range input falls back to the last valid local
  value and never reaches gameplay code.

## Local storage and authority boundary

Settings are local presentation preferences. They may use the existing local
user/configuration path alongside `GameUserSettings`, but must not be written
to world saves, player progression saves, sparse world deltas, or replicated
properties. Changing a setting sends no RPC and cannot alter world identity,
terrain, population, interaction validation, weather, exposure, inventory,
construction, combat, discoveries, learned effects, rewards, or persistence.

The server continues to own gameplay outcomes. A client may use a local
control binding to express the same existing movement or narrow action intent,
but it cannot use settings to select a target, damage, quantity, transform,
reward, status, weather value, or effect execution. Local UI must render the
authoritative replicated result where one exists and retain a text/shape
equivalent for colour-independent use.

The master audio preference is applied independently by each running game
process. In a listen-server session it affects that process's local output; it
does not change audio or state on a connected remote client.
The category values are local configuration too. Ambient and one-shot
presenters read them from the current process and apply them only to its local
player components or cue submissions; a connected peer has separate settings.

## Acceptance and limits

The runtime implementation is acceptable when a fresh local profile can open,
navigate, change, cancel, apply, reset, and persist each option group with
keyboard and controller input, while a second player observes no replicated
settings state. The Controls increment covers bounded local remapping and
restore defaults; the rendered check must still cover text scale, contrast,
focus, non-colour feedback, mute/restore, and the Escape modal flow at the
supported viewport sizes.

`Scripts/Verify-SettingsAccessibilityContract.ps1` checks this contract
without Unreal. It proves the documented option groups and boundaries are
present, not that the runtime menu or packaged accessibility flow is complete.
The focused `Kalmala.UI.Settings.LocalPresentation` automation checks
master-volume bounds, local config round-trip, immediate mute and restore,
category-level bounds and config round-trips, bounded control labels, local
control persistence and restore defaults, and text-scale/contrast bounds and
round-trips. It does not render the Settings or Controls tabs, simulate physical
controller input, establish audible quality, or prove that the larger modal fits
every supported viewport. Colour-independent feedback preferences and the full
rendered settings-persistence verification remain queued.
