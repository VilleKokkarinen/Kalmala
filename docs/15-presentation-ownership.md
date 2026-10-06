# M5 presentation ownership audit

This document records the remaining presentation seams and the source that is
allowed to supply them. It keeps the M5 visual/audio pass original and
project-owned without changing gameplay contracts, the generated-world
identity, or the multiplayer authority model.

Every audited seam is presentation-only: it may improve readability or
feedback, but it cannot become a gameplay source.

M11 supersedes the historical status placement below: active status/weather
now appears only in the transparent owner-local top-right hotbar, with live
details in Escape > Status and weather details. The lower-left widget retains
only ocean travel, and pack preparation guidance retains no active timer.
Original catalogue line art is shared by inventory/tool rows and the build
selector. See `36-status-icons.md` for identities and verification boundaries.

## Ownership ledger

| Presentation seam | Project-owned source | Current contract | Runtime status |
| --- | --- | --- | --- |
| Player | `UKalmalaPlayerModelComponent` procedural mesh and the generated bark/terrain/rock materials | Nine local, collision-free cosmetic parts; shape and pose never author gameplay | Faceted mantle/hood presentation verified in the rendered offscreen host/client controls fixture |
| Wildlife | `AKalmalaWildlifeSpawn::BuildArchetypePresentation` procedural low-poly geometry and vertex colours | Server-owned replicated actor state; mesh is presentation only and has no collision | Mireling's low forward hunch, reaching arms, and split crown read as a distinct close-view silhouette in the rendered host fixture; dark body planes merge somewhat. Boar has a low wedge-backed profile, broken bristle ridge, tapered muzzle, and paired tusks; deer has a lighter, long-legged alert profile with paired forked antlers |
| Environment | `AKalmalaGeneratedTerrainPatch`, campfire, and construction procedural meshes using generated materials | Terrain collision and shelter collision remain the gameplay authority; decorative meshes do not add routes or hidden content | Existing generated terrain, water, rock, tree, hearth, and kit sources are audited here |
| UI | `KalmalaUI` C++ widgets, local Slate vector glyphs, and disposable local raster textures | Local presentation reads visible/replicated state and never creates a gameplay source | The near-crosshair prompt names only the actor hit by the owning pawn's current short view trace, shows its action and current keyboard/controller mappings, and clears for no target or modal input; it does not enumerate targets or authorize actions. The lower-left survival strip reads the owning pawn's replicated Wet/food entries, exposure, active support, and weather, with text-plus-shape categories, server-time/intensity context, source, and recovery guidance; the inventory HUD explains prepared-food effects and the active meal timer from that same owner's pack/status; the crafting panel reads the owning pawn's owner-only skill ledger for six skill bars and the nearest recipe unlock; the owner HUD retains original support glyphs; minimap/map remain local; the separate weather badge reads the replicated server tier and pairs circle/diamond/triangle markers with explicit text; settings Audio/Controls/Settings tabs expose local options, including independent text/interface scales and reduced motion in the existing local settings config |
| Feedback | Text and shape/icon treatments in the inventory, crafting, combat, discovery, and settings widgets | Readable without colour or audio; feedback reports accepted replicated results rather than client claims | Support glyphs reflect only the owner's learned/selected state and retain explicit text names/status; the optional owner-only Text + markers overlay adds bracketed Wet, hearth, construction, combat, discovery, and support markers |

The ledger is an ownership and scope check, not a claim that the complete M5
art or audio pass has shipped. The player presentation passed the rendered
offscreen host/client controls fixture. The support glyphs passed rendered
offscreen host/client inspection in the unavailable state; learned-state and
live-cast transitions remain unverified. The weather activity badge's tier
mapping and viewport slot passed focused UI automation; rendered viewport
readability remains unreviewed. The survival strip's data mapping and lower-left
viewport slot pass focused UI automation, but rendered host/client layout,
multi-row clipping, and scaled-font legibility remain unreviewed.

## Owner-local prepared-food inventory detail

When the owning player's private pack contains roasted field meat, Hearth
Broth, or smoked field meat, the read-only inventory HUD explains that one
serving grants Steady Meal, reducing stamina cost by 10% for 120 seconds. If
the same pawn's existing replicated status contains an active meal, the panel
shows its remaining server-published time and the wait-for-expiry rule. The
display adds no input, RPC, gameplay mutation, or persistence; shape markers
and explicit text remain visible with the owner's configured text scale and
contrast. `Kalmala.UI.Inventory.PreparedFoodDetails` checks the bounded benefit,
active timer formatting, and fail-closed invalid timer behavior. The
host/client inventory reconnect fixture continues to verify owner-only pack
visibility; rendered multi-row layout and scaled-font appearance remain open.

## Owner-local crafting skill and unlock detail

The Camp crafting panel reads detailed progression only from the local owning
pawn's `UKalmalaSkillProgressionComponent`. It lists the six allowlisted skills
with the current level and experience toward the next level, then names the
nearest skill-gated recipe unlock and the XP progress required. When an
accepted recipe can advance that skill, the panel names its per-request XP
award. Missing or incomplete owner replication is described as unavailable;
no aggregate peer badge is used as a substitute for private skill detail.
The selected-recipe details and skill block follow the local text-scale and
high-contrast settings inside the existing scroll view. This is read-only
presentation: it adds no RPC, focus target, progression mutation, or save
field. `Scripts/Verify-Crafting.ps1` checks the fresh-owner values and
host/client presentation; `-Rendered` retains both 1280x720 captures.

The selected-result preview reads only the existing selected recipe output,
description, requirements, and availability already shown by the local
crafting panel. It resolves a canonical icon through the shared catalogue
mapping and keeps the availability text visible when a recipe cannot currently
be crafted. Keyboard/controller selection updates this read-only panel; it
sends no request and creates no actor. No inventory, recipe, item, replicated,
or save property is added.

The inline Iron Axe upgrade comparison reads the owning player's carried
Bronze Axe level and condition plus the existing progression/lifecycle
definitions for the selected Iron Axe. It refreshes from local owner state and
reports missing or incompatible comparisons as unavailable. It adds no
request, equipment state, stat authority, replication field, or save data.

## Allowed and forbidden sources

Allowed visual sources are original project code, the committed Kalmala
materials under `Content/Kalmala/World/Materials`, project-owned map content,
and transient textures derived from local generated-world presentation. A
procedural mesh is acceptable when its geometry and colours are authored in
Kalmala code and it remains inside the documented authority boundary.

Do not introduce engine basic-shape meshes, Starter Content, Marketplace or
third-party assets, copied visual identity, or a new asset service. Developer
biome-debug materials may diagnose generation but must not become the normal
presentation path. Audio delivery includes local project-owned wind,
nearby-visible-water, nearby-visible-lit-hearth, sampled-biome, rain, and Wet
status cues, plus generated-ocean entry/exit cues from the owning pawn's local
movement-mode transitions. These traversal cues do not set or replicate swim
state. A short support-acceptance cue reads only the owning pawn's
existing owner-only accepted feedback serial. The biome layer reads only the
owning pawn's current generated-world location and does not scan or suggest a
route. All gameplay state must retain readable non-audio feedback; local
interaction/gathering acceptance and rejection cues now read owner-only crafting
results and accepted inventory increases. Discovery, movement, combat, and
support cues read their existing owner-local or replicated presentation state;
the remaining visual and accessibility work stays in M5.

## Static audit and runtime limits

`Scripts/Verify-PresentationOwnership.ps1` checks the committed project-owned
material files, the source anchors for player, wildlife, environment, UI, and
feedback presentation, and the absence of known external/prototype asset
paths. It also checks this ledger's scope and authority statements without
launching Unreal.

The audit cannot prove material loading, triangle winding, visual readability,
animation, audio mixing, packaged startup, or host/client screenshots. Those
remain runtime verification work after Unreal build access is restored.

## Multiplayer and persistence boundary

Presentation has no authority to select a world, spawn content, choose a
target, apply damage, grant a reward, change weather/exposure, mutate an
inventory or construction record, learn an effect, or write a gameplay save.
Server-owned replicated state remains the only gameplay result rendered to a
peer. Local cosmetic geometry, UI preferences, and transient raster textures
are not replicated and do not change saved-data schemas.

The expanded map's `WorldMapPanelImage` is static project artwork painted
behind its existing locally generated terrain and fog. Its owner-local legend
and personal-pin/co-op-player/temporary-ping filters read only pins already
loaded for that local player and co-op markers already returned through the
owner's visibility-gated map-awareness component. A separate local fog check
still gates every co-op marker. Legend counts describe eligible markers in the
current map view. Filters suppress painting only; they do not request hidden
content, change exploration, alter co-op consent, or mutate/remove pin data.
Their transient widget state has no authority, RPC, replication, or persistence
path.

## Rendered deer silhouette review

`Scripts/Verify-DeerPeer.ps1 -Rendered` captures the existing target after the
bounded development fixture positions the deer and its herd mate. The
2026-09-21 host capture at 1280×720 shows the refined long-legged profile and
antlers clearly enough to distinguish its silhouette. The dark vertex colour
has low contrast against this scene; other lighting and viewing distances
still need review. The fixture camera is development-only, and its host/client
combat, reward privacy, and defeat-persistence checks use the existing
server-authoritative actor state.

## Rendered Mireling silhouette review

`Scripts/Verify-MirelingPeer.ps1 -Rendered` captures the normal generated
Mireling after the bounded listen-server fixture positions the existing target.
The 2026-09-21 host capture at 1280×720 makes its low hunch, reaching arms, and
split crown distinguishable in close view. Dark body planes merge against the
scene, and readability at other distances or lighting remains unreviewed. The
transient host camera does not alter the replicated actor; the same run checks
server combat, client rejection, owner-only rewards, and same-world defeat
persistence.

## Local tutorial prompt presentation

`UKalmalaAccessibilityFeedbackSubsystem` belongs to each `ULocalPlayer`. When
the local preference is **Text + markers**, it displays an owner-only text
overlay derived from that player's Wet, nearby hearth/construction, combat,
discovery, and support state. The bracketed markers remain meaningful without
colour, and the overlay follows the local high-contrast palette. It reads no
hidden actor or reward and sends no request.

`UKalmalaTutorialSubsystem` belongs to each `ULocalPlayer`. It displays an
optional text card and a high-contrast compass mark from normal possession,
visible local view focus, the local movement offset, the existing crafting
shell, or that owner's replicated Wet/learned-effect state. It samples no
hidden population or discovery descriptors, and it sends no gameplay request.
Dismissal and revisit input are non-consuming local bindings; prompt history
ends with the local-player session.

`Scripts/Verify-PlayerControls.ps1 -Rendered` captured the arrival prompt for
both host and client at 1280×720 while retaining the existing movement checks.
The lower-centre card exposes current W/A/S/D, left-stick, mouse/right-stick,
jump, sprint, dismiss, and revisit labels. Other prompt contexts, viewport
scales, and keyboard/controller prompt-button presses remain for the follow-up
onboarding acceptance check.

## Rendered survival status HUD layout

`UKalmalaSurvivalStatusWidget` remains a non-focusable local-player view of the
owning pawn's replicated Wet/food entries, exposure, active support, and server
weather. Its wrapped lower-left status column uses a 400-unit content width and
stays clear of the centered arrival card at the documented 1280×720 viewport.
The rendered host/client capture shows the weather row and recovery guidance
without overlap; `Kalmala.UI.SurvivalStatus.LocalPresentation` checks the full
Wet, food, weather, temperature, support, and empty-state text cases. This
layout adjustment changes no gameplay, replication, or persistence contract.

## Skill-level notification boundary

`UKalmalaNotificationSubsystem` reads only the local controller's owning pawn
skill component and its owner-only detailed progression. Initial complete
snapshots and replacement-pawn reconnects are silent; later accepted server
level changes feed a bounded local queue. The widget uses original shared
line-art icons and themed text. It adds no RPC, gameplay mutation, peer query,
external asset or persistence field. Queue/widget behavior is focused-test
covered; rendered multiplayer placement acceptance remains pending. See
`docs/40-notifications.md` for lifecycle and limits.

Accepted item-gain feedback uses reliable server-to-owner receipts only after
validated inventory publications. The local 32-receipt buffer is transient;
initial attachment is silent and the shared three-row widget does not inspect
peer inventory. No server mutation RPC or persistence field is added. See
40-notifications.md for net-gain and dropped-notice limits.

## Owner-local recipe Favorites

`UKalmalaCraftingSubsystem` keeps canonical recipe/build bookmark IDs for its
own `ULocalPlayer` session. The crafting menu reads the set only to filter the
existing owner-visible catalogue and show Favorite text. A selected-entry
button changes this transient local set; it sends no request, changes no
availability, and cannot place or craft anything. The bounded set is cleared
with the local-player subsystem and is not written to gameplay saves, user
settings, replicated state, or peer-visible data. Usage counts, ranks, and
Recent IDs are local and derive only from unique server-accepted action
receipts, under the contract in `docs/41-recipe-activity.md`.

## Owner-local recipe activity

The crafting component publishes a bounded owner-only receipt after a server
craft or construction placement succeeds. Each receipt identifies the accepted
recipe/menu entry and action kind; it cannot authorize a request. The local
crafting subsystem consumes unique sequences, counts only IDs in the active
catalogue, and derives usage ranks and one Recent entry per action kind. Its
history remains local across menu and pawn replacement and clears with the local
player subsystem. Failed requests, cooking counted as other crafting, peer
state, inventory guesses, and reconnect snapshots do not create local history.
See `docs/41-recipe-activity.md` for the bounded receipt and deterministic rank
contract. Each recipe/build card reserves separate right-aligned rows for
Rank and Recent, with Favorite at the lower right, so their labels can coexist
without covering the canonical name, availability, or each other. Recent-only
entries appear in the Favorites filter without becoming bookmarked; all marker
changes remain static under reduced motion.
