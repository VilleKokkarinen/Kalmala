# M5 presentation ownership audit

This document records the remaining presentation seams and the source that is
allowed to supply them. It keeps the M5 visual/audio pass original and
project-owned without changing gameplay contracts, the generated-world
identity, or the multiplayer authority model.

Every audited seam is presentation-only: it may improve readability or
feedback, but it cannot become a gameplay source.

## Ownership ledger

| Presentation seam | Project-owned source | Current contract | Runtime status |
| --- | --- | --- | --- |
| Player | `UKalmalaPlayerModelComponent` procedural mesh and the generated bark/terrain/rock materials | Nine local, collision-free cosmetic parts; shape and pose never author gameplay | Faceted mantle/hood presentation verified in the rendered offscreen host/client controls fixture |
| Wildlife | `AKalmalaWildlifeSpawn::BuildArchetypePresentation` procedural low-poly geometry and vertex colours | Server-owned replicated actor state; mesh is presentation only and has no collision | Mireling's low forward hunch, reaching arms, and split crown read as a distinct close-view silhouette in the rendered host fixture; dark body planes merge somewhat. Boar has a low wedge-backed profile, broken bristle ridge, tapered muzzle, and paired tusks; deer has a lighter, long-legged alert profile with paired forked antlers |
| Environment | `AKalmalaGeneratedTerrainPatch`, campfire, and construction procedural meshes using generated materials | Terrain collision and shelter collision remain the gameplay authority; decorative meshes do not add routes or hidden content | Existing generated terrain, water, rock, tree, hearth, and kit sources are audited here |
| UI | `KalmalaUI` C++ widgets, local Slate vector glyphs, and disposable local raster textures | Local presentation reads visible/replicated state and never creates a gameplay source | The owner HUD adds original vector glyphs for the four support effects; minimap/map remain local; settings Audio/Controls/Settings tabs expose local options |
| Feedback | Text and shape/icon treatments in the inventory, crafting, combat, discovery, and settings widgets | Readable without colour or audio; feedback reports accepted replicated results rather than client claims | Support glyphs reflect only the owner's learned/selected state and retain explicit text names/status; the optional owner-only Text + markers overlay adds bracketed Wet, hearth, construction, combat, discovery, and support markers |

The ledger is an ownership and scope check, not a claim that the complete M5
art or audio pass has shipped. The player presentation passed the rendered
offscreen host/client controls fixture. The support glyphs passed rendered
offscreen host/client inspection in the unavailable state; learned-state and
live-cast transitions remain unverified.

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
