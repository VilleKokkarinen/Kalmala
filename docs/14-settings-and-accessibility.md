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
  actions. The presenter must expose the actual bound label and a restore
  defaults action without changing what the server validates.
- **Settings:** local text scale and contrast choices, together with the
  colour-independent feedback preference used by Wet, hearth, construction,
  combat, discovery, and support presentation.

Exact control ranges and device-specific labels remain implementation details;
they must stay bounded, reversible, and compatible with the current input
bindings. This increment makes no platform, visual-identity, or audio-content
decision.

## Existing input baseline

The current normal-player baseline in `Config/DefaultInput.ini` is the source
for the Controls tab's initial labels. It must remain available while local
remapping is added:

| Action or axis | Keyboard/mouse baseline | Controller baseline |
| --- | --- | --- |
| MoveForward | W/S and Up/Down | Existing movement axis extension |
| MoveRight | A/D and Left/Right | Existing movement axis extension |
| Turn / LookUp | Mouse X / Mouse Y | Right stick X / Right stick Y |
| MinimapZoom | Mouse wheel | — |
| Interact | E | Face button bottom |
| Attack | Left mouse button | Right shoulder |
| Jump | Space | — |
| Sprint | Left Shift / Right Shift | — |
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

## Acceptance and limits

The runtime implementation is acceptable when a fresh local profile can open,
navigate, change, cancel, apply, reset, and persist each option group with
keyboard and controller input, while a second player observes no replicated
settings state. The rendered check must cover text scale, contrast, focus,
non-colour feedback, mute/restore, and the Escape modal flow at the supported
viewport sizes.

`Scripts/Verify-SettingsAccessibilityContract.ps1` checks this contract
without Unreal. It proves the documented option groups and boundaries are
present, not that the runtime menu or packaged accessibility flow is complete.
Runtime UI and persistence verification remain queued for when Unreal build
access is available.
