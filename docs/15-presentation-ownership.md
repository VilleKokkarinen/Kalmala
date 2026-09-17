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
| Player | `UKalmalaPlayerModelComponent` procedural mesh and the generated bark/terrain/rock materials | Nine local, collision-free cosmetic parts; shape and pose never author gameplay | Faceted mantle/hood increment is pending Unreal verification |
| Wildlife | `AKalmalaWildlifeSpawn::BuildArchetypePresentation` procedural low-poly geometry and vertex colours | Server-owned replicated actor state; mesh is presentation only and has no collision | Existing Mireling, boar, and deer presentation remains in place |
| Environment | `AKalmalaGeneratedTerrainPatch`, campfire, and construction procedural meshes using generated materials | Terrain collision and shelter collision remain the gameplay authority; decorative meshes do not add routes or hidden content | Existing generated terrain, water, rock, tree, hearth, and kit sources are audited here |
| UI | `KalmalaUI` C++ widgets and disposable local raster textures | Local presentation reads visible/replicated state and never creates a gameplay source | Minimap/map and text HUD are project-owned; settings Audio/Controls/Settings tabs remain placeholders |
| Feedback | Text and shape/icon treatments in the inventory, crafting, combat, discovery, and settings widgets | Readable without colour or audio; feedback reports accepted replicated results rather than client claims | Existing Wet, hearth, construction, combat, and discovery text remains the baseline |

The ledger is an ownership and scope check, not a claim that the complete M5
art or audio pass has shipped. The player presentation change already in the
working tree still requires the focused Unreal verification before its backlog
item can be closed.

## Allowed and forbidden sources

Allowed visual sources are original project code, the committed Kalmala
materials under `Content/Kalmala/World/Materials`, project-owned map content,
and transient textures derived from local generated-world presentation. A
procedural mesh is acceptable when its geometry and colours are authored in
Kalmala code and it remains inside the documented authority boundary.

Do not introduce engine basic-shape meshes, Starter Content, Marketplace or
third-party assets, copied visual identity, or a new asset service. Developer
biome-debug materials may diagnose generation but must not become the normal
presentation path. Audio remains a separate M5 increment; until it exists,
every relevant state must retain readable non-audio feedback.

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
