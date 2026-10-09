# M5 local settings and accessibility contract

This document defines the local settings and accessibility surface for the
existing Kalmala settings shell. It covers presentation and input preferences
only; it does not add gameplay tuning, a replicated option, a server setting,
or a new save schema.

M11 places Wet, meal, exposure, support and weather status markers in one
transparent owner-local hotbar. Escape > Status and weather details exposes
their live text without hover. The optional feedback overlay retains action
results and nearby hearth/construction context, with no duplicate active
status rows. See `36-status-icons.md` for the updated presentation contract.

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

The Escape home/settings shell and all four option tabs use the local shared UI
theme for their background image. Separate per-view keys allow a theme to
customize the shell and tabs independently; the default reuses one original
spruce-and-slate texture. High contrast suppresses decorative images and keeps
the black panel, white text, and focus borders. This presentation does not
change settings, input bindings, or modal ownership.

The Escape panel opens with a short theme-configured downward slide. Focus and
modal input ownership take effect immediately, and closing/reopening or
resizing during the animation does not delay gameplay actions or change menu
content. Theme configuration can disable the animation for an instant path, and
the local Reduced motion setting below also overrides decorative animation.

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

The Settings tab also provides a whole-interface scale from 80%, 90%, 100%,
110%, or 120%, defaulting to 100%, plus a Reduced motion On/Off choice that
defaults to Off. Interface scale is a local multiplier over the project's
existing DPI curve and `UUserInterfaceSettings::ApplicationScale`; it changes
the game-layer DPI scale so all game UI reflows together and hit testing follows
the displayed geometry. It remains separate from text scale and is stored only
in the existing local `GameUserSettings` configuration. Reduced motion
overrides theme animation defaults for the options-panel slide, button/card
highlight transitions, and animated scrolling. Static focus, selection,
contrast, labels, status changes, and menu actions remain immediate and
visible. Theme animation keys continue to control those effects when Reduced
motion is Off. Neither option creates a gameplay setting, RPC, replicated
value, world/player save field, or new save schema.

The Settings tab also provides a local colour-independent feedback choice:
**Text only** keeps the existing readable state lines, while **Text + markers**
adds explicit bracketed state markers for Wet, hearth, construction, combat,
discovery, and support status in a local owner-only overlay. The marker mode
uses text rather than colour as the distinction, follows the local contrast
palette, follows the scaled viewport, and updates as the existing accepted
gameplay state changes. It is stored beside the other local settings and never
becomes a gameplay signal.

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
| Build and crafting menu (`CraftMenu`) | B | Special left |
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
- Whole-interface scale is independently adjustable from text scale, persists
  locally, reflows the HUD and menus, and keeps their controls inside the
  supported viewport. Reduced motion bypasses decorative movement while
  keeping essential state feedback, focus, and actions immediate.
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
The colour-independent feedback mode is likewise local to each player process.
Its overlay reads only that owner's existing pawn components and replicated
results; it creates no request, target, reward, or hidden-content indication.

## Acceptance and limits

The runtime implementation is accepted when a fresh local profile can open,
navigate, change, cancel, apply, reset, and persist each option group with
keyboard and controller bindings, while a second player observes no
replicated settings state. `Scripts/Verify-SettingsAccessibility.ps1` runs
isolated host/client profiles, opens the Escape shell and live Video, Audio,
Controls, and Settings tabs; it checks focusable targets, modal input
ownership, and Escape recovery, persists every local option, and compares the
pawn health, transform, and server-selected world identity before and after
the probe. It checks interface scale and reduced motion in local config, then
restarts both peers with the same user directories and confirms both choices
reload and the scale is applied over the project default. It retains fourteen
host/client PNG captures at either 1280x720 or 1024x768: standard-contrast
Escape, Video, Settings, reduced-motion Settings, and restored HUD views plus
high-contrast Controls and Audio views. Run both viewport sizes and inspect the
modal, HUD, and artwork; these checks do not replace physical
keyboard/controller or packaged verification.

`Scripts/Verify-SettingsAccessibilityContract.ps1` checks this contract
without Unreal. The focused `Kalmala.UI.Settings.LocalPresentation` automation
and the rendered peer probe cover local round-trips, bounded input mappings,
focusable controls, and the authority boundary. They do not simulate physical
controller hardware, establish audible quality, prove packaged persistence,
or replace the final full settings acceptance.
The focused `Kalmala.UI.Settings.LocalPresentation` automation checks
master-volume bounds, local config round-trip, immediate mute and restore,
category-level bounds and config round-trips, bounded control labels, local
control persistence and restore defaults, and text-scale/contrast bounds and
round-trips plus the colour-independent feedback mode bounds and local
round-trip. It also checks the 80–120% interface-scale bounds, application over
the project scale, independent text-scale state, and local reduced-motion
persistence. The rendered peer probe covers the live tabs, viewport reflow,
focus, contrast, both animation paths, local restart persistence, and gameplay
state boundary at 1280x720 and 1024x768; physical controller hardware, audible
quality, and packaged persistence remain outside this verification.

## M11 Settings label polish

The Settings tab uses two-line option controls: the option name above its current
value, with the shared theme body size plus three. This gives the 150% text-scale
view an explicit desired height without stale centered-text auto-wrap width and
removes repeated activation hints from individual values. Existing button focus,
activation, immediate local application, contrast treatment, and persistence stay
unchanged. Other option tabs remain part of the broader view polish pass.

## Development increment: Options tab memory

The owner-local settings widget retains its last Options tab (Video by default)
across ordinary close/reopen. Escape still opens the main shell; choosing Options
rebuilds the remembered tab with current labels and its normal focus target.
Restoration invokes no setting-changing click handler. The index is bounded to
four existing tabs, with Video as the dispatch fallback. Memory ends when the
widget is recreated, including subsystem teardown; it writes no config/save data.
Scroll position and exact previously focused control are not yet retained.
The rendered settings accessibility fixture now returns from the Settings tab
to the main shell, reopens Options, and requires the Settings tab and a focusable
control to be restored on both peers while local interface/text scales and
master volume remain unchanged. This covers the Options-tab-only increment;
other menus, scrolling, exact control focus, and full per-menu restoration remain
part of the broader M11 task.
