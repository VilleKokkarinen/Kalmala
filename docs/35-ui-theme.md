# M11 local UI theme

The first foundation increment migrates the survival-status panel and weather
activity badge to `FKalmalaUITheme` shared panel/text styling. Edit
`Config/DefaultKalmalaTheme.ini`, section `[Kalmala.UI.Theme]`, and restart the
game/editor process. Unreal's local `KalmalaTheme` config hierarchy is loaded
once, read-only; widgets share the resolved theme. No live reload is supplied.

| Key | Default | Accepted values |
| --- | --- | --- |
| Panel | (0.025, 0.035, 0.04, 0.94) | Linear RGBA components in 0–1 |
| Heading | (0.75, 0.82, 0.79, 1) | Linear RGBA components in 0–1 |
| Text | (0.93, 0.96, 0.94, 1) | Linear RGBA components in 0–1 |
| HighContrastPanel | (0, 0, 0, 0.98) | RGB always black; alpha at least 0.98 |
| HeadingSize | 10 | 8–32, rounded to nearest integer |
| BodySize | 13 | 8–32, rounded to nearest integer |
| EmphasisSize | 17 | 8–32, rounded to nearest integer |
| PaddingX / PaddingY | 12 / 9 | 0–24 logical units |
| RowSpacing | 5 | 0–16 logical units; survival rows only |

Colours use `(R=...,G=...,B=...,A=...)` syntax. Missing, malformed,
non-finite, and out-of-range values fall back independently to the established
defaults. Text falls back to Unreal's existing default font. Local 100/125/150% text
scale applies after theme sizes; local high contrast forces white text on a
black panel. The weather badge now uses the same panel padding as survival
status. No input, owner-state query, RPC, timer, authority, or gameplay/save
contract changes.

Verify the foundation with `Kalmala.UI.Theme.LocalPresentation`,
`Kalmala.UI.SurvivalStatus.LocalPresentation`, and
`Kalmala.UI.WeatherActivity.LocalPresentation` using the isolated headless
pattern in `07-development-setup.md`. Theme automation checks shared widget
application, accessibility precedence, malformed values, and missing config.
It does not establish rendered layout or packaged config inclusion.

The second foundation child adds shared components with real consumers:
settings buttons (including controls and tabs) use themed normal/hover/pressed/
disabled brushes and content padding; the settings panel uses the shared panel
and optional image; its ordinary action labels use themed font styling; the
Controls scroll view uses theme animation; existing support glyph boxes use
shared icon dimensions. HUD panels/text also consume the extended styling.
The final foundation child migrates representative inventory, build/craft,
expanded-map, and options views as described below.

| Extension key | Default | Accepted values / consumer |
| --- | --- | --- |
| FontAsset | empty | Project-owned runtime UFont object path; shared text |
| FontFace / HeadingFace | Regular / Bold | Regular or Bold; missing custom face uses font default |
| PanelImage | empty | Project-owned Texture2D object path; shared panels |
| InventoryPanelImage | `/Game/Kalmala/UI/InventoryPanel.InventoryPanel` | Texture2D override for the modal Inventory panel |
| BuildPanelImage | `/Game/Kalmala/UI/BuildPanel.BuildPanel` | Texture2D override for build/craft selection |
| WorldMapPanelImage | `/Game/Kalmala/UI/WorldMapPanel.WorldMapPanel` | Texture2D override for the expanded map shell |
| EscapePanelImage | `/Game/Kalmala/UI/OptionsPanel.OptionsPanel` | Escape home/options shell; fallback source for empty tab overrides |
| VideoOptionsPanelImage | `/Game/Kalmala/UI/OptionsPanel.OptionsPanel` | Video tab; empty falls back to EscapePanelImage |
| AudioOptionsPanelImage | `/Game/Kalmala/UI/OptionsPanel.OptionsPanel` | Audio tab; empty falls back to EscapePanelImage |
| ControlsOptionsPanelImage | `/Game/Kalmala/UI/OptionsPanel.OptionsPanel` | Controls tab; empty falls back to EscapePanelImage |
| SettingsOptionsPanelImage | `/Game/Kalmala/UI/OptionsPanel.OptionsPanel` | Settings tab; empty falls back to EscapePanelImage |
| OutlineSize | 0 | 0–3, rounded; black text outline |
| BorderWidth / CornerRadius | 1 / 3 | 0–3 / 0–12; geometric panels and button states |
| BorderColor | (0.35, 0.45, 0.42, 1) | Linear RGBA 0–1 |
| ButtonNormal | (0.11, 0.16, 0.19, 1) | Linear RGBA 0–1 |
| ButtonHovered | (0.18, 0.25, 0.28, 1) | Linear RGBA 0–1 |
| ButtonPressed | (0.07, 0.10, 0.12, 1) | Linear RGBA 0–1 |
| ButtonDisabled | (0.08, 0.08, 0.08, 1) | Linear RGBA 0–1 |
| SlotPadding | 3 | 0–16; button content, pressed adds one unit |
| IconWidth / IconHeight | 64 / 42 | 24–96; existing support glyph slots |
| AnimateScrolling / ScrollSpeed | False / 15 | Boolean / 1–60; Controls wheel scrolling |
| AnimateOptionsOpening | True | Boolean; set False for an instant/reduced-motion opening |
| OptionsOpeningDuration | 0.18 seconds | 0–0.8 seconds; zero selects the instant path |
| OptionsOpeningTravel | 32 logical units | 0–96; zero selects the instant path |
| OptionsOpeningEasing | `EaseOutCubic` | `EaseOutCubic`, `EaseOutQuad`, or `Linear` |

Asset keys accept only valid `/Game/Package.Asset` object paths shorter than
180 characters. Empty, invalid, missing, or wrong-type objects retain the
built-in font or geometric panel. Offline bitmap fonts also fall back safely. No asset is imported/generated by this
increment. A panel image replaces the geometric panel border; high contrast
ignores it, uses a black-backed panel and white border/text, and forces button
borders to at least one unit. High contrast uses neutral button fills; pressed
padding and Unreal's existing keyboard focus indicator remain available.
Accessibility text scale still takes precedence over theme size. No focus,
click delegate, navigation, modal ownership, authority, or save binding changed.

The view-specific image keys override `PanelImage` only for their named view.
The inventory, build, and expanded-map textures are original Kalmala artwork
imported from `Content/Kalmala/UI/Source/` into `/Game/Kalmala/UI`; missing or
invalid view paths keep the geometric fallback. High contrast intentionally
suppresses the decorative backgrounds.

`ApplyScroll` has an explicit reduced-motion override; scroll animation defaults off.
The local Reduced motion choice also overrides scrolling. The options-opening
slide is documented below. Custom
font and image packages must already be available to the runtime; this child
does not establish cooking/inclusion of config-only asset references.

Focused theme automation exercises changed brushes, padding, font face/outline,
icon dimensions, scroll animation and reduced-motion override, high contrast,
invalid numbers/paths, missing assets, and a transient in-memory image reference.
The settings and Inventory menu tests remain required alongside it. No
custom project font or rendered image appearance has been reviewed.

## Representative view migration

The modal Inventory menu's panel, labels, and scrolling use the shared theme.
The former left-side pack panel and its bespoke wrapping layout were removed in
M12. Build/craft uses
ApplyMenu over the existing widget tree: ordinary labels and actions use
BodySize + 5, its title uses EmphasisSize + 11, and all sizes are bounded before
local text scaling. Disabled buttons, delegates, selection, tooltips, and
server-derived costs remain intact. Accessibility changes restyle the whole
build tree rather than only its detail label.

Options labels, tab text, and control-binding labels now share font assets,
faces, outlines, colours, and semantic-size offsets. Existing layout offsets,
modal backdrop and content navigation stay local and retain their established
placement. Some ancillary HUD/support glyph styling still uses semantic
colours; this parent establishes representative shared components, not the
later per-view graphical-polish tasks.

The expanded map consumes MakePanelBrush and MakeFont directly in Slate.
Controls wrap to available width, use a readable theme-backed strip above
terrain, and stack the selected-pin heading beneath the wrapped main hint.
High contrast forces the same black surface and white text as UMG. Terrain,
fog, personal pins, co-op visibility, marker colours, map geometry, and map
input are unchanged. Slate painting explicitly passes the shared brush tint;
UMG handles that tint through its border widget. The developer map fixture
waits for its closed-map exploration record before opening, avoiding a race
with joining-client world-identity arrival. Normal M-key opening does not wait.

The theme foundation did not add new background art, item icons, or slot grids.
The local interface-scale and reduced-motion settings are documented below.
The Escape opening animation is covered below. Theme-only
font/image assets must be runtime-available; packaged config/asset inclusion
and custom project-font appearance are not established by editor integration
checks.

## Inventory and build backgrounds and slot grids — 2026-10-02

Inventory uses the portrait birch/timber background and a fixed four-column,
16-slot read-only pack grid. Each stack keeps its catalogue icon, display name,
and current quantity. Carried tools appear in a separate framed row with their
existing level and condition. Empty cells remain visible. The grid reads the
owner-local inventory snapshot and adds no per-slot action, transaction, or
saved slot identity.

The construction/build menu uses the original lake/camp background and a
four-column grid of canonical recipe/build icons. Each card reports its name
and current selected, focused, available, or unavailable state in text as
well as themed styling. Existing selected-result text remains the source of
truth for material costs, station/skill requirements, availability, and
rejection details. Keyboard arrows and controller D-pad continue to select
through the focused menu; the scroll view preserves access to the full recipe
list after text scaling or smaller viewports.

Both views use the shared theme and existing local text-scale/contrast
settings. Populated inventory and the full build menu report positive scroll
extent in the rendered checks. High contrast hides background art and keeps
black panels, white text, and borders. Selected host captures and exact checks
are retained in `docs/ui-inventory-build/`.

Parent verification passed with the forced UE 5.8.2 editor build, all 101
Kalmala automations, and rendered host/client inventory and crafting runners
at 1280x720 standard and 1024x768 at 150% text/high contrast. The automated
input probe sends keyboard Down/Up and gamepad D-pad Down/Up through the same
focused widget preview handler, confirms selection updates and restoration,
and checks the measured scroll extent. This does not replace a physical
keyboard/controller playthrough or a cooked/package asset-inclusion check.

## Parent verification — 2026-10-02

The foundation parent is complete. The isolated UE 5.8.2 editor build passed;
all 99 Kalmala automations passed with exit 0. Inventory authority/privacy,
rendered build/craft, settings/accessibility and expanded-map host/client
regressions passed. Default map checks covered 1024x768, 1280x720 and
2560x1080; settings checks covered 150% text and high contrast. Modified-theme
build/HUD, map and options checks passed, including unavailable font/image
fallback; a further 1024x768 map check confirmed 150% text/high-contrast
precedence. All checks used isolated profiles and the disposable mirror.

The test theme changed BodySize 13 to 15, EmphasisSize 17 to 19, panels and
buttons to blue, and heading/body text to warm gold. FontAsset and PanelImage
pointed at deliberately missing project objects. Production config remains
unchanged. Map comparison is especially useful: terrain/fog remain the same
while frame/text styling changes, and high contrast restores black/white.

Reviewed captures are in `ui-theme/`: `build-default.png`, `build-custom.png`,
`map-default.png`, `map-custom.png`, `map-hc.png`, `settings-hc.png`,
`controls-hc.png`, `audio-hc.png`, and `settings-custom-hc.png`.
`ui-theme/verification.txt` retains selected test and peer assertions. Full raw
logs remain under the temporary roots listed in PROGRESS.md and that file.
These captures review initial menu positions; later menu polishing and full
M11 acceptance retain the broader physical-input/layout requirements.

## Expanded map background — 2026-10-02

`WorldMapPanelImage` selects an original, static spruce-and-slate texture for
the full-screen expanded map shell. It contains no map, routes, symbols, or
world data. Slate paints this image before the live grid, generated terrain,
fog, pins, co-op markers, and control labels, so the existing local map and
visibility rules remain the only source of map information. Empty, invalid,
missing, or wrong-type paths use the existing geometric theme brush; local
high-contrast mode suppresses the image and uses the black shared surface.

The source art is `Content/Kalmala/UI/Source/WorldMapPanel.png`, imported to
`/Game/Kalmala/UI/WorldMapPanel`. `Kalmala.UI.Theme.LocalPresentation` checks
the configured path, valid Slate image resolution, missing-asset fallback,
and high-contrast suppression. Parent verification also renders the existing
host/client map at three viewport sizes and runs the map tile/reconnect checks.
These editor checks do not establish packaged asset cooking or physical-input
playthrough.

## Expanded-map legend and marker filters

The owner-local map legend uses the existing theme panel, body/heading fonts,
button fills, focus outline, and text/contrast palette. The facing triangle is
an unfiltered reference row; personal pins, co-op players, and temporary pings
have separate checkboxes, textual shown/filtered states, and eligible-marker
counts. Focus uses the shared focused-button treatment and a visible outline;
high contrast uses a black panel, white text, and white row/checkbox outlines.
Shape and text continue to distinguish marker categories without colour.
The legend keeps marker names and shown/filtered state, but the map no longer
prints keyboard/controller help beside its controls. Share, ping, and pin action
names remain concise; current input labels appear in Options > Controls only.

No theme keys or map content sources were added. The three checkbox states are
local to the map widget session and affect only marker drawing. Existing owner
visibility/fog gating and personal pin data remain the source of truth; see
the map verification contract in `docs/07-development-setup.md`.

## Escape options backgrounds — 2026-10-02

The Escape home/settings shell and each Video, Audio, Controls, and Settings
view now select a theme-configured image through the shared panel component.
Each view has a separate key so a project theme can give the tabs distinct
art. The defaults use the same original dark spruce-and-slate panel texture
for a consistent shell. Empty tab keys inherit `EscapePanelImage`; if that
key is also empty, the shared `PanelImage` applies. Syntactically invalid,
missing, or wrong-type objects keep the geometric panel fallback. High
contrast suppresses every decorative options image and uses the established
black surface, white text, and focus borders.

The source is `Content/Kalmala/UI/Source/OptionsPanel.png`, imported to
`/Game/Kalmala/UI/OptionsPanel`. `Kalmala.UI.Theme.LocalPresentation` checks
all five keys, valid shared-panel image resolution, invalid-path fallback, and
high-contrast suppression. The settings accessibility peer probe visits the
Escape shell and all four views, verifies each image resolves for host and
client, then confirms high contrast removes it while existing value, focus,
text-scale, input ownership, and local persistence checks pass. Ten retained
host/client captures include standard-contrast Escape, Video, and Settings
views plus high-contrast Controls and Audio views at 1280x720; inspect these for
background framing and label readability. Other viewport sizes, cooked asset
inclusion, and physical keyboard/controller walkthrough are unverified.

## Escape options opening animation — 2026-10-03

The local Escape panel drops a short distance into its centered final position
when the modal first opens. The shared panel's `UCanvasPanelSlot` position is
animated, so its hit-test geometry follows the visible panel each frame; the
content is not scrolled or re-laid out. The center anchor keeps the panel
aligned when the viewport changes during the transition. Focus, cursor, modal
input mode, and movement/look suppression are set as soon as the panel opens.
Animation progress advances with widget ticks, with catch-up capped at 1/30
second per tick so a long frame hitch cannot skip the visible motion; sustained
frame rates below 30 FPS can therefore extend the wall-clock duration. Closing
resets the slot to its final position and cancels the transition; reopening
starts at the configured offset again.

`AnimateOptionsOpening`, `OptionsOpeningDuration`, `OptionsOpeningTravel`, and
`OptionsOpeningEasing` are theme keys. Defaults are enabled, 0.18 seconds, 32
logical units, and `EaseOutCubic`. Duration accepts 0–0.8 seconds and travel
0–96 units; zero duration/travel or a false animation flag selects the instant
path. Easing accepts `EaseOutCubic`, `EaseOutQuad`, or `Linear`; invalid values
retain the cubic default. The instant path also serves as the reduced-motion
fallback; the local M11 Reduced motion preference selects it automatically.

The rendered host/client settings probe opens, resizes 1280x720 → 1600x900 →
1280x720 while the panel moves, closes and reopens mid-transition, and checks
intermediate/final slot positions, centered anchoring, immediate focus, modal
input suppression, restoration on close, keyboard/controller mappings, and
gameplay stability. `Kalmala.UI.Theme.LocalPresentation` checks theme bounds,
easing, invalid-value fallback, and the instant path. This remains a local
presentation change: it adds no gameplay timing, RPC, replicated state, or
saved-data setting. The automated peer probe does not replace physical
keyboard/controller hardware or packaged-build verification.

## Shared hover, focus, selection, and disabled feedback — 2026-10-04

`ButtonFocused` and `FocusBorderWidth` style keyboard/controller focus;
`ButtonSelected` and `SelectedBorderWidth` style active tabs and selected
controls; `DisabledBorderWidth` keeps unavailable controls visibly distinct.
`InteractionTransitionDuration` accepts 0–0.5 seconds and defaults to 0.12.
`AnimateInteractionStates=False` or a per-call reduced-motion override applies
the same final cues immediately. Local text scale and high contrast still take
precedence: high contrast uses a white focus outline and a neutral selected
fill. Selected options tabs also prepend `> `, and selected/focused build
cards retain their text labels, so state does not depend on colour.

The shared themed button handles pointer hover and keyboard/controller focus
across settings actions and tabs, inventory browsing controls, and crafting or
build actions. Its short eased transition changes only the local Slate brush;
it stops safely when a widget is destroyed, refreshes from the current logical
state, and ends immediately when motion is disabled. Inventory detail cards,
support-effect slots, and build/craft cards use the same selection treatment
and short fill transition while keeping their existing navigation. High-contrast
cards preserve stronger outlines for unavailable states. No new pointer
selection, action, input binding, RPC, authority, gameplay, or save contract was
added. Active tabs use a `>` text marker, inventory keeps its `>` selection marker, the
support HUD keeps its geometric selection ring, and build/craft cards retain
`SELECTED`, `FOCUSED`, and `UNAVAILABLE` text cues.

Theme automation checks the normal/high-contrast focus outline, selected fill,
disabled outline, short transition scheduling, and immediate reduced-motion
path. Parent verification uses the full editor automation queue and rendered
host/client settings, inventory, and crafting checks at standard and
high-contrast/text-scale layouts. These checks do not replace physical input
hardware or packaged-build verification.

## Local interface scale and reduced motion — 2026-10-05

The Settings tab has a whole-interface scale separate from its 100/125/150%
text scale. It cycles through 80, 90, 100, 110, and 120%, defaults to 100%, and
persists in the existing local `GameUserSettings` configuration. At launch and
when changed, this value multiplies the project's configured
`UUserInterfaceSettings::ApplicationScale`; it does not save over the project
default. The game-layer DPI scale applies to HUD and menu layout and hit testing.
The Escape panel tracks the available local viewport with a 16-unit outer
margin, up to 1000×980 logical units, and reduces inner padding on smaller views.
The owner-only colour-independent feedback overlay follows the scaled viewport
and stays in the center-right area clear of the left HUD column.

Reduced motion defaults off and is stored alongside the scale. When on, the
options opening is immediate and themed button/card highlights and wheel
scrolling take their final state without a transition. If enabled while a
transition is running, it completes at its target. Focus outlines, active and
selected labels, contrast, state text, and actions remain static and immediate.
The theme's animation defaults continue to apply when the local preference is
off. Neither setting changes gameplay authority, RPCs, replicated state,
world/player saves, or a save schema.

`Kalmala.UI.Settings.LocalPresentation` checks scale bounds, project-scale
composition, local persistence, reduced-motion persistence, and its independence
from text scale. `Kalmala.UI.Theme.LocalPresentation` checks immediate motion-
free focus, selected fills, and opening behavior. The rendered peer verifier
checks standard and reduced-motion menu paths, focus, panel fit, HUD closure,
local restart persistence, and two viewport sizes. Physical input and packaged
build verification remain separate acceptance work.


## M11 support HUD caption polish candidate (blocked)

The four existing support glyph cards now use theme SlotPadding, ButtonNormal,
ButtonPressed and ButtonDisabled fills. Their MEND/WARD/VIGOR/CALL captions use
BodySize minus two with HeadingFace, shared font/outline handling and the local
100/125/150% text scale. Captions remain single-line; the existing full effect
names, learned/unavailable text, selection marker, remapped bindings, stamina
and server feedback remain in the HUD details.

Local high contrast paints glyphs/captions white and card fills black. Selection
retains its geometric ring, so it does not depend on colour. Accessibility
changes restyle cached glyph states immediately without a gameplay transition.
The HUD remains non-focusable and hit-test-invisible. No input, RPC, replication,
server timing or save contract changes.

Increment verification: compile the affected UI module in the disposable mirror,
run Theme.LocalPresentation and Inventory.PreparedFoodDetails, then run rendered
Verify-Inventory.ps1 at 1280x720/100% standard and 1024x768/150% high contrast.
Inspect both peers' glyph captions and retain representative captures. Use
Verify-SettingsAccessibility.ps1 for existing modal/input and local accessibility
regression, plus the ownership/documentation audits. This bounded caption/card
polish does not complete the wider UI-polish parent; dense HUD details and custom
font/theme geometry still require later acceptance.

The uncommitted inventory developer capture candidate waits three seconds per
state; the runner reads fresh logs after captures. This wait applies only to
explicit non-shipping capture mode, never gameplay. It did not reliably repair
missing glyph/text fragments at 1024x768/150% high contrast. Automated checks
passed but final visual acceptance failed on both peers. The HUD child remains
blocked; do not treat these candidate styling changes as verified or complete.

## Near-crosshair interaction prompt — 2026-10-07

The owner-local prompt sits just below screen centre and uses the shared panel,
font, text-scale, and high-contrast styles. It reads the single actor hit by the
owning pawn's current short visibility trace and presents its target and
supported action as readable text. The prompt contains no key/button names or
control legends; current bindings and remapping remain in Options > Controls.
No hit, unsupported actor, or modal input clears the prompt. A recognized
action that is locally unavailable remains named and is labelled Unavailable
with a reason where owner-visible state supports one. Earlier host/client
captures in `docs/interaction-prompts/` predate this text-only binding contract
and are retained as historical evidence.

The view trace is an advisory candidate only. The server keeps its own trace,
range checks, target selection, tool validation, seat/launch checks, and action
validation. The prompt never submits a target or queries other actors. Water
launch preview uses only the hit water sample; the server still checks session
capacity and hull placement. Skiff exit safety and network-delayed state may
also cause a server rejection after a visible prompt.

`Kalmala.UI.InteractionPrompt.Presentation` checks target/action readability,
absence of binding labels, unavailable, no-target, and modal text. It also
confirms that live remapped labels remain available from Options without leaking
into the prompt. The disposable render fixture supplies available/unavailable
actions and silent modal/no-target states on both local peers for standard and
high-contrast review. Rendered crafting acceptance also confirms the live
prompt hides while its modal is open. This editor evidence does not certify
physical keyboard/controller hardware or packaged builds.
