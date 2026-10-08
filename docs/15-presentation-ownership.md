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
| UI | `KalmalaUI` C++ widgets, local Slate vector glyphs, and disposable local raster textures | Local presentation reads visible/replicated state and never creates a gameplay source | The near-crosshair prompt names only the actor hit by the owning pawn's current short view trace and shows its supported action plus any owner-visible unavailable reason; it clears for no target or modal input and contains no key/button legend. The lower-left survival strip reads the owning pawn's replicated Wet/food entries, exposure, active support, and weather, with text-plus-shape categories, server-time/intensity context, source, and recovery guidance; the Inventory menu reads only that owner's pack/tools and the crafting panel reads its owner-only skill ledger; a separate top-centre support strip reflects the local character's selected effect and the owner's learned set; minimap/map remain local; the separate weather badge reads the replicated server tier and pairs circle/diamond/triangle markers with explicit text; settings Audio/Controls/Settings tabs expose local options, including independent text/interface scales and reduced motion in the existing local settings config |
| Feedback | Text and shape/icon treatments in the owner notification queue, crafting, discovery, and settings widgets | Readable without colour or audio; feedback reports accepted replicated results rather than client claims | Support glyphs reflect only the owner's learned/selected state and retain explicit text names/status; a bounded owner-only queue presents combat/support results and discovery acknowledgements; the optional Text + markers overlay remains for nearby hearth/construction context |

The ledger is an ownership and scope check, not a claim that the complete M5
art or audio pass has shipped. The player presentation passed the rendered
offscreen host/client controls fixture. The former panel glyphs have moved to a
separate owner-local strip; its learned-state, selection, scaling and rendered
host/client behavior remain for M12 final verification. The weather activity badge's tier
mapping and viewport slot passed focused UI automation; rendered viewport
readability remains unreviewed. The survival strip's data mapping and lower-left
viewport slot pass focused UI automation, but rendered host/client layout,
multi-row clipping, and scaled-font legibility remain unreviewed.

## Owner-local station context shell — 2026-10-08

The shared themed station shell opens from an owner-only event emitted after
the server accepts and revalidates a construction interaction. It binds the
current section to the exact replicated station actor and stable construction
ID. The Cooking Rack embeds its station-filtered Cook view, limited to cooked
boar/deer meat; the Cauldron uses the same owner-local shell for stew and soup,
with ingredient, quantity, and current hearth-heat feedback. Workbench opens a
Craft section scoped to its supported recipes, matching Tool Rack production,
and the owner-local Bronze Axe operation; its effective level and Tool Rack
state come from the accepted station and replicated placement presentation.
Forge exposes separate Craft, Upgrade, and Repair sections. Upgrade compares the
owner-only carried Bronze Axe against its authored Iron Axe target and reads
the owner's tool, material, and skill snapshots for prerequisites; the accepted
Forge's effective level and Anvil state stay tied to the exact context actor.
The existing progression RPC remains responsible for authoritative checks,
material exchange, and persistence.
The owner can switch to a separate Repair section without changing the recipe
selection or Forge Upgrade presentation. Repair rows read only the owning pawn's
owner-only carried-tool array, show its real level and condition, and submit
only the selected tool ID to the existing repair RPC. The response remains
owner-only. The legacy CraftMenu path remains available.
While open, the local subsystem asks the owning crafting component to verify
the same actor reference, stable ID, station kit, pawn world, and range. The
server validated sight during the original interaction. It closes on target
destruction, loss of range, or pawn replacement, then restores the prior
movement/look-ignore and cursor states.
The shell is presentation only: recipe and inventory changes remain on the
existing server paths, which revalidate their own station, heat, costs, and
owner inventory. No client target request, RPC, replication authority change,
or persistence field is added.

## Owner-local prepared-food inventory detail

When the owning player's private pack contains roasted field meat, Hearth
Broth, or smoked field meat, the on-demand Inventory menu explains that one
serving grants Steady Meal, reducing stamina cost by 10% for 120 seconds. If
the same pawn's existing replicated status contains an active meal, the menu
shows its remaining server-published time and the wait-for-expiry rule. The
Eat action sends only the selected supported food ID through the existing
server transaction; the server revalidates quantity and the active meal slot.
The menu adds no new RPC, replicated field, or save data. Shape markers and
explicit text remain visible with the owner's configured text scale and
contrast. `Kalmala.UI.Inventory.PreparedFoodDetails` checks the selected menu
detail and disabled action while owner data is unavailable. The host/client
inventory reconnect fixture continues to verify owner-only pack visibility;
rendered multi-row layout and scaled-font appearance remain open.

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

## Local HUD prompts and onboarding status

`UKalmalaAccessibilityFeedbackSubsystem` belongs to each `ULocalPlayer`. When
the local preference is **Text + markers**, it displays an owner-only text
overlay derived from that player's Wet, nearby hearth/construction, combat,
discovery, and support state. The bracketed markers remain meaningful without
colour, and the overlay follows the local high-contrast palette. It reads no
hidden actor or reward and sends no request.

`UKalmalaTutorialSubsystem` remains local-player state but is disabled at
runtime. Its retained design source uses action names without key/button
legends; no onboarding card mounts during fresh start, possession changes, or
reconnect.

The live near-crosshair prompt reads the actor hit by the owning pawn's current
short view trace. It presents the target, supported action, and any
owner-visible unavailable reason as text, then clears for no target or modal
input. It contains no binding labels; current keyboard/controller mappings
remain in Options > Controls. Its text remains available without relying on
colour or a control glyph. Earlier `Verify-PlayerControls.ps1 -Rendered`
captures predate this contract and retain historical key labels; they are not
current presentation acceptance evidence.

## Rendered survival status HUD layout

`UKalmalaSurvivalStatusWidget` remains a non-focusable local-player view of the
owning pawn's replicated Wet/food entries, exposure, active support, and server
weather. Its wrapped lower-left status column uses a 400-unit content width.
The centered arrival card described by the earlier M5 screenshot was removed
in M12; current gameplay has no bottom tutorial banner.
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
