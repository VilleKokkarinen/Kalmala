# Technical architecture

## Basic Torch equipment and flame — 2026-10-21

`BasicTorch` is a registered Utility equipment profile with no attack list,
upgrade cost or repair path. Its Inventory Craft recipe outputs a carried-tool
record rather than an inventory stack: the server consumes 2 Wood and 1 Amber,
checks shared-grid capacity, assigns an instance GUID and persists the new
300-second fuel state before accepting the transaction. Failed persistence
restores the prior carried tools and materials.

Only the exact BasicTorch GUID selected in Main Hand burns. Unequipped and
stored torches retain their fuel. The server exposure step decrements fuel,
extinguishes it at zero or when precipitation is at least 0.05 and the player
has no roof, and keeps a rain-extinguished torch out until it is unequipped and
re-equipped in dry conditions. Fuel is checkpointed to the existing player
slot every ten held seconds and on extinguish, unequip, logout, craft and
refuel. Refuelling is a GUID-addressed server transaction that consumes one
Wood and restores 300 seconds; it rejects full, malformed or unowned torches.

Player-discovery save schema 3 adds bounded remaining burn seconds to each
carried-tool record. Basic Torch accepts finite fuel in [0,300]; every other
tool requires zero. The former schema 2 is incompatible and remains untouched
when rejected; no development migration is performed. `bHeldTorchLit` is the
only new replicated torch-state field. It drives a local 600 cm point light;
the existing public held-tool visual enum also gains a Torch silhouette. A
server flame-eligibility query is ready for A8a mosquito integration, whose
targeting and repulsion behavior remains a later task. Fuel, instance GUIDs
and other inventory data remain owner-only. Torch attacks, warmth, wetness
removal and world ignition are unsupported.

## Hammer/Bow input and authoritative arrows — 2026-10-18

Character routes Attack pressed/released by authoritative inventory selection:
hammer performs no combat request (the existing crafting subsystem binds left
click to selected-piece placement and right click to its build menu); Bow uses
StartBowDraw/ReleaseBowDraw; other held weapons retain melee. Right-click during
pending placement now cancels that preview and opens the menu in the same press.

Bow and LightBow profiles require finite `DrawDuration` in [0.05,10] seconds;
their current durations are 1.5 and 2.0 respectively. Non-bows normalize
omitted values to zero before strict JSON conversion and must retain zero. Item
inspection displays the duration. Draw progress is elapsed /
duration, clamped to [0,1]. A bounded [1,4] speed multiplier parameter is a future
server Archery integration hook; runtime currently uses 1 and adds no skill.

Reliable owning-pawn component begin/release RPCs carry monotonically increasing
sequence and cancellation only. Server checks authority, fresh sequence, living
owner, exact drawn bow GUID, level, condition, ammo and idle/cooldown for either
bow profile. It owns
start time, charge, camera aim, random spread, damage and projectile selection.
Drawing flag/start time replicate for presentation; local prediction starts the
circle immediately, while server charge alone determines the shot. Menu-open,
weapon swap, missing ammo and invalid condition cancel; cancellation spends
nothing. New draws cannot bypass the existing 0.36-second recovery. The model
holds a draw pose and does not play a melee chop when the shot serial arrives.

PreferredArrowStack is an owner-only transient stack-row GUID. Inventory's new
action submits a selected stack/unit GUID; server accepts only an owned nonempty
WoodenArrow or authored arrow stack, resolving unit to row, or clears preference
with an empty GUID. Missing/exhausted preference falls back to WoodenArrow,
then StoneArrow. Release removes one physical unit from that exact stack, spends
one bow condition,
persists the combined player snapshot, and only then finishes spawning the
server arrow. Failed save restores stacks/condition and destroys the deferred
actor. Preference is session-only; the inventory and condition use existing saves.

Original procedural ArrowShaft and ProjectileMovement supply replicated flight,
gravity and facing. Only the server simulates flight. After movement, the actor
sweeps a radius-2 sphere from previous to current position using ECC_Visibility,
which is the existing wildlife/terrain hit contract; its own movement ignores
object-channel collision to avoid missing wildlife or stopping behind a creature.
Owner is ignored. Impact destroys the arrow and may invoke a private, friend-only
wildlife ranged-damage helper, retaining resistances/defeat/attacker rewards while
bypassing only the melee 220 cm range cap. Existing melee checks are unchanged.
No client sets projectile transform, damage, charge or hit actor. No PvP or tree
harvesting from arrows. Lifetime is five seconds. WoodenArrow charge maps 10..40
Pierce; StoneArrow deals 25 Pierce at any charge; FangArrows deals its authored
50 Pierce at any charge. An accepted FangArrows hit also applies weak poison to
living wildlife for five seconds at one Poison damage per second. Rehits refresh
the remaining duration up to five seconds without stacking or resetting the
shared poison tick clock. Poison damage uses the existing resistance resolution
with no new attacker, preserving the last accepted arrow hit's reward credit.
Only the server owns the transient poison timer and its ticks; it does not add
replicated status or save data. All three ammunition types retain 1000..4500
cm/s velocity and 8..0.25 degree spread. Authored arrow attack lists are keyed
by ordinary item ID in `attacks/bows.json`. No projectile recovery.

A local hit-test-invisible full-viewport crosshair widget draws a 50..4 radius
circle during drawing and a small idle dot, hiding during modal input. It is
released on controller change. LightBow is a separate carried equipment profile
with catalogue recipe/upgrade data and an original imported inventory icon; it
uses the existing bow attack list, authority checks, projectile and save fields.
No new RPC, save field or Archery skill data is introduced.

## Automatic and manual loose-item pickup — 2026-10-17

The local settings widget stores `LocalAutomaticPickup` in GameUserSettings
under its existing settings section, default true. Its local-player subsystem
sends a reliable owning-pawn boolean preference on possession/controller changes
and option changes. A new pawn defaults false until that preference arrives;
the flag is transient and carries no save schema or public replication field.
Clients supply no radius, target or reward. The server scans materialized harvest
nodes every 0.25 seconds, stopping after four successful collections. The active
streamed actor set is scanned; this does not generate unloaded drops.

`GetPickupInfo` admits only uncollected source-less material nodes or tree Drop
stage, exposing canonical item identity and deterministic server-world quantity
for the existing crosshair UI. `TryAutomaticPickupFromServer` revalidates server
roles, same world, living/enabled interactor, 150 cm physical radius to the bundle
anchor, visibility occlusion and complete inventory capacity before invoking the
existing interaction transaction. Full-pack attempts remain quiet and leave drops
intact. Standing/Falling/Trunk and tool-gated harvesting are excluded. Existing
wood-bundle persistence-before-depletion and rollback remain authoritative.

Manual Action/Interact still uses the server-retraced crosshair target and its
existing reach/capacity checks regardless of preference. Pickup prompt text is
one line, `Pickup [quantity only when >1] [catalogue display name]`; all other
interaction prompt formats and modal hiding remain unchanged. Source item name
lookup now resolves the authoritative selection rather than the legacy random
Wood/Stone label. The 150 cm radius is fixed tuning, not an options slider.

## Held-tool cosmetic replication — 2026-10-17

`UKalmalaPlayerModelComponent` replaces the unconditional construction hammer
with three reusable, non-colliding procedural parts attached to PlayerRightArm.
The nine body parts plus this fixed pool total twelve components, regardless of
selection or visibility. Only transforms/visibility change on selection; parts
are never accumulated across swaps. Unknown or empty selections hide the pool.

Silhouette centres and part rotations are composed through a grip transform
before being applied to the pool. The authored shaft grip `(8,0,-50)` maps to
arm-local palm `(0,0,-52)` with pitch -90 degrees, turning +Z shafts into +X
forward shafts and the axe edge down. Bow instead keeps its upright rotation
and maps its central limb join `(18,0,-44)` to the same palm. The bow string
joins the limb tips at authored X=6, behind the grip. Rotation is about the
shared grip, so multipart heads stay assembled and the palm remains on the
handle throughout parent-arm animation. No extra component or replicated
transform is introduced.

The authority derives `EKalmalaHeldToolVisual` from the validated inventory's
active item and its known tool definition, then replicates only that family enum.
No client setter/RPC, inventory ID, instance GUID, level or condition is added.
Repnotify reconstructs the same primitive silhouette independently of remote
inventory; BeginPlay also applies the current enum in case replication preceded
mesh creation. Dedicated servers keep the lightweight selection tick but create
no body/tool geometry. Inventory's detailed selection and carried state remain
owner-only. Tier-specific models/materials and two-handed bow poses remain out
of scope; existing attack and sharpening arm motion carries the attached parts.

## Server-approved free melee swings — 2026-10-17

The combat start gate accepts a living character with empty hands or any known,
usable melee profile (valid authored level and condition 1..maximum), regardless
of whether a nearby target exists. Hammer and Bow are excluded from this path. Authority, fresh
nonzero sequence and idle-action gates remain required. Unknown/invalid/depleted
held states fail closed. The action-start helper no longer supplies synthetic
target facts to the strict combat mutation contract: actual damage remains
separately guarded by the server target path.

Pending targets remain server-selected and are rechecked at execution. The held
profile, condition and living owner are revalidated after windup, including swaps
or breakage. Valid weapons retain authored Basic damage; profiles with no Basic
attack use the existing 25 Crush fallback. Bow firing is a separate draw/release
transaction described above, and hammer is construction-only. Misses enter
recovery, publish owner feedback None, spend no condition and grant nothing.
Tree material/axe gates and persistence rejection remain in the existing tree
transaction, so universal swings do not enable non-axe tree damage.

The procedural player model observes the existing replicated combat action serial
and phase. A new active action starts a bounded local right-arm raise/strike/return
pose; an idle historical serial does not replay. No tool identity, target,
condition or new animation RPC is replicated. Remote playback begins when its
action state arrives; network delay and phase coalescing remain untested. Existing
sharpening presentation takes precedence while active.

## Wood Shield station — 2026-10-15

The sole `WoodShieldRecipe` lives in `recipes/stations/workbench.json` and
requires Workbench. Its current costs are 10 Wood, 3 Boar Hide and 3 Deer Hide.
The matching level-one upgrade-cost record also requires Workbench; existing
costs remain unchanged. This supersedes earlier Forge station/cost descriptions
for Wood Shield. Wearable upgrade execution remains pending.

## Two-handed equipment — 2026-10-15

Equipment profiles may set the JSON boolean `twoHanded` to true; omitting it
means false. The loader maps the authored field to its internal boolean and
rejects the retired `bTwoHanded` JSON key. `Bow` is a crafted two-handed
profile, with 100 base condition and +50 maximum durability per level; it is
not granted in the starting kit. Its acquisition recipe is on the Workbench.
Attack damage remains zero while ranged shooting is deferred.
The melee request/execution path rejects a Bow, including a switch during
windup, so it cannot fall through to unarmed damage. Shooting is deferred by
explicit user scope.

`SetMainHandFromServer` builds candidate stacks/gear/layout before publishing.
For a two-handed selection it returns the Shield through the existing validated
GUID transaction, counting the incoming weapon outside the pack and the old
weapon inside it. A full pack plus both occupied hands rejects without changing
either item; exactly one free cell permits the swap. With only a shield held,
the incoming weapon's freed cell is sufficient. Shield equip similarly validates
the complete candidate grid with the two-handed weapon returned before clearing
active state. Other wearable slots and same-pawn shield wear are preserved.
No new RPC, replication field, save field, version or migration is introduced.

`Sinew` and bow-only `BowString` material records live in
`items/materials/crafting-ingredients.json`; `WoodenArrow`, `StoneArrow`, and
`BronzeArrow` ammunition items live in sibling `items/arrows.json`, rather than
under materials. The Stone and Bronze Arrow Workbench recipes are in
`recipes/stations/workbench.json`; their fixed Pierce damage records are in
`attacks/bows.json`. The server resolves an owner-selected stack GUID first;
when that stack is no longer usable, it scans the owner's replicated grid in
row-major order and selects the first supported arrow. A shot commits one arrow
and one bow-condition point only after projectile validation and player-state
save succeed. Wooden Arrow damage follows draw progress; Stone and Bronze
Arrow damage use their fixed 25- and 50-Pierce attack records. The owner-local
crosshair presents the selected type and current damage from inventory and
catalogue data; it does not choose or authorize the server shot. Selection
remains session-only and owner-only replicated; no save field is added.
Credited deer defeats grant 1 Sinew on a server-owned 50% roll. At the Workbench,
`recipes/stations/workbench.json` converts 4 Sinew into 1 Bow String; Bow
acquisition and upgrades use 5 Wood and 1 Bow String each. The same file keeps
the ordinary arrow recipe (5 Wood for 20 arrows). Bow, Bow String, and Sinew originals and
imported textures are documented in `icon-provenance.md#archery-icons` (new material icons use
batch K).
The owner Inventory shows both-hand occupancy and the reverse shield swap. Full
gameplay compilation/runtime verification is deferred.

## Bone fragment material — 2026-10-15

`BoneFragment` (Bone fragment) is a stackable crafting ingredient in
`items/materials/crafting-ingredients.json`, with stack limit 40 and weight
0.1 kg. Each server-confirmed Boar or Deer defeat grants one alongside the
existing meat/hide rewards; a credited A8a Badger defeat queues one for its
authenticated attacker. Badger's stable reward claim is saved before defeat
replication and retries as an all-or-none inventory transaction until the
owner's pack can accept it, including after reconnect or restart. Uncredited
defeats grant nothing; the one-shot defeat reward gate prevents duplicate
payment, and server inventory validation enforces authority and capacity. Wood
Shield crafting requires 3 Wood, 2 Bronze,
1 Boar Hide and 1 Bone fragment at Forge. Its
level-one upgrade cost requires 10 Wood, 5 Boar Hide and 1 Bone fragment at
Forge. Boar Hide Cape crafting and its level-one upgrade cost each require one
Bone fragment at Workbench in addition to their hide costs. No new RPC,
replicated field, or save schema is needed.
The shared icon widget resolves `BoneFragment` to its original imported
`/Game/Kalmala/UI/Icons/Items/BoneFragment.BoneFragment` texture; batch H
retains the transparent original and prepared 64x64 PNG. See `icon-provenance.md#bone-icon`.

Use the baseline sections below for module/network boundaries. The [consolidated contracts](#consolidated-system-contracts) preserve current system details from former docs 08–47:

- [World generation](#world-generation), [harvest sources](#harvest-source-catalogue), and [ocean travel](#ocean-travel).
- [Tools/progression](#tools-and-station-progression), [camp services](#camp-crafting-and-station-services), and [combat/support](#combat-and-support-magic).
- [Discoveries](#optional-land-discoveries), [save formats](#save-identity-and-format-policy), and [shared inventory](#shared-inventory-and-hotbar).
- [Theme](#shared-ui-theme), [icons](#status-and-catalogue-icons), [details](#item-and-recipe-details), and [browsing](#menu-browsing).
- [Notifications](#owner-notifications), [recipe activity](#recipe-and-build-activity), and [onboarding reference](#onboarding-reference).
- [Settings/accessibility](#settings-and-accessibility), [presentation ownership](#presentation-ownership), and [audio](#audio-cues).

## UE5 baseline

- Create a C++ UE5 project named `Kalmala`, using Enhanced Input, Common UI, Gameplay Ability System (GAS), and Online Subsystem interfaces.
- Use source control and Unreal-friendly ignore rules before creating content.
- Target a current supported UE 5.x version chosen during project bootstrap; record the exact version in the decision log.
- Prefer Data Assets/Data Tables for tunable game content; avoid gameplay constants scattered through Blueprints.

## Multiplayer contract

The server owns player state, inventories, construction, damage, AI decisions, simulation state, loot, and progression. Clients send intent through validated server RPCs and receive replicated state. Clients may predict responsiveness only where reconciliation is supported.

| System | Authority | Replication approach |
| --- | --- | --- |
| Movement | UE Character Movement server authority | built-in movement replication |
| Interaction | server reruns a short trace from the owning pawn | client sends intent only; no target actor is trusted |
| Abilities | server validates activation and outcomes | GAS replication/prediction where suitable |
| Inventory/crafting | server | replicated components / owner-only detail |
| Building | server | replicated building actors; save stable IDs |
| AI | server | replicate actor state, not decision logic |
| Element grid | server | replicate sparse changes near relevant players |
| World identity | server | replicate immutable `WorldSeed` through `GameState` |
| Terrain surface | shared seed function | convert Elevation to continuous height and normal for terrain, collision, and server-selected spawns |
| Terrain activation | server | initialize an invisible 3x3 neighborhood of continuous terrain patches around the generated start, placing every replicated patch actor at its deterministic patch centre; then refresh the bounded union of server-observed player neighborhoods at a one-second interval up to 25 patches, retiring patches outside every player neighborhood so open-ocean travel can continue without increasing streaming density; activation cells never define biome or gameplay boundaries |
| Terrain rendering | client cosmetic | derive one continuous local mesh from the replicated identity and patch descriptor; mesh geometry is never replicated |
| Biome debug material | local developer cosmetic | with `-KalmalaBiomeDebug`, replace the generated terrain material with classifier-driven vertex colours from the replicated identity and patch descriptor; it changes no terrain, collision, or gameplay state |
| Terrain collision | server | create collision from the server's same continuous terrain mesh; clients derive matching local collision only for prediction, never as authority |
| Surface water | client cosmetic | clip sea-level water against the same linear terrain triangles from the replicated identity and patch descriptor, retaining partially submerged areas |
| Shimmering Lakes treatment | client cosmetic | the compiled default-off basin policy produces no inland lake-water or shore mesh; when analytic basins are explicitly re-enabled in code, derive collision-free geometry from basin-supported regional samples; no lake geometry or physics state is replicated |
| Generated player start | server | resolve a Meadow-preferred seed-specific transform above the sampled terrain height, then place each pawn with its actual collision half-height plus server clearance after the terrain patch collision exists |
| Meadow rocks | client cosmetic | derive non-interactable low-poly procedural rocks from the replicated identity, terrain sample, and biome classification |
| Meadow trees | client cosmetic | derive non-interactable low-poly procedural trunks and canopies from the replicated identity, terrain sample, and biome classification |
| Gameplay population layout | server | activate a bounded set of invisible spatial keys around pawns, then spawn replicated server-owned harvest nodes, minimal wildlife spawns, and minimal hazard spawns from deterministic per-kind, field-informed descriptors; each carries a stable spatial ID for sparse server persistence; clients never select gameplay placements or defeat outcomes |
| Environmental exposure | server | sample ambient temperature, precipitation, wind exposure, and shelter for each pawn; update clamped wetness and warmth at a fixed server interval; replicate the resulting state for display only |
| Biome expansion | server | `FKalmalaBiomeExpansionContract` supplies deterministic per-biome terrain-feature intent, bounded population multipliers, normalized exposure modifiers, and stable discovery candidates. The Shimmering Lakes biome label and its field-based profile remain, but the default-off analytic-basin policy suppresses lake-water-adjacent discovery candidates and basin-derived shoreline wetness. The Elderwood slice applies its profile only in Elderwood-classified keys and pawn samples, materializes at most one gently sloped lower-flora clearing discovery per active key, and locally derives dense field-driven canopy and roots with natural clearings. The Mossy Mire slice applies its profile only in Mire-classified keys and pawn samples, applies bounded server-owned footing drag through the existing replicated exposure state, and materializes at most one gently sloped relatively dry hummock harvest discovery per active key. The Freezing Tundra slice applies its sparse-cover, stronger-wind profile only in Tundra-classified keys and pawn samples, and materializes at most one exposed, gently rolling high-ground harvest discovery per active key. The Thunder Mountains slice applies its high-wind profile only in Mountain-classified keys and pawn samples, and materializes at most one steep-but-traversable storm-carved overlook discovery per active key. All active discoveries use the normal validated harvest path and persist only sparse depletion deltas. Clients receive no candidate list or discovery location. |
| Companion minimap | client UI | `UKalmalaMinimapViewModel` derives a local terrain/water sample grid from the replicated world identity and owning pawn transform; `UKalmalaMinimapSubsystem` creates a local top-right circular presentation that draws only in-circle terrain/water samples and a centred facing marker. Local mouse-wheel input adjusts a session-only view radius between tunable 2,500–10,000 cm bounds only while CommonUI allows normal game input; a modal UI retains wheel ownership. The UI never reveals hidden server-owned content. |
| Settings menu | client UI | `UKalmalaSettingsSubsystem` binds local Escape input and opens/closes a local `UKalmalaSettingsWidget`. Options presents selected Gameplay, Keyboard & Mouse, Controller, Graphics, Audio, and Accessibility groups; Gameplay stays hidden while it has no selected option. Existing bindings split by device; resolution, V-Sync, window mode, and a 64–576 m render-distance cap (32 m increments) remain in Graphics. The cap applies to registered local-world primitive components while preserving shorter authored cull distances. Unreal view-distance quality is kept at its full scale so the selected meter cap maps directly to the component draw-distance limit. Audio retains local volume controls and uses the shared stone checkbox for mute/restore. Text scale, whole-interface scale, contrast, feedback, reduced motion, and render distance remain in local configuration. These choices affect only presentation, are never replicated, and cannot mutate gameplay authority or save schemas. |
| Inventory menu | client UI with validated server slot intent | UKalmalaInventoryMenuSubsystem owns the Tab/I panel and an upper-left gameplay hotbar for each local player. One 10×4 runtime grid contains item stacks and tools; its numbered top row is the hotbar. Grid positions and active identity replicate owner-only. Drag or keyboard/controller moves send source/target indices and expected identities for server validation; quantities, grants, repairs and food outcomes remain server-owned. The popup has a separate Worn Gear section for Main Hand, Head, Cape, Chest, Shield, Legs, and Boots. The six wearable slots display owner-only equipped item names and send server-validated equip, replacement, and return requests; Main Hand references the owner-private equipped tool GUID within its unchanged grid cell. The menu retains selected-item details and existing food/repair actions. The gameplay bar displays only occupied cells in 1–9, 0 order and hides during modals. Tool metadata and save schemas retain their established contracts; arrangement is session state. See `02-technical-architecture.md`. |
| Expanded world map | client UI plus bounded temporary-ping relay | The local M-toggle overlay uses asynchronous terrain/water tiles derived from the immutable world identity. The view remains centred on the owning local pawn while it is open; mouse-wheel and Page Up/Page Down zoom around that centre, with no pan or recenter action. A bounded 64-entry least-recently-used cache keys terrain and fog rasters by world-tile size and coordinate, reusing entries across zoom; identity changes clear the cache and invalidate older workers. At most two terrain jobs run concurrently, and only matching-size tiles are drawn for the current view. Escape closes the map before Settings. Personal fog and pins remain independent identity/local-player saves. The map exposes no co-op sharing control and draws no remote player positions; direct temporary pings are server-validated and relayed independently to connected players. Neither path makes terrain, discoveries, annotations, or gameplay state authoritative. |
| Cosmetics | client | derive from replicated state/events |

The expanded map's personal-pin interaction is local UI state. The bottom-right palette selects original Cairn, Lantern, and Thread icons; selecting an icon and clicking the map opens a keyboard label draft at that position. Labels are optional, sanitized 0–32-character local text; a blank label renders an icon-only pin. Shift-click still opens the same draft directly. Completion, visibility, and removal remain local UI actions. A selected-pin summary states its style, label or unlabeled state, completion, and visibility, and remains visible when the marker is hidden. `Tab` selects each pin, including hidden pins; `Enter`, `H`, and `Delete` toggle completion, visibility, and removal, while `P` begins centred placement. `1`–`3` select a marker style; during label entry, keyboard numbers or gamepad D-pad left/right change the style. Gamepad face buttons offer selection, placement/toggle, and visibility/cancel actions; left trigger removes the selected pin, while shoulders zoom around the player. D-pad remains available for marker-filter navigation and pin-style selection. A version-1 independent local save retains the newest 256 valid pins only when its immutable seed and local-player index match; a mismatched slot is discarded. Pins do not query actors or discoveries, send an RPC, claim world content, or become server gameplay instructions.

Pointer coordinates use an explicit inverse of the world-to-map transform and are regression-tested at the minimum, standard, and maximum map zoom. When visible pins overlap, selection is deterministic: the newest rendered marker wins; if it is hidden, the next visible marker becomes selectable. The map widget deliberately takes local keyboard focus when opened so keyboard/controller pin actions cannot leak into gameplay input. These checks exercise only local widget and save-memory state; they create no network request or gameplay mutation.

### Expanded-map privacy, accessibility, and authority contract

The local map derives terrain/water tiles only from the immutable replicated `WorldSeed` and the owning pawn's normal replicated transform. It never queries population, loot, hazards, discoveries, or remote personal-map data. Current sight clears only a 6,500 cm circle around that owning pawn; coarse 500 cm personal cells retain at most 8,192 locally saved entries under seed and local-player index. A mismatched identity starts fresh. Personal memory uses sea-glass teal; lichen-ember remains a reserved, deliberately data-less treatment for future shared cartography. The 64-entry cache is bounded to 5,341,696 CPU pixel bytes across 64 entries of 65x65 overview terrain and 129x129 fog tiles. Pan and zoom reuse world-aligned entries; pawn movement or a change to local discovery invalidates only cached fog textures, while a world-identity change clears all tiles and rejects prior-identity workers. Open/pan/zoom/recenter and map input stay local, and the map must be profiled before increasing tile density or range. Expanded-map terrain uses original world-anchored biome patterns whose broad features remain recognizable at overview zoom; fine detail fades as sample spacing grows. The patterns are cosmetic output from local biome samples and are not saved or replicated.

Pins are private, finite local annotations: Cairn, Lantern, and Thread icon styles; optional sanitized labels from 0 through 32 characters; and at most 256 newest valid entries in a separate version-1 local save. The bottom-right palette selects a style, and the next map click opens label entry; Enter saves the pin even when its label is blank. Shift-click and P begin placement directly; click completes, Ctrl-click hides, and Delete removes the selected pin. Right-click on a map point requests a temporary global ping. Labels appear beside the icon only when nonempty. The selected-pin summary communicates style, label or unlabeled state, completion, and visibility independently of colour. `Tab` includes hidden pins in selection; `Enter`, `H`, `Delete`, and `P` control completion, visibility, removal, and centred placement. Arrow keys/Page Up/Page Down and D-pad/shoulders pan/zoom; face buttons select/place/toggle/visibility/recentre and left trigger removes. The map uses local keyboard focus while open. Screen-reader integration and remapping remain later accessibility work.

Co-op awareness is private by default and resets off when connecting or reconnecting. The expanded map exposes no opt-in control, displays no co-op status or player-presence entries, and draws no remote pawn positions. The only peer-visible map action is a temporary ping, independent of any pawn-position sharing state: right-clicking a map point requests one, while middle-click and `Q`/right-stick click remain cursor/centre alternatives. The server rejects unauthorized, non-finite, or out-of-world positions, applies a two-second per-sender cooldown, and adds the same server-timestamped record to every connected player's owner-only inbox; each inbox retains at most 16 unexpired records, so high-volume traffic may displace older records before expiry. Records use deterministic sender/sequence order, remain transient for ten seconds, and expire without saving; an unlabeled beacon may overlay fog, but the fogged terrain beneath it remains opaque and reveals no terrain, landmark, actor, or discovery. No remote exploration, pin, coverage, discovery, or persistent waypoint is exchanged. Construction-gated shared cartography remains absent until M2 supplies its server-owned construction and persistence prerequisites.

## Water presentation

The classifier and terrain-height sampler share `FKalmalaRegionalGeneration`. Samples carry seed/position metadata and four environmental fields. Weights and shaping are recomputed; only derived crop transforms and bounded hydrology splines are cached. Variable inland water clips against final collision triangles, and local maps interpolate depth on that lattice.

Biome classification always uses the current generator. World identity contains only the immutable server-owned `WorldSeed`. Production tuning changes may change existing seed layouts during development. Current saves use seed-only slots; former version-specific slots are neither loaded nor deleted.

The generator uses `FKalmalaMasterMap`: an independent fixed-seed 128 km land/water atlas, sampled through a game-seeded crop and rotation at the existing 16 km world radius. Its sign constrains Elevation and final ocean/land terrain; existing game-seeded Perlin fields still provide relief and environmental suitability. Special land biomes compete above hard origin-distance minima: Lakes 0.35–3 km, Elderwood at least 0.75 km, Mire 3–16 km and Tundra at least 4 km. The inclusive 350 m centre is Meadows/Ocean only. Meadows ends at 4 km; Mountains is the remaining land fallback beyond it and the elevated fallback outside the centre. Smooth eligibility ramps stay entirely within allowed distances. Lakes and Mire share regional, climate and relief suitability; only their distance ranges differ. Analytic basin generation is disabled by compiled policy. See `02-technical-architecture.md` for exact precedence and tuning.

Final land height blends complete biome candidates by the existing normalized
land-biome weights. Those weights include the continuous source-land/coast
attenuation; height adds each candidate's delta from the shared source height
once, then keeps the existing independently weighted ocean offset. Biome labels,
ecological fields and eligibility bands remain classifier inputs and do not
select a hard height switch.

The Meadows candidate adds separate seed-derived absolute-world hill and
rolling signals at provisional 160 m / 6 m and 45 m / 1.2 m settings. BH12a
adds a third independently seeded 12 m / 0.2 m floor-detail signal. These
layers enter the smooth, monotonic shoulder curve; candidates below 10 m remain
unchanged and the curve approaches a 40 m upper bound. The broad layer replaces
the previous 40 cm Meadows detail term. All amplitudes remain provisional:
BH25a/BH25b/BH25c/BH25d run fresh-source profile sweeps and select measured settings.
BH26 retains rendered gentler-route and under-vegetation floor inspection; no
floor aliasing or visual result is claimed before that gate.

Elderwood and Tundra share the same uncompressed candidate over the absolute-
world 1,000 / 150 / 30 m relief stack, plus its bounded legacy detail. The
provisional broad/middle/local amplitudes are 9 / 2.4 / 0.4 m, with 4–16 /
0.6–4 / 0.1–1 m trial ranges scheduled for validation. Tundra directly calls
the Elderwood function, so the candidate height cannot drift between those
biomes; continuous classification weights still determine each final blend.
BH25b owns fresh-source trial selection for rolling/Elderwood controls, BH25d
owns Lakes blend/lowland selection, and BH26 owns rendered profile acceptance.

Thunder Mountains now increase gain only above the named `MountainHighlandThreshold`
of 1,000 cm. The increase ramps with smoothstep across a further 1,000 cm,
scales by the existing continuous Mountain region field, and reaches a provisional
0.4 gain at full region strength. The separate detail term is clamped to ±1 and
uses a provisional 80 cm amplitude. Commandlet preview bounds are 500–2,000 cm
for the threshold and blend range, 0.1–0.8 for gain, and 40–160 cm for detail.
Thus low source-height fallback and foothills retain their base height (apart
from bounded detail), while only high terrain receives the broad amplification.
Biome weights, distance eligibility, coast attenuation, river order and seeded
sampling are unchanged. These source settings are provisional; rendered
mountain/foothill acceptance remains BH26.

The master seed is a compiled constant. Crop transforms derive from the game seed. Rivers remain enabled and small streams are disabled. Authority, population IDs, collision and maps derive from the same server-owned seed and current build.

### Sea-level river terrain and hydrology contract

`FKalmalaRegionalGeneration` preserves seeded candidate selection, stable-ID
coalescing and spline XY placement. Segment endpoint Z still records source
relief for path construction; it is not the river surface. The compiled
`FKalmalaRegionalTuning::bEnableAnalyticBasins` policy defaults to false, so
production rivers use `SeaLevelWorldHeight` (Z=0 cm) without lake-basin support.
Rivers retain their land-biome identity; carved water does not reclassify land as Ocean.
Small streams remain disabled; explicitly enabled debug streams retain their
existing interpolated-height profile.

For shortest XY distance `d` to a clamped segment projection and segment width
`w` (currently 600 cm for rivers), the bank weight is
`B = 1 - smoothstep(0.2*w, w + RiverBankExtent, d)`, with clamped cubic
`smoothstep(t) = t*t*(3-2*t)`. `RiverBankExtent` is currently 1800 cm and
`RiverBedDepth` is 80 cm in `FKalmalaRegionalTuning`. This symmetric profile has
full central influence and zero derivative at both ends. The maximum B across
segments is applied once, avoiding cumulative carving at joins. The spatial
index includes the complete bank envelope, including across hydrology cells.
The separate narrow `RiverWeight` still ends at `w`; it does not bound the
carved banks or determine whether final terrain is wet.

Shaping order is base relief and biome relief, optionally enabled analytic
bowls, optional stream shaping, source land/coast clamp, then priority river
carving. With pre-carve height H, bank influence B and the current disabled-basin
policy, the river bed is `-RiverBedDepth` and final height is
`lerp(H, min(H, -RiverBedDepth), B)`. High source land cannot refill a channel,
and already deeper ground is never raised. The central bed reaches at least
80 cm below sea and river water is exactly Z=0. Shoulders blend back to H;
points outside every bank envelope retain their prior terrain and source-coast
sign. The shoulder can remain dry where its final height is above water.
Regional wet state is strictly `WaterLevel > Height`.

Analytic basin generation is currently disabled by the compiled shared policy.
The regional sampler therefore skips bowl/rim height shaping, inland basin
water-level assignment and positive basin weights without changing biome
identity. Lake-basin flood-fill and mesh helpers remain available as reusable
utilities for synthetic fixtures; production samplers and consumers now require
basin support, so the former lake-precedence path is not current production
behavior. No derived basin-result cache survives independently of its source.

The same immutable server-owned seed and current tuning must produce identical
height, water level, wet state and river influence regardless of query order,
cache warmth or interleaved sampling of another seed. Crop and spline caches
are disposable derived acceleration, not world state. Terrain rendering and
collision consume final carved samples; sea water clips their shared 125 cm
triangles at Z=0. Detailed maps interpolate signed water depth on that lattice
before deciding wetness, including negative dry-bank vertex depths. Expanded
overview uses final regional height; coarse pixels can miss narrow channels.
No alternate river height raster, RPC, replicated property or save field is
introduced. Clients derive presentation/prediction from the replicated identity;
server terrain and gameplay decisions remain authoritative.

Focused regression source covers high-ground centers, both banks and unchanged
exterior, same-seed/cache/order repeatability and different-seed variation,
hydrology-grid and terrain-patch seams, and unchanged terrain/coast/lake controls.
These cases are not recorded as executed acceptance: compilation, automation,
preview inspection and host/client agreement remain separate queued gates.

The developer `ExportWorldMaps` commandlet can temporarily install thread-local preview tuning, with installation rejected outside headless commandlets. Each render restores defaults and invalidates derived crop/hydrology caches. Its fast biome query shares the normal regional classifier while skipping hydrology and height-only work; analytic-basin generation is disabled by the compiled policy. Default output is checked against full sampling. A warm parameter-file watcher writes PNGs without starting gameplay. This adds no runtime console setting, client authority, replicated property, production-layout setting or save change; see `07-development-setup.md` for the workflow.

Water presentation uses `FKalmalaWaterSurfaceMesh` to clip shared terrain triangles at sea or sampled regional water levels. Analytic enclosed-basin generation is disabled by the compiled shared policy, so the regional sampler emits no inland basin bowl, basin water-level assignment or positive basin weight. Lake samplers, water/shore meshes, map depth, audio proximity, shoreline exposure and lake-adjacent discoveries all require basin support. `FKalmalaLakeBasin::Find` remains a reusable six-connected 125 cm lattice utility for synthetic fixtures; it is stateless and is not production lake generation. The only lake-basin origin memoization stores the generated player-start transform, not basin/water results. Minimap and expanded-map presentation rasters retain derived water colours until their normal inputs change or their presentation objects are rebuilt; they do not cache basin results independently. Generated patch sections are cleared when rebuilt, and the regional cache contains hydrology splines only. The policy is compile-time, so changing it requires a new build; no persistent basin-result cache needs invalidation. Sea and sea-level river classification/clipping remain active. Shore treatment covers only actual shallow supported terrain; there are no additional water collision surfaces, gameplay fields, replicated values or saved data. The configured startup map supplies lighting without Unreal's template landscape, which otherwise intersects the generated terrain.

### River terrain/water consumer audit — 2026-10-18

Source inspection confirms no consumer correction is needed for the sea-level
channel. `FKalmalaTerrainHeightSampler::SampleHeight` returns the final regional
height after river carving. `AKalmalaGeneratedTerrainPatch::BuildVisualSurface`
uses that height for both rendering and collision in the same mesh section
(collision enabled). `FKalmalaWaterSurfaceMesh::BuildPatch` samples that same
height on the same 24-cell, 3000 cm patch lattice (125 cm spacing), with the same
two triangle diagonals and world-bound clipping. Its sea-water path clips each
triangle against Z=0 without a source-elevation or biome filter, so carved inland
river triangles are eligible as well as ocean triangles. Water is collision-free;
the carved terrain section supplies the physical bed and banks.

Eleven focused source assertions and twelve numeric bed/bank sea-plane
intersection cases passed. This is a source/formula audit, not compiled mesh,
physics or rendered verification. Sub-cell curvature remains represented by the
existing linear triangles; river preview, patch-seam and peer gates remain queued.

## Companion minimap presentation

Phase 7 begins with `FKalmalaOceanSampler`: a derived sea-depth query using the same 125 cm lattice and triangle diagonal as terrain collision and clipped ocean water. The normal overload derives the lattice origin from the immutable world's generated start and caches only that origin by identity. The explicit-origin overload supports patch callers. It returns validity, interpolated terrain height, and nonnegative depth below the existing zero-height sea surface; dry ground and the exact shore have zero depth. Invalid identity and nonfinite coordinates are rejected. The minimap consumes this triangle height and sea-depth result instead of sampling curved Perlin height between mesh vertices, so ocean and carved Z=0 river water follow the visible surface. Analytic inland basins are disabled by compiled policy and their consumers explicitly gate on basin support.

This query adds no world field, terrain deformation, island placement, collision volume, RPC, replicated value, or saved data. `FKalmalaIslandLocator` remains a developer-only query over existing emerging seed terrain and never reserves or creates content. The first gameplay consumer is `UKalmalaCharacterMovementComponent`: server and predicting owner sample the replicated immutable identity at the pawn's current position, enter its generated-ocean custom mode at 100 cm sea depth, and return to normal walking below 75 cm. The server remains authoritative through normal Character Movement replication and correction; the client supplies neither a depth nor a requested mode. The custom mode floats the existing pawn capsule at the zero-height visible sea and retains swept collision with the shared terrain mesh. Server terrain activation retires patches outside the bounded union of all player neighborhoods before activating needed ones, so ordinary long-distance sea movement refreshes collision/rendering without expanding the 25-patch budget. `OceanTravel.ps1` confirms a current-generator host and conflicting-seed client independently derive the same existing island, enter ocean, and arrive over normal replicated movement; its travel target and temporary pawn-collision relaxation are verification-only. Boats and inland lake-depth gameplay remain future Phase 8 work.

The companion minimap uses a `ULocalPlayerSubsystem` per local player and rebuilds its widget/input binding when that player's controller changes. Viewport sizing and positioning precede anchoring because UE 5.8 resets anchors in both setters. A transient 129x129 sRGB texture fills the circle with world-anchored original biome patterns, bilinear filtering, and a transparent feathered edge; sea-level water overrides land treatment, while inland lake depth requires basin support. This is disposable presentation of the existing classifier, not a new world-generation field or persisted biome map. Unchanged position/radius/world identity reuses the raster; facing still refreshes. Texture uploads update an existing GPU resource. The configured CommonUI viewport client routes modal input; Menu mode always retains wheel ownership, including menus with captured previews. No discovery/landmark visibility contract exists yet, so the minimap does not query or draw harvest nodes, hidden discoveries, hazards, or other players.

The expanded map places a local fog texture over all generated terrain/water tiles and clears a fixed 6,500 cm circle around the owning pawn's normal replicated transform. It records the revealed 500 cm coverage cells in a separately versioned, bounded (8,192-cell) local `SaveGame`, keyed by the immutable seed and local-player index; a mismatched identity is never reused. This is separate from server-owned generated-world sparse saves and stores no terrain classification, actor, discovery, or gameplay data. Current sight is fully clear; remembered personal coverage carries a translucent sea-glass teal treatment. A contrasting warm lichen-ember palette is reserved for a future explicitly opted-in shared-cartography source, but this phase does not create, load, query, or render shared coverage. Map pan and zoom merely transform current/personal coverage into the view; neither can widen it, choose another player, issue an RPC, or query actors. Opaque fog pixels contain no terrain, water, landmark, population, or discovery treatment.

World-map tiles and the companion minimap use the same world-coordinate terrain/water classification. The expanded map applies original zoom-stable biome textures; the companion HUD retains its existing local palette, so their RGB rasters need not match. The dimensioned raster is fully opaque even when square; only the HUD crop applies a circular alpha mask. Tile and fog drawing share the map-panel clip, preventing terrain outside the panel from disclosing unexplored surroundings. The local-player subsystem creates the collapsed map before the first M press and records owning-pawn coverage every 0.5 seconds during gameplay, saving only changed coverage through the existing identity-scoped slot. Closed maps generate no terrain tiles or fog textures. Thus walking accumulates connected explored terrain independently of opening the map; pan and zoom never record exploration.

## M2 item catalogue

All player-placed objects use direct-material Building Hammer recipes. Portable construction items are forbidden. The loader partitions the shared source definition files into inventory `Items` and presentation-only `Buildables`; `FindItem` and stack/grant/save validation never admit a placed-object identity. `FindBuildable`/`FindDefinition` supply names, descriptions and icons for construction and station UI only. Inventory catalogue validation rejects both known construction identities and unknown item IDs ending in `Kit`. Each buildable metadata entry must have a valid direct-material recipe. This keeps construction definitions usable for presentation without creating a portable inventory object.

Buildable metadata remains authored under `items/buildItems/` using the shared definition schema and registered through `itemFiles`. These records are partitioned into `Buildables`, never inventory `Items`. Legacy `MaxStack` fields in these metadata records do not permit inventory stacks. Internal actor/icon identities ending in `Kit` identify placed objects only; they are not inventory items or recipe outputs. All 19 placement recipes in `recipes/building.json` normalize to `BuildableOutput`, with empty inventory `Output` and no crafting-station requirement. Chest, Cooking Rack, Cauldron, Frying Pan, Grinding Stone, Charcoal Kiln, Smelter, Garden Plot, Timber Door, Timber Beam, and Timber Stairs use this same path. Authored costs are unchanged (Frying Pan consumes 5 Iron; Grinding Stone consumes 6 Wood and 4 Stone; Charcoal Kiln consumes 20 Stone; Smelter consumes 20 Stone and 20 Wood). Server placement has no kit-consumption fallback: it revalidates Hammer ownership for ordinary builds and a usable Main Hand Planting Trowel for Garden Plot placement, target terrain, raw material payment and persistence. Workbench Tool Rack and Forge Anvil still require their matching station through the existing server attachment checks. Campfire retains its existing initial raw-fuel payment. Removed/unknown portable kit stacks fail inventory/storage/save validation without conversion or migration.

The A1 `CharcoalKilnKit` construction identity is directly placeable through presentation-only buildable metadata and its direct 20 Stone recipe. Its server-rechecked preview uses the kiln footprint, a clear 1 m approach, and 2 m overhead clearance. The server offers one-Wood load and one-Coal collection controls at the interacted kiln, keeps input/output at 10 units each, reserves Wood when a job starts, and advances one Coal per 30 active server seconds. Output state is saved before collection is granted, and the remaining active seconds are saved as processing advances; no offline time is applied.

The A1 `SmelterKit` construction identity is presentation-only and directly placeable for 20 Stone and 20 Wood. Its preview uses a 60 × 60 × 82 cm half-extent, clear 1 m interaction approach, and 2 m overhead clearance. The server accepts only one Copper Ore, Tin Ore, Iron Ore, or Coal per fixed-item request at the interacted Smelter. Ore is processed in accepted FIFO order; each ore and one reserved Coal produce one matching Copper, Tin, or Iron bar after 30 active server seconds. Ore capacity includes a reserved job and is 10; Coal capacity likewise includes a reserved job and is 10; all bar types share 10 output slots. Full output pauses queued work. Input queues, output counts, active ore type, and remaining seconds are saved in construction schema 6 and replicated from the server. Collection saves the output decrease before granting one bar, with rollback if the inventory transaction fails. No offline time applies. The Forge's level-1 Bronze alloy recipe immediately consumes 2 Copper and 1 Tin for 1 Bronze, with a maximum batch of 5 and no extra Coal or skill requirement. Schema-6 record removal rejects a Smelter while any input, output, or job remains; an empty record can be removed. Earlier incompatible development construction saves are rejected without rewrite.

The A7 Garden Plot uses the same paid construction record. Its existing direct-material Hammer recipe is also the only structure exposed by the Planting Trowel's Farming menu. Right-clicking while aiming at a visible Garden Plot with a usable Main Hand Planting Trowel opens the plot-specific Cultivation menu. The server accepts Carrot food planting for 1 Carrot Seed, Carrot seed planting for 1 Carrot, Potato tuber planting for 1 Potato, Rutabaga food planting for 1 Rutabaga Seed, Rutabaga seed planting for 1 Rutabaga, Onion food planting for 1 Onion Seed, or Onion seed planting for 1 Onion, spends one Trowel condition only after an accepted planting, and saves the crop mode with its remaining active seconds. These entries cannot plant without the current Garden Plot context. Growth advances only while the plot terrain patch is active; it pauses for inactive regions and while the server is stopped. After 600 active seconds, a mature crop grants three matching Carrots, Carrot Seeds, Potatoes, Rutabagas, Rutabaga Seeds, Onions, or Onion Seeds once; PotatoSeed remains unused. A full pack leaves the crop on the plot, and a planted plot cannot be removed. No offline time applies.

The A8 Timber Door is a directly paid replacement for a saved Windbreak Wall segment: the Building Hammer consumes 8 Wood, the door keeps the segment's construction ID and transform, and no portable kit is created. Construction schema 6 stores its open state. Interacting within 2 m toggles the replicated door state; the server rejects closing while a pawn occupies the opening. Closed doors retain Windbreak shelter behavior and open doors do not.

The A8 Timber Beam is a presentation-only buildable placed directly for 2 Wood. Its collision and generated presentation are a 200 cm member with a 15 cm square section. Looking near a structural endpoint and keeping the view within 60 degrees of level selects a horizontal span; both ends must reach level terrain or an existing structural endpoint. A steeper view selects a vertical post whose lower endpoint must rest on ground or a supported structural endpoint. Supported endpoints come from the existing Floor, Wall, Timber Door, Roof and Timber Beam construction records. The beam uses the matching Wall-category health and rain-wear behavior without becoming a windbreak. The client preview and server placement request use the same evaluator; the server rechecks endpoint support and clearance before charging materials. The ordinary construction record stores its snapped transform in schema 6, so no new save field or compatibility migration is added.

The A8 Timber Stairs are a presentation-only buildable placed directly for 6 Wood. Five solid, walkable 40 cm treads form a 200 cm run, 100 cm width and 100 cm rise; the stair actor's ordinary schema-6 transform places the lower endpoint on dry, level generated ground. A second route aims at a placed Floor and attaches the upper stair edge to its outward side; the lower endpoint must still meet firm terrain. The server reruns the same placement evaluation, ground/floor support, dry-world and clearance checks before charging materials. The root bounds are non-blocking and each of the five step volumes supplies collision, so players walk the treads without a ladder/climb mechanic. Lower and upper endpoints join the structural support set. Stairs use the Wall-category health and rain-wear behavior without acting as a windbreak or roof. No save field, migration, portable inventory item, or kit fallback is added.

The A8 Camp Bed is a presentation-only buildable paid directly with 4 Wood, 2 Boar Hide and 2 Deer Hide. Its 200 × 100 cm footprint must rest fully on level, dry generated ground or on an existing Timber Floor, with a clear 1 m approach at one short end. Preview and server placement share the support, water, bounds and overlap checks. The generic schema-6 construction record stores its placed identity and transform; it uses the Storage-category health and rain-wear behavior and has no item profile, inventory item, acquisition recipe, upgrade or attack data. Interacting within 2 m reports the accepted Rest action and poses only the local player's procedural model across the bed. The pose ends on movement or health loss. It changes no stats, world time or spawn point and is neither saved nor replicated; no new RPC or save field is added.

Manifest category file lists, catalogue arrays, and their aggregate JSON text have no project-defined count or size cap; each record remains subject to per-entry identity, schema, reference, and gameplay validation.

`UKalmalaItemCatalogue`, `UKalmalaRecipeCatalogue`, and `UKalmalaToolCatalogue` load schema-version-16 data from the manifest at `Content/Data/GameCatalogues.json`. Ordinary material items are grouped by role in `items/materials/wood.json`, `items/materials/ore.json`, `items/materials/metal.json`, `items/materials/cooking-ingredients.json`, `items/materials/crafting-ingredients.json`, `items/materials/hides.json`, and `items/materials/others.json`. Other ordinary item and recipe definitions live in their category files under `Content/Data/GameCatalogues/items/` and `Content/Data/GameCatalogues/recipes/`; ammunition such as WoodenArrow is in the sibling `items/arrows.json` category; buildable stations and construction items are grouped under `items/buildItems/`. Recipes that require Workbench, Forge, Cauldron, Cooking Rack, or Frying Pan are grouped by station under `recipes/stations/`; the Building Hammer recipe is grouped under `inventory.json`, and build-menu construction recipes remain in `recipes/building.json`. Carried weapon profiles are grouped by family under `items/equipment/weapons/` (axes, bows, knives, maces, pickaxes, spears, and swords); wearable item definitions are grouped under `items/equipment/armor/`. The reusable Building Hammer profile remains at `items/equipment/building-hammer.json`; `itemProfileFiles` routes the weapon and Building Hammer profiles into the runtime tool catalogue. Equipment acquisition records share each station file under `recipes/stations/` in its `recipes` array; the loader routes records with `ItemId` into the runtime tool catalogue. Wearable gear uses ordinary output recipes in the same station files, validated against item definitions and visible server station context. Weapon level-one costs are grouped by family under `item-level-up/costs/weapons/`, including Bow; wearable costs are grouped by type under `item-level-up/costs/armor/`. Stone and Iron costs generally use their acquisition ingredients. Bronze costs generally retain each weapon wood quantity and replace its Stone component with Bronze bars at the same quantity. Bronze Axe, Knife, Spear, and Mace acquisitions and upgrades use Lightwood in place of Wood; Iron Axe acquisition and upgrade both use Lightwood; Iron Knife, Spear, and Mace acquisitions and upgrades use Densewood. Pickaxe upgrade costs remain unchanged. The user-directed A1 acquisition targets are Stone Pick: 10 Wood + 5 Stone, Bronze Pick: 10 Wood + 10 Bronze, and Iron Pick: 10 Wood + 10 Iron; apply the metal-cost recipes with the A1 production route. Stone upgrades use Workbench materials, while Bronze and Iron upgrades use Forge materials. The Construction Hammer cost lives in `item-level-up/costs/construction-hammer.json`; wearable costs are split by type into the six files under `item-level-up/costs/armor/`. All files are listed by `itemUpgradeFiles`, use an `upgrades` array, and key records only by `ItemId`, which may identify either a carried tool profile or a wearable gear item. Gear costs pass item, station, material, and scaling validation but are not yet converted into carried-tool progression entries; gear upgrade transactions remain pending equipment integration. The loader sorts ordinary items and carried profiles by `ItemId`, ordinary recipes and equipment acquisitions by `RecipeId`, and upgrade costs by `ItemId`, so file organization does not define catalogue order. These arrays are no longer configured in `DefaultGame.ini`. Each ordinary item has a clean external catalogue ID, player-facing name, bounded description, and stack limit. No property or value in the ordinary item/recipe arrays contains the old "Kit" suffix. The loader maps clean buildable IDs and station arrays back to stable runtime item and construction IDs so placement and save compatibility remain unchanged. Runtime equipment profiles hold stable ItemIds, action/skill requirements, base durability, and typed damage bonuses. Shared attack styles live in `attacks/attacks.json`, with a named attack list and explicit ItemIds for each family across Stone/Bronze/Iron tiers; profiles contain no Attacks field. Shared item stat gains live in `item-level-up/rewards/item-stats.json`: each entry has `ItemIds` and a `LevelUpReward`, with no reward property on individual item profiles. Entries may list multiple known IDs but cannot overlap or repeat an ID; unlisted items receive zero gains. Stats at level L equal base + (L−1) × reward for durability and each damage channel; zero rewards leave those stats unchanged. Durability consumption is a shared server rule of one condition point per accepted consuming use; profiles do not author DurabilityCost or per-item exceptions. Acquisition recipes use stable IDs such as `RecipeId: BronzeAxe` and `Level: 1`. Upgrade cost records define the level-one-to-two cost once; for carried-tool upgrades, current level L consumes base quantities × 2^(L−1) and requires station level L+1. Acquisition and upgrade-cost JSON omit zero skill requirements; the loader supplies None/0 internally. Upgrade-cost JSON also omits `RequiredStationLevel: 0`, which the loader supplies before strict conversion; acquisition recipes keep their explicit station level. Predecessor IDs/levels are derived internally and are not JSON fields. The server generates carried-tool station steps through level 10 for profiles with authored upgrade costs, validates material references and overflow, and rejects malformed data. Scaled requirements may exceed a current stack capacity without invalidating authored levels; the actual inventory exchange still enforces capacity and payment. Station upgrades have an explicit level-10 item cap, while every item source shares a hard level-20 cap. Authored stats, carried-item validation and save validation are independent of station recipes, so authored levels 11–20 can be granted by future non-station sources but cannot be station upgrade targets. Item stats are computed through the hard level-20 cap from the reward rule. Bow gains +50 maximum durability per level and has no damage reward while shooting remains unimplemented. Stone Axe, Bronze Axe and Iron Axe share +50 maximum durability and +5 slash per level; Knives gain +5 Slash/+5 Stab, pickaxes and maces +5 Crush/+5 Pierce, and spears +5 Stab; each family also gains +50 durability. Building Hammer has zero rewards. Bronze Axe level-one/two values are preserved. Station content still tops out at level 2; higher stations and non-station grants are separate work. C++ retains enum mapping and server validation/action logic; it no longer authors equipment stat numbers or item progression costs. The loader reads each file through Unreal's file layer, including packaged UFS builds; all three catalogues fail closed if parsing, schema, bounds, cross-references, or duplicate-ID validation fails. `DirectoriesToAlwaysStageAsUFS=(Path="Data")` keeps the manifest and category files available in packaged builds.

The server's JSON-backed catalogues are the authority for inventory and tool-progression transactions; a client's copy may only inform presentation. `IsValidStack` rejects unknown/empty IDs and any quantity outside 1 through the configured stack limit. `CanAddToStack` permits zero existing quantity but requires a positive addition and sufficient capacity, using subtraction after bounds checks to avoid integer overflow. A missing/duplicate/invalid catalogue fails closed, with no project-defined catalogue-count or aggregate JSON-size ceiling; each item, recipe, profile, upgrade, and progression entry remains independently validated. Per-entry constraints include two authored levels per tool and four materials per tool recipe, and 999 units per stack. Tool upgrades point to the same item at the immediately preceding level. The Construction Hammer's level-one item is granted by its enabled, station-free Building Hammer recipe in Inventory Craft; its station-upgrade chain begins at level two and validation requires that inventory acquisition route. Other tool acquisitions have no predecessor. Definition lookup uses Unreal's case-insensitive `FName` identity. These pure validation functions grant no items and do not themselves establish caller authority: the inventory and crafting components still check server ownership, capacity, station context, and transaction intent before mutation. The item catalogue uses schema 16 for integer Armor on wearable items and level-up rewards; equipment acquisition and upgrade records use `ItemId`. This change does not alter RPCs, inventory replication, or harvest behavior. The current player-discovery save schema is 2 and stores `ItemId` directly; there is no compatibility migration during development.

`WoodenClub` is a non-tiered, crafted melee profile stored in the maces family
file. Its dedicated Basic attack list has zero style bonus, and its acquisition
and upgrade use the existing server-validated Workbench progression path. It
adds no harvesting action, RPC, replicated field, or save data.

Initial wearable content is defined as ordinary, non-stackable items grouped by type under `items/equipment/armor/`. `GearSlot` is one of Head, Cape, Chest, Shield, Legs, or Boots; `MaxDurability` defines condition and integer `Armor` defines the only wearable defense stat. Wearable Armor reduces only Slash, Crush, Pierce and Stab once per hit, clamped to zero; it never reduces Lightning, Fire, Water, Holy or Poison. General Armor is then applied once to the remaining physical plus Lightning damage. Shared entries in `item-level-up/rewards/item-stats.json` may target either a carried tool profile or a wearable gear item. `UKalmalaItemCatalogue::GetGearLevelStats` computes condition and integer armor as base values plus `(level - 1) * reward`, bounded by the shared level-20 cap. Each gear item now has a station-validated ordinary acquisition recipe and a level-one upgrade-cost record; the six files under `item-level-up/costs/armor/`. Crafting uses normal server-side station, material, and capacity checks; gear upgrade costs are data only until equipment upgrade transactions are implemented. The inventory component now supports owner-only, session-only equip/unequip for these six wearable slots: equip requests identify an exact carried item GUID, the server derives its compatible slot from the catalogue, and replacing gear returns its original GUID to the newly freed pack cell. Unequip requires a free cell. Equipped gear is excluded from the forty-cell grid. The Inventory popup displays equipped names and offers owner-facing actions. For server-resolved wildlife damage, the server sums level-one Armor from non-depleted equipped items and subtracts it once from physical damage. Each accepted wildlife hit spends one condition on every equipped wearable after resolving that hit. Depleted gear contributes no defense on subsequent hits. ConditionSpent replicates only through owner-private gear state; a server-only GUID ledger preserves wear through same-pawn pack/equip swaps. Wear is not transferred to another pawn or saved; repair and upgrade transactions remain pending. Main Hand retains its numbered grid cell and marks the equipped GUID blue; gear persistence remains pending. Gear slots remain session-only; no equipped-slot or client-authored defense value is added to saves.

## M2 player inventory

`UKalmalaInventoryComponent` is a replicated default subobject on each `AKalmalaCharacter`. Its private stack array uses `COND_OwnerOnly`; simulated remote pawns receive no inventory contents. Inventory starts empty and lasts only for the pawn lifetime. Stack rows are bounded by each item's server catalogue limit and all stacks plus carried tools share the forty-cell capacity; duplicate item IDs are valid. Every acquired unit receives a server-generated item GUID, which remains attached when stacks merge, split, or transfer. Trusted server gameplay calls `TryGrantFromServer` or `TryConsumeFromServer`; both reject non-authority callers and invalid quantities before mutation. Failed operations leave contents unchanged; consumption removes the GUIDs of consumed units and removes an exhausted slot. There are no client add/remove/reorder/set RPCs. The existing interaction intent is the harvest entry point described below; regular player inventory persistence and reconnect restoration are not wired yet.

The on-demand `UKalmalaInventoryMenuWidget` reads only its local controller's pawn inventory and carried tools. It shows the owner's pack, equipment, item details and supported actions when the player opens Inventory; it is collapsed during normal gameplay and does not query remote inventories. Display catalogue data never controls transaction limits or authority. The former persistent pack HUD and its left-side panel subsystem were removed in M12.

Generated harvest nodes now grant one material through that inventory. The server derives Wood or Stone by `FCrc::StrCrc32(PersistentSpawnId) % 2` in that order, including existing discovery nodes; fuel and construction supplies remain later crafting outputs. Initialization is one-use and rejects non-harvest descriptors or non-finite locations. An interaction requires an initialized, available server node and an authoritative same-world character within 250 cm. The normal no-argument character RPC still performs the server view trace; clients cannot supply a target, item ID, or quantity. The complete catalogue/capacity-validated inventory grant precedes depletion, collision removal, and the existing sparse-save delegate. A full material stack leaves the node intact for retry; repeated accepted requests cannot grant twice. Stable IDs, generated placements, and sparse-save schemas remain unchanged. Pawn inventory is still transient, so persisted node depletion does not imply item restoration after reconnect.

## M2 workbench and storage

Accepted workbench/storage kits use the existing replicated construction actor and current schema-6 construction save. Visible workbenches within 250 cm satisfy camp kit assembly. Chest contents live only in a separate server-owned schema-2 save, bounded to 128 stable construction IDs with 16 unique catalogue-limited stacks each, under exact seed identity. Every unit in a chest stack has a saved item GUID. Current-format records must already contain valid item identities and GUIDs; invalid or incompatible saves fail closed without overwrite. The game mode retains both construction and storage save containers through reflected transient references.

An accepted construction-station interaction may publish an owner-only station-context event containing the exact replicated actor reference, its station kit, stable construction ID, and a bounded serial. This is presentation context only: the client cannot choose the target, and every recipe, repair, upgrade, and storage action continues to resolve and validate its own current server-side station, range, sight line, inventory, and requirements. Context events add no RPC payload, save field, or gameplay authority.

The owning crafting component accepts no-target inspect/close intents and item-GUID-only one-unit deposit/withdraw intents. The server chooses the chest, validates the exact GUID against the current source container, controlled-pawn range and sight line plus its registered construction record, checks both inventory results, saves the candidate contents, and only then publishes the private pack. A transfer preserves that GUID across stack merges and container changes. Failed validation/writes leave live containers unchanged. Owner-only snapshots refresh or expire every 0.25 seconds and clear when the menu closes; shared construction actors carry no contents. UI uses only this component seam. Carried inventory still resets with its pawn; storage persistence does not imply player persistence. Full controls, save limits, and verification are in `02-technical-architecture.md`.

## Module boundaries

- `KalmalaCore`: tags, logging, shared data types, save interfaces.
- `KalmalaGameplay`: character, GAS, inventory, crafting, construction, interaction.
- `KalmalaWorld`: world generation, element grid, weather, harvesting, AI encounter logic.
- `KalmalaUI`: Common UI screens and view models.
- `KalmalaServer`: dedicated-server configuration and server-only services.

Do not make UI call world actors directly. Use components, interfaces, gameplay messages, or subsystem APIs.

## Environmental exposure contract

Each pawn has server-owned, replicated display state: ambient temperature, precipitation intensity, wind exposure, wetness, warmth, and shelter. Inputs are normalized to `[0,1]` except ambient temperature; wetness and warmth are clamped to `[0,100]`. On each fixed server tick, precipitation and wind increase wetness and reduce warmth, while shelter reduces both effects; future fire warmth is an additional server input. Clients never submit or simulate inputs or outcomes.

The server derives ambient temperature from the continuous Temperature field. Ground wetness combines Humidity with a continuous low-ground contribution and a short deterministic Shimmering Lakes adjacency probe; wind exposure combines continuous ridge elevation and terrain slope, then subtracts continuous Flora-derived natural cover. Server weather supplies precipitation and wind strength. These inputs are sampled at the pawn's server transform; they are environmental conditions, never authored zones or client-selected values. At a fixed one-second server interval, exposure advances wetness and warmth, replicates both with a warmth-derived travel-speed multiplier, and applies that multiplier through Character Movement. Cold, wet, wind-exposed travel can reduce movement to 68% at minimum warmth; this is reversible, not a hard gate. Sheltered players dry and recover slowly once dry, and a nearby lit campfire materially accelerates both recovery paths.

Shelter is also sampled by the server at the pawn. Natural cover supplies continuous partial shelter from Flora. A player-built roof contributes only when an overhead `ECC_Visibility` trace hits collision geometry tagged `KalmalaShelterRoof`; a player-built windbreak contributes only when a trace toward the current wind source hits collision geometry tagged `KalmalaShelterWindbreak`. Construction must add those tags only after server-authoritative placement. The sampler never queries authored shelter volumes, accepts client hit results, or treats untagged terrain/level geometry as shelter.

The campfire seam is a replicated, server-owned actor. Only a validated server interaction may light dry fuel; the server advances fuel wetness from the replicated weather's precipitation and wind, replicates the resulting lit state and effective warmth, and extinguishes soaked fuel. Its nearby warmth contribution is a server-readable falloff value for the later per-pawn exposure update; clients only render the replicated firelight and cannot submit fuel wetness, weather, or warmth.

`FKalmalaCampConditionSampler` evaluates any freely chosen position from the same deterministic terrain, exposure, lake, and harvest-descriptor contracts. It reports natural cover, ground wetness, bounded nearest-water distance, and nearby generated harvest-node availability for developer inspection and validation only. It does not spawn, reserve, reveal, or direct players to a camp site; `GameMode` invokes the optional inspection only on the server.
## Bounded interaction-grid contract

M3 adds one server-owned, ground-aligned interaction grid; it is a local simulation budget, not a world map, streaming partition, navigation aid, or client prediction system. A cell key is the signed integer pair `(floor(WorldX / 200 cm), floor(WorldY / 200 cm))`. It has no client-selected Z coordinate: the server samples the generated terrain height at the key's 200 cm-square centre. The key is stable across an active session and can be derived again after reload from the same immutable `WorldSeed`; cells never determine terrain, biome, population, routes, or construction placement.

The authoritative baseline material comes only from that server terrain/material sample: `Ground`, `Vegetation`, `Stone`, or `ShallowWater`. `Vegetation` is flammable; `Ground`, `Stone`, and `ShallowWater` are not. A future placed-wood or metal interaction must be introduced as a separately validated server-owned material source; ordinary client construction actors do not rewrite grid material in M3. Each active cell carries bounded server state: material, a continuous temperature in `[0,100]` with display bands `Cold` (below 25), `Normal` (25-59), `Hot` (60-84), and `Burning` (85+), plus wetness in `[0,100]` with display bands `Dry` (below 25), `Damp` (25-74), and `Soaked` (75+). The bands are presentation labels, not alternative client simulation rules. All calculations clamp non-finite values before they become state.

Only `GameMode` activates cells. At each one-second server simulation tick it takes the union of a 9x9-cell square around every possessed, non-spectator pawn and a 7x7-cell square around every lit replicated campfire. It keeps at most 1,024 cells total, selecting campfire neighborhoods first and then pawn neighborhoods in ascending stable server actor identity, with duplicate keys removed before the cap. No cell is activated by a client map position, camera, cursor, requested target, or locally reported environmental value. A cell without an eligible source leaves the active set after the tick; its derived transient heat and wetness are discarded rather than simulated in the background.

The server alone initializes a newly active cell from the deterministic terrain/material baseline and current server weather/exposure inputs, advances rain, drying, heat, and any later bounded spread, and chooses every resulting material, temperature, wetness, ignition, and delta outcome. It may replicate a compact sparse snapshot only for active cells relevant to a receiving player's existing server-observed pawn neighborhood. That snapshot contains the key and authoritative state needed for nearby rendering; it never exposes inactive cells, unactivated terrain/material classification, remote camps, hidden population, or a complete grid. Clients interpolate or render received state only and must clear snapshots that leave normal relevancy; they do not initialize cells, infer spread, retain an authoritative cache, or feed grid values back into gameplay.

Persistence is deliberately sparse and versioned independently of generated population, construction, storage, and player UI saves. The future `InteractionGrid` save container will carry a schema version, immutable `WorldSeed`, and sorted cell-key deltas. It may save only a server-accepted gameplay-changing departure from deterministic baseline (for example, a later permanent consumed or charred material transition). Rain wetness, temperature, heat, active membership, and ordinary extinguishing are transient and are never saved. M3's initial rain/heat slice creates no permanent material transition, so it must neither write empty grid saves nor add a delta merely because a cell was wet or hot. A mismatched seed, unsupported schema, malformed key, invalid material transition, or non-finite/out-of-range value fails closed and cannot overwrite a valid save.

There is no client cell-state RPC. Existing interactions continue to submit only their established intent; if a later interaction can affect a cell, the server must derive the candidate key from a fresh authoritative trace or source actor, validate controlled-pawn range, sight, source eligibility, active membership, material, wetness, temperature, rate limit, and capacity, and then compute the result itself. Client payloads may never select a cell coordinate, material, temperature, wetness, ignition/spread result, persistence delta, or save identity. This preserves the existing weather, exposure, campfire, inventory, and construction authority seams.

### Weather-cycle contract

`GameMode` advances a server-owned weather cycle and writes the current immutable-in-session state to the replicated world `GameState`. A state contains a monotonically increasing `WeatherCycleIndex`, server start time, duration, precipitation and fog intensity, a server-derived storm intensity and activity tier, wind direction, and wind strength. `WeatherCycleIndex` is derived from no client input; the server selects it at session start as zero and increments it only after the active duration elapses. A late-joining client consumes the replicated active state rather than inferring it from local time.

Each state is deterministically derived from `WorldSeed` and `WeatherCycleIndex` using a dedicated weather sub-seed. It selects a duration from 120-240 server seconds, precipitation and fog intensity in [`0,1`], wind direction as a quantized yaw in [`0,360`), and wind strength in [`0,1`]. Rain, fog, and wind remain continuous rather than making a biome a hard weather zone. Restarting a local/listen-server session restarts the deterministic sequence at index zero; persisting mid-cycle weather is intentionally deferred until persistent world-time is introduced.

Only the server advances the index, computes values, or changes the replicated state. It derives storm intensity as normalized precipitation multiplied by wind strength, and publishes a matching Calm, Active, or Highly Active tier from fog, rain, wind, and storm thresholds. The server refreshes this enum before replication; clients can read it but cannot request a weather outcome, intensity, tier, duration, direction, strength, or clock adjustment.

### M7 bounded hazard descriptors

The replicated weather state now includes deterministic fog intensity, the normalized `StormIntensity` property, and the server-derived `ActivityLevel` enum. StormIntensity is derived from the authoritative rain/wind pair rather than accepted as a separate value. The server's per-pawn exposure snapshot also reports normalized heat and cold intensities from the current generated ambient temperature; values refresh with the existing one-second environmental update and reverse when the pawn or weather conditions change. Non-finite climate samples fail closed to zero intensity, and every reported intensity is bounded to [0,1].

The per-pawn heat/cold and normalized fog/storm descriptors remain summaries; raw intensity alone does not directly apply a penalty. The server-selected weather activity tier drives the bounded M7 exposed-rain rule below. Existing server rain/wetness, ambient-temperature/warmth, wind/shelter, and hearth rules derive the environmental state. The owner receives the pawn's existing replicated exposure state, and all peers receive the existing replicated world weather state. There is no new RPC, client mutation, save field, or persistence schema. Shelter and fire recovery and calm weather timing counter wetness and warmth; low warmth also reduces stamina recovery as described below.

The first M7 hazard outcome uses that same server-owned warmth result: the movement component recovers stamina at the normal 15 points per second at 40 warmth or above, scaling smoothly down to 12 points per second at zero warmth. Cold, wetness, wind and weather lower warmth through the existing exposure sampler; accepted shelter and lit-fire warmth restore it and return stamina recovery to normal. This is applied only by the authoritative stamina update, uses the already replicated exposure and stamina state, and adds no client input, RPC, status entry, or save field.

The second M7 hazard outcome shortens the existing unroofed-rain Wet trigger by 25% only when the authoritative weather tier is Highly Active: 7.5 uninterrupted seconds instead of the ordinary 10. Calm and Active retain the prior trigger. The server resets accumulated time when rain stops, the pawn enters water, or accepted roof shelter is found; standing water still applies Wet immediately. Existing lit-fire recovery and the 120-second maximum remain unchanged. The tier and timer are read or advanced only by the server; no client input, RPC, replicated field, or save schema is added. The third M7 hazard outcome uses storm intensity above the shared 0.65 Highly Active threshold to increase exposed campfire fuel wetting by up to 25%, ramping linearly to full rain/wind intensity. The campfire receives rain only after the server accepted-roof trace and wind only after its windbreak trace, so a roof removes rain pressure and a windbreak removes the storm surge. This updates the existing replicated fuel condition through the normal server tick without a new request, RPC, replicated field, or save schema.

## Content conventions

- `/Game/Kalmala/Core`, `/Characters`, `/World`, `/Items`, `/Abilities`, `/UI`, `/Audio`, `/Maps`, `/Developer`.
- Prefixes: `BP_`, `WBP_`, `DA_`, `DT_`, `GA_`, `GE_`, `GCN_`, `IA_`, `IMC_`, `M_`, `MI_`, `T_`, `SK_`, `SM_`, `SFX_`.
- Use Gameplay Tags for semantic states (for example `State.Wet`, `State.Sheltered`, `Damage.Fire`, `Ability.Support.Mending`, `Effect.Shielded`).

## Persistence and online progression

Vertical slice persistence is a versioned `SaveGame` schema for local/listen-server testing. Isolate persistence behind interfaces so a later backend can replace it. Do not connect production identity, payments, analytics, or cloud databases until there is an explicit product decision.

Generated population saves store only sparse server deltas. `UKalmalaWorldPopulationSaveGame` records its schema version and immutable world identity, then separately records harvested and defeated stable spawn IDs plus optional credited Rat reward rows; it never serializes the generated base population. During a session, `GameMode` owns this container, records harvests only after the server accepts them, and consults it before recreating a generated harvest node. A Rat defeat saves its stable spawn ID and owner identity before replication; the server retries its Bone Fragment and Rat Tail as one grant while the owner's pack is full. Future wildlife and hazards must use the defeated set only after server-owned defeat validation and must consult it before activation.

The developer-only reconnect harness verifies both harvest and wildlife paths across two listen-server processes with the same world identity: it writes one accepted server delta, reloads the slot, and succeeds only when the corresponding deterministic spawn is not recreated.

## Biome-expansion shared contract

Each land-biome slice consumes `FKalmalaBiomeExpansionContract` rather than creating a parallel seed, placement, exposure, or persistence scheme. A profile supplies bounded terrain-feature intent, per-kind population multipliers, and normalized wetness, wind, and natural-cover modifiers. The current exposure simulation continues unchanged until a completed biome slice explicitly adopts its profile. Discovery candidates are deterministic, terrain-aligned server inputs with stable IDs derived from world identity, biome, and invisible spatial key; candidates are neither spawned, replicated, revealed, nor saved by this contract alone.

The Shimmering Lakes slice consumes that contract without adding water traversal physics or a boat requirement. The biome label and field-based profile remain active, but the compiled default-off analytic-basin policy builds no inland lake/shore mesh, no water-adjacent Rillstone discovery, and no basin-derived shoreline wetness. The Rillstone content definition remains dormant until genuine basin water is enabled by a reviewed code change. Existing ocean and sea-level river clipping, mapping and swimming remain separate. Clients only receive the resulting server-derived exposure state.

The Elderwood slice consumes the same contract without a route, reserved camp, or authored clearing. Local terrain patches derive larger field-driven trees, canopy, and non-colliding root buttresses from the replicated identity; continuous Flora controls density so lower-flora pockets remain natural clearings. On the server, only Elderwood-classified active spatial keys apply the bounded population profile and may materialize one terrain-aligned, gently sloped lower-flora harvest discovery. A server pawn in Elderwood receives the profile's increased natural cover and reduced wind exposure before the unchanged shelter/fire response. The discovery follows the normal validated interaction and sparse depletion path; clients receive only the ordinary replicated node when relevant.

The Mossy Mire slice consumes the same contract without a crossing, route, or authored dry ground. Only Mire-classified active spatial keys apply the bounded population profile and may materialize one terrain-aligned, gently sloped, relatively dry hummock harvest discovery. A server pawn in the Mire receives its wetter profile through the unchanged shelter/fire response, then a bounded 12% footing drag through the existing replicated travel-speed state; minimum travel speed remains the existing 68%, so the ground is slower but never impassable. Dry hummocks are optional terrain-selected preparation sites for raised shelter and drainage choices, not reserved camps. The discovery follows the normal validated interaction and sparse depletion path; clients receive only the ordinary replicated node and replicated exposure state when relevant.

The Freezing Tundra slice consumes the same contract without an authored ridge, camp, route, travel gate, or client-selected location. Only Tundra-classified active spatial keys apply the bounded sparse population profile and may materialize one terrain-aligned, gently rolling exposed-high-ground harvest discovery. A server pawn in the Tundra receives reduced natural cover and stronger wind exposure before the unchanged shelter/fire response, making a player-built enclosed roof and windbreak meaningful preparation rather than a mandatory site. The discovery follows the normal validated interaction and sparse depletion path; clients receive only the ordinary replicated node and replicated exposure state when relevant.

The Thunder Mountains slice consumes the same contract without a designed passage, precision gate, authored ridge, or client-selected location. Only Mountain-classified active spatial keys apply the bounded mountain population profile and may materialize one terrain-aligned, steep-but-traversable storm-carved overlook harvest discovery. A server pawn in the Mountains receives stronger wind exposure during the existing server weather cycle before the unchanged roof/windbreak shelter response, so a player-built lightning-safe enclosure is useful preparation without reserving a site. The discovery follows the normal validated interaction and sparse depletion path; clients receive only the ordinary replicated node and replicated exposure state when relevant.

`-KalmalaBiomeFeatureInspection` is a server-only developer switch. After a player joins, it logs the sampled biome, nearby classifier seam flag, profile values, and the non-materialized stable discovery candidate. It accepts no client location, creates no actor, and never directs a player toward a feature.

The sparse container must round-trip through `SaveGame` memory serialization before any slot-writing integration is added. The automated round-trip test verifies that immutable world identity and harvested IDs survive serialization without creating project `Saved/` output.

Magic-scroll discoveries and learned support effects are server-authoritative progression. Save stable scroll IDs and learned-effect IDs, validate scroll rewards once, and replicate only the effect state needed by other players (such as an active shield or stat boost), not private inventory detail.

M13 represents each learned effect as a reusable item in the owner's transient inventory; reconstruct those items from the existing learned-effect entitlement when that inventory initializes. The numbered hotbar sends only its cell index, and the server resolves the item/effect and invokes the existing support activation gates. Each accepted cast starts a shared 300-second per-caster cooldown. Hearth Shield, Bear's Vigor, and Deer Call last 180 seconds; Mending remains instantaneous. Deer Call's bounded server-selected wildlife influence lasts 180 seconds. Only one support effect may be active per caster; the server rejects another cast while one remains active and does not stack or refresh effects. Scroll activation does not consume the item or add save fields; server-owned stamina, learned entitlement, and effect-specific validation remain in force.

## Prototype player presentation and movement

`AKalmalaCharacter` constructs a collision-free, nine-part original humanoid through `UKalmalaPlayerModelComponent`. Each rendering peer generates the same rigid geometry using existing project materials; velocity drives a simple limb swing and airborne pose. Dedicated servers skip the model. The character capsule remains the collision authority.

Space uses Character Movement's built-in single jump (500 cm/s vertical launch, 0.25 air control). Held Shift requests sprint through `UKalmalaCharacterMovementComponent`: saved moves preserve the intent in `FLAG_Custom_0`, restore it during prediction replay, and prevent combining moves across sprint transitions. The server decodes intent and applies its configured 1.5 multiplier to the existing exposure-adjusted walking speed, only while grounded and not crouching. Clients send no numeric speed or new RPC. Ignored local movement clears held sprint/jump. This adds no stamina or save-data contract.

## Verification minimum

Regional sampling shares warp and motion calculations, and rejects distant supports early. The minimap reuses collision vertices within each raster call; scratch data is discarded afterward. Classification and hydrology always use the current implementation.

Minimap raster sampling runs on the thread pool from value snapshots of the replicated identity, position, radius, and resolution. Each view model retains at most one pending job, polls without waiting, and coalesces movement into the next request. Completed results from an obsolete identity or zoom are discarded. Workers access no actors or UObjects; texture publication stays on the game thread. The last completed map remains visible during movement, so terrain presentation can briefly lag the pawn while generation finishes. Reinitialization drops the future without waiting or giving the worker an object reference. The existing hydrology cache remains protected by its mutex.

Every feature needs an automated test where practical, plus a reproducible multiplayer test: host + one client or dedicated server + two clients. Profile before increasing simulation area, actor count, or replication frequency.

`-KalmalaExposureReplicationTest` is the Phase 4 host/client smoke path. It is server-configured only: the server records its sampled weather, shelter, fire contribution, and exposure result; the client logs only replicated weather, campfire, and exposure state. It starts each verification pawn wet and low on warmth beside a temporary server-owned fire so the logs show recovery without changing persistent world data.

`-KalmalaCampChoiceTest` adds a non-shipping two-player scenario through the existing GameMode exposure tick. Its server-only fixtures compare two separated naturally generated camp conditions, initialize wet/cold pawns, and light temporary fires through the normal authority/range gate after an unprepared interval. No client supplies a location, environmental input, lighting result, or exposure value. Log comparison uses replicated player IDs because actor instance names can differ between peers. The harness adds no gameplay path, camp recommendation, construction piece, or save contract.

## Gathered hearth and crafting contract

`UKalmalaCraftingComponent` accepts owner intent only; configured recipes and atomic inventory exchanges determine all costs and outputs on the server. A new character carries a level-one Construction Hammer record in the owner-only carried-tool array, and the local hammer menu requires that record. The server independently checks the same authoritative tool before any construction request. Hearth placement uses the `CampFire` catalogue output and consumes its JSON-backed raw recipe cost of 5 Stone and 3 Wood plus one Wood, Lightwood, Densewood, or Coal to start the hearth with 60 seconds; normal crafting cannot create the internal CampfireKit item. The canonical `CampFire` catalogue output maps to CampfireKit. Floors, walls, roofs, Workbench, Forge, Workbench Tool Rack, and Forge Anvil are direct hammer builds: their raw-material costs come from `recipes/building.json`, and no catalogue item is granted or consumed. Stable internal construction identities remain for placement and save compatibility. Other buildable items retain their existing validated recipe and placement behavior. Construction RPCs accept only a buildable identity or the existing payload-free hearth placement intent; position, yaw, materials, terrain, collision, persistence, and outcome remain server-selected or server-validated. Each peer derives original collision/presentation from the replicated construction identity. The server adds the shelter sampler's roof or windbreak tag only to accepted matching constructions, never from a client request. See `02-technical-architecture.md` for recipe costs, access rules, controls, replication and current persistence limits.

M9 supersedes the M7 tool-output replacement recipes and material-paid repair. The owner-local Inventory action submits only a carried tool ID; the server selects the owner's current record, validates a visible same-world Workbench or Forge within 250 cm, and restores that damaged or broken known tool to its authored maximum at no cost. Rejected requests preserve condition, and accepted repair awards no Crafting experience. The Workbench and Forge station menus expose a payload-free Repair All action instead of a tool-selection page. The server validates a visible same-world Workbench or Forge within 250 cm, builds a complete candidate from the bounded owner-only carried-tool array, then persists and publishes the accepted repairs together; clients send no list, tool ID, or condition. The paid buildable Grinding Stone keeps its in-world Repair All action and existing construction/save record, with the same atomic carried-list validation. The owner Inventory panel shows private tool condition and its selected-tool repair action; station Repair All does not open a menu. Current-format tool and station progression is persisted directly in player-discovery schema 2; construction records use schema 6. No compatibility migration runs during development. The parameterless sharpening multicast remains presentation-only and carries no repair state.

The normal Chest is the sole storage construction. Current schema-4 food processing uses the Cooking Rack, Cauldron, and Frying Pan; it has no Smoke Frame or smoke recipe. Recipes consume direct Wood and Stone, and other listed material costs; intermediate Fuel and ConstructionSupply items no longer exist. Cooking recipes consume their listed ingredients only. Wood, Lightwood, Densewood, and Coal each add 60 seconds of hearth fuel, and a lit hearth burns one fuel second per elapsed server second. The server derives cooking heat requirements from the resolved station and a usable lit hearth near both the player and station. Current storage schema 2 requires current item IDs and per-item GUIDs in saved stacks; it does not convert retired IDs or backfill legacy GUIDs. Construction save schema 6 stores Kiln and Smelter processing state plus Garden Plot crop mode and remaining active-game seconds.

M9 persistence uses the existing construction and player-discovery save owners and slots. Construction saves use schema 6; player-discovery saves use schema 3. World construction uses exact seed/revision-7 identity and bounded records for paid station attachments, Charcoal Kiln and Smelter queues/jobs, and Garden Plot crop mode/active time; station level remains derived from accepted nearby attachments. Player discovery uses exact seed/revision/player identity and bounded carried-item and discovery records, with carried identities stored directly as `ItemId` and Basic Torch fuel stored as bounded remaining seconds. The separate M7 sparse ledger remains schema 1. Normal writes validate complete candidates before replacing a slot. Player writes merge only server-revalidated current M9 claims and the owner's current carried-item state; a rejected candidate leaves the existing save and live gameplay unchanged. There are no user save files during development, so save formats may change directly; do not implement save compatibility migrations until the game is released.

## M3 wetness and rain override (2026-09-14)

This section supersedes earlier references to continuous player wetness, warmth-derived wetness penalties, fuel wetness extinguishing, or rain damage below. `Wet` is only a server-owned player debuff with a reusable parameter definition: default maximum duration 120 seconds, unroofed-rain trigger 10 uninterrupted seconds in Calm or Active weather and 7.5 seconds in Highly Active weather, movement multiplier 0.92, and stamina-cost addition +0.15 (a 15% increase when it is the only modifier). Standing in server-confirmed water applies it immediately. Rain applies it only when the server finds no accepted roof above the pawn. Reapplication cannot exceed 120 seconds; a nearby lit campfire with nonzero authoritative heat removes it. Surface moisture remains a grid material/fire input and is never a second player wetness system.

The pawn's replicated status container is the sole player-facing Wet state. Each entry has a stable status identifier and finite remaining duration; the first entry is `State.Wet`. The server alone creates, refreshes, decrements, and removes entries, and it publishes the remaining duration for display. This replaces the old continuous wetness/warmth values and warmth-derived movement penalty rather than running a second player moisture simulation. The status definition supplies the fixed movement multiplier and additive stamina-cost adjustment, so clients render/use only replicated presence and remaining duration and never send a status ID, duration, source, multiplier, expiry, or removal request. Later debuffs add a definition and entry through this same container rather than a pawn field.

### Construction and hearth rain-response contract

Every accepted `FloorKit`, `WallKit`, `WorkbenchKit`, `StorageKit`, and `ReinforcedChestKit` starts with replicated server-owned `Health = 100.0`; `RoofKit` has the same presentation field but is rain-immune. At each one-second server environmental update, an exposed eligible construction loses `0.10 * clamp(PrecipitationIntensity, 0, 1)` health. Health is clamped to `[50.0, 100.0]`, so rain alone takes at least 500 seconds of full precipitation to reach its floor and can never destroy, remove, refund, or change collision for a construction. The rate, maximum, and floor are configuration-owned defaults, not client settings. Repair and any persistence decision are deferred: M3 does not yet save rain wear, and a restart returns the existing accepted construction record without a new health delta.

An accepted roof protects an actor only when the server's `ECC_Visibility` vertical trace from the actor's collision-centre plus 60 cm first hits an actor tagged `KalmalaShelterRoof` within 400 cm. The query ignores the protected actor itself, never accepts an untagged mesh, and is rerun by the server each environmental update; a client, construction transform, camera, or local overlap cannot assert roof protection. This deliberately shares the campfire protection distance and tag rather than introducing a second shelter classifier.

The replicated server-owned hearth state is exactly one of `Extinguished`, `Lit`, or `Smouldering`. `Extinguished` has no positive fuel (or was never successfully lit), zero heat/light, and requires the existing server-validated no-payload lighting intent. `Lit` has positive fuel, was successfully lit, and is either roof-protected or under precipitation below `0.05`; it provides normalized heat `1.0` in the existing 600 cm falloff. `Smouldering` has positive fuel, was previously lit, and is exposed to precipitation at or above `0.05`; it consumes fuel at the normal one fuel-second per server second but has zero heat/light. A smouldering hearth automatically returns to `Lit` as soon as its server roof trace succeeds (or precipitation again falls below the threshold); it never self-lights from `Extinguished`. Fuel exhaustion always transitions to `Extinguished`. The state, fuel seconds, effective heat, and roof-protected bit replicate for presentation, while weather input, traces, transition selection, fuel burn, and the automatic reignition remain server-owned.

The initial M3 defaults are: Wet maximum `120 s`; normal unroofed-rain trigger `10 s` (M7 Highly Active trigger `7.5 s`); Wet movement multiplier `0.92`; Wet stamina-use multiplier `1.15`; roof trace origin offset `60 cm`; roof trace distance `400 cm`; construction maximum/floor `100.0`/`50.0`; full-rain wear `0.10 health/s`; rain threshold `0.05`; hearth heat `1.0`; hearth heat radius `600 cm`; fuel burn `1 fuel-second/s`; raw-fuel item `60 s`; fuel cap `300 s`. Clients submit no wetness, duration, health, roof, weather, fire state, fuel rate, or relight value.

### M7 weather descriptors and local activity badge

`AKalmalaWorldGenerationGameState` replicates deterministic fog, normalized
rain/wind storm intensity, and the server-derived Calm/Active/Highly Active
weather tier. The server also publishes normalized per-pawn heat/cold summaries
from its sampled environmental temperature. `UKalmalaWeatherActivitySubsystem`
reads that existing tier for each local player and shows a compact badge below
the minimap: a circle plus `CALM`, a diamond plus `ACTIVE`, or a triangle plus
`HIGHLY ACTIVE`. The text and distinct shapes carry the state without colour;
font scale and contrast follow local settings. The badge creates no request,
replicated field, gameplay mutation, or persistence entry. Weather selection,
intensity, exposure, mitigation, and all resulting gameplay remain server-owned.

### Wet modifier runtime increment

The shared status component resolves finite active entries through compiled definitions, deduplicating IDs and ignoring unknown definitions. Character Movement applies the movement multiplier after sprint selection or the ocean speed cap on server and prediction paths. Legacy exposure no longer scales MaxWalkSpeed; its continuous values remain transitional telemetry pending removal. Defaults are compiled constants shared by all peers, never client settings. CalculateStaminaCost adds Wet cost adjustments to other active deltas and applies the bounded combined factor once to a finite nonnegative authoritative base cost. The M5 balance pass intentionally keeps Wet duration, triggers, and campfire recovery unchanged while reducing the movement/stamina tax to preserve optional travel and recovery choices. No save schema or RPC changes.

### M3 sprint stamina runtime

The movement component now owns a transient replicated stamina pool (maximum 100). On authoritative Character Movement updates only, grounded, uncrouched, moving sprint intent consumes 10 stamina/second through CalculateStaminaCost; Wet therefore consumes 11.5/second when no other cost adjustment is active. Each finite positive movement step is capped at 0.25 seconds. Otherwise stamina recovers at 15/second, capped at 100. At zero, sprint speed is disabled until recovery reaches 20; ordinary walking and swimming remain available. Swimming, crouching, standing still, and airborne movement do not consume sprint stamina. These are compiled prototype tuning defaults, not client-supplied values.

Stamina and the exhaustion gate replicate with the movement component. Prediction reads the replicated gate; it does not mutate stamina or add a stamina RPC. The server validates normal movement intent and owns all consumption/recovery. Brief corrections around exhaustion are possible under latency because stamina is not predicted or included in saved-move replay. No persistence/schema change: a new pawn starts full. Wet continues to use only the shared status definition, with no new per-status pawn field.

### Campfire removal of Wet

The server environmental update attempts campfire removal after the water/rain refresh. A same-world authoritative lit campfire must produce finite positive warmth at the pawn's authoritative location (the existing 600 cm falloff); the radius boundary contributes zero and does not qualify. Heat therefore wins within that update, including during continued water/rain exposure; when heat ceases, ordinary triggers can apply Wet again. Roof shelter alone does not remove an existing status. Only the Wet entry is removed; movement and stamina costs immediately resolve their defaults through the shared definitions. The existing status array publishes removal without a new RPC or save field. Client-local removal calls fail before mutation.

This increment uses the existing IsLit/effective-warmth interface. The later three-state hearth implementation must keep Smouldering and Extinguished at zero heat; it is not implemented by this change. Legacy continuous exposure remains transitional telemetry, not a second source of Wet modifiers.

### Construction rain-wear runtime

Construction actors now replicate Health, initialized to 100 on a new actor. The one-second GameMode environmental update supplies trusted elapsed time and precipitation. Only initialized FloorKit, WallKit, WorkbenchKit, StorageKit and ReinforcedChestKit actors participate; RoofKit and unknown kits are immune. Finite positive rain is clamped to one and causes 0.10 health/second at full intensity, bounded at 50. Zero rain and malformed inputs do nothing. The actor samples its own collision-centre +60 cm upward by 400 cm, ignoring itself; only a first blocking hit tagged KalmalaShelterRoof protects it. Protection stops further wear without healing. Clients receive health but local mutation calls are rejected before sampling or writing. Health changes do not rebuild mesh/collision, remove actors or modify construction/storage saves. Restoration into a new actor starts at 100; rain wear remains transient.

### Three-state hearth runtime

The replicated HearthState enum now replaces the boolean: Extinguished (0), Lit (1), Smouldering (2). IsLit is a compatibility query for Lit only. Both active states burn one fuel-second per server second; exhaustion becomes Extinguished. Server weather advancement samples roof collision internally, so its trusted rain input cannot assert protection. Exposed rain >=0.05 selects Smouldering; dry or roof-protected conditions select Lit for an already active fire. Heat is exactly 1 for Lit and 0 otherwise, retaining the existing radial falloff. Wind and legacy fuel-wetness telemetry no longer reduce active heat or prevent automatic reignition. The existing validated initial lighting gate remains required for Extinguished, and fuel alone cannot self-light. Text presentation explicitly names SMOULDERING. There is no new client mutation path or saved-data schema.

## M3 rain-response authority audit

Construction rain wear runs from server GameMode weather; its mutation seam rejects clients before tracing roof collision. Hearth Tick samples weather/wind only on authority, and the shared rain transition independently rejects clients before its roof trace. Replication callbacks derive local collision, tags, meshes and light from accepted state. Local tags cannot influence the server's separate trace world. Wet modifiers may drive client movement prediction, but status mutation remains server-owned.

`-KalmalaPersistedCampTest` also performs a live remote-client isolation probe against replicated Wet, floor/roof, hearth, and weather objects. It directly calls the server-only mutation seams on client copies with extreme forged elapsed/rain values; every call must remain local no-op, retain the replicated values, and have no authoritative GameMode/save owner. The probe is development-only and sends no RPC or save mutation; the server-created camp, its normal paid placement, and owner-only storage path remain unchanged.

Construction save/load is authority-guarded in GameMode with world-identity and record validation. Storage also validates the accepted construction ID/kit/transform and saves a candidate before replacing live contents. Construction schema 1 contains only ID, kit and transform. Rain health resets to 100 on restore; hearth fuel/state and Wet remain transient. This audit adds no persistence or schema change.

Hearth.AuthorityContract guards the absence of server RPCs on state owners, actual lifetime replication of health/hearth/protection fields, and the unchanged construction record shape. Behavioral authority, roof collision and persistence checks remain in RainWear, RainState, Wet, Crafting.NetworkContract and Storage. The combined live M3 scenario remains a later acceptance gate.

### Local Wet feedback

`UKalmalaSurvivalStatusWidget` reads only its owning pawn's replicated status component. While Wet is active, it shows the rounded-up remaining seconds, movement reduction and stamina-cost increase from the shared Wet definition, plus a lit-campfire recovery hint. Removal or expiry replaces this with explicit `Wet: inactive` text. The lower-left status strip retains its wrapping/scroll behavior. It never counts down locally, changes a status, reads legacy exposure wetness, or submits a network request. Construction/fire feedback and rendered layout verification remain queued.

### Local hearth recovery feedback

The crafting panel places nearby hearth status before recipes so state and recovery instructions are visible on opening. Text derived from the existing replicated hearth fields states whether heat is on, that nearby heat removes Wet, that Smouldering has no heat but still burns fuel, and that a roof or dry weather restores it. Extinguished hints distinguish empty fuel, wet fuel and readiness to light. No state, trace, RPC or persistence contract changes. Construction rain-wear feedback remains queued.

### Local construction rain-wear feedback

The crafting panel shows the nearest initialized, visible construction within 250 cm of the owning pawn (stable construction ID breaks distance ties). A local visibility trace selects presentation only; it never determines server shelter or permission. The panel reads the existing replicated kit and health, uses the catalogue name, and states health to one decimal plus no wear, rain-worn, rain-wear floor reached, or roof immunity. A generic roof-prevention hint does not claim current roof protection; no roof-state field is replicated. Missing or obscured construction clears to an explicit none-visible message. No RPC, authority or saved-data contract changes.

### Weather mutation runtime guard

SetWeatherStateFromServer now rejects non-authority and invalid input at runtime instead of relying on assertions that can compile out. Weather validity also requires a finite, nonnegative server start time. Rejected calls preserve the complete previous weather interval and do not force replication. Valid server selection remains unchanged; there is still no weather mutation RPC or weather save field. This prevents client-local writes through this C++ seam, without claiming a remotely exploitable server endpoint previously existed.

## M7 first food transaction

The first prepared food reuses the server-local item and recipe catalogues, UKalmalaInventoryComponent::BuildExchange, the existing hearth actor, and the replicated player-status component. Roasting requires a same-world fire that the server observes as usable, within 250 cm, Lit, and producing finite positive heat; a workbench, extinguished fire, or Smouldering fire cannot satisfy it. The server checks recipe identity, batch, inputs, and output capacity before changing the private pack. No additional fuel charge is applied beyond the hearth's normal fuel burn.

Food use is an owning-client request containing only an item identity. The server allowlists the seven current cooking outputs (`CookedBoarMeat`, `CookedDeerMeat`, `HearthBroth`, `MeatStew`, `RootVegetableSoup`, `RoastedRootVegetables`, and `DeerRootRoast`) plus foraged `Lingonberry` and `ForestChanterelle`, checks the owner's pack and meal slot, commits one-item consumption through a candidate inventory exchange, then publishes a fixed-duration State.Food.SteadyMeal entry. The entry adds a -0.10 stamina-cost delta for 120 server seconds; it sums with Wet and accessory deltas in the shared cost calculation. Only one meal entry is permitted: an active effect neither stacks, refreshes, nor gets replaced; rejected consumption leaves pack and status unchanged. Berry Recovery Draught uses the same identity-only request and atomic pack exchange, but checks for missing health and the shared draught cooldown before consuming. The server then publishes a ten-second Berry Recovery entry and a sixty-second shared draught cooldown; it restores two health per server second, clamps to 100, and removes the healing entry if the character is dead. The local status strip reads the replicated recovery and cooldown timers without advancing them. The existing status replication carries effect identity and remaining time, while the private inventory and crafting result remain owner-only. Food, cooking progress, meal effects, draught healing, and cooldowns are transient and add no save field or schema.

Rat-tail Tonic uses the same identity-only request and atomic pack exchange. The server rejects use at full health, while Berry Recovery or Tonic healing is active, while dead, or during the shared draught cooldown; rejected use preserves the dose. On acceptance, it publishes a ten-second owner-only recovery entry and the same sixty-second cooldown, restoring three health per server second up to 30 total and clamping to 100. Healing ends early at full health or on death; neither case refunds the cooldown or dose. The recovery entry and cooldown are transient and add no save field or schema.

Root Stamina Draught uses that same identity-only request and atomic inventory exchange. The server rejects use while dead, at full current stamina, or during the shared cooldown; otherwise it consumes one dose, restores up to 20 stamina immediately through the authoritative movement component, and clamps to the current maximum (including an equipped maximum-stamina bonus). It publishes only the shared cooldown entry; no healing timer, RPC, or save field is added. A rejected request preserves both the dose and stamina.

The active food recipes cook boar or deer meat on the Cooking Rack, prepare Hearth Broth, meat stew, root vegetable soup, Berry Recovery Draught, Root Stamina Draught, and Rat-tail Tonic in the Cauldron, and prepare roasted roots or deer-root roast in the Frying Pan. Berry Recovery Draught costs three Lingonberries and one Carrot; Root Stamina Draught costs two Potato and one Onion; Rat-tail Tonic costs two Rat Tails and two Lingonberries. Each draught yields one dose and awards ten Cooking experience. The server resolves the declared station, live hearth heat, inventory costs, and accepted outputs from the catalogue. Food and recovery effects remain transient; no saved-data schema changes.

### Accepted cooking experience

The server recipe catalogue may assign an allowlisted skill and a bounded award of 1-25 experience to a recipe. The first food recipes award 10 transient Cooking experience once after a successful atomic inventory exchange, regardless of batch size. Missing stations or heat, invalid batches, insufficient inputs, output-capacity failures, and client-side calls award nothing. Recipe definitions cannot pair an unknown skill, an out-of-range award, or experience without a skill. The existing level and unlock contract derives progression on the server; no client-provided experience, new RPC, persistence field, or save schema is introduced.

The current recipe schema has no skill-level requirement field. Recipe access is gated by material quantities and any required station, hearth state, and bounded batch. Accepted food preparation still grants its existing server-owned Cooking experience after the inventory exchange; it does not block a recipe behind a skill level. Rejection leaves the private pack, station state, and progression unchanged, with no learned-recipe save field or client-authored value.
### M7 local survival status strip

`UKalmalaSurvivalStatusSubsystem` belongs to each `ULocalPlayer` and builds the persistent lower-left strip from that player's existing replicated status entries, exposure summary, support-effect fields, and `AKalmalaWorldGenerationGameState` weather. The widget reports Wet's movement/stamina modifiers, the shared non-stacking prepared-meal effect, Berry Recovery or Rat-tail Tonic healing and the shared Berry/Root/Tonic/Antidote draught cooldown, active rain/fog/wind intensities and weather interval, low-warmth cold pressure and recovery rate, heat signals, and active support magnitude/expiry. Poison effects and the Antidote's timed resistance remain owner-visible through the status hotbar. Shape markers pair with explicit category and status names; text scale and high contrast follow local settings. The separate Calm/Active/Highly Active badge remains below the minimap.

The status component publishes remaining seconds for Wet, food, Berry Recovery and Rat-tail Tonic healing, the shared draught cooldown, poison doses, and Antidote resistance; presentation reads those values without advancing them locally. Antidote is server-applied: it removes weak poison, converts active major poison to weak for at most five seconds, blocks new weak poison, and reduces new major poison to that weak profile during its 600-second resistance. Death removes resistance while the consumed dose and shared cooldown remain. Support and weather timers use their replicated absolute server expiry/start/duration with `GetServerWorldTimeSeconds`. Wet source is labelled as exposed rain or water because the shared status entry does not retain which trigger applied it; food is labelled as prepared food because the shared meal entry does not retain the consumed recipe identity. Recovery guidance describes existing roof/fire and shelter/fire rules. The subsystem has no input binding, RPC, gameplay setter, persistence path, new replicated field, or save-schema change.


## Consolidated system contracts

The sections below consolidate the durable parts of former docs 08–47. They take precedence over older incremental descriptions above where explicitly superseded. Catalogue data and compiled definitions remain the source for exact current costs and tuning. Dated requests describe intended behavior; BACKLOG.md and PROGRESS.md determine implementation, integration, and verification status. M13 and integrated M14 acceptance are still open at this cleanup.

## World generation

## Direction

Kalmala is a seed-generated, player-directed open wilderness. The world contains no authored gameplay areas, fixed camp zones, prescribed routes, or required quest sequence. Players choose where to travel, build, gather, and take risks.

The world is built from continuous procedural maps, not a visible square grid. Streaming limits and server spatial partitions exist only to manage performance, spawning, and persistence; they must never shape biome boundaries or create gameplay zones.

Kalmala may learn from broad survival-world principles—shared seeds, player-made homes, environmental risk, and landmark-led discovery—but must remain wholly original in its world layout, content, names, visual language, and implementation.

## Goals

1. **A world worth wandering.** Terrain, weather, and natural features should invite curiosity without giving players a required route.
2. **Shelter changes travel.** Different environments alter how players prepare, build, and move through the world.
3. **A shared world.** The same seed produce the same world for every player in a session.
4. **Optional discovery.** Resources, wildlife, hazards, and points of interest enrich exploration without becoming a checklist.
5. **Multiplayer-safe scale.** The server owns gameplay state while clients receive only the information needed to render and play nearby world content.

## World seed and generation

The server owns one immutable `WorldSeed` (unsigned 64-bit). Every session runs the current generator. Generation compatibility versions and their command-line selectors are removed. Changing production tuning can change existing seed layouts during development; old version-specific save slots are not loaded or deleted. Current sparse saves and local maps use the seed as their world identity.

### Master map and environmental fields

First generate a continuous Perlin **land/water master map with its own seed**, independent of the game seed. The 128 km square atlas uses the constant `FKalmalaMasterMap::MasterSeed = 0x4b616c6d616c6137`, a 3 km main wavelength and a second 1.29 km octave (75%/25% amplitudes). Positive noise is land; zero or negative noise is ocean. It contains no biome identities, authored islands, or gameplay placements.

The game seed select an atlas crop centre and an arbitrary rotation. The crop retains the current **16 km radius / 32 km diameter** playable circle. The centre is selected from up to 128 seeded candidates: use the first with land signal above 0.10, or the strongest candidate if none qualifies. This favours inland terrain for the start without painting land into the atlas. The entire rotated circle fits inside the master map. World zero is the crop centre; rotation preserves distance and scale. No baked map image is required.

The existing four independently seeded game fields remain:

| Map | Controls |
| --- | --- |
| **Elevation** | Inland relief from the existing Perlin noise, constrained to the master's land/water sign and tapered continuously to the coast. |
| **Humidity** | Surface and ground moisture, wetland suitability, vegetation support. |
| **Temperature** | Climate, snow, frost, warmth pressure, cold-biome suitability. |
| **Flora** | Local vegetation density, undergrowth and clearings; it does not fragment broad biome identity. |

Changing the game seed changes the crop, rotation and environmental fields, but not the underlying master atlas. Master tuning and its seed are compiled developer settings, never client-controlled. All peers must run the same build.

### Land biome placement and origin distance

On land, the existing regional Perlin signals and environmental suitability select **Shimmering Lakes, Elderwood, Mossy Mire and Freezing Tundra**. **Meadows and Thunder Mountains are the default land biomes** wherever no special biome wins. Distance is measured from game-world XY `(0, 0)`, not the player start or master-map origin.

| Rule / biome | Distance from world zero |
| --- | --- |
| **Meadows or Ocean only** | **0–0.35 km inclusive** |
| **Meadows** | **At most 4 km** |
| **Shimmering Lakes** | **0.35–3 km**, with the inclusive starter rule taking precedence |
| **Elderwood** | **At least 0.75 km** |
| **Mossy Mire** | **3–16 km** |
| **Freezing Tundra** | **At least 4 km** |
| **Thunder Mountains** | Default elevated land outside the starter zone; default remaining land beyond 4 km |

Minimum-distance weights rise smoothly over 50 m on the eligible side, never leaking into excluded radii. The lowland Meadows fallback fades into Mountains over the last 50 m before 4 km, reaching zero at 4 km. Elevation also favours the mountain fallback nearer the centre, with no mountain influence at or inside 0.35 km. A winning special biome takes precedence over the fallback, including on high terrain. Remaining land beyond 4 km is Thunder Mountains even where source relief is low. These restrictions replace the earlier Gaussian distance preferences; they are not enemy levels or travel gates.

Shimmering Lakes and Mossy Mire share the same regional noise, humidity, elevation and temperature suitability. Their distance ranges differ: Lakes 0.35–3 km, Mire 3–16 km. Both ends ramp over 50 m inside the allowed range; their weight is zero at the exact endpoints. Their height candidates differ: Lakes continuously mixes the Meadows and Elderwood candidates, then adds humidity-weighted compression across a bounded source-height band. Mire now uses a provisional monotonic rational compression toward 300 cm with a 700 cm scale and clipped ±25 cm residual detail; preview bounds are 200–500 / 300–1200 / 10–60 cm and preserve positive height at the coast-side source minimum. BH15b adds fixed profile diagnostics and stale-binary-safe overrides; BH25e selects values from fresh-source profile and river/coast measurements. Mire candidate blending stays continuous, and priority river carving remains the later corridor override. The Lakes/Mire lowland terms are height-only and independent of basin support. Biome labels do not require standing water. The compiled shared policy disables qualified bowl/rim shaping, basin water-level assignment and positive basin weights in both wetlands without changing those labels, their field-based profiles or distance limits.

Shared suitability defaults: humidity rises from 0.48 to full strength at 0.65, elevation falls from full strength at 0.43 to zero at 0.58, and temperature rises from 0.18 to full strength at 0.32. `FKalmalaRegionalTuning` and the preview parameter file expose these same six settings. Rivers remain enabled; small streams are disabled. Inland water can occur in a land biome without making it Ocean. Islands come from the master crop. The player-start resolver searches dry Meadows within 320 m of zero.

Terrain, collision, sea water, inland water, minimap and expanded map consume the same generator. Regional weights, biome identity and terrain remain computed functions. Only a small crop transform is memoized per worker alongside the existing bounded hydrology spline cache; no authoritative biome raster is stored or replicated. Weather, wildlife, ruins, discoveries and harvest nodes are derived later.

```text
MasterSeed -> continuous land/water atlas
WorldSeed -> crop centre and rotation -> base land/water mask
WorldSeed -> Elevation, Humidity, Temperature, Flora
  -> master-constrained terrain + regional signals + distance eligibility
  -> special land biomes; Meadows/Mountains fallback; Ocean from master water
  -> seeded content, weather, survival and sparse player changes
```

## Finite world

The playable radius is always 16 km. There is one generation implementation and no alternate stream-debug world identity.

Terrain and water triangles retain the existing radial clipping. Exterior patches, population and discoveries are rejected; decoration keeps a 10 m edge margin and new hearth/construction placement keeps 3 m clearance. Character Movement enforces the radius minus capsule clearance on authority and owner prediction while preserving tangential/vertical movement. No edge wall, actor budget, asset or authority change is introduced.

## Biome palette

Development order is not player progression. The seed decides which biomes are nearby; players decide whether and when to enter them.

| Biome | Character | Shelter and travel pressure | Discovery focus | Development order |
| --- | --- | --- | --- | --- |
| **Meadows** | Gentle hills, moderate humidity, open sightlines. | Introduces fire cover, simple timber shelter, and dry storage. | Deer routes, stones, birch stands, and calm camp locations. | First |
| **Shimmering Lakes** | Interlocking lakes, shore fog, and saturated low ground. | Favors bridges, boats, dry stores, and raised shelter. | Small islands, shallow shores, and lake-edge resources. | Second |
| **Elderwood** | Dense canopy, deep shade, and heavy growth. | Rewards marked trails, compact camps, and careful visibility management. | Ancient roots, wildlife dens, and overgrown stone sites. | Third |
| **Mossy Mire** | Wet ground, slow travel, and dry hummock paths. | Rewards raised floors, drainage, and waterproof fuel storage. | Bog iron, causeways, and Mireling scavenging sites. | Fourth |
| **Freezing Tundra** | Bitter wind, sparse cover, and rolling high ground. | Rewards insulated clothing, enclosed roofs, and windbreaks. | Ice-fed springs, exposed shrines, and weather-read routes. | Fifth |
| **Thunder Mountains** | Sheer ridges, thunder squalls, and exposed passes. | Rewards lightning-safe shelter, durable construction, and route planning. | Storm-carved overlooks, mineral seams, and deep cave systems. | Sixth |
| **Ocean** | Open water, currents, waves, and storms. | Rewards seaworthy construction, anchors, and coastal shelters. | Distant islands, sea caves, and rare shoreline materials. | Seventh |

Biomes should differ primarily through environment, travel, and shelter—not a linear increase in enemy strength. Every reachable biome needs a viable lower-risk approach and a riskier shortcut or reward opportunity.

## Seeded world content

Terrain and biomes establish the world; separate seeded systems populate it. No area is assigned a mandatory purpose.

- Wildlife, harvest nodes, and hazards use deterministic server-side spatial seeds and spawn budgets.
- Weather is a server-owned deterministic cycle derived from the immutable world identity and a cycle index; it is not a biome map and never creates weather zones.
- Landmarks and major discoveries are optional points of interest generated by their own seed rules.
- Decorative vegetation, rocks, and ambient detail are cosmetic where possible; gameplay-relevant content is server-owned.
- Player-made changes override generated content through persisted save data.

## Multiplayer, persistence, and performance

- The server generates and persists all gameplay-affecting placements, AI, harvest state, hazards, construction, and survival state.
- Clients may generate cosmetic detail locally, but never decide gameplay placement, loot, damage, or rewards.
- Save only sparse player/world deltas keyed by `WorldSeed` and a server spatial key; never serialize the entire generated base world.
- Server spatial partitions are implementation-defined and invisible to players. They may manage activation and budgets, but may not create square biome borders or authored gameplay areas.
- Use instancing or pooling for non-interactable vegetation and rocks. Promote only nearby interactive content to replicated actors.
- Profile generation time, memory, replicated actor count, save size, and late-join synchronization before increasing content density or streaming distance.

## Harvest source catalogue

## Canonical source definitions

IDs are closed canonical design tokens. Source and presentation IDs use
lower kebab case, matching the M7 biome catalogue; inventory item IDs use the
existing PascalCase item-catalogue convention. Each ID is unique and selected
from the server's generated descriptor. The client may render the replicated
source and submit the existing interaction intent, but cannot choose an ID or
reward.

| Biome | Source ID | Presentation ID | Item ID / display name | Max stack | Local presentation |
| --- | --- | --- | --- | ---: | --- |
| Meadows | `meadows-birch-trunk` | `birch-trunk-harvest` | `Wood` / Wood | 50 | Mark the lower portion of an existing generated birch trunk with pale vertical bark and two dark harvest notches; do not add a duplicate tree. |
| Meadows | `meadows-aspen-trunk` | `aspen-trunk-harvest` | `Lightwood` / Lightwood | 50 | Use a pale gray-brown trunk and brighter green canopy on deterministic aspen candidates in existing Meadow forest patches. |
| Elderwood | `elderwood-ironheart-trunk` | `ironheart-trunk-harvest` | `Densewood` / Densewood | 50 | A broad dark heartwood trunk with sparse copper-brown grain and a readable lower-trunk cut face. |
| Mossy Mire | `mire-peat-amber-seam` | `peat-amber-seam` | `Amber` / Amber | 40 | A low irregular peat-bank face with one narrow amber seam, grounded to wet terrain. |
| Freezing Tundra | `tundra-frost-salt-deposit` | `tundra-salt-crystals` | `FrostSalt` / Frost Salt | 40 | A small wind-scoured cluster of pale, angular salt crystals rooted in the tundra surface. |

The item display names are **Wood**, **Lightwood**, **Densewood**, **Amber**, and
**Frost Salt** respectively. Presentation stays original and collision-free;
the existing server interaction collision and generated terrain remain the
authority for reach and placement. The trunk sources reuse their generated tree anchors. Aspen species is assigned to accepted Meadow forest candidates whose stable candidate index is divisible by three; this does not change candidate positions, dimensions, or persistent IDs. Aspen uses the existing tree felling, condition, capacity, and sparse-save transactions, with a fixed four-Lightwood reward on the final bundle pickup and the Stone Axe-or-better tier. The peat and salt forms add no platform, trail, camp, or route.
Wood source stacks cap at 50 like current `Wood`; mineral source stacks cap at
40 like current `Stone`.

### A0 hand-gathered crop patches

The existing deterministic `HarvestNode` descriptors assign the first dry,
safe Meadow candidate in each 6,000 cm spatial key to a carrot patch. Its
persistent seed is `Mix(candidate-seed XOR 0xA0CA22071019)`, which keeps the
`meadows-wild-carrot-patch` identity distinct from its former biome-source
occupant. The next dry, safe Meadow candidate in that key becomes a potato
patch with its own persistent seed domain, `Mix(candidate-seed XOR
0xA0B070A71019)`. The two patches use separate positions and stable sparse-save
identities; remaining descriptors retain their biome sources. Their original
collision-free procedural meshes use green foliage with orange carrot roots or
tan potato tubers, and the node's existing interaction sphere remains the
target.

The first dry, safe Shimmering Lakes candidate within the shared 400 cm
cardinal shoreline probe in each eligible key becomes a rutabaga patch. It is
classified as Shimmering Lakes land, outside lake and ocean water, and uses the
stable identity domain `Mix(candidate-seed XOR 0xA0B07ABA71019)`. The original
procedural presentation shows purple shoulders, pale roots, and foliage. Other
Lakes candidates keep their normal source identity.

The first dry, safe Elderwood harvest candidate in each eligible key becomes an
onion patch. Its `elderwood-wild-onion-patch` identity uses the separate stable
seed domain `Mix(candidate-seed XOR 0xA0E10A71019)`, preserving its position
while preventing depletion from colliding with the former biome-source
occupant. Its original procedural presentation uses pale bulbs and green shoots.

The next dry, safe Elderwood harvest candidate becomes a Forest Chanterelle
patch with the `elderwood-wild-chanterelle-patch` identity and independent
seed domain `Mix(candidate-seed XOR 0xF0E57C4A71019)`. It preserves deterministic
placement, uses the existing server-owned hand-gather transaction, and grants
two Chanterelles on accepted depletion; it adds no recipe or save field.

The server retraces each interaction and accepts hand gathering without a
tool. Carrot patches preflight 2 `Carrot` and 1 `CarrotSeed`; potato patches
preflight exactly 2 `Potato` and grant no `PotatoSeed`; rutabaga patches
preflight 2 `Rutabaga` and 1 `RutabagaSeed`; onion patches preflight 2 `Onion`
and 1 `OnionSeed`. Each complete result must fit before inventory/player
persistence and stable-identity depletion are committed through the existing
population save callback. A full pack, failed player-state persistence, invalid
range, or repeated request leaves the patch available and grants no item. Crop
seed use remains deferred to A7; no Potato Seed source is added in A0.

## Deterministic placement and population budget

- Use the existing invisible 6,000 cm `SpatialKey` and the current world seed,
  generator revision, source ID, and key coordinates for deterministic
  placement. The generator revision is part of the matching world-save
  identity; it is not a visible region boundary.
- Use standard FNV-1a 64-bit (offset basis `14695981039346656037`, prime
  `1099511628211`) over the ASCII bytes of each specified string.
- Compute FNV-1a 64-bit over the ASCII string
  `m9-place-v1|<world-seed>|<generator-revision>|<spatial-x>|<spatial-y>`;
  the remainder modulo four selects exactly one source in the table order
  above. Format the seed as unsigned base-10 and the revision and coordinates
  as signed base-10 with no leading zeroes. Thus each source can be selected on
  at most one in four keys. If that source's biome is absent from the key,
  place nothing; do not fall back to a different source.
- For the selected source, examine at most eight candidates in fixed ordinal
  order, ordinal 0 through 7. Derive each candidate from FNV-1a 64-bit over
  `m9-place-candidate-v1|<world-seed>|<generator-revision>|<spatial-x>|<spatial-y>|<source-id>|<ordinal>`;
  use the same decimal formatting and the canonical source token, then map the
  low and next 16 bits to fractions `(bits / 65536.0)` for X and Y within the
  key. Accept the first point that is inside the 16 km world, has the source's
  exact classified land biome, is dry under the existing ocean and lake
  samplers, and has a terrain normal Z of at least 0.88. If none qualifies,
  place no source on that key; do not fall back to a client or actor-selected
  location.
- The combined M9 source ceiling is at most one node per spatial key,
  regardless of how many candidates are examined. It occupies one slot from
  the existing post-biome `HarvestNode` budget, only when that budget is at
  least two, and leaves at least one first-wave harvest slot. Never append a
  source beyond `GetSpawnBudget` plus the existing biome multiplier, raise the
  25 active terrain-patch ceiling, or create a parallel population budget.
- Source positions, IDs, and presentation selection must reproduce for the
  same seed, revision, and key on every peer. Clients receive only relevant
  materialized actors, not candidate lists, placement rolls, or routes.

## Stable sparse depletion identity

Derive the source's current population spawn identity through
`FKalmalaWorldPopulationLayout::GetPersistentSpawnId` and wrap it in this
reserved resource key:

```text
resource:m9:v1:<source-id>:<kind>/<spatial-x>/<spatial-y>/<spawn-seed>
```

Store it only as a world-scoped `ResourceDepleted` sparse delta under the
existing exact world seed and generator-revision save identity. The ID contains
only the approved source token and server-derived population identity; a client
never submits it. These IDs fit the current 128-character stable-ID limit and
remain separate for different sources or spatial spawns. Persist depletion only through the approved identity-scoped sparse ledger and its validated writer; source identities add no save field.

## Bounded optional loot

Each accepted source harvest guarantees one unit of its table's primary item.
It may grant exactly one additional unit of that same item; there are no
secondary item IDs, open-ended rolls, quantity ranges, or chained drops. The
bonus is awarded when the FNV-1a 64-bit hash of the ASCII string
`m9-loot-v1|<world-seed>|<generator-revision>|<source-id>|<population-spawn-id>`,
interpreted as an unsigned integer, is divisible by four. The outcome is
therefore deterministic for a source identity and has a strict total yield of
one or two items.

The server computes the bonus and preflights the full one- or two-item grant
against the existing item catalogue and owner inventory before publishing any
reward, tool wear, or in-session depletion. A full pack, invalid source,
wrong-world target, failed server trace, invalid range, or mismatched tool
intent leaves inventory and source state unchanged. Client requests carry no
reward ID, quantity, bonus result, target, or spawn identity. The separate
server-side field-gate implementation maps the approved birch trunk to
Stone-or-better and the ironheart trunk to the Iron woodcutting tier; it does not check a
station at the source. Tool acquisition and upgrades validate their matching
visible Workbench or Forge separately.

First-wave sources remain birch bark and resinwood for Wood, reed clusters for Wood, frostmoss for Frost Salt, and mountain slate veins for Stone. A separate deterministic pass adds at most one dry, slope-safe Stone outcrop in each land biome per 6,000 cm spatial key when that biome has no existing slate source, including Meadows; it keeps a 250 cm separation from other harvest nodes and preserves the ordinary source budget and existing crop/ore nodes. Outcrops reuse the slate presentation and server selection, requiring Stone Pick or better and yielding one Stone. The bounded extra-candidate search can leave a key without an outcrop; Ocean receives none. Deterministic secondary ore nodes add three Copper Ore per dry, safe Elderwood rock and three Tin Ore per dry Shimmering Lakes shoreline gravel node. Both are Bronze-grade mining resources available to Stone Pick or better. The Iron-grade Mossy Mire bog-iron vein yields exactly three Iron Ore per exhausted node and requires Bronze Pick or better; the Smelter converts one Iron Ore and one Coal into one Iron bar. Stone Picks mine through Bronze-grade resources; Bronze and Iron Picks currently mine through Iron. Amber and Frost Salt require Stone Pick/Mining; birch trunks yield Wood and Meadow Aspen trunks yield four Lightwood, both requiring Stone Axe or better; ironheart trunks require the Iron woodcutting tier. Harvesting tier is determined by the axe item identity: Bronze Axe level 2 remains Bronze, and only Iron Axe meets the Iron woodcutting gate. Older Iron Axe level-two records remain valid. Accepted harvest preflights the entire inventory reward, spends one tool-condition point, and depletes once. Failure preserves inventory, condition, and availability. Clients never choose reward, quantity, target, sparse ID, or bonus.

The Copper, Tin, and Iron Ore sources use the existing server-authoritative harvest transaction and stable sparse population identities; each key has at most one of each secondary source, and Lakes Tin requires the shared dry-shoreline classification. The user-directed level-one Iron Axe Forge acquisition and upgrade now both require Lightwood instead of Densewood, resolving the axe's Densewood dependency. Iron Knife, Spear, and Mace acquisitions still require Densewood; Iron Pick's recipe uses Iron, Wood, and Stone. Meadow Aspen trees provide Lightwood without a Bronze prerequisite. See the A0 source audit in `alpha-items.md`.

## Tools and station progression

All five Stone-tier equipment acquisitions have repeatable level-one recipes at the Workbench: Axe and Spear cost 3 Wood and 2 Stone each, Knife and Mace cost 1 Wood and 1 Stone each, and Pick costs 3 Wood and 3 Stone. They create a new GUID-bearing item even when the player already carries another copy. The server validates station context, materials, and shared inventory capacity before committing each craft.

Every carried entry occupies one of the same forty inventory cells; there is no separate carried-tool record cap. Each stack row has a cell GUID, and every physical item unit in its quantity has a separate GUID, including units in stackable items. Carried tools retain their own per-item GUIDs and metadata. Matching item IDs may occupy multiple cells; stackable quantities fill stacks up to their catalogue limit and split across cells as needed, while non-stackable copies each occupy a cell. The Stone-tier starting loadout is Stone Knife (`StoneKnife`, 16 durability, 30 slash), Stone Axe (`StoneAxe`, 24 durability, 34 slash), Stone Pick (20 durability, 26 crush), Stone Spear (36 durability, 42 stab), and Stone Mace (16 durability, 30 crush), plus a level-one Construction Hammer. New characters begin with the hammer in hotbar slot 1 and active; right-click opens the Build menu while it is active. Current player-discovery schema 2 stores each canonical `ItemId` and valid instance GUID directly. Old carried IDs or missing GUIDs are rejected rather than normalized during development. The hammer has 100 condition. Construction does not currently invoke durability consumption; consuming uses always spend one point. Spear combat damage is server-authored and uses the same active-carried-tool validation as other weapons; its condition does not wear from attacks.

Bronze Axe level 1 costs 4 Lightwood and 3 Stone at a level-one Forge. Its authored stats are 100 maximum durability and 50 slash bonus. Bronze Axe level 2 upgrades that same carried item at a level-two Forge, has no skill requirement, and costs 4 Lightwood and 3 Bronze; its stats become 150 maximum durability and 55 slash bonus. An upgrade changes only those item-authored stats; tool level never changes the item's harvesting tier or unlocks resources. It preserves item ID, inventory cell, and current condition. The server-selected station family and effective level must match the authored progression step exactly; rejection leaves both candidates unchanged.

Iron Knife (80 durability, 58 slash bonus), Iron Axe, Iron Pick (160 durability, 52 crush bonus), and Iron Spear (150 durability, 76 stab bonus) are independent level-one Forge acquisitions. Their costs are Knife (2 Iron, 2 Densewood), Axe (4 Iron, 3 Lightwood), Pick (4 Iron, 2 Wood, 2 Stone), and Spear (3 Iron, 3 Densewood). Iron Axe upgrades from level 1 to level 2 at a level-two Forge for 4 Iron and 3 Lightwood. Iron Ore is mined from the Mossy Mire bog-iron vein with a Bronze Pick or better and refined with Coal at a Smelter. No Bronze tool is an upgrade predecessor for Iron gear; the server validates each item as a separate acquisition.

Stone, Bronze and Iron Maces use the matching knife tier durability (16/48/80) and damage amount (30/42/58), authored only as Crush bonuses. Their profiles have no harvesting action or skill requirement. Stone Mace is part of the six-item new-player starter loadout. Bronze Mace costs 2 Lightwood and 2 Stone at Forge level 1; Iron Mace costs 2 Iron and 2 Densewood at Forge level 1. Their explicit station Craft actions dispatch only the mace ID through the existing server progression transaction; owner-visible context, current carried state and materials gate the buttons. Full listing of all other tool acquisitions remains separate backlog work. Mace level rewards currently add zero; upgrade costs use the corresponding acquisition ingredients.

A paid Tool Rack or Forge Anvil within 125 cm adds one level to its matching station. One attachment per station, a level-two ceiling, 32 attachments, and 128 total construction records bound progression. Placement and station use recheck visibility and the normal 250 cm range. Station levels are derived from placed actors, not saved as numbers.

Inventory's selected-tool repair is free at a visible same-world Workbench or Forge within 250 cm. The Workbench and Forge Repair All actions restore every damaged carried tool in one operation without changing levels or granting Crafting XP; the server validates the nearby station and the complete carried-tool candidate before saving or publishing anything. Grinding Stone Interact performs the same atomic Repair All against its validated carried list; malformed records reject the whole action. Its accepted 1.2-second sharpening pose is cosmetic and carries no tool state. M9 free repair supersedes historical M7 material-paid repair and broken-tool replacement recipes. Accepted tool changes save the validated schema-3 player candidate before publication.

## Camp crafting and station services

## Current recipe rules

Recipe access is limited by the required raw materials, any visible same-world
station listed in the `RequiredStation` array, the bounded batch, and live heat
at food-processing stations. The server derives the heat rule from the
resolved Cooking Rack, cauldron, or Frying Pan identity; recipe metadata does
not opt recipes into a generic hearth gate. Schema 4 contains no Smoke Frame
item or recipe. There are no recipe skill-level
requirements, `AlternateStation`, `OutputTool`, or legacy fire flags. Cooking
experience remains a server-owned reward after a successful inventory
exchange.

## Current food recipes

These seven food recipes and their outputs are the active cooking catalogue.
Every batch is capped at five, and every output stack is capped at twenty.
The server requires the declared visible same-world station and a usable lit
hearth with positive heat within 250 cm of both the player and station.

| Recipe ID | Ingredients per serving | Required station | Output item |
| --- | --- | --- | --- |
| `CookedBoarMeatRecipe` | `BoarMeat` x1 | Cooking Rack | `CookedBoarMeat` |
| `CookedDeerMeatRecipe` | `DeerMeat` x1 | Cooking Rack | `CookedDeerMeat` |
| `HearthBrothRecipe` | `DeerMeat` x1, `Onion` x1 | Cauldron | `HearthBroth` |
| `MeatStewRecipe` | `BoarMeat` x1, `DeerMeat` x1, `Carrot` x2, `Potato` x2 | Cauldron | `MeatStew` |
| `RootVegetableSoupRecipe` | `Carrot`, `Potato`, `Rutabaga`, `Onion` x1 each | Cauldron | `RootVegetableSoup` |
| `RoastedRootVegetablesRecipe` | `Carrot`, `Potato`, `Onion` x1 each | Frying Pan | `RoastedRootVegetables` |
| `DeerRootRoastRecipe` | `DeerMeat`, `Rutabaga`, `Onion` x1 each | Frying Pan | `DeerRootRoast` |

`HearthBroth` is produced by the reviewed Cauldron recipe above and remains
stacked to twenty. The former `RoastedFieldMeat`, `SmokedFieldMeat`, and
Smoke Frame IDs are absent from schema 4. The steady-meal item use remains
transient and does not add save data.

All construction recipes consume the raw materials listed in the catalogue.
Current construction recipes use raw materials directly; retired
`ConstructionSupply` saves are not converted during development. Duplicate
ingredients in current recipes are merged. The Construction Hammer places the hearth,
floor, wall, roof, Workbench, Forge, Workbench Tool Rack, and Forge Anvil
directly from their recipe costs without creating a carried kit. Other buildable
construction recipes retain their existing validated placement path.

| Retired item or recipe | Current behavior |
| --- | --- |
| `Fuel` | Removed. Hearth placement and refuelling consume raw Wood, Lightwood, Densewood, or Coal directly. Cooking recipes consume only their listed ingredients; old saved IDs are not converted during development. |
| `ConstructionSupply` | Removed. Current saves do not convert the retired ID into Wood. |
| `RaisedStorage` | Removed. `Storage` produces the normal Chest only. |
| `Timber` | Removed as an intermediate recipe; construction consumes Wood directly. |
| Smoke Frame and legacy smoke/roast recipe IDs | Absent from schema 4. Current cooking uses the six recipe IDs in the table above. |

Any one of Wood, Lightwood, Densewood, or Coal adds 60 seconds of hearth fuel.
The server burns one fuel second per elapsed server second while the hearth is
lit. Cooking does not consume extra inventory fuel per serving or batch, and
clients cannot select or charge fuel through a recipe. Coal is accepted as a
fuel item, but the current world loot catalogue has no natural Coal source.

## Compatibility and authority

The JSON source remains schema 4 and the storage SaveGame is schema 2. Current
chest records must already use active catalogue IDs and include a stable item
GUID per unit. Incompatible schema versions, retired item IDs, or missing GUIDs
fail validation and are left untouched; no saved chest data is converted or
rewritten during development.

The server validates recipe identity, batch bounds, item IDs, required
stations, station-local hearth access and heat for food processing, and output
capacity before publishing an inventory change. It selects a visible station
in the same world; clients cannot choose station actors, ingredient costs,
outputs, rewards, fuel type, or construction results. The normal Chest keeps
its existing server-selected transfer rules, bounded contents, and owner-only
inventory view.

## Optional content still deferred

A storm-rated shelter piece remains deferred until its behavior differs from
the existing roof and windbreak. An insulated wrap remains deferred until
there is an approved equipment-slot and effect contract.

Build supports direct hearth, floor, wall, and roof construction plus placement of supported produced station/utility items. Campfire is construction-only, never a carried item. Raw construction costs and one available ignition fuel item are paid server-side; fuel priority is Wood, Lightwood, Densewood, then Coal. Placement checks the carried hammer, finite-world bounds, ground, clearance, and collisions using server-derived transforms. Local previews cannot place or pay.

Interacting with a Workbench opens Craft; Forge opens Craft/Upgrade. Each station also has a direct Repair All action that repairs all damaged carried tools without opening a separate page. Cooking Rack, Cauldron, and Frying Pan show only their two declared dishes. Forge creates Frying Pan from five Iron; Build places the produced pan. Station views bind to the exact accepted actor/stable construction ID and close on destruction, range loss, or pawn replacement. Every action independently revalidates the server station and transaction.

Chest Store/Take transfers one selected allowlisted item through the existing server RPCs. Its owner-local pack/chest views do not create extra player capacity. Chest contents are bounded, private, and saved as a validated candidate before the live pack is changed; rejected/stale transfers preserve both. Campfire refuelling adds 60 seconds from one raw fuel item up to 300 seconds without lighting it. M13's requested direct Interact lighting replaces the older Build relight button only when implemented and accepted; see Menu browsing.

## Combat and support magic

The committed basic attack sends only a monotonically increasing request sequence. The server traces living wildlife within 220 cm, uses a 0.18-second windup, rechecks target validity, and enforces 0.36-second recovery. It applies the active usable carried weapon's authored mixed damage bonuses (Stone Spear: 42 stab; Bronze Axe level 1: 50 slash and level 2: 55 slash); with no item in Main Hand, it applies 25 unarmed crush damage. Empty hands and usable melee profiles without an authored Basic attack use the existing 25 Crush melee fallback; Construction Hammer and Bow have separate building and shooting controls. Swing initiation requires no target; impact still requires a validated server target. Weapon level records now define `DamageBonuses` with Slash, Stab, Crush, Pierce, Fire, Water, Holy, Lightning and Poison instead of `CombatDamage` and a single DamageType. Axes/knives use slash, melee spears stab and picks crush; ranged weapons use the distinct pierce channel. Any number of channels can be active together. Each family authors bounded unique `Attacks` (including Basic) in the shared attack file, with additive bonuses in the same channels. Axes, Knives, Pickaxes, Spears and Maces have separate lists; Construction Hammer has no authored attack list and never enters melee while in Main Hand. Each combat profile resolves exactly one list, and unknown IDs, duplicate list/style IDs, repeated item IDs, overlapping lists or missing assignments fail closed. The current server-selected Basic attack adds those bonuses to the item-level bonuses. A validated hit first resolves each active channel against its flat resistance. Wearable Armor first reduces the combined physical Slash, Stab, Crush and Pierce subtotal. Lightning bypasses wearable Armor, then general Armor is subtracted once from the remaining physical plus Lightning damage. Fire, Water, Holy and Poison bypass both Armor values. Negative channel resistance is a flat weakness (Slash −5 adds 5 to incoming slash); inactive channels remain zero. Channel resistances must be finite and within −100000 to +100000; damage bonuses and general Armor remain nonnegative. `UKalmalaResistanceComponent` defines these defenses on players, wildlife, construction, campfires, harvest nodes, discovery entities, hazards and skiffs. Defaults are zero for all channels and Armor, editable in entity/component defaults; static defenses are not client-settable or new saved fields. Only the existing player and wildlife health paths consume typed hits; other entities gain definitions without gaining health/destructibility. Hearth Shield absorbs the already-mitigated player damage. No new ranged/elemental attack actions, projectiles, damage-over-time effects or magic abilities are introduced. Unknown attack IDs and malformed/nonfinite/out-of-bound profiles, negative damage, or negative general Armor fail closed. Bear's Vigor continues to apply its server-owned strength multiplier. Wildlife has 100 health; defeat joins the existing unique server-owned sparse callback. There is no arbitrary damage endpoint or client target. Public action phase/serial and owner-only target-free results provide readable feedback.

Mirelings scavenge and threaten camps, boar defend territory with committed charges, deer use bounded wary-herd flee behavior, and Meadows Rats use a 3 m approach trigger with a visible 0.5 s telegraph, a server-validated 100 cm Pierce bite, and 0.8 s recovery. The Rat's small low-poly silhouette and bite cue are derived from its server-selected archetype. Server-owned ecological behavior and defeat rewards never use client-selected archetypes, targets, or loot. Optional discoveries and boss scrolls commit authenticated-player entitlement before acknowledgement; duplicate claims grant nothing and failed saves leave the encounter/discovery recoverable.

Mending heals one other living damaged ally within a 350 cm forward/visibility trace by up to 30 health, without revival. Hearth Shield gives its caster 40 absorption. Bear's Vigor raises the stamina cap from 100 to 140 and supplies 1.4 strength without refilling stamina. Deer Call affects at most three existing eligible living deer within 900 cm; it cannot spawn, teleport, damage, harvest, or reward. Existing runtime behavior uses a bounded flee influence despite the broader design's attraction wording. Effects have an 18-stamina base cost through the shared Wet modifier; server definitions own magnitude, targeting, and expiry.

### M13 reusable support-scroll inventory and hotbar integration

The 2026-10-10 user request supersedes the F1–F4/Q effect-selection path above:
each existing learned support effect is represented by a reusable scroll item
in the standard carried inventory, can be assigned to one of numbered hotbar
cells, and activates from that hotbar cell. Scroll use does not consume the item.
The learned-effect entitlement remains the persisted progression gate; when a
pawn's transient inventory is initialized, the server reconstructs the matching
scroll items from that existing entitlement without adding save fields. This
preserves discovery/learned-effect records and reconnect behavior while keeping
scrolls in the ordinary inventory UI.

Hotbar intent identifies only the cell index. The server derives the scroll's
canonical item/effect mapping from that current owned cell, confirms the learned
entitlement and existing server-side request/rate, cooldown, stamina, and
effect-specific target gates, then uses the existing support activation path.
This M13 timing override supersedes earlier M4 cooldown and duration values: each
accepted support cast starts a shared 300-second per-caster server cooldown.
Hearth Shield, Bear's Vigor, and Deer Call last 180 seconds; Mending remains an
instant heal with no active duration. Deer Call's existing bounded, server-
selected deer influence lasts 180 seconds. At most one support effect may be
active per caster; reject any new cast while an ongoing effect remains active,
and do not stack or refresh effects. The cooldown remains active after the
effect expires. Preserve current costs, magnitudes, targets, owner-only feedback,
and relevant-peer effect presentation. Never accept a client effect ID, target,
cost, duration, magnitude, or result. No new spell or saved-data migration is
introduced.

| Effect | Server-selected target and result |
| --- | --- |
| Mending | Living allied pawn in range and line of sight; positive heal clamped to maximum health; reject full health; no revive |
| Hearth Shield | Caster; temporary bounded absorption; expiry removes remaining protection; reject refresh while active |
| Bear's Vigor | Caster; temporary bounded stamina/strength modifiers; reject refresh while active; do not refill stamina on activation or expiry |
| Deer Call | Bounded nearby living deer selected by server; behavior influence only; no spawn, teleport, damage, harvest, or loot |

The M13 scroll implementation is reported in the handoff files; fresh integrated acceptance remains pending. These requested 300/180-second values supersede the older five/ten-second prototype timings when that implementation is integrated.

## Ocean travel

## Travel medium and identity

The first medium is a project-original open-water skiff with two seats: one helm and one passenger. It carries no cargo. This contract does not prescribe its mesh or recipe. Each skiff is a transient server-created actor; the primary skiff receives the stable world-scoped identity `ocean-skiff:primary` when its moored state is saved, while the live actor reference remains transient.

The existing server-owned immutable `WorldSeed` remains the only generated-world identity. The skiff adds no generator input, biome, route, authored waterway, or alternate world identity. It operates on master-map Ocean; inland lakes do not qualify.

## Server-owned state

The server owns and mutates:

- the skiff actor's transform and velocity;
- movement mode: `Moored`, `Underway`, or `Blocked`;
- the helm and passenger seats, each occupied by at most one authenticated session player;
- the currently accepted helm input sequence and freshness timer; and
- launch, boarding, disembark, collision, and movement outcomes.

The server assigns seats. Only the helm occupant steers. A passenger has no movement authority. Vessel state is never created, selected, or moved by directly applying client-supplied actor transforms.

`Moored` is the launch/stopped state at or below 50 cm/s with no active throttle. `Underway` applies while speed is above 50 cm/s or a nonzero helm input is active, including coasting after input expiry. `Blocked` is entered only for a collision or invalid-water movement rejection; a fresh valid helm intent may move the skiff clear.

## Client intent and movement bounds

Launch, board, and disembark are action intents through the existing server-validated interaction path. A request may identify a candidate vessel for boarding or leaving, but the server validates that reference against the caller's current world, interaction range, visibility, and seat availability. A launch request carries no spawn point. Seat choice and exit placement are server-derived.

The helm sends only finite throttle and rudder values in `[-1, 1]` plus a monotonically increasing per-helm sequence number. Accept no more than 10 steering updates per second and expire the current input after 0.5 seconds without a fresh sequence. Older, duplicate, malformed, non-finite, or out-of-range input is rejected without state mutation. Initial shared compiled movement caps are:

| Limit | Initial cap |
| --- | ---: |
| Forward speed | 700 cm/s |
| Reverse speed | 200 cm/s |
| Acceleration | 100 cm/s^2 |
| Yaw rate | 35 degrees/s |

The server simulates bounded movement and sweeps the skiff against generated terrain collision. A locally controlled helmsman may predict presentation using the shared caps, but server movement and correction are authoritative. Disembark is accepted only at server-measured speed of at most 50 cm/s and only when the server can resolve a safe pawn capsule placement; clients cannot choose an exit location.

## Water, collision, and failure

The skiff may travel only over master-map Ocean where the existing generated-ocean depth sampler reports at least 100 cm of water. The server retains the existing 16 km playable radius and shared 25-patch terrain-streaming cap. Movement may not cross generated land, skip a collision surface, or continue beyond the finite-world boundary.

If a server sweep or water sample blocks movement, the server clamps to the last safe transform, zeros velocity, sets `Blocked`, replicates that mode to relevant peers, and sends the failure reason to the helmsman. A fresh valid helm intent may steer away from the obstruction. Stale input or a disconnected helm occupant clears throttle and server-decelerates the vessel to `Moored`. Invalid launch, boarding, steering, or disembark requests leave the affected actor and occupancy unchanged; the requester receives the denial reason.

`Blocked` describes a collision or invalid-water response only. This first session-only, no-cargo skiff has no hull health, disabled mode, repair, salvage, or accepted hull-damage source. Collision or invalid-water rejection already has a recoverable outcome: the server clamps to the last safe transform, stops the skiff, and accepts fresh helm input to steer clear; stale or disconnected helm input clears controls and decelerates the skiff to `Moored`. Adding durability now would require a new damage source and repair or salvage loop without an accepted role in this bounded travel medium. Revisit hull damage only if a later approved M8 hazard explicitly affects vessel integrity, and define its server-owned recovery before implementation.

## Replication and persistence

Replicate skiff movement at no more than 10 updates per second while moving. Replicate movement mode and the two session occupant references to relevant peers. Do not replicate raw steering samples, input sequence/freshness, or per-player control data to other players. Send transient action and failure feedback only to the requesting player; do not multicast every input sample.

The versioned M8 persistence gate below now permits one server-created skiff's moored snapshot and authenticated player seat references. Cargo, velocity, steering input, and live actor references remain excluded. Ocean discovery claims continue in the separate M7 sparse ledger.

### Versioned travel-save contract and runtime restore

`UKalmalaOceanTravelPersistenceSaveGame` defines a separate schema-1 gate and
does not extend or rewrite the M7 sparse-discovery container. It reuses the
M7 identity contract so every record is bound to the exact `WorldSeed`,
generator revision, and explicit world or authenticated-player scope. A
world-scoped record holds at most one stable `ocean-skiff:` identity and its
last server-accepted safe location and heading. The writer may capture that
snapshot only when the server has stopped the skiff in `Moored`; coordinates
must be finite and inside the 16 km world radius, with bounded vertical offset
and heading. A player-scoped record holds at most one stable vessel reference
and a `Helm` or `Passenger` seat, keyed by the authenticated player identity
in the enclosing save identity. The two records pair only when world seed,
generator revision, and vessel ID agree.

This skiff has no cargo, so the contract has no cargo field. It also omits
velocity, steering samples, session actor references, and saved live occupancy;
seat references are rebuilt from authenticated player records. The runtime
writes world state only when the server has a `Moored` skiff, then writes each
authenticated occupant's seat under that player's scoped slot. A disembark
clears the player's seat slot before moving the pawn, and fails closed if the
slot cannot be cleared. Startup accepts an existing world record only when its
identity, safe bounds, sea-level position, and full generated-ocean hull
footprint remain valid; actor spawn must also pass collision handling. On
login, the server pairs the player's exact identity and saved seat with that
world vessel, then rejects an already occupied seat, a duplicate player, or an
already attached pawn. Invalid or incompatible records are preserved without
overwrite. Ocean discovery `DiscoveryClaimed` ledgers are loaded for the
authenticated player at login, so reconnects keep duplicate rewards rejected.

Schema zero and unknown future versions fail closed. Do not add compatibility
migrations before release; development save formats may change directly.
The focused
`Kalmala.World.OceanTravel.PersistenceContract` automation round-trips both
scopes in memory and through a local save slot, and rejects mismatched
identities, malformed bounds, orphaned vessel references, and invalid seats.
`Kalmala.Gameplay.OceanTravel.SkiffRestoreContract` covers server-only seat
restore gates, duplicate player/seat rejection, and safe disembark-save
feedback. Normal save and restore now run in the server game mode; the full
two-player restart/reconnect journey remains in M8 acceptance.

## Multiplayer assessment

This contract preserves server authority: clients send bounded action and steering intent, while the server selects seats, validates world/collision constraints, simulates movement, and publishes accepted outcomes. The only public replicated vessel state is relevant movement, mode, and current session occupancy. Private input and all persistent state remain excluded.

The world uses a fixed origin throughout the finite 16 km radius; travelling does not rebase origin or change identity. Server weather applies bounded crosswind pressure through the existing skiff movement authority. Optional sea discoveries use server-derived stable identities and authenticated-player claims; no cargo, hull durability, mandatory voyage, or authored sea route is added. Late join and restart must preserve moored-vessel/seat identity, owner-private discovery claims, and exact-once rewards. M8 dry-shore acceptance was waived; safe capsule disembark must not be described as proof of dry shoreline arrival.

## Optional land discoveries

## Candidate slate

| Candidate ID | Player-facing name | Biome | Observation cue | Reward |
| --- | --- | --- | --- | --- |
| `lakes-rillworn-marker` | Three-Run Rillstone | Shimmering Lakes | A low, walkable shoreline stone has three water-cut channels that meet at a shallow pocket holding a small cache. The channels and pocket read by shape and relief, not colour alone. | `Stone` x2 |
| `mountains-leeward-grain` | Leeward Grain | Thunder Mountains | Parallel wind-scored lines cross a broad, walkable ridge-foot shelf; a sheltered ridge shelf catches a small bundle of dry kindling. The marks and bundle read by shape and text, not colour alone. | `Wood` x2 |

Each candidate rewards noticing a local feature and inspecting it through the
ordinary interaction flow. Its short observation text describes the feature
itself; it does not point toward another candidate or reveal coordinates.
Rewards use existing catalogue item IDs and are intentionally small. They do
not add a new item, tool tier, skill unlock, magic effect, or access to
`Lightwood`, `Densewood`, `Amber`, or `FrostSalt` outside their approved
sources.

## Design boundaries

- Both candidates are optional and self-contained. There is no order, quest
  chain, waypoint, compass bearing, route, or required clue sequence.
- Neither candidate requires a boss, combat, a specific tool, special weather,
  a timing window, swimming, a precision jump, or a dangerous climb. The
  intended approach is ordinary walking on stable terrain.
- The reward belongs to the player who successfully inspects the candidate;
  one player's claim does not prevent a nearby co-op partner from discovering
  the same feature. Duplicate claims are tracked independently by authenticated
  player identity.
- Use the existing world presentation and fog-of-war rules. Do not publish an
  undiscovered candidate list, add a map/minimap marker, or query hidden
  discovery positions from the client.
- The server derives one candidate for a spatial cell only when its center is
  in the candidate's biome. Placement starts from the existing biome discovery
  point and probes up to 24 deterministic offsets to avoid overlapping its
  first-wave marker. The final position must be in bounds, dry, walkable, and
  in the same biome; the Rillstone also requires lake water within 350 cm, and
  Leeward Grain requires elevation of at least 0.80. Since the compiled basin
  policy is disabled, the Rillstone candidate is currently dormant; the
  Leeward Grain remains eligible on qualifying mountain terrain.
- Stable sparse identities use
  `land-discovery:m9:1:<candidateId>:<x>,<y>`, where the coordinates are the
  population spatial key. The server re-derives the descriptor before use.
  Clients receive only the presentation identity and submit interaction
  intent; they do not receive candidate descriptors, rewards, or hidden
  discovery positions.
- A claim requires server authority, the same world, an active population
  cell, the canonical descriptor, an authenticated player identity, and a
  distance of at most 250 cm. The server preflights the exact inventory grant,
  records the sparse identity for that player, and commits the inventory
  transaction; failed commits roll back the claim. The in-memory claim set is
  capped at 64 discoveries per player per session, and accepted feedback is
  owner-only.
- Schema-2 player saves now preserve validated land claims. Failed saves reject the claim and inventory transaction. Ordinary material rewards remain pawn-lifetime and are never re-awarded on reconnect.

The slate adds no boss encounter or combat gate. Boss rewards remain optional
under the existing game-design contract and can be considered separately if a
later accepted candidate needs them.

## Save identity and format policy

## Identity and ownership

All new M9 records match the exact server-owned world seed, generator revision
7, and declared scope. World construction records use
`FKalmalaM7SaveIdentity::ForWorld(WorldSeed)`. Player tool and discovery records
use `ForPlayer(WorldSeed, AuthenticatedPlayerIdentity)`, with the existing
128-character identity bound and validation. The player identity comes from
the authenticated `PlayerState` unique ID; a local player index, display name,
client-provided ID, or shared fallback is never accepted. A missing or invalid
authenticated ID disables player-save restore and writes for that session.

Before loading or caching a player slot, the server refreshes its world
generation config from the authoritative `GameState`. This covers `PostLogin`
running before `GameMode::BeginPlay` copies the selected seed. Cached player
saves are reused only after their full world and owner identity still matches;
a stale cache fails closed without restoring or writing it.

The existing construction and player-discovery slot names remain in use. Their
schema-2 payloads carry the explicit M7 identity in addition to the legacy
seed/player fields where those fields are retained. A record is usable only
when the seed, revision, scope, and (for player state) owner all match exactly.
The existing `UKalmalaM7PersistenceSaveGame` schema-1 sparse world/player
ledger remains unchanged; its resource, creature, and ocean-discovery entries
are not reinterpreted or moved.

## State scope and bounds

| State | Scope and saved value | Bound and validation |
| --- | --- | --- |
| Carried tools | Authenticated player: canonical item ID, authored tool level, and condition for each carried tool | At most the existing inventory-slot bound. IDs must be unique and in the tool catalogue; levels must match that item's authored tier; condition must be finite and within its authored range. No skill ledger or pack inventory is included. |
| Workbench/Forge progression | World: accepted Workbench tool-rack and Forge anvil construction records (stable construction ID, existing kit identity, transform) | Existing 128 construction-record total and 32 attachment cap. At most one compatible attachment may upgrade each station. Effective level is re-derived from same-world placed actors and capped at level 2; no numeric station-level save field is added. |
| Land discoveries | Authenticated player: existing stable discovery IDs, including the two M9 land-discovery IDs | Preserve the old 64-entry first-wave allowance and add at most 64 M9 claims; schema 2 caps the combined set at 128 unique valid IDs, each no longer than the existing 128-character stable-ID limit. |
| Other camp structures | World: supported construction records in the current schema-6 format | Count all structures against the same 128-record cap. Unsupported or malformed records fail validation. |

No saved record contains a client-selected result, cost, tool level, station
level, reward, actor pointer, or hidden discovery descriptor. Clients continue
to submit intent only. World records remain server-owned and replicate through
the existing relevant construction actors. Detailed tool state remains
owner-only through the existing carried-tool replication; the save adds no
replicated field.

This contract does not persist ordinary carried materials or inventory,
skill/experience, active food or support effects, weather, exposure, tool
cooldowns, or other unapproved equipment slots. In particular, an M9 discovery
claim remains durable even if its common material reward was in the existing
pawn-lifetime inventory and is lost on reconnect. The claim is not re-awarded;
the materials remain available from their ordinary sources. This contract does
not claim general inventory persistence.

## Development save-format policy

The game is still in development and has no user save files to preserve. Do not
add SaveGame compatibility migrations before the game is released. During
development, change the active save format and schema directly as needed;
incompatible or unsupported saved data fails closed and is left untouched.
This applies to player discoveries and equipment, world construction,
storage, travel, sparse world/player records, and local map saves. After the
game is released, introduce a migration only when released user data needs to
be preserved, with an explicit migration design and focused verification.

All current writes still validate the complete candidate, including identity,
duplicate IDs, catalogue IDs, transforms, condition/level ranges, and bounds,
before replacing a slot or publishing the live transaction. Failed writes
preserve previous bytes and reject or roll back associated gameplay changes.
Current construction saves use schema 6; player-discovery and chest storage
saves use schema 2. Other save owners retain their current versions.

## Current save and catalogue versions

These are the current formats in the source tree. Save versions may change
directly during development; compatibility migrations are deferred until after
release.

| Save owner or data contract | Current version | Scope |
| --- | ---: | --- |
| Construction records, station attachments, Kiln/Smelter processing, Garden Plot crops, and Timber Door state (`UKalmalaConstructionSaveGameV2`) | Save schema 6 | Server world |
| Player discoveries, learned effects, M9 claims, and carried items (`UKalmalaPlayerDiscoverySaveGameV2`) | Save schema 2 | Authenticated player and exact world |
| M7 sparse persistence ledger (`UKalmalaM7PersistenceSaveGame`) | Save schema 1 | Identity-scoped sparse world/player facts |
| Generated population deltas (`UKalmalaWorldPopulationSaveGame`) | Save schema 1 | Server world |
| Chest storage (`UKalmalaStorageSaveGame`) | Save schema 2 | Server world |
| Ocean vessel and seat records (`UKalmalaOceanTravelPersistenceSaveGame`) | Save schema 1 | Server world and authenticated player |
| Local map exploration coverage (`UKalmalaWorldMapExplorationSaveGame`) | Save schema 1 | Local player |
| Local map pins (`UKalmalaWorldMapPinsSaveGame`) | Save schema 1 | Local player |
| `Content/Data/GameCatalogues.json` and `Content/Data/GameCatalogues/{items,recipes,tools}/*.json` | Catalogue schema 16 | Manifest and authoritative item-family, recipe-family, Building Hammer, and tool progression definitions; not a save schema |

Before release, do not add compatibility migrations; change development save
formats directly and reject incompatible local data. After release, any change
that affects user saves needs an explicit migration and recovery plan, focused
round-trip/rejection coverage, and an updated release rationale.

The accepted M10 gameplay baseline stays frozen except for explicit subsequent user-directed scope such as M13/M14. No routine documentation or release cleanup changes save identities, platform, business model, or online-service commitments.

## Shared inventory and hotbar


The owner's 2026-10-09 request supersedes the earlier sixteen-slot pack and
separate carried-tool presentation. The player has one ten-column, four-row
inventory. Materials, meals, kits and tools all occupy this same forty-cell
space; tool condition/level records are metadata for those cells, with no
additional toolbelt capacity. Six wearable slots are a separate owner-private
session inventory, and equipped items do not occupy grid cells. Existing
station and chest inspection panels read the same holdings and do not create
another player inventory.

The upper row is labelled `1 2 3 4 5 6 7 8 9 0`. Its cells are the hotbar.
Moving or swapping an item into a numbered cell assigns that number directly;
moving it into a lower row removes its hotbar entry. Exhausting a stack leaves
its cell empty and preserves every other cell's position. New holdings enter
the first free cell. Existing starting tools occupy ordinary cells. Tool
upgrades retain the replaced tool's cell.

Tab / I opens the panel at the upper left. Drag between cells to move or swap.
Arrows / D-pad navigate all forty cells, including empty cells; Enter / A picks
up and places, and Escape / B cancels a pending keyboard move before closing
the panel. The selected item retains its shared description, food-use action,
and applicable free selected-tool repair. Search, category, sorting, and the
separate tool section are removed from this player panel. Its outer scroll
retains access to descriptions/actions on small viewports and at larger text
scales. The grid remains ten by four while fitting the panel width.

Normal gameplay shows only occupied numbered cells in a compact upper-left
row, preserving their original numbers in `1` through `9`, then `0` order.
For example, occupied cells 1, 4 and 0 render three entries labelled 1, 4, 0.
The row hides during inventory and other movement-blocking modals, and when
all ten cells are empty. Original catalogue icons, quantities, condition bars,
and an active-item underline remain visible. Number keys select the server's
item in that cell; supported food uses the existing validated meal transaction.
An active tool supplies subsequent harvest intent, which the server still
independently validates. The M13 support-scroll integration supersedes the
earlier F1–F4/Q support-effect selection: a reusable scroll item occupies an
ordinary inventory cell, can be assigned to a numbered hotbar cell, and is
activated from that cell. Hotbar use sends the cell index only; the server
derives the current item and its allowlisted effect, checks learned entitlement
and existing activation gates, then routes through the support-magic authority
path. Scrolls are not consumed; the existing server cooldown controls reuse.
Each accepted support cast starts the shared, server-owned five-minute cooldown.
Hearth Shield, Bear's Vigor and Deer Call last three minutes; Mending is
instantaneous. Deer Call's bounded wildlife influence also lasts three minutes.
Only one support effect may be active per caster at a time. The current
controller D-pad/Q support-magic bindings are retired when this integration is
complete.

Below the grid, show one compact line such as `Armor 0 · Weight 8/300`.
Current armor remains 0 because equipped defenses are not yet applied to
combat. Carry weight includes stack quantities and tools. Current prototype defaults are
1 kg per item unit, 2 kg per carried-tool profile, and a 300 kg displayed
capacity. `FKalmalaItemDefinition::WeightKg` and
`FKalmalaToolProfileData::WeightKg` are optional validated catalogue values;
tool profiles default to 2 kg when omitted. Capacity is a component class
default. These are presentation metrics; this
increment introduces no encumbrance penalty or weight-based transaction
rejection. Armor items, equipment rules, and final weight balance require
separate gameplay work.

## Player inventory layout revision — 2026-10-11

This user-directed revision supersedes the shared backplate and generic player
crafting list in the historical M13 handoff. Inventory and Crafting are separate
owner-local viewport widgets, anchored top-left and top-right with 16 logical
pixel margins. They retain one modal lifetime and cursor/input restoration.
Each panel directly uses the original Inventory wood texture in standard
contrast. Inventory has no title, and player Crafting hides its title and
introduction. Tab is handled before Slate focus navigation in either panel,
including the recipe search field, and closes both panels.

Player Crafting exposes only Building Hammer, costing 3 Wood + 1 Stone, one
hammer per request with no station prerequisite. It creates the existing
ConstructionHammer carried-tool record at full durability rather than an
inventory material stack. The server validates recipe/batch, ownership,
materials and capacity, rejects duplicate carried hammers, and persists the
tool and material payment together with rollback on save failure. Starter
hammer grants remain in effect. Cooking recipes appear only in matching cooking
station views and retain existing server station/heat requirements. No saved
schema, new tool identity, or client gameplay authority is introduced.

The subsequent user-directed presentation revision uses a compact search field
and adjacent x clear button. Player crafting has a vertical recipe list on the
left and a larger detail column on the right: large icon, centred item title,
description, authored tool durability, icon/amount costs and a bottom Craft
button. The legacy grid markers, numbered selection summary, result/quantity/
unavailable text, Favorites/order controls and Previous/Next buttons are not
part of this player view. Missing quantities use colour plus cost tooltips;
Craft is disabled for insufficient materials, duplicate hammer ownership or
capacity failure, including keyboard/controller activation. Station views
retain their existing layout. No stats or additional player recipes are invented.

Shared textured panel brushes explicitly use FullColor image type, actual
texture dimensions and white tint. Converting a rounded fallback to Image
without resetting its NoImage type/dark fill could suppress or obscure the
wood. This correction applies to Inventory, player Crafting and Settings tabs;
high contrast/missing assets retain their solid fallback and Escape home retains
its expressly transparent surface.

The following layout/loading correction removes the armor/weight summary from
Inventory entirely; those values are reserved for a later location. Its grid
height is exactly four tenths of the content width, without the former extra
24-pixel subtraction or reserved scrollbar space. Player Crafting's overlay
children fill their allotted viewport area, so the drawn wood panel reaches the
right anchor rather than stopping at a narrow desired-content width. Its size
is 52% of logical viewport width capped at 860, with height capped at 760 and
16-pixel outer margins. Warm translucent framed list/detail sections separate
the two columns; high contrast uses black sections with white borders.

Runtime theme loading reads the project default file if the custom ini fails
to resolve, supplies bundled Inventory/Build/Options paths when absent, and
loads object paths through the soft-object loader without a disk-existence
preflight. The full /Game/Kalmala/UI directory is explicitly cooked, including
panel packages that were previously outside the icon-only cook directories.
Configured valid custom image paths still take precedence. Actual view-package
loading and fill alignment now have regression assertions; earlier brush-only
checks did not prove that the image was supplied or packaged.

## M13 inventory crafting companion

At the standard viewport, the companion's item-list frame and search field
are 256 units wide; the selected-item frame is 384 units wide. The panel totals
676 units with its 12-unit gutter and 12-unit outer padding. The columns retain
their 2:3 ratio when the available viewport constrains the panel. Station views
retain their existing column proportions and search-field width.

The 2026-10-10 UI request extends Inventory's existing Tab / I modal. Opening
it shows the same 10x4 inventory on the left and the standard player crafting
list and selected-recipe details on the right. The right panel reads the
existing owner-visible recipe and ingredient presentation; crafting continues
through the existing server-validated action. Station-specific service views
and the separate Build menu keep their current contexts. After implementation,
the presentation rules below supersede the selected-detail and scroll
presentation described earlier in this handoff; the grid and authority
contracts remain in effect.

On open, the inventory panel slides down from above and the crafting panel
slides in from the right. Both use the existing options-opening duration and
easing by default, and both appear immediately when opening animation is
disabled or the owner's Reduced motion preference is enabled. The panels are
one owner-local modal: they share existing close, cursor/input restoration, and
keyboard/controller focus behavior. Layout, focus, reduced motion, transition
start/interruption/completion, and recipe-action visibility are part of M13's
rendered host/client acceptance at supported viewport and accessibility
settings.

Keep the inventory's always-visible summary to the compact armor/weight line.
Do not show the slot-count line, a `Selected:` heading, control instructions,
or a persistent full-width item-detail card. Hovering an occupied cell opens a
nearby, dark, opaque detail tooltip with the item name, description and any
existing authored stats arranged as tidy, separate lines. Keyboard/controller
focus or selection on an occupied cell shows the same contextual details so
the information remains reachable without a pointer. Existing item actions
remain available only for applicable items and retain their current
server-validated path.

One original Kalmala wood texture spans the complete combined Inventory and
Crafting backplate, including the narrow inset and gap around and between the
panes. The Inventory pane and Crafting pane each use an opaque solid theme
fill. Inventory cells and crafting cards use opaque, slightly darker brown
rounded surfaces with a few UI units of gap and icon inset. Keep canonical
icons, quantities, condition bars, focus states and selected states legible
over those fills. Existing high-contrast behavior suppresses decorative wood
art and uses opaque black accessibility surfaces, white text and borders.

This is a UI presentation change only. It adds no recipe, crafting outcome,
RPC, replicated field, gameplay authority, or saved-data field. Tooltip stats
are limited to existing catalogue/tool data; this task adds no combat-stat
fields such as Pierce, Poison or Knockback.

## Authority and compatibility

The server reconciles trusted item stacks and tool records into one runtime
cell array, retaining valid positions and removing depleted identities. Cell
positions and active identity replicate with `COND_OwnerOnly`. A move RPC
carries only source/target indices and their expected identities: bounds,
current server ownership, stale contents, and request cadence are checked
before swapping. It cannot grant, delete, split, duplicate, repair, or author
quantities. Hotbar intent carries only an index in 0–9; the server derives its
identity and validates food through the existing transaction path.

Wearable gear lives in a separate owner-only replicated array and remains
session state. `ServerEquipGear` accepts only an item GUID; the server resolves
the carried stack, requires a known non-stackable wearable, and derives its
slot from the catalogue. `ServerUnequipGear` accepts a bounded gear-slot enum.
Both transactions validate the full inventory/gear snapshot and carried-tool
identity set before publishing either array. Equipping moves the exact physical
item GUID out of its pack stack and into its slot; replacing a slot returns the
displaced item with its original GUID, using the cell freed by the incoming
gear. Unequipping fails without changing state if no pack cell is available.
Equipped identities never enter the forty-cell layout. The Inventory popup
displays current owner-only wearable names and submits exact item GUID or slot
requests. An owner-only client result reports accepted or rejected actions.
Main Hand is an owner-private equipped tool GUID referencing an existing grid
cell. All carried tools, including the held tool, count toward forty cells.
Equipping, switching and unequipping preserve every cell and physical identity;
there is no extra hand storage cell. Pressing a tool's number toggles that exact
copy on/off. The equipped cell receives a subtle blue fill in the popup and HUD;
duplicate copies are distinguished by GUID. Moving the equipped tool moves its
highlight. Empty numbers do nothing; food/support activation retains the hand.
Ordinary item selection clears the hand without needing free pack space.
Server validation of living owner, unique identity, catalogue level/condition,
source ownership, collisions and complete candidate capacity remains in force.
Bow equip returns a worn shield only when a free grid cell exists. Equipping a
shield clears Bow held state while leaving its grid cell intact. Worn gear still
occupies its separate six slots. Tools retain condition, progression, combat,
harvesting, building and existing saved records. Equipped state and arrangement
remain session-only; replication stays owner-only. No save fields or versions change.

Live grant, exchange, storage withdrawal and tool progression account for
both tools and stacks within forty cells; equipped wearables stay outside that
grid. The standard Chest keeps sixteen shared stacks; Reinforced Chest uses
twenty-four with the same per-item limits and owner-only GUID transfer path.
Storage schema 3 records the placed buildable identity so saved capacity is
validated against the right chest. Older development storage schemas are
rejected without rewrite under the pre-release save policy. Grid arrangement
and active selection are session state; reconnects rebuild arrangement from
available holdings. Existing item and tool persistence limitations still apply.

The forty-cell and companion-panel contracts supersede older sixteen-slot player pack, separate toolbelt, searchable player-grid, and persistent item-detail panel descriptions. Legacy station/chest inspectors remain scoped to their respective services. M13 companion and scroll handoffs are implementation reports, not fresh integrated rendered acceptance.

## Shared UI theme


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

`ItemIconTileSize`, `ItemIconTileMargin`, and `ItemIconTileFill` define the shared
square used by item icons in Inventory, Build, and crafting views. The 4-unit
margin is the gap between neighboring tiles; each adjacent layout slot applies
half of it to its edge. The icon fills the 64-unit square. A labeled tile keeps
its text as a separate row beneath the same framed square; crafting material
quantities use this layout, while Build material quantities remain in their
tooltips.

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
| InventoryPanelImage | `/Game/Kalmala/UI/InventoryPanel.InventoryPanel` | Texture2D for separate Inventory and Crafting wood panels |
| BuildPanelImage | `/Game/Kalmala/UI/BuildPanel.BuildPanel` | Texture2D override for build/craft selection |
| WorldMapPanelImage | `/Game/Kalmala/UI/WorldMapPanel.WorldMapPanel` | Texture2D override for the expanded map shell |
| ButtonImage | `/Game/Kalmala/UI/StoneButton.StoneButton` | Shared transparent stone slab, imported at 160×53 Slate-oriented size so nine-slice margins preserve its chipped ends; high contrast uses geometric brushes |
| SliderThumbImage | `/Game/Kalmala/UI/StoneSliderThumb.StoneSliderThumb` | Separate irregular slate thumb image for sliders; high contrast uses a geometric thumb |
| OptionsDividerImage | `/Game/Kalmala/UI/OptionsDivider.OptionsDivider` | Transparent generated medieval knotwork ornament layered over the shared Options separator; suppressed in high contrast |
| EscapePanelImage | `/Game/Kalmala/UI/OptionsPanel.OptionsPanel` | Fallback source for empty option-tab overrides; M13 requests a transparent Escape home menu |
| VideoOptionsPanelImage | `/Game/Kalmala/UI/OptionsPanel.OptionsPanel` | Legacy Video tab key retained for config compatibility; M13 Options groups share InventoryPanelImage |
| AudioOptionsPanelImage | `/Game/Kalmala/UI/OptionsPanel.OptionsPanel` | Legacy Audio tab key retained for config compatibility; M13 Options groups share InventoryPanelImage |
| ControlsOptionsPanelImage | `/Game/Kalmala/UI/OptionsPanel.OptionsPanel` | Legacy Controls tab key retained for config compatibility; M13 Options groups share InventoryPanelImage |
| SettingsOptionsPanelImage | `/Game/Kalmala/UI/OptionsPanel.OptionsPanel` | Legacy Settings tab key retained for config compatibility; M13 Options groups share InventoryPanelImage |
| OutlineSize | 0 | 0–3, rounded; black text outline |
| BorderWidth / CornerRadius | 1 / 3 | 0–3 / 0–12; geometric panels and button states |
| BorderColor | (0.35, 0.45, 0.42, 1) | Linear RGBA 0–1 |
| ButtonNormal | (0.11, 0.16, 0.19, 1) | Linear RGBA 0–1 |
| ButtonHovered | (0.18, 0.25, 0.28, 1) | Linear RGBA 0–1 |
| ButtonPressed | (0.07, 0.10, 0.12, 1) | Linear RGBA 0–1 |
| ButtonDisabled | (0.08, 0.08, 0.08, 1) | Linear RGBA 0–1 |
| ListViewRowNormal / ListViewRowHovered / ListViewRowSelected / ListViewRowDisabled | warm translucent row fills | Linear RGBA 0–1; crafting item-selection rows |
| ListViewRowBorderColor | (0.45, 0.27, 0.12, 1) | Linear RGBA; selected/focused crafting rows |
| ListViewRowPadding / ListViewRowCornerRadius / ListViewRowBorderWidth | 5 / 2 / 0 | 0–24 / 0–12 / 0–3 logical units; list rows |
| RequirementIconBorderColor / RequirementIconBorderWidth | (0.45, 0.27, 0.12, 1) / 1 | Linear RGBA / 0–4 logical units; crafting/build material icons |
| ItemIconTileSize / ItemIconTileMargin | 64 / 4 | 24–96 / 0–16 logical units; shared square size and inter-tile gap |
| ItemIconTileFill | CSS `rgba(159, 86, 28, 0.5)` (stored as linear `(0.346704, 0.093059, 0.011612, 0.5)`) | Shared Inventory, Build, and crafting tile fill; high contrast replaces it with the black panel fill |
| SlotPadding | 3 | 0–16; selectable-panel content |
| ButtonPadding | 9 | 0–24; shared button content, pressed adds one unit |
| IconWidth / IconHeight | 64 / 42 | 24–96; existing support glyph slots |
| AnimateScrolling / ScrollSpeed | False / 15 | Boolean / 1–60; Controls wheel scrolling |
| AnimateOptionsOpening | True | Boolean; set False for an instant/reduced-motion opening |
| OptionsOpeningDuration | 0.18 seconds | 0–0.8 seconds; zero selects the instant path |
| OptionsOpeningTravel | 32 logical units | 0–96; zero selects the instant path |
| OptionsOpeningEasing | `EaseOutCubic` | `EaseOutCubic`, `EaseOutQuad`, or `Linear` |
| FavoriteMarkerStyle | `Both` | `Star`, `Border`, or `Both`; local recipe/build cards |
| FavoriteMarkerBorderWidth | 2 logical units | 1–4; Favorite frame width when enabled |
| FavoriteMarkerColor | (0.98, 0.78, 0.20, 1) | Linear RGBA; Favorite star/frame |
| RankGoldColor / RankSilverColor / RankBronzeColor | gold / silver / bronze tones | Linear RGBA; top-three usage markers |
| RecentMarkerColor | (0.38, 0.82, 0.90, 1) | Linear RGBA; Recent clock badge |

Asset keys accept only valid `/Game/Package.Asset` object paths shorter than
180 characters. Empty, invalid, missing, or wrong-type objects retain the
built-in font or geometric panel/button brushes. Offline bitmap fonts also fall
back safely. The shared stone button image uses an alpha silhouette with
nine-slice margins so its beveled irregular edges remain visible at different
button sizes; its 160×53 import size keeps the 0.16/0.18 UV margins near 26×10
Slate units instead of hundreds of units. Hover, press, selected, and disabled
states tint the same texture.
Slider thumbs use their own high-resolution round slate puck with a smooth
continuous silhouette, restrained bevel, and fine low-contrast grain. The thin,
softly rounded recessed track has no standard-mode outline. High contrast retains a
white track outline. Audio slider pointer drags stay continuous, with 0.5%
keyboard/controller steps. Buttons in vertical, horizontal, scroll, overlay,
grid, and canvas slots use content-driven width instead of filling the parent;
horizontal slider tracks remain fill-sized within their rows.
High contrast suppresses decorative images and retains the opaque geometric
button surface and visible focus outline. A panel image replaces the geometric
panel border; high contrast ignores it, uses a black-backed panel and white
border/text, and forces button borders to at least one unit. High contrast uses
neutral button fills; pressed padding and Unreal's existing keyboard focus
indicator remain available.
Crafting item-selection rows use the dedicated list-view theme rather than the
shared stone button surface. Item names stay left-aligned beside their icons,
and selection uses the row treatment without a leading marker; pointer hover,
keyboard/controller focus, disabled state, and high-contrast borders remain
distinct. Requirement icons use their own theme-configurable frame, with a
white border over the black high-contrast surface.
Accessibility text scale still takes precedence over theme size. Button
activation, menu navigation, modal ownership, gameplay authority, and saved
data schemas remain unchanged; audio values keep their existing local keys.

Recipe/build activity cards reserve separate right-aligned rows for Rank and
Recent, followed by the Favorite cue at the lower right, so a single card can
show all three without overlap. Favorite style selects a star, an outer frame,
or both; Favorite text is always present. Rank markers name Gold/Silver/Bronze
and their rank. Recent uses a clock glyph and the word Recent. Local high
contrast forces marker text and the Favorite frame to white; static labels
remain visible under reduced motion. Marker changes are immediate and do not
animate.

The view-specific image keys override `PanelImage` only for their named view.
The inventory, build, and expanded-map textures are original Kalmala artwork
imported from `Content/Kalmala/UI/Source/` into `/Game/Kalmala/UI`; missing or
invalid view paths keep the geometric fallback. The Inventory texture covers
each independent Inventory and Crafting panel directly; no opaque pane fill
covers the wood. High contrast suppresses the decorative wood.

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

Hover, keyboard/controller focus, selection, and disabled states share theme brushes and non-colour cues. Decorative images/animation yield to local high contrast and reduced motion. The Forge Upgrade card shows the Bronze Axe and a level icon for the target level, then every nonzero authored item stat across the current and target levels: maximum durability and each damage-bonus channel. Zero-valued channels are omitted. The wider right detail frame keeps this comparison at the top and places the material tiles and Upgrade action together at the bottom; the left frame retains prerequisite and availability text. Stat increases and decreases use the shared `LevelUpColor` and `LevelDownColor` theme values; unchanged values use the neutral text color. The compact card is owner-only and submits no request.

## Status and catalogue icons

The active-only owner HUD is transparent, non-focusable, and hit-test-invisible, beside the minimap. Stable order is Wet, meal, Hot, Cold, current support, then Storm. Known active effects appear once; expired entries disappear immediately and empty groups collapse. Finite player/support timers use authoritative remaining time or synchronized server expiry. Hot/Cold/Storm have no countdown. Storm requires the existing 0.65 threshold and a current server weather interval. Start/refresh cues are local; obsolete M11 Ended rows must not survive M12's active-only contract.

Each raster fills its scaled 64x64 box. Status layout wraps within a 384-unit row cap and preserves minimap/support separation. Visible names are omitted but hidden accessible labels remain. The Escape home menu contains Options and Quit; it does not open a separate status/weather detail page.

The earlier accepted icon counts (48 baseline identities, then Stone Spear, six Bronze/Iron weapons, and three maces) are historical. The current catalogue ledger contains 72 imported item textures, including the six formerly unmapped Bronze/Copper/Tin bars and Copper/Tin/Iron ores. The global icon automation traverses every validated item, tool profile, and recipe output and checks its runtime mapping and imported texture. Views reuse canonical IDs and one image per identity; aliases and recipes do not get duplicate art. Unknown IDs retain a question-mark fallback; missing imported textures retain existing vector fallbacks. Current runtime/catalogue additions must receive explicit coverage rather than assuming historical counts include them.

Original PNGs live in Content/Kalmala/UI/Source/IconOriginals and prepared PNGs in Source/Icons below that UI directory. Catalogue and status importers prepare transparent 64x64 artwork and import UI textures with sRGB, no mipmaps, and no streaming. Bronze and Iron weapon icons use catalogue batch E; batch D contains the Stone Spear, batch I the archery items, and batch J the six ore/alloy materials. Code constructs icon paths dynamically; both catalogue/status directories must be included in package cooks. The CSV manifests are machine inputs used by import/validation scripts and must remain available with those tools. Material artwork and exact generation prompts are recorded in `docs/icon-provenance.md#material-icons`.

## Item and recipe details

Details use canonical item/tool name, description, image, quantities, condition/level, authored weapon damage/type, and food stats. Empty/removed selections clear stale details; unknown identities use an explicit fallback. M13 Inventory uses hover/focus tooltips and compact armor/weight, superseding its old persistent detail card.

Ingredients show owner-held quantities against one-batch requirements, with explicit Enough/Missing or Waiting for pack. Direct builds use raw material costs. Requirement blocks describe applicable station alternatives, reusable tools, hearth heat, and existing placement limits; sufficient ingredients never promise server acceptance.

## M12 construction detail template

Direct construction details use the existing result description once, followed
by the existing owner-local ingredient counts and a compact Build requirements
block. It names the carried Construction Hammer state, clear/dry/gently sloped
placement, and the hearth's one-of-four raw-fuel requirement and 60-second
starting duration. The current first blocker appears once when present; a
ready recipe does not add success, skill, unlock, request, or rejection prose.
The result name and material totals stay in their existing header and ingredient
rows. Catalogue descriptions and all recipe, cost, placement, authority, and
save behavior remain unchanged.

The `Kalmala.UI.Crafting.Requirements` contract covers each direct build's
placement and single-blocker copy, the hearth fuel summary, pending/present/
missing hammer states, and omission of generic boilerplate. The rendered
Crafting presentation gate checks the selected hearth copy and ensures
its old repeated costs, output label, skill list, and request prose are absent.
Build, full automation, and rendered verification remain deferred to M12's
milestone-final run.

## M12 food and general recipe detail template

Non-construction recipe details show the canonical output description once,
the existing owner-local ingredient rows, and a compact requirement summary.
The summary names the actual result count per batch, one batch per menu press,
and the catalogue's supported maximum batch when it is greater than one. It
lists only real station alternatives, the cooking-heat rule for rack/cauldron/
pan recipes, and a reusable tool when the recipe requires one. The first
current availability blocker appears once; a ready recipe has no synthetic
success message. Recipes without station or reusable-tool requirements get no
empty-state lines, and the selected details omit unrelated skill lists,
no-lock claims, output-stack limits, request/rejection boilerplate, and repeated
ingredient totals. Catalogue prose and server recipe validation remain intact.

`Kalmala.UI.Crafting.Requirements` checks result/quantity coverage, station and
heat requirements, reusable-tool text, omission of generic boilerplate and
single-blocker behavior. `Kalmala.Gameplay.Food.CookingStationHeat` continues
to verify the server heat gate and confirms the detail description comes from
the canonical result item. The rendered Crafting review checks the live
cooking result, ingredients, heat line and blocker count for both owners. Build,
full automation and rendered checks remain deferred to M12's milestone-final
run.

Catalogue prose is authored manually in the category files under Content/Data/GameCatalogues; tool prose comes from tool definitions. Preserve intentional flavor. Player-facing names omit internal Kit suffixes while stable runtime/save aliases remain unchanged. Retired Fuel, Timber, ConstructionSupply, RaisedStorage and Smoke Frame names must not reappear as active content. The copy audit and image-generation batch diaries are historical; current data and lookup code define live copy/identity.

## Menu browsing

Recipe, Build, station, and Chest inspector filters are owner-local presentation. Search trims to 64 characters and matches visible names case-insensitively, not hidden IDs/descriptions. Sorting works on copies, keeps canonical tie-breakers and original owner order, and never changes inventory or save order. Preserve a still-visible selection, otherwise select the first result; empty results clear details/actions and expose recovery controls. Text editing owns cursor keys.

Local per-menu memory keeps query/category/sort/selection/scroll in the widget session and clamps offsets after layout/viewport changes. Station scopes remain separate. Build categories contain only supported structural pieces, stations, and camp utilities; production remains in the corresponding station service. The expanded map retains its zoom and filter selection for the session and opens centred on the owning player. Personal pins and exploration use only their existing local save owners.

## M13 requested Build-menu refresh

The 2026-10-10 M13 request supersedes the standalone Build menu's older entry,
card, search-memory, and lighting-button behavior above. With the Construction
Hammer active in gameplay, right-click opens Build; no separate action binding
is provided. Build cards display only their
original Kalmala icon. Hovering or focusing a card shows its name, short
description, applicable required-station icon, and existing material costs and
owner-visible carried counts in a tidy detail area below the menu. Existing
category/filter and grouping controls move into the top row as icon-only
controls, with no visible text in the menu header. Controls retain accessible
names and useful hover/focus labels.

Left-clicking a build card (or activating its focused entry) closes Build and
enters the existing local placement mode for that canonical buildable; selection
does not place it. Right-clicking a card toggles its existing local Favorite
state and does not trigger menu entry, close, or placement. The separate **Add
to favorites**, **Build / place selected**, and **Light hearth** actions are
removed. Existing placement controls and server validation remain in effect.
The build search does not receive focus on open; clear its query on menu close
and when placement mode starts. Other existing per-menu browse memory may remain.

Lighting moves to the Campfire's direct **Interact** action. When a visible,
same-world Campfire is unlit and eligible under existing server rules, Interact
shows **Light Campfire** and reuses the existing payload-free lighting intent.
Otherwise, the existing **Add fuel** interaction remains available when
applicable. The server still validates target visibility, access, range, fuel,
and fuel condition; this request adds no new client authority, recipe, cost, or
saved state. The attached screenshot guides layout only; use existing Kalmala
icons and UI materials.

M13 rendered/input verification replaces the earlier retained-relight-button
expectation: confirm those three named buttons are absent; test right-click
menu entry and Favorite toggling, left-click/focused entry into placement mode,
search clearing and initial focus, and direct Light Campfire interaction on
eligible host/client-owned fires. Existing server rejection/payment checks and
placement validation remain required.

This M13 request remains governed by the live backlog; do not infer completion from its specification.

The 2026-10-11 standalone Build presentation update supersedes the earlier
detail-area-under-menu wording above. Build uses the same original wood
backplate as Inventory. Its icon grid uses ten left-aligned columns of 64×64
catalogue tiles with the shared 4-unit gap, rather than stretching four columns
across the menu. The Build card border paints the warm background behind each
icon; the icon itself is drawn above it. Catalogue art keeps its aspect
ratio and is scaled slightly within the fixed slot to compensate for transparent
pixels around the artwork. The main Build window ends after the item grid and its
browse controls; it does not show selected-item details, ingredient rows,
placement preview, status text, or action buttons below the grid. A separate,
compact wood panel sits below the main window and shows the selected item's
requirements. Material costs follow Inventory Crafting's icon presentation but
display only material icons; hovering an icon reveals the owned and required
counts. Build cards use the same warm brown overlay as
Inventory, and cards missing any requirement receive a subtle gray tint and
subdued icons. The requirements panel tooltip retains hammer, placement,
station, fuel, and availability conditions. Clicking a build card still enters
the existing local placement mode, and the server continues to validate
placement and costs.

## Owner notifications


The first M11 child adds passive icon/text feedback for skill **levels**, not
each experience award. The local-player subsystem reads only its local
controller's owning pawn for detailed progression and accepted item/discovery
feedback. It never reads peer-private presentation or changes gameplay state.
Gameplay's existing server-owned ledgers and owner-only replication remain the
authority boundary.

The first complete valid six-skill snapshot is a silent baseline. Missing,
partial, duplicate or invalid snapshots cannot advance that baseline. A later
level increase displays the skill name and reached level with original shared
line art. Repeated replication and XP changes within a level do not renew or
replay feedback. Pawn replacement clears the queue and baseline; a level
rollback silently rebaselines all skills. Reconnect therefore does not announce
existing levels. These are transient session notices, with no save-schema change.

At most three rows are active. A further increase of the same active skill
updates its reached level and renews its lifetime. New skills evict the oldest
row at capacity; simultaneous increases follow canonical skill order. Expired
rows are removed and never reconstructed from unchanged snapshots.
`NotificationLifetime` in `DefaultKalmalaTheme.ini` defaults to four seconds;
theme values outside 1–10 seconds fall back to the default. Runtime queue
durations are also bounded. No entrance, exit or scrolling animation occurs,
including reduced-motion use. Text scale and high contrast use the shared theme.

The widget cannot focus or hit-test. Its bottom-right anchor is 280 logical
pixels above the viewport bottom, with a maximum width of 320; this clears the
centered arrival prompt at the supported 1024x768/150% layout.
It collapses while the controller ignores movement for a modal; timers continue
so closing a modal does not replay expired messages. Tiny/unavailable viewports
collapse the widget. The combined acceptance below checks both supported
layouts and the existing movement-ignore modal signal; it does not open a
gameplay menu.

## Accepted item gains

Accepted server inventory publications send a reliable client-only
`ClientAcceptedGain` receipt to the owning connection. Direct grants record the
accepted quantity. Exchanges, validated candidate commits and successful
storage withdrawals record each positive net item increase after publication;
consumption, deposits, unchanged commits, rejected/stale candidates and failed
storage writes produce no gain. This feedback adds no server mutation RPC and
cannot mint or consume inventory. Receipts do not depend on a later inventory
snapshot, so gain followed by consumption between UI refreshes still announces
the accepted gain. A same-item exchange announces its positive **net** increase,
not its gross output. Carried-tool records are a separate inventory contract and
are not newly announced by this child.

The owner stores only the latest 32 transient receipts with locally monotonic
sequence numbers. Each receipt quantity is the total positive net gain for one
item across the inventory, so it may exceed a single stack's limit up to that
item's stack limit multiplied by the inventory slot count. The first observed
buffer silently establishes a baseline, including already-delivered receipts
on pawn attachment. Pawn replacement
clears it through the existing session reset. Refresh, expiry, modal close and
unchanged buffers never replay receipts. A UI polling gap exceeding 32 receipts
drops older notices; it cannot change inventory. There is no receipt persistence
or save-schema change. Reliable delivery follows the existing pawn's owning
connection; there is no multicast or peer detail replication.

New item receipts share the existing three-row queue with skill notices. Same
canonical item gains coalesce into a summed `Gained N Item` label and renew the
bounded lifetime; summed labels saturate at int32 maximum. Overflow evicts the
oldest row. Canonical catalogue names and original catalogue icons are used.
Late initial skill replication does not clear already accepted item feedback.

Run `Kalmala.UI.Notifications.ItemGains` and `.SkillLevels`,
`Kalmala.Gameplay.Inventory.NetworkContract`, `Kalmala.Gameplay.Crafting.Transactions`
and `Kalmala.UI.Theme.LocalPresentation` after the affected editor build.
`Scripts/Tests/Inventory/Inventory.ps1` additionally requires live owning-client receipt
presence/bounds and empty remote receipt buffers, alongside existing inventory
privacy and rejected-client-mutation gates. Rendered combined notification
placement and real reconnect pixels remain final-child acceptance scope.

## Accepted discoveries

The local notification subsystem also observes the owning pawn's existing
owner-only discovery acknowledgement. Its feedback serial establishes a silent
baseline on first observation and after pawn replacement, so joining or
reconnecting with an existing acknowledgement does not replay it. A newer
`LandmarkFound`, `ScrollFound`, `AlreadyFound`, or `Unavailable` serial adds the
server-provided acknowledgement label as a passive discovery row. Empty labels
use concise outcome-specific fallbacks, and all labels are trimmed and bounded
to 48 characters. An unchanged serial does not renew the row, and expired
feedback is never reconstructed from the last replicated label.

Discovery rows share the three-row, theme-timed queue and use an original
monochrome inspection glyph. Labels are trimmed and bounded to 48 characters,
with a safe fallback for an empty accepted label. They are transient, local
presentation: no new request, claim, reward, persistence, RPC, peer query, or
saved-data field was added. Accepted discovery rewards may also produce an
existing item-gain notice for the actual inventory increase.

Run `Kalmala.UI.Notifications.Discoveries` with the affected editor build. It
covers silent initial/reconnect baselines, accepted/already-found/unavailable
acknowledgements, unchanged refresh, expiry, label bounds, passive text and
owner-local queue behavior.

## Combat and support action results

The local queue also observes the owner's existing owner-only combat and
support feedback serials. Initial attachment and pawn replacement silently
baseline both values; a newer serial adds one short text-and-icon notice for
`Hit confirmed`, `Defeated`, `Attack unavailable`, `Support accepted`, or
`Support unavailable`. Repeated snapshots do not renew a notice, serial
rollback silently rebaselines, and the combat notice contains no target name or
identity. Discovery acknowledgements remain their own existing notice source.

These events use the same three-row bound, theme lifetime, passive presentation,
and modal collapse as skill, item-gain, and discovery notices. The notification
subsystem reads only the local owning pawn. The server-owned components still
validate and publish every result; this presentation adds no request, RPC,
replicated field, save data, or gameplay mutation. The Inventory menu no longer
duplicates combat, support, or discovery result text.

`Kalmala.UI.Notifications.CombinedPresentation` checks silent action baselines,
serial deduplication, concise text, expiry, and absence of hidden target
identity. The existing rendered capture fixture continues to review the
skill/item/discovery combination; action-result rendering remains part of the
M12 milestone-final HUD matrix.

## Recipe and build activity

## Ownership and lifetime

Favorites, usage counts, rank results, and Recent shortcuts are local
presentation state owned by the `ULocalPlayer` crafting UI subsystem. They do
not change catalogue definitions, availability, server transactions, gameplay
state, replicated properties, or world/player save schemas. They survive menu
close/reopen and pawn replacement while that local-player subsystem exists;
they are cleared when it is destroyed and are not persisted across process
restarts. The local UI never infers an accepted action from selection, a button
press, inventory appearance alone, another peer, or a replayed snapshot.

Bookmarks use canonical active `RecipeId` values. Unknown or retired IDs are
ignored and pruned. The set is bounded by the smaller of 256 entries and the
current recipe catalogue size. The Favorites category intersects the existing
station scope and search query and keeps the existing unavailable-state text;
it does not add hidden entries or change recipe ordering. Removing the selected
bookmark from Favorites follows the menu's established selection fallback,
and an empty result disables the existing action.

## Accepted-action history contract

The crafting component emits owner-only accepted-action receipts only after an
existing server-side craft or placement has committed. Each receipt carries a
monotonic sequence for that character component, a canonical recipe/menu ID,
and one of the three action kinds below. The server retains a rolling maximum
of 64 receipts so multiple accepted actions can survive property replication
coalescing; the 64-bit sequence saturates instead of wrapping. The local
subsystem silently establishes a baseline on first
observation and pawn replacement, then consumes each newer sequence once. A
component replacement does not reset the local player's accumulated history.
Rejected, failed, stale, duplicate, replayed, selected, previewed, or merely
requested actions do not update history. The receipt reports an outcome
already chosen by the server; it cannot authorize a transaction or supply a
reward. A build output with no unique current recipe/menu ID is omitted.

Accepted actions have three disjoint buckets:

- `BuiltPiece`: successful direct placement of a construction piece.
- `CookedRecipe`: successful recipe action whose existing catalogue skill is
  Cooking.
- `CraftedItem`: successful non-cooking recipe action. Cooking never enters
  this bucket.

Counters are local, non-negative, saturating integers keyed to current
canonical menu IDs. Records for IDs absent from the active catalogue are
pruned, and unused entries with count zero receive no rank. Each bucket ranks
at most three non-zero entries by descending accepted-action count, then by
`RecipeId.LexicalLess` for a locale-independent deterministic tie. The displayed
medals are gold, silver, and bronze in that order; ties do not share a medal.

Recent state holds at most one latest accepted ID per bucket. A new accepted
build replaces only the previous built piece; a new cooking action replaces
only the previous cooked recipe; a new other-crafting action replaces only
the previous crafted item. The Favorites category includes these current
Recent shortcuts even when they were not manually bookmarked. A canonical ID
appearing in multiple sources is shown once, with separate text markers so
Favorite, Rank, and Recent remain distinguishable. Ordinary recipe/build
cards also mark their matching Recent entry.

The local-player subsystem exposes bucket count, rank, and Recent queries for
the menu presentation. It validates that receipt IDs still resolve in the
current catalogue and that the receipt kind agrees with the existing Cooking
classification or build-menu membership. Each count map holds at most one entry
per current recipe ID per bucket and saturates at the unsigned 32-bit maximum.

## Presentation contract

Favorites, rank, and Recent remain distinguishable without colour. The theme
configures Favorite treatment as a bottom-right star, border, or both; rank
treatment uses a medal-coloured label with `Rank` text; and Recent uses a clock
badge plus `Recent` text. The default Favorite treatment is both star and
frame. Cards reserve separate right-aligned Rank and Recent rows, then place
Favorite at the lower right; all three labels retain their meaning and do not
overlap. Reduced motion keeps marker changes immediate and static; it does not
suppress an essential marker. Existing text scale, contrast, focus, selection,
and modal input behavior continue to apply.

## Onboarding reference


**M12 runtime status:** the local tutorial presenter is disabled. No arrival or
contextual help card appears during fresh start, pawn transition, or reconnect,
and no replacement banner is shown. The prompt rules and beat matrix below are
retained as a route-free design reference if onboarding returns in a later
approved increment.

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
- Prompts describe choices rather than objectives and use concise action names;
  they must not select a destination, camp site, creature, reward, recipe
  outcome, or support target.
- Input-binding labels are not displayed in prompts. Key/button names and
  control legends stay in Options > Controls and Options > Controller.
  Prompt text and non-colour
  icon/shape cues remain available, and a player can dismiss, revisit, or
  ignore all prompts without losing normal movement or menu access.
- Prompt timing is local presentation timing. It cannot pause the server, grant
  an interaction, alter stamina or Wet, confirm a hit, complete a discovery, or
  advance a reward.

## Route-free beat matrix

| Beat | Local trigger | Player-facing prompt | Required boundary |
| --- | --- | --- | --- |
| Arrive | Fresh local pawn becomes controllable | “Move and look around. Jump or sprint when the terrain calls for it.” | Does not name a direction, coordinate, route, or destination. |
| Interact | The player first receives normal interaction focus on a visible actor | “Face a nearby usable object and choose its displayed action.” | The prompt reflects only the existing local focus; the server still validates the request and outcome. |
| Gather | The player has a visible, usable harvest node in normal range | “Gather from the visible node. Your pack shows what was accepted.” | Does not reveal distant nodes, quantities, depleted state, or a preferred resource. |
| Prepare | The player opens the existing camp crafting UI | “Choose what to make, then place it where the terrain and your materials allow.” | No fixed camp, construction transform, payment result, or shelter success is promised locally. |
| Weather | The local HUD already shows an active Wet or exposure-related cue | “Weather changes comfort and travel. Shelter, cover, and a lit hearth are options.” | Uses existing replicated presentation only; it never invents a local weather value or a guaranteed recovery. |
| Explore | The player leaves the immediate start area through normal movement | “Pick a heading and see what the generated land offers. The map is for orientation.” | Never points to an authored corridor, biome, encounter, discovery, or required return point. |
| Optional encounter | A relevant creature is already visible or the player has initiated the normal attack intent | “You can engage or move on. Attacks are committed by the server.” | Does not reveal hidden creatures, target selection, damage, cooldown, loot, or defeat outcome. |
| Discovery | A visible server-owned discovery is already interactable | “This discovery is optional. Interact normally if you want to investigate.” | Does not show undiscovered IDs, reward contents, private entitlement, or duplicate-claim state. |
| Support magic | The entitled player has already learned an effect and opens its existing local UI | “Support effects help an eligible ally or situation; choose a valid target when the UI allows.” | Learned effects, targets, durations, stamina, cooldowns, and execution remain server-owned; no effect deals direct damage. |
| Return | The player chooses to head back or remains near a visible camp | “You can return, shelter, use the hearth, store materials, or keep exploring.” | Return is a choice, not a quest completion, timer, route requirement, or persistence grant. |

The first three beats introduce movement, interaction context, and gathering.
The remaining beats are opportunistic: a player may encounter them in any
order, never encounter some of them, or dismiss them all. A fresh-player
walkthrough may therefore record which prompts were applicable rather than
requiring every prompt to appear.

## Presentation and accessibility acceptance

If onboarding returns in a later increment, a fresh local session must
demonstrate the following without developer commands or fixed fixture
coordinates:

1. Arrival teaches movement, jump, and sprint through plain action names, with
   no key/button legend.
2. A visible normal interaction can teach interaction and gathering without
   promising acceptance before the server response.
3. Camp, weather, exploration, optional encounter, discovery, support, and
   return prompts appear only when their local visible context exists.
4. Every prompt can be understood from text and shape/icon cues without colour
   or audio. Keyboard/controller mappings remain readable in Options >
   Keyboard & Mouse and Options > Controller, and prompts retain their full
   action text.
5. Dismissing or ignoring prompts leaves movement, interaction, combat intent,
   support UI, map access, and reconnect behaviour unchanged.
6. A two-player observation shows no prompt-driven replication, reward leak,
   client-selected outcome, or private discovery/learned-effect disclosure.

The former runtime presenter was a `ULocalPlayerSubsystem`. Its source remains
as a dormant design implementation, but its tick is disabled and it cannot
mount a card. The default F1/F2 dismiss/revisit mappings have been removed with
the gameplay prompts. The source-level route-free audit now also verifies that
the presenter remains inactive.

`Scripts/Tests/Presentation/OnboardingContract.ps1` validates the retained design
specification; it is not a runtime presentation test. `Scripts/Tests/Presentation/TutorialRouteFree.ps1`
checks the route-free source contract and confirms that the presenter is
disabled. `Scripts/Tests/Presentation/LocalInputContract.ps1` checks the current
keyboard/controller movement and interaction bindings. Fresh-player prompt
walkthrough and prompt rendering are not part of current runtime acceptance.

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


## Settings and accessibility


This document defines the local settings and accessibility surface for the
existing Kalmala settings shell. It covers presentation and input preferences
only; it does not add gameplay tuning, a replicated option, a server setting,
or a new save schema.

M11 places Wet, meal, exposure, support and weather status markers in one
transparent owner-local hotbar, with hidden accessible labels and existing
timers. The Escape home menu contains Options and Quit only. The optional feedback overlay retains nearby
hearth/construction context, with no duplicate active status rows.
Owner-local notifications now carry concise combat/support outcomes and
discovery acknowledgements; a separate text-plus-glyph strip shows the selected
support effect without control-binding labels. The colour-independent feedback
system covers Wet, hearth, construction, combat, discovery, and support: the
optional owner-only overlay shows nearby hearth/construction context while the
other cues use their dedicated text/icon HUD surfaces. See
`02-technical-architecture.md` for the status presentation contract and
`02-technical-architecture.md` for feedback rules.

Gameplay HUD and interaction prompts name the visible action without showing
key/button names or control legends. Inventory and build/crafting/repair/storage
views, the expanded map, status/detail views, and the Settings home screen follow
the same rule while keeping useful action/status names and existing focus
navigation. Options > Controls and Options > Controller are the only
player-facing views that display current mappings; prompt actions stay readable
as text and remain remappable.

## Existing shell and option groups

The existing local menu opens and closes with **Escape**, owns modal input while
open, and exposes the Options and Quit actions. Options follows the six-group
reference order: Gameplay, Keyboard & Mouse (shown as Controls), Controller,
Graphics, Audio, and Accessibility. Gameplay stays hidden while no candidate
option is selected. Existing Video controls move to Graphics; existing input
bindings split into Keyboard & Mouse and Controller. Resolution, V-Sync, window mode, and
render distance uses a 64–576 m local cull-distance cap in 32 m increments.

- **Audio:** local master, music, ambient, and interaction/combat feedback
  levels plus a mute path. Audio changes must have a readable text confirmation
  and must not be required to understand gameplay state.
- **Keyboard & Mouse / Controller:** local bindings for the existing movement,
  look, jump, sprint, interact, attack, map, settings, and craft
  actions. Each device page shows its own current labels, applies a change to
  only the owning local `UPlayerInput`, and provides restore-defaults without
  changing what the server validates.
- **Accessibility:** local text scale and contrast choices, together with the
  colour-independent feedback preference for the optional nearby hearth and
  construction text overlay. Combat/support outcomes, discovery notices, and
  selected-support text remain explicit in their separate HUD surfaces.

The Escape home menu remains transparent in standard contrast. All five visible
Options groups use the original `InventoryPanelImage` shared with the Inventory
backplate. Buttons use shared-theme hover, pressed, selected, and focus states.
High contrast suppresses the decorative image and keeps the opaque black panel,
white text, and focus borders. This presentation does not change settings,
input bindings, or modal ownership.

The Escape panel opens with a short theme-configured downward slide. Focus and
modal input ownership take effect immediately, and closing/reopening or
resizing during the animation does not delay gameplay actions or change menu
content. Theme configuration can disable the animation for an instant path, and
the local Reduced motion setting below also overrides decorative animation.

The M13 presentation request makes the main Escape home menu transparent over
gameplay, with a visible theme highlight for hovered and keyboard/controller-
focused options. High contrast must retain a clear non-colour focus cue. This
does not change the separate Options tabs or the menu's existing input ownership.

The later 2026-10-10 M13 Settings refresh request uses the same original wood
texture as the Inventory backplate and the shared theme's button treatment.
The Escape home list remains transparent. High contrast continues to replace
the decorative texture with an opaque black surface. A follow-up request asks
for the option groups and change controls to follow the attached references;
the candidate options and their current selection state are listed below.

## M13 Settings groups and reference-option checklist — 2026-10-10

Use the six reference group names: **Gameplay**, **Keyboard & Mouse**,
**Controller**, **Graphics**, **Audio**, and **Accessibility**. Move Kalmala's
existing Video options into Graphics, split existing Controls by device family,
and place existing Settings options in Accessibility. Audio remains Audio.
Gameplay currently has no documented Kalmala setting. Do not leave an empty
group visible if no Gameplay candidate is selected. Keep all existing Kalmala
settings available regardless of the candidate checks below.

The checkboxes are for the user to select candidate options for later M13
scope; checking one does **not** mean it has been implemented. New candidates
must have a Kalmala-supported behavior, use local configuration, and remain
reachable with keyboard/controller focus. Their default, range, and persistence
behavior should be specified when selected. Do not copy reference artwork.

### Gameplay candidates

- [ ] Language selector — previous/next arrows cycle available languages.
- [ ] Auto-run — on/off toggle.
- [ ] Attack towards look direction — on/off toggle.
- [ ] Show button hints — on/off toggle.
- [ ] Enable game hints — on/off toggle. (add but hide for now)
- [ ] Reduce background performance — on/off toggle.
- [ ] Enable Console — on/off toggle.
- [ ] Show build piece author — on/off toggle.
- [ ] Skip intro cinematic — on/off toggle. (add but hide for now)
- [ ] Auto-backup history — slider with a visible selected count.

### Keyboard & Mouse candidates

- [ ] Mouse sensitivity — slider with a visible percentage/value. [1% to 1000%]
- [ ] Invert mouse — on/off toggle.
- [ ] Direct keyboard/mouse rebinding — show one row per action and its current
  binding; activating a binding captures the next valid key or mouse input,
  Escape cancels capture, and Reset controls restores defaults. This replaces
  the current bounded-choice interaction if selected.
  - actions:
    - Attack
    - Secondary Attack
    - Block
    - Use (Action)
    - Jump
    - Run
    - Crouch
    - Dodge
    - Alternative Dodge
    - Holster Weapon
    - Hotbar keys for 1,2,3,4,5,6,7,8,9,0
    - Move keys Forward, Backward, Left, Right
    - Inventory
    - Alternative Inventory
    - Map
    - Map Zoom in
    - Map zoom out
    - Deconstruct (while holding hammer)
    - Alternative placement (while holding hammer, normal by attack key)
    - Prev snap point (in build mode)
    - Next snap point (in build mode)


### Controller candidates

- [ ] Gamepad enabled — on/off toggle.
- [ ] Swap triggers — on/off toggle.
- [ ] Invert camera X axis — on/off toggle.
- [ ] Invert camera Y axis — on/off toggle.
- [ ] Vibration strength — slider with a visible percentage/value.
- [ ] Controller sensitivity — slider with a visible percentage/value.
- [ ] Glyphs — selector; previous/next arrows cycle supported glyph styles. (xbox/ps/switch controller)
- Same re-bindable actions as above.

### Graphics candidates

- [ ] Resolution — selector cycles supported limits.
- [ ] Full screen - on/off toggle.
- [ ] 3D resolution limit — selector cycles supported limits.
- [ ] Upscaling method — selector cycles supported methods.
- [ ] Framerate limit — slider, including the supported Unlimited choice.
- [ ] Graphics preset — previous/next selector, including Custom when applicable.
- [ ] Vegetation quality — slider.
- [ ] Level of detail — slider.
- [ ] Particle lights — slider.
- [ ] Shadow quality — slider.
- [ ] Active point lights — slider.
- [ ] Active point light shadows — slider.
- [ ] SSAO — slider.
- [ ] Cloth quality — slider.
- [ ] Draw distance — slider with a readable value.
- [ ] Distant shadows — on/off toggle.
- [ ] Tessellation — on/off toggle.
- [ ] Bloom — on/off toggle.
- [ ] Depth of field — on/off toggle.
- [ ] Motion blur — on/off toggle.
- [ ] Chromatic aberration — on/off toggle.
- [ ] Sun shafts — on/off toggle.
- [ ] Soft particles — on/off toggle.
- [ ] Anti-aliasing — on/off toggle.

### Audio candidates

- [x] Replace the existing 25%-step audio controls with sliders for the same
  master, ambient, music, and interaction/combat-feedback settings; keep
  visible percentage/value labels.
- [ ] Environmental — volume slider with a visible percentage/value.
- [ ] Continuous music — on/off toggle.

### Accessibility candidates

- [ ] Immersive camera — on/off toggle.
- [ ] Camera shake — slider 0-100%.
- [ ] Reduce flashing lights — on/off toggle.
- [ ] Toggle block — on/off toggle.

### Existing Kalmala settings retained in the new groups

- **Graphics** (currently Video): resolution, V-Sync, window mode, and
  render distance. Resolution uses common presets with an optional
  custom width/height input (320–7680 by 240–4320 pixels) and explicit Apply;
  V-Sync uses the shared checkbox, window mode uses the shared dropdown, and
  render distance uses a snapped 64–576 m cull-distance cap in 32 m steps.
- **Audio:** master volume, mute/restore, music, ambient, and
  interaction/combat feedback levels. The selected audio candidate now uses
  focusable sliders with visible percentage values.
- **Keyboard & Mouse / Controller** (currently Controls): existing allowlisted
  per-action binding choices and restore-defaults behavior for each device
  family. The optional direct-capture checkbox changes only the keyboard/mouse
  interaction model if selected.
- **Accessibility** (currently Settings): text scale, interface scale,
  Low/Standard/High contrast, colour-independent feedback preference, and Reduced
  motion. Interface scale is the existing equivalent of the reference's Scale
  GUI control; keep it in this group with a visible value and do not add a
  duplicate setting.

Keep the selected tab, current values, and focus order readable. Sliders expose
their values as text; toggles expose on/off text; selectors expose the current
choice and can be changed with visible previous/next buttons plus keyboard or
controller navigation. Long Graphics and binding pages remain scrollable.
High contrast, local persistence, modal ownership, and reduced-motion behavior
continue to apply. Candidate options stay out of implementation scope until the
user checks them and their underlying behavior is confirmed.

Exact control ranges and device-specific labels remain implementation details;
they must stay bounded, reversible, and compatible with the current input
bindings. This increment makes no platform, visual-identity, or audio-content
decision.

The Audio tab provides a focusable master-volume slider and a mute/restore
button. Ambient, music, and interaction/combat feedback each have a focusable
0–100% slider with a live percentage label. Slider changes update immediately
and save to the local `GameUserSettings` config after a brief idle delay or when
the menu closes. The master value applies through the
engine's primary output-volume multiplier; the ambient value scales the local
wind, rain, water, fire, and biome loops, and the interaction/combat value
scales owner-local movement, status, crafting/gathering, discovery, combat, and
support one-shots. The music value is ready for a future music playback path;
there is no music track in the current runtime.

The Keyboard & Mouse group is labeled Controls in Options. It and the Controller
group store only allowlisted local choices in the existing
`GameUserSettings` configuration. Both input tabs present their bindings in two
equal-width, centered columns. Keyboard movement presents Forward, Backward,
Right, and Left as separate rows; Controller keeps the paired left-stick axes.
Each Options page uses its selected tab as the page label without repeating a
page heading in the content area. The five Options tabs divide the full content
width into equal buttons with matching horizontal gutters. A shared divider
uses the transparent medieval knotwork ornament over a fine rule, followed by a
10-unit gap; high contrast uses a geometric rule without the decorative image.
Pages omit explanatory footer text.
The Controls content fills its available viewport and scrolls vertically only.
Its rounded bronze thumb sits on a dark wood-toned track; high contrast uses a
white thumb on a black track.
Audio places its shared stone mute checkbox above a grouped set of volume sliders.
Each row changes only its own key within the existing positive/negative
movement-axis pair. Action and look bindings replace only the selected keyboard
or controller device family.
Applying or restoring a choice rebuilds the local player's input map
immediately, and a fresh controller instance rehydrates the same local choices.
The project baseline in
`Config/DefaultInput.ini` is never rewritten. Escape remains available for the
modal close path even when the alternate Settings action key is changed.

The Accessibility group now provides shared themed sliders for text scale
(50%, 75%, 100%, 125%, and 150%) and Low, Standard, or High contrast. Text scale
defaults to 100%; contrast defaults to Standard, centered between Low and High.
The sliders snap to those values and show explicit text values. Low contrast
uses muted text while retaining the standard surfaces; high contrast keeps its
black-backed panels and white text. Text changes rebuild the modal
with scaled, auto-wrapped labels and buttons inside its larger bounded panel;
the binding list remains scrollable at the largest size. Contrast updates the
local backdrop, panel, button surfaces, text, and focusable state controls
together. Both settings persist in the existing local `GameUserSettings`
configuration and affect no gameplay widget, replicated property, or server
request.

The Accessibility group also provides a shared themed whole-interface scale
slider at 80%, 90%, 100%, 110%, or 120%, defaulting to 100%, plus a stone-themed
Reduced motion checkbox that defaults to Off. Interface scale is a local multiplier over the project's
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

The Accessibility group provides a local colour-independent feedback choice in a
labeled dropdown using the shared stone-control theme:
**Text only** keeps the existing readable state lines, while **Text + markers**
adds explicit bracketed state markers for Wet, hearth, construction, combat,
discovery, and support status in a local owner-only overlay. The marker mode
uses text rather than colour as the distinction, follows the local contrast
palette, follows the scaled viewport, and updates as the existing accepted
gameplay state changes. It is stored beside the other local settings and never
becomes a gameplay signal.

## Existing input baseline

The current normal-player baseline in `Config/DefaultInput.ini` is the source
for the primary keyboard/mouse and controller mappings. The only default
alternative is Tab for Inventory; the other alternate-key rows start unbound.
The Build input is a fixed right-click action while the Construction Hammer is
active, so it is not a remappable Control-bound key.

| Action or axis | Primary keyboard/mouse | Alternative default | Controller baseline |
| --- | --- | --- | --- |
| MoveForward | W/S | — | Left stick Y |
| MoveRight | D/A | — | Left stick X |
| Turn / LookUp | Mouse X / Mouse Y | — | Right stick X / Right stick Y |
| MinimapZoom | Mouse wheel | — | — |
| Interact | E | — | Face button bottom |
| Attack | Left mouse button | — | Right shoulder |
| Jump | Space | — | Face button left |
| Sprint | Left Shift | — | Left stick click |
| SettingsMenu | Escape | — | — |
| WorldMap | M | — | — |
| InventoryMenu | I | Tab | — |
| Build menu (Construction Hammer active) | Right mouse button | — | — |
| Numbered inventory hotbar | 1–9, 0 | — | Inventory grid via D-pad and A |
| Support selection | F1–F4 | — | D-pad directions |
| Support activation | Q | — | Face button top |

Keyboard & Mouse shows primary bindings in its first group and configurable
alternative bindings in a second group beneath it. Unbound alternatives display
`Not bound`; clicking cycles through the existing allowlisted keys and back to
unbound. Movement alternatives can be configured independently for each axis
direction. A bound alternative invokes the same existing action or axis. Look
axes and Attack remain mouse-bound. Controller bindings remain in their
separate group. Runtime remapping stores primary and alternative choices
locally and applies them to the same action or axis. A remapping presenter must
display the current bindings, retain a keyboard/controller path to cancel or
reset, and send the same existing intent rather than a new gameplay payload.

The owner-local Inventory menu opens only when no other modal is ignoring
movement or look input. This keeps it exclusive with crafting, Settings, and
the world map, whose existing modal paths retain their opening/closing rules.
Its Escape path uses the existing SettingsMenu action to dismiss Inventory
before Settings can open. Opening captures the current cursor visibility and
only the movement/look ignore state it acquires; closing restores those values
and returns input to gameplay. While an editable text control has keyboard
focus, Tab/I do not toggle the menu. Gamepad B closes it when focus is outside
text entry. The menu reads only the owning pawn's owner-only replicated pack
stacks and bounded carried-tool records. Previous/next controls and arrow/D-pad
input select a pack or equipment row, whose icon, description or level/condition
appears in the shared detail panel. If a selected record disappears, selection
falls back to the first remaining row; an empty owner view clears selection and
hides the detail panel. A damaged selected tool exposes the existing repair
action. It sends only the tool ID, and the server validates the carried record
and nearby visible Workbench or Forge before changing condition. A selected
supported food item shows its existing Steady Meal benefit, owner-visible
quantity and active timer, and an Eat one serving action. That action sends only
the selected food ID through the existing station-free server transaction;
rejected or replayed requests consume nothing and cannot refresh or replace an
active meal. The owner sees the server result. The previous HUD pack view
remains until the separate HUD-removal task.

Inventory also has local search, category, and sort controls. Search trims outer
whitespace, is limited to 64 characters, and matches only names already present
in the owner's visible pack/tool rows; it cannot query descriptions, quantities,
or hidden catalogue content. Category filters All, Items, or Carried tools, and
sort chooses owner order, name, or category/name without changing pack order.
Page Up/left shoulder cycles category, Page Down/right shoulder cycles sort,
arrows/D-pad change the selected row, and Tab reaches search and labelled
buttons. While text editing has focus, its cursor keys remain available. Query,
filter, sort, canonical selection, and row/menu scroll offsets survive closing
and reopening this local widget for the current session. An entry hidden by a
filter or query keeps its remembered selection; an entry removed from the owner
snapshot falls back to the first remaining result, while no results clears
details and explains recovery. Separate menu and row scroll areas keep the
browsing controls reachable when enlarged text or viewport changes need scroll
fallback. The panel follows a resized viewport up to its normal 640×560 UI-unit
size and leaves a 16-unit inset at each edge when the available area is smaller.

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
- The master-volume control remains a focusable slider with a visible value.
  Mute uses a focusable shared stone checkbox whose checked state means muted;
  the control remains understandable without audio and retains the menu's
  Escape return path.
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
replicated settings state. `Scripts/Tests/Presentation/SettingsAccessibility.ps1` runs
isolated host/client profiles, opens the Escape shell and live Keyboard & Mouse,
Controller, Graphics, Audio, and Accessibility groups; it checks focusable targets, modal input
ownership, and Escape recovery, persists every local option, and compares the
pawn health, transform, and server-selected world identity before and after
the probe. It checks interface scale and reduced motion in local config, then
restarts both peers with the same user directories and confirms both choices
reload and the scale is applied over the project default. It retains sixteen
host/client PNG captures at either 1280x720 or 1024x768: standard-contrast
Escape, Graphics, Accessibility, reduced-motion Accessibility, and restored
HUD views plus high-contrast Keyboard & Mouse, Controller, and Audio views. Run
both viewport sizes and inspect the
modal, HUD, and artwork; these checks do not replace physical
keyboard/controller or packaged verification.

`Scripts/Tests/Presentation/SettingsAccessibility.ps1 -ContractOnly` checks this contract
without Unreal; the same entry point runs this check before its rendered peer
probe. The focused `Kalmala.UI.Settings.LocalPresentation` automation
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

## Development increment: Options group memory

The owner-local settings widget retains its last Options group (Keyboard & Mouse by default)
across ordinary close/reopen. Escape still opens the main shell; choosing Options
rebuilds the remembered group with current labels and its normal focus target.
Restoration invokes no setting-changing click handler. The current M11 runtime
index was bounded to four tabs. M13 replaces that set with applicable groups
from the six-group Settings checklist above; the five visible groups remain
within bounds, with Keyboard & Mouse as the default. Memory ends when the widget
is recreated,
including subsystem teardown; it writes no config/save data.
Scroll position and exact previously focused control are not yet retained.
The rendered settings accessibility fixture now returns from the Accessibility group
to the main shell, reopens Options, and requires the Accessibility group and a focusable
control to be restored on both peers while local interface/text scales and
master volume remain unchanged. This covers the Options-tab-only increment;
other menus, scrolling, exact control focus, and full per-menu restoration remain
part of the broader M11 task.


## Presentation ownership


This document records the remaining presentation seams and the source that is
allowed to supply them. It keeps the M5 visual/audio pass original and
project-owned without changing gameplay contracts, the generated-world
identity, or the multiplayer authority model.

Every audited seam is presentation-only: it may improve readability or
feedback, but it cannot become a gameplay source.

M11 supersedes the historical status placement below: active status/weather
now appears only in the transparent owner-local top-right hotbar, with hidden
accessible labels and existing timers. The Escape home menu contains Options
and Quit only. The lower-left widget retains only ocean travel, and pack
preparation guidance retains no active timer.
Canonical catalogue images are shared by Inventory items, carried tools,
selected-item details, ingredient rows, and the chest selectors; the existing
vector assignment remains the missing-texture fallback. Build and crafting
result grids remain on vector assignments until the next M12 icon integration
child. See `02-technical-architecture.md` for identities and verification boundaries.

## Ownership ledger

| Presentation seam | Project-owned source | Current contract | Runtime status |
| --- | --- | --- | --- |
| Player | `UKalmalaPlayerModelComponent` procedural mesh and the generated bark/terrain/rock materials | Nine local, collision-free cosmetic parts; shape and pose never author gameplay | Faceted mantle/hood presentation verified in the rendered offscreen host/client controls fixture |
| Wildlife | `AKalmalaWildlifeSpawn::BuildArchetypePresentation` procedural low-poly geometry and vertex colours | Server-owned replicated actor state; mesh is presentation only and has no collision | Mireling's low forward hunch, reaching arms, and split crown read as a distinct close-view silhouette in the rendered host fixture; dark body planes merge somewhat. Boar has a low wedge-backed profile, broken bristle ridge, tapered muzzle, and paired tusks; deer has a lighter, long-legged alert profile with paired forked antlers |
| Environment | `AKalmalaGeneratedTerrainPatch`, campfire, and construction procedural meshes using generated materials | Terrain collision and shelter collision remain the gameplay authority; decorative meshes do not add routes or hidden content | Existing generated terrain, water, rock, tree, hearth, and kit sources are audited here |
| UI | `KalmalaUI` C++ widgets, local Slate vector glyphs, and disposable local raster textures | Local presentation reads visible/replicated state and never creates a gameplay source | The near-crosshair prompt names only the actor hit by the owning pawn's current short view trace and shows its supported action plus any owner-visible unavailable reason; it clears for no target or modal input and contains no key/button legend. The lower-left survival strip reads the owning pawn's replicated Wet/food entries, exposure, active support, and weather, with text-plus-shape categories, server-time/intensity context, source, and recovery guidance; the Inventory menu reads only that owner's pack/tools and the crafting panel reads its owner-only skill ledger; a separate top-centre support strip reflects the local character's selected effect and the owner's learned set; minimap/map remain local; the separate weather badge reads the replicated server tier and pairs circle/diamond/triangle markers with explicit text; Settings' Controls (keyboard/mouse), Controller, Graphics, Audio, and Accessibility groups expose local options, including independent text/interface scales and reduced motion in the existing local settings config |
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
boar/deer meat; the Cauldron uses the same owner-local shell for stew and soup;
the Frying Pan shows only roasted root vegetables and deer/rutabaga roast.
All cooking views include ingredient, quantity, and current hearth-heat feedback.
Workbench opens a Craft section scoped to its supported recipes and matching Tool Rack
production. Its header shows the accepted station's effective level, starting at level 1
and increasing by one for a nearby compatible Tool Rack. Forge Craft includes its station
recipes plus the owner-local Bronze Axe and Bronze Mace actions; Bronze and Iron equipment
acquisitions are Forge-scoped. Forge exposes Craft and Upgrade views; Upgrade compares the owner-only carried
Bronze Axe level 1 with the same Bronze Axe at level 2, showing only its
item-specific stat gains and no harvesting-tier change. It reads the owner's tool,
material, and skill snapshots for prerequisites. Both station shells provide a
direct Repair All action that repairs all damaged carried tools without opening
a separate menu. Their headers show the accepted station's effective level,
starting at level 1 and increasing by one for a nearby compatible attachment.
The section/action buttons sit at the top-left of the shell; the station
catalogue icon, name, and level sit at the top-right.
The selected Workbench/Forge tab uses the shared gold text accent without a
leading selection marker.
Forge Upgrade uses the same wood-backed content area and top inset as station
Craft. Its split follows the Craft list/detail proportions: the left frame keeps
prerequisite and availability text, while the wider right detail frame shows
the owner-only comparison above a centered material tile row and Upgrade action
at the bottom. Its
comparison header reads `Bronze Axe -` followed
by a level icon and target level; subsequent rows show every nonzero stat from
the current and target levels, including maximum durability and all nonzero
damage-bonus channels. Zero-valued channels are omitted. Increases and decreases
use the shared level-up and level-down colors. The Upgrade button is
text-only and labeled `Upgrade`. Repair All keeps the current Craft or Upgrade view
open and reports the server result in the station header.
The existing progression RPC remains responsible for authoritative checks,
material exchange, and persistence.
These Workbench and Forge level lines remain visible in their parent Craft
sections. The standalone Build menu no longer duplicates
inventory, food, repair, upgrade, or storage controls; it keeps placement and
the Light hearth action, while raw-fuel addition uses Campfire Interact.
The station Repair All button sends a payload-free server request and keeps the
current recipe or Upgrade selection unchanged. The server requires a visible
same-world Workbench or Forge within 250 cm, validates the entire bounded
owner-only carried-tool array, repairs every damaged known tool, and persists
the complete candidate before publication. The Inventory panel retains its
selected-tool repair action. The standalone Build menu opens by right-clicking with the Construction Hammer active.
An accepted Chest interaction opens a Store section with separate selectors
backed by the owning pawn's private pack and its owner-only current chest view.
Each list groups matching item IDs and shows their total count; the summary
reports the 40-cell pack and 16-cell chest limits and disables a transfer when
no matching stack has room and no destination cell is free. Store/Take submit only the selected item ID through the existing
one-item server transactions, which revalidate the active chest and persist
before changing the pack. Closing the shell, losing range, changing pawn, or
destroying the chest expires the view and shell.
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

When the owning player's private pack contains a supported meal food, the
on-demand Inventory menu explains that one serving grants Steady Meal,
reducing stamina cost by 10% for 120 seconds. Berry Recovery Draught detail
explains its up-to-20 health recovery over ten seconds and the shared
sixty-second draught cooldown; its action is labelled "Use one dose" and is
disabled at full health, while dead, or during cooldown. The request sends
only the selected item ID through the existing server transaction; the server
revalidates quantity and the matching effect gate before payment. The menu adds
no new RPC, replicated field, or save data. Shape markers and explicit text
remain visible with the owner's configured text scale and contrast.
`Kalmala.UI.Inventory.PreparedFoodDetails` checks the selected menu detail and
disabled action while owner data is unavailable. The host/client inventory
reconnect fixture continues to verify owner-only pack visibility; rendered
multi-row layout and scaled-font appearance remain open.

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
field. `Scripts/Tests/Construction/Crafting.ps1` checks the fresh-owner values and
host/client presentation; `-Rendered` retains both 1280x720 captures.

The selected-result preview reads only the existing selected recipe output,
description, requirements, and availability already shown by the local
crafting panel. It resolves a canonical icon through the shared catalogue
mapping and keeps the availability text visible when a recipe cannot currently
be crafted. Keyboard/controller selection updates this read-only panel; it
sends no request and creates no actor. No inventory, recipe, item, replicated,
or save property is added.

The inline Bronze Axe upgrade comparison reads the existing level-one to
level-two progression and lifecycle definitions. It shows the same item ID,
the level increase, and item-specific stat gains without changing harvesting
tier or resource access. It refreshes the
carried level from local owner state and reports an already-upgraded tool
without offering another upgrade. It adds no
request, equipment state, stat authority, replication field, or save data.

The post-roadmap Bronze tier also defines independent Bronze Knife (48
durability, 42 slash), Bronze Pick (90 durability, 38 blunt), and Bronze Spear
(90 durability, 58 pierce) identities. Their level-one acquisitions, plus the
existing Bronze Axe acquisition, are authored at Forge level 1. The
existing Bronze Axe level-two entry remains a same-item upgrade (150 durability
and 55 slash); it cannot create an Axe, Pick, Knife, or Spear identity. Bronze
Knife and Pick use their own action profiles and can satisfy Stone-tier
gathering/mining sources; Bronze Axe satisfies matching woodcutting sources.
Every harvesting tier is derived from the item's identity, never from its
upgrade level. The 40-cell inventory capacity is shared by all stacks and
carried tool instances; there is no separate tool count cap and duplicate item
IDs are allowed. Dedicated Bronze icon textures and generic station-menu exposure for
the new acquisitions remain in the next post-roadmap increments.

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

`Scripts/Tests/Presentation/PresentationOwnership.ps1` checks the committed project-owned
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


The tutorial presenter and persistent inventory HUD are retired. Current active-only status, notification, recipe activity, and on-demand Inventory contracts are defined in the preceding consolidated sections. Historical rendered captures do not establish fresh M13/M14 acceptance.


## Audio cues


This document defines the original audio layer for the existing vertical slice.
It is a cue contract, not an imported sound library or a new gameplay system.
All future sound assets must be project-owned and every cue must have a readable
non-audio equivalent so silence never hides authoritative state.

## Cue matrix

| Cue group | Normal trigger | Audio intent | Required non-audio equivalent | Authority/privacy boundary |
| --- | --- | --- | --- | --- |
| Ambient wilderness | Local player is in normal play | Quiet wind, water, fire, and biome atmosphere establish place without a fixed route | Terrain, weather, hearth, and exposure text/shape cues remain readable | Only local visible context; no hidden population, discovery, or route hint |
| Movement and traversal | Local movement, jump, sprint, landing, or water entry is already visible | Sparse original footfall, jump, landing, and movement-state cues reinforce action | Existing movement pose and HUD state remain readable; current bindings remain in Options > Controls and Options > Controller | Local cosmetic response; no movement result, speed, stamina, or terrain authority is authored by audio |
| Weather and exposure | Existing replicated weather/Wet presentation changes | Rain, wind, and shelter contrast communicates changing conditions | Wet duration, shelter, warmth, hearth, and recovery text/shape cues remain visible | Server owns weather/exposure/Wet; audio reads accepted replicated state only |
| Interaction and gathering | A normal interaction receives an accepted or rejected result | Short, distinct confirmation or rejection cue avoids ambiguous input | Existing interaction text and inventory result identify accepted, rejected, or unavailable state | Server validates actor, range, payment, item, quantity, and outcome; audio carries no request payload |
| Combat | Replicated committed action or feedback changes for the owning player/relevant actor | Windup, recovery, hit, defeat, and unavailable cues clarify timing and outcome | Combat phase and colour-independent HIT/DEFEAT/UNAVAILABLE text remain authoritative presentation | Server selects target, damage, cooldown, and defeat; clients never infer or author them from sound |
| Discovery | Owner receives a server-confirmed landmark or scroll feedback result | Brief discovery motif acknowledges only an accepted discovery | Owner-only text names the accepted discovery category; remote players retain relevant visible actor presentation | No sound or prompt reveals undiscovered IDs, private rewards, or duplicate state |
| Support magic | Entitled player receives accepted activation, rejection, or expiry presentation | Gentle effect-specific cue communicates support activation and end state | Existing learned-effect, target-validity, cooldown, stamina, and active-state text remains available | Server validates entitlement, effect, target, sequence, stamina, duration, and non-damaging execution |
| Camp and storage | Hearth, construction, or storage result is already visible | Fire, placement, payment, and storage cues reinforce a chosen camp action | Hearth, construction health/rain wear, inventory, and storage text remain readable | Server owns placement, payment, fuel, health, storage, and persistence; audio is presentation only |

The matrix is intentionally event-driven. No cue may start because a hidden
actor, hidden reward, private pin, or future route exists outside the player's
normal visible/relevant context. Repeated state changes should be coalesced or
rate-limited locally so audio cannot become a timing advantage or a source of
network traffic.

## Original asset and mix requirements

Sound assets must be created for Kalmala and stored under a project-owned audio
content path when the runtime pass begins. Do not copy music, field recordings,
voice, sound effects, or named audio identity from a third party. Use the local
Audio settings contract for master/music/ambient/interaction-combat levels and
mute/restore. A muted or unavailable device must still expose all state through
the text/shape equivalents above.

Every local ambient component multiplies its live bed level by the saved
Ambient category value. Owner-local movement, Wet, interaction/gathering,
discovery, combat, and support one-shots multiply their submitted cue level by
the saved Interaction/Combat Feedback value. Master volume continues to apply
to the whole game process. Music volume is saved and shown in the Audio tab,
but no music playback path or track currently exists.

The runtime ambient layer includes loop-seamed, project-generated beds at
`/Game/Kalmala/Audio/WindBed`, `/Game/Kalmala/Audio/WaterBed`,
`/Game/Kalmala/Audio/FireBed`, `/Game/Kalmala/Audio/BiomeBed`, and
`/Game/Kalmala/Audio/RainBed`, plus the one-shot
`/Game/Kalmala/Audio/WetStatusCue` and
`/Game/Kalmala/Audio/SupportAcceptedCue`,
`/Game/Kalmala/Audio/SupportMendingCue`,
`/Game/Kalmala/Audio/SupportHearthShieldCue`,
`/Game/Kalmala/Audio/SupportBearsVigorCue`,
`/Game/Kalmala/Audio/SupportDeerCallCue`,
`/Game/Kalmala/Audio/SupportHearthShieldExpiryCue`, and
`/Game/Kalmala/Audio/SupportBearsVigorExpiryCue` and
`/Game/Kalmala/Audio/CombatResultCue`,
`/Game/Kalmala/Audio/InteractionAcceptedCue`, and
`/Game/Kalmala/Audio/InteractionRejectedCue`,
`/Game/Kalmala/Audio/MovementFootfallCue`,
`/Game/Kalmala/Audio/MovementJumpCue`, and
`/Game/Kalmala/Audio/MovementLandingCue`,
`/Game/Kalmala/Audio/GeneratedOceanEntryCue`, and
`/Game/Kalmala/Audio/GeneratedOceanExitCue`.
`UKalmalaAmbientAudioSubsystem` starts wind only
for a local player whose normal generated-world state and pawn are ready. It
smoothly adjusts the wind bed from accepted replicated
`FKalmalaWeatherState::WindStrength`. Rain starts only when accepted replicated
precipitation reaches `0.01`, follows its intensity with a quiet local fade,
and stops when rain ends. WetStatusCue plays only when the owning pawn's
replicated server-owned `State.Wet` entry first appears (or is already present
when local play starts); it does not infer the cause or change the status.
Neither cue reads another pawn's private status or sends a request.
An effect-specific support activation cue plays once when the owning pawn's
existing owner-only support `FeedbackSerial` advances with
`EKalmalaSupportFeedback::Accepted`; the component's existing active effect
selects Mending, Hearth Shield, Bear's Vigor, or Deer Call. The cue does not
infer an effect from client input. A generic `SupportAcceptedCue` remains a
fallback if a specific cue cannot load. Existing learned-effect, cooldown,
stamina, and active-state text remains the readable result; unavailable
feedback does not play an acceptance cue.
Hearth Shield and Bear's Vigor each play a quieter effect-specific expiry cue
only when the owning pawn's existing replicated absorption/strength and expiry
state transitions to inactive. A shield depleted by damage uses the same end
cue. Mending and Deer Call are immediate effects without a timed gameplay
state, so they use activation cues only. No local timer guesses when an effect
ends.
CombatResultCue plays once when the owning pawn's existing owner-only combat
`FeedbackSerial` advances with `EKalmalaCombatFeedback::Hit` or `Defeat`.
Unavailable results remain readable through the existing text and do not use
the confirmation cue. Playback carries no target identity and never infers a
hit, defeat, or damage from a client request or remote actor.
MovementFootfallCue is submitted at a quiet, distance-based stride while the
owning local pawn is grounded and moving; cadence and pitch follow sampled
local speed, including actual sprint pace. MovementJumpCue plays only when that
pawn transitions from ground movement into an upward falling state, and
MovementLandingCue plays when its sampled state returns from falling to ground.
The subsystem samples only the local controller's own character and its local
movement component; it does not inspect remote pawns, claim server movement
acceptance, or trigger an RPC. Existing movement pose and HUD state remain the
readable movement cues; current input bindings remain available in Options >
Controls.
GeneratedOceanEntryCue and GeneratedOceanExitCue each play once when that same
local movement component's `IsSwimmingInGeneratedOcean()` state transitions
into or out of its generated-ocean custom movement mode. Initial state sampling
and pawn replacement establish a new baseline without a false entry/exit cue.
Water, Wet duration, exposure, and
server-authoritative movement remain readable and unchanged.
InteractionAcceptedCue plays for a new accepted owner-only crafting result or
when an existing owner-only inventory stack increases after server validation;
the first inventory snapshot only establishes a local baseline. A rejected
owner-only crafting result uses the distinct, quieter InteractionRejectedCue.
Inventory decreases do not cue. Crafting result text, an accepted-result bit,
and a monotonic serial replicate to that pawn's owner only; the server writes
all three, and a small local cooldown coalesces adjacent interaction/gathering
changes. Neither cue includes an item ID, quantity, target, or request data.
Only owner-only `LandmarkFound` or `ScrollFound` feedback plays the original
`DiscoveryAcknowledgedCue`, once per new server-confirmed feedback serial.
Duplicate or unavailable results remain readable text and do not cue; the audio
carries no discovery ID, reward, or hidden-content hint.
It samples a bounded set of points around that pawn from the existing immutable
world identity, accepts only the existing sea surface or visible inland-lake
surface, and requires an unobstructed visibility trace from the local view
before water can be heard. The water loop probes every 0.75 seconds, fades by
distance within 1,600 cm, and uses a maximum volume of 0.07. Fire probes locally
replicated campfire actors every 0.75 seconds and accepts only a lit hearth
within 1,400 cm with an unobstructed trace from the owning player's view. Its
non-spatial bed fades with distance, reaches full volume by 275 cm, and caps at
0.075. Biome ambience samples only the owning pawn's current XY from the
locally available immutable seed and existing four-field classifier every
0.75 seconds. One quiet bed uses a small biome-specific pitch/level profile
with smoothed transitions; it does not scan ahead, reveal landmarks, or imply a
route. All five local loops stop when local play ends; no hidden population,
discovery, or remote-biome query is used.

`Scripts/Generate-AudioSources.ps1` optionally recreates the original mono
sources under `Content/Kalmala/Audio/Source`. Use `-ListGenerators` to see the
groups, `-Generator WaterAmbience -Force` to rebuild one group, or
`-All -Force` to regenerate all twenty-two source WAVs. The source WAVs and
their imported Unreal `.uasset` packages are checked in as game content, so
normal builds and runtime do not need this tool. It only writes WAV sources;
use Unreal's `ImportAssets` commandlet to refresh packages after changing them.
The rain bed is seam-crossfaded
and the WetStatusCue, SupportAcceptedCue, six effect-specific support cues,
CombatResultCue,
DiscoveryAcknowledgedCue, interaction cues, MovementFootfallCue,
MovementJumpCue, MovementLandingCue, GeneratedOceanEntryCue, and
GeneratedOceanExitCue are short one-shots. When audio sources change, import
the affected assets to `/Game/Kalmala/Audio`, then run
`Scripts/Tests/Audio/AmbientAudio.ps1`, `Scripts/Tests/Audio/AmbientAudioPeers.ps1`,
`Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario CombatFeedback`, `Scripts/Tests/Progression/DiscoveryPeer.ps1`,
`Scripts/Tests/Presentation/PlayerControls.ps1 -MovementAudio -OceanTraversalAudio`, and the forced editor build.
Pass
`-WeatherExposureOnly` to the peer runner when
focusing on the weather and Wet cues; its default mode also requires visible
water activation. The first verifier checks source format, imported
assets, local-player ownership, context gates, and teardown. The peer runner
confirms independent host/client probes for visible water and hearth context
plus each local player's current sampled biome, accepted rainy weather/Wet
state, and effect-specific Hearth Shield/Bear's Vigor activation and expiry
cues. The `CombatFeedback` scenario confirms that
owner-local Hit and Defeat feedback submit CombatResultCue, Unavailable remains
text-only, and the other peer receives no private combat result cue. The
discovery peer fixture confirms that only the entitled local owner submits the
acknowledgment for a new landmark/scroll result; duplicate, unavailable, and
remote undiscovered feedback remain text-only. The
ambient peer fixture also verifies owner-local accepted crafting, accepted
gathering, and rejected crafting cues for both host and client. Its gathering
node is transient and invokes the ordinary server harvest path; it does not
write a world save. The no-build audit checks that the crafting result text,
serial, and outcome and the inventory stacks remain owner-only. The
development-only listen-server fixture selects a light rainy/windy
interval below the hearth smoulder threshold, applies Wet on the server, and
creates a visible server-lit hearth. A test-only server exposure hook keeps
Wet observable while it checks the status cue alongside the hearth bed.
Existing Wet duration, shelter, warmth, hearth, recovery, and support result text remains the
readable fallback. These checks do not establish
audible quality, hardware mixing, or packaged playback, and do not complete
other event cues or audio options.

Keep cues short, bounded, and non-blocking. Ambient layers may be local and
spatial, but they must not stream a second world simulation or reveal a server
actor outside ordinary relevant replication. Combat, discovery, support, hearth,
inventory, and construction cues should follow the same replicated feedback
serials already used by their presentation rather than adding an RPC or save
field.

## No-build audit and runtime limits

`Scripts/Tests/Audio/AmbientAudio.ps1 -ContractOnly` checks all eight cue rows, the
non-audio/accessibility requirement, project-owned asset rule, local mix scope,
server authority/privacy boundary, the owner-only combat/support feedback
sources, owner-only discovery cue gating, generated-ocean movement transitions,
and the absence of a new RPC or save field. `PlayerControls.ps1
-OceanTraversalAudio` confirms both local peers submit one entry and one exit
cue through the local movement-state transition path. Its development-only
flag injects a three-second local sample sequence because the fixed seed-418
spawn has no deep water in the fixture's short search area; it does not change
the character movement mode or network state. Normal play reads the actual
`IsSwimmingInGeneratedOcean()` mode.
It does not create sound assets, prove mixing, test spatialization, or launch
Unreal. The ambient-audio, combat, and discovery peer checks do not prove audible
playback, device mixing, spatialization, or packaged playback. The broader
original presentation pass remains in M5.

## Multiplayer and persistence boundary

Audio is local presentation. The server remains authoritative for world
identity, terrain, population, weather, exposure, interactions, inventory,
construction, combat, discoveries, learned effects, rewards, and saves. A cue
cannot select a target, damage an actor, grant an item, reveal private state,
change a timer, or mutate persistence. Audio settings and cue history are local
preferences and must not be replicated or added to gameplay save schemas.


M13's reusable-scroll timing overrides earlier instant Deer Call and short support-duration descriptions above when integrated. Audio still reads accepted replicated results; this cleanup adds no cue or gameplay behavior.

Item profile `DamageBonuses` may omit inactive channels: the loader fills omitted Slash/Stab/Crush/Pierce/Fire/Water/Holy/Poison with zero before strict conversion. Keep the object itself (`{}` for zero damage); other required profile fields remain strict. Nonzero authored values retain existing validation. Shared attacks retain explicit channels, including zero Lightning where unused. Shared reward DamageBonuses also allow omitted zero channels, and omitted MaxDurability defaults to zero; the reward and DamageBonuses objects remain required.

Axes and pickaxes require integer `MaterialTier` in their item profiles: 1 = Stone, 2 = Bronze, 3 = Iron tool identity, independent of item level. Axe resource access continues to use the authored tool tier against the source's minimum requirement. Mining selections also carry the source resource tier and enforce the pick ceiling: Stone Pick mines through Bronze, Bronze Pick through Iron, and Iron Pick through Iron for now. Other profiles default to zero and cannot author a nonzero MaterialTier. Missing/invalid harvesting tiers fail catalogue loading; existing action, skill and condition checks still apply.

Carried equipment classification is derived from the validated item family; profiles contain no `ItemCategory` field. Knives/axes/maces/spears are weapons across all tiers; pickaxes and Construction Hammer are tools. Inventory browsing separates Tools and Weapons and details show the classification. Classification does not replace harvesting Action or MaterialTier and does not change shared equipment lifecycle, server repair/crafting/combat or saved IDs.

The schema-12 profile format introduced `ItemId`, `DisplayName` and `Description` instead of `ToolId`, `ToolSet`, `ToolKind`, `Action` and `RequiredSkill`. Stats and MaterialTier remain authored; omitted MinimumSkillLevel defaults internally to 1. The loader derives Starter for Stone identities, Crafted for Bronze/Iron, Utility for ConstructionHammer; Axe maps to Woodcutting, Pick to Mining, Knife to Gathering, Mace/Spear/Hammer to None, with the same required skill as action. Unknown identities and retired authored fields fail closed. At schema 12 this was a catalogue-only change; current player-discovery schema 2 stores `ItemId` directly, with no development-time save migration.

## Attack-driven resource trees (2026-10-13)

Wood-resource nodes (meadows-birch-bark, elderwood-resinwood, meadows-birch-trunk, meadows-aspen-trunk, elderwood-ironheart-trunk) now use the equipped axe Basic attack instead of interaction harvesting. Standing tree health is 100; the accepted felling hit starts a server-timed 1.2-second Falling phase with fresh trunk health of 60. Falling rejects further hits and pickup. A server timer then exposes the settled Trunk phase; breaking that trunk exposes a wood bundle for interaction pickup, yielding the existing Wood amount for Meadow Birch, exactly 4 Lightwood for Meadow Aspen, and the existing Densewood amount for Elderwood. Birch and Aspen require Stone Axe or better; Elderwood requires Iron Axe. A hit cannot carry excess damage into the next stage. The same replicated source actor represents each stage. The replicated fall start uses synchronized server time for clamped elapsed-progress reconstruction, including newly relevant peers; clients cannot settle or damage the tree. The original procedural trunk and canopy hinge towards the tree's local forward direction using that synchronized progress. At settlement the canopy is removed and the full-size tapered timber rests horizontally at the original root, with exposed cut faces. The mesh alone moves; the authoritative actor transform and persistent identity stay fixed. Tick runs only during Falling. Settled logs disable the root sphere, use the timber mesh for targeting, and validate server axe reach against the timber surface rather than the identity root. Breaking the log places the original two-piece wood bundle at its midpoint on the root plane. The bundle mesh supplies visibility collision and server physical pickup range uses that same reconstructed anchor; the identity-root sphere stays disabled. Rendered acceptance remains pending.

The server selects and rechecks the blocking target after windup, applies validated equipped-axe damage through tree resistances, enforces MaterialTier and carried condition, and persists one durability point per accepted hit before tree mutation. Trees do not use harvesting skill gates or client tool/action values. A low melee sweep reaches fallen trunks. Pickup validates the complete grant and player persistence before committing existing sparse depletion callbacks; rejected/full-pack/duplicate pickups retain the drop without reward. When a live wood-drop pickup fails because inventory capacity is exhausted, the server also publishes an owner-only `Inventory is full` interaction result. The existing local interaction-result handler plays its rejection cue from that owner-only result. Tree health/stage replicate to relevant peers, including late joins. Health, trunks and uncollected drops are session state and reset on actor regeneration/restart; no save schema was changed. Existing second-wave source depletion retains its session policy.

Generated Meadow/Elderwood trees now participate in this same axe-driven lifecycle as patch-owned harvest actors, with stable seed/patch/candidate identities and matching cosmetic suppression. Successful pickup records sparse depletion; eviction and restart restore uncollected candidates as standing trees. Focused stage, exact-ID peer delivery/pickup, isolated patch lifecycle and real-slot process-restart checks have passed; see the [current forest acceptance summary](07-development-setup.md#generated-forest-current-acceptance-2026-10-17) for commands and coverage limits. Settled-log targeting/range and midpoint bundle placement are implemented with focused regression coverage; rendered usability, final integrated live streaming/restart acceptance and full tree-feature verification remain pending.

Resource suitability enforces the item family before MaterialTier: axes only damage Woodcutting resources (trees/trunks), pickaxes only Mining resources (stone/ore), and knives only Gathering sources. Tier and item level cannot grant cross-family access, even if a runtime definition has a mismatched Action. Both axe tree damage and mining interaction consume this shared server check before condition, health, reward or depletion mutation. New `Kalmala.Gameplay.Harvest.ResourceFamilyGates` covers all six axe/pick identities against twelve wood/mining sources at levels 1/10/20, altered action rejection and unrelated families. Automation execution remains deferred.

Equipment acquisition and upgrade-cost records omit `RequiredSkill: None` and `RequiredSkillLevel: 0`. Upgrade-cost records also omit `RequiredStationLevel: 0`; the loader defaults these values before strict conversion. Acquisition recipes keep their explicit station level. Existing server validation and zero-skill progression behavior are preserved; harvesting skill contracts and nonzero requirements in other systems are unchanged.

Crafting/acquisition and item upgrades have no player-level, skill-level or skill-unlock eligibility gate. Their server transactions and owner requirement/blocker text use the station context/level, materials, capacity and current item/predecessor level only. Skill XP awards from accepted ordinary crafting/cooking remain rewards, not prerequisites. Equipment progression no longer reads the character skill component; its legacy pure-function skill-ledger argument is ignored for caller compatibility. Station-family and existing level matching, sight/range, payment and authority checks remain. The station progression regression now includes an uninitialized skill ledger alongside low-level and missing ledgers; Forge presentation verification requires station-only wording. Build/automation/UI runtime verification remains deferred.

Ordinary material definitions, including natural materials, are grouped in `items/materials/wood.json`, `items/materials/ore.json`, `items/materials/metal.json`, `items/materials/cooking-ingredients.json`, `items/materials/crafting-ingredients.json`, `items/materials/hides.json`, and `items/materials/others.json`. Wood contains Wood, Lightwood, and Densewood; Ore contains Stone; Metal contains Iron; Crafting Ingredients contain Sinew, Bow String, Amber, and Bone Fragment; Cooking Ingredients contain Frost Salt; Hides contain Boar Hide and Deer Hide; Others contains Coal. Ammunition is grouped separately in `items/arrows.json`. Current save data must use `Amber` directly; old `PeatAmber` save IDs are not converted during development. Buildable items under `items/buildItems/` use the root `items` array and ordinary `itemFiles`. Weapon profile files under `items/equipment/weapons/`, wearable item files under `items/equipment/armor/`, and the Building Hammer profile at `items/equipment/building-hammer.json` use the same root array. The loader permits `buildItems` and `materials` only for ordinary items, `equipment` only for item profiles, and `stations` only for ordinary and item acquisition recipe files. The manifest separates ordinary `itemFiles` from equipment `itemProfileFiles`; all equipment profiles, including Building Hammer, load into the runtime tool catalogue. Profiles are sorted by ItemId; bounds, validation and behavior remain unchanged; the retired toolProfiles root is no longer read.

Recipe category JSON uses the root `recipes` array, including equipment acquisitions. Ordinary recipes under `recipes/stations/` are grouped by their crafting station; the Inventory group contains the Building Hammer recipe, while build-menu construction remains in `recipes/building.json`. Equipment acquisition records share the corresponding station file and `recipes` array; the loader routes records with `ItemId` into the runtime tool catalogue. Individual station recipe arrays may be empty. Equipment upgrade-cost files are grouped by family under `item-level-up/costs/weapons/`; this includes Bow Workbench costs and tiered weapon costs. Wearable costs are grouped by type under `item-level-up/costs/armor/`. Stone and Iron generally use acquisition ingredients; Bronze generally replaces Stone with Bronze bars at the same quantity while retaining the wood cost. Bronze Axe, Knife, Spear, and Mace acquisitions and upgrades use Lightwood in place of Wood; Iron Axe acquisition and upgrade both use Lightwood; Iron Knife, Spear, and Mace acquisitions and upgrades use Densewood. Pickaxe upgrade costs remain unchanged. The user-directed A1 acquisition targets are Stone Pick: 10 Wood + 5 Stone, Bronze Pick: 10 Wood + 10 Bronze, and Iron Pick: 10 Wood + 10 Iron; apply the metal-cost recipes with the A1 production route. Stone upgrades use Workbench, and Bronze/Iron upgrades use Forge. The Bow Workbench upgrade reuses the acquisition materials (5 Wood and 1 Bow String). The Construction Hammer upgrade cost lives in `item-level-up/costs/construction-hammer.json`. Upgrade costs accept Starter, Crafted, and Utility profiles. The loader sorts ordinary recipes and acquisitions by RecipeId, and upgrade costs by ItemId. Cost scaling, station requirements and validation are preserved; retired `toolRecipes`/`toolUpgradeCosts` roots are not read.

Inventory Craft may create multiple Construction Hammers. Each copy has its own GUID, occupies one of the forty shared inventory cells, and is saved as a separate carried-tool record. The starter hammer is placed first in the hotbar. Inventory groups matching hammer copies in one row with their count. Other non-stackable item types may also have multiple GUID-addressed copies; every unit in stackable items also has a GUID, generated when granted or crafted and preserved through stack changes and storage transfers. The server-authoritative material transaction remains in place.

## Worn Gear interaction follow-up — 2026-10-15

Presentation update (2026-10-18): the inventory grid and equipment share one
InventoryPanelImage backplate, with a 56-unit gap and no equipment heading or
footer instructions. The gap centers the existing OptionsDividerImage ornament
at its 3:1 aspect ratio (168×56); high contrast or a missing texture uses a
simple rule, following the Options divider convention. Content mounts directly in the backplate's vertical box;
there is no inventory ScrollBox, scroll offset or scroll-shadow hint.
At that time, the seven supported slots used a centered body
layout: Head at (row 0, column 1), Cape (1,0), Main Hand (2,0), Chest (2,1),
Shield (2,2), Legs (3,1), Boots (4,1). Necklace (1,1) and Ring (4,2) are disabled
visual placeholders; jewellery items and equip transactions were not yet authored.
Empty cells use original detailed muted vector silhouettes; occupied cells resolve the existing catalogue icon from the owner's
equipped gear or active tool identity. Item names, condition and action guidance
remain in tooltips, including pending/request results. Cells use the inventory
ItemIconTileFill, including live contrast updates. No gameplay equipment
categories, transactions, replication or persistence were introduced by this
presentation-only update.

The Inventory popup reads the owner's six replicated wearable slots and two accessory slots and displays their current item names. Selecting compatible gear enables its slot action; selecting a different item for an occupied slot replaces the equipped GUID, and selecting an empty grid cell before clicking occupied gear requests its return to the pack. The client sends only the item GUID or slot enum; the server revalidates identity, compatibility, capacity, and the full candidate transaction before publishing the owner-only gear and inventory state. An owner-only client result reports accepted or rejected requests. Server-resolved wildlife damage uses the summed integer Armor of non-depleted equipped gear against physical damage only. Accessories have no condition capacity or wear; their `Armor` contributes while equipped. The Main Hand tile selects or clears an exact tool GUID without moving its grid cell. The owner sees remaining/maximum condition for durable wearables; accepted wildlife hits spend one condition per wearable and depleted items cease contributing defense. Same-pawn unequip/re-equip preserves wear.

Carried-item acquisition records load into the tool progression catalogue,
separately from ordinary item recipes. The owner sees the same-item option in
the station's tool progression view and requests it through `ServerProgressTool`.
The server validates the visible station and level, material costs, carried
capacity, and progression; it creates one GUID-addressed level-one
`CarriedTools` instance, persists it, and rolls the payment and tool state back
if persistence fails. The owner selects that instance through the normal Main
Hand GUID request. Accepted combat uses its authored profile and server-owned
condition; generic `ServerCraft` remains for ordinary item recipes.

Equipped accessories' additive `StaminaCostMultiplier` deltas sum with active Wet and Steady Meal adjustments and apply once through the shared authoritative cost calculation for sprint, dodge and support spells. `SupportSpellStaminaCostMultiplier` adds only on support spells. Cost factors are bounded to `[0.5, 2.0]`. Accessory maximum-health/stamina are recomputed from the owning equipped slots after accepted equip/replace/unequip. Equip preserves current values; removal clamps to the new maximum. Bear’s Vigor adds its 40 stamina independently. Maximum health replicates owner-only; no new gear persistence is added. Active stamina recovery and incoming Mending/draught healing each apply their current equipped delta once using clamp(1 + sum, 0.5, 2.0); other recovery rules, costs, unlocks and cooldowns remain unchanged.

### Generated-forest harvesting contract (reconciled 2026-10-17)

`FKalmalaForestLayout::Generate` supplies individual Meadow/Elderwood descriptors
shared by terrain rendering and server materialization: original candidate ordinal,
world/local position, biome, dimensions and yaw. Each accepted descriptor has a
`forest:v1:<world-seed>:<patch-x>:<patch-y>:<candidate>` identity. Patch coordinates
are exact world-space centers formatted with 17 significant digits and normalized
signed zero; full unsigned seeds and original ordinals prevent filtered-order and
cross-patch aliasing. Invalid centers/ordinals yield an empty identity; rejected
candidates do not renumber later trees or alter the existing random draw order.

GameMode loads the matching population save before terrain activation. It generates
the shared descriptors once, skips IDs already harvested, and initializes each
remaining node through the private descriptor initializer. Public
`AKalmalaHarvestNode::InitializeForestServer` reconstructs from server config,
patch center and ordinal through that same path. Authority, unused actor state,
candidate identity and finite positive dimensions are validated before mutation.
There is no client descriptor or reward RPC. Nodes use the Meadows birch-trunk or
Elderwood ironheart-trunk tier/yield contracts and bind `OnHarvested` to GameMode's
existing sparse population callback. Forest IDs bypass the session M9 overlay.

### Patch ownership, suppression and replication

The terrain patch owns materialized nodes; GameMode tracks weak references by
patch coordinate. `SetSuppressedForestCandidatesServer` validates original
ordinals against the shared layout, rejecting duplicate/absent candidates
atomically and sorting accepted lists. Saved depleted candidates and successfully
materialized actors suppress their matching cosmetic trunks, canopies and root
buttresses. Allocation/initialization failure retains the candidate's cosmetic
geometry; suppression failure rolls back the new patch before registration.
Thus ordinary materialized trees are gameplay actors, while the unsuppressed
fallback remains cosmetic and cannot grant resources.

The replicated suppression list rebuilds only tree geometry and tolerates arrival
before generation data. Replicated `ForestDimensions` rebuilds actor presentation
independently of source/stage notification order. Standing box primitives use the
shared sizes, 0.62 trunk taper and canopy base at 68% trunk height; zero dimensions
retain legacy geometry. They do not reproduce the cosmetic mesh topology. The
original trunk and canopy rotate in actor space during Falling; settlement retains
the full-size timber at the final pose, adds cut faces and removes the canopy.
The actor root and exact identity stay fixed. Settled logs disable the root sphere
and use their procedural mesh visibility collision. Combat windup and execution
validate reach against the closest point on the server-built timber triangles,
including cut faces, rather than the identity root. The final axe-damage guard
uses the same check; standing trees retain root range and Falling remains
untargetable. No client hit point is accepted. Drop presentation translates the
original two-piece bundle to actor-local half trunk height along +X on the root
plane, matching the settled timber midpoint. Its procedural mesh is the sole
visibility target; the root sphere stays disabled. Server physical pickup range
uses this same dimension-derived anchor, independent of client hit points or
mesh availability. Stage/dimension repnotifies reconstruct the bundle position;
actor transform, exact identity, reward/persistence transaction and rejection
feedback remain unchanged. Non-shipping pickup fixtures aim/position at this
anchor while identity/suppression observations continue to check the original root.

Forest initialization enables movement replication, sets generated position/yaw
and forces a network update. `bNetUseOwnerRelevancy` follows the always-relevant
terrain patch so suppression and replacements share an audience; unowned
reconstruction falls back to ordinary relevance. Legacy nodes retain independent
relevance. Exact-ID host/client replacement coverage and separate Trunk/Drop
third-peer late joins have passed; shared-audience performance remains unmeasured.

### Sparse persistence and lifecycle limits

Only successful pickup records the forest ID through the bound population callback.
A rejected/full-pack/duplicate request cannot grant extra rewards or deplete a live
drop. The final player leaving a patch destroys tracked actors without invoking
harvesting; one departure retains a patch shared with another player. Reactivation
and process restart filter collected IDs and recreate uncollected trees. Partial
health, trunks and uncollected drops are session state and reset to Standing when
actors regenerate. This uses the existing save schema without migrations.

Focused acceptance covers exact-ID stage delivery, late joins, ordinary-camera
pickup, forged/range/physical-distance and capacity rejection, competing owned
pickup requests and owner-only full-pack feedback. Isolated server-world tests
cover shared departure, eviction and reactivation; a two-process real-slot test
covers disk persistence using production callbacks/loading. These are distinct
boundaries: the restart fixture does not repeat pickup or establish live peer
streaming/restart. Headless feedback proves cue submission, not audible playback.
Two remote contenders, adverse networks, rendered manual usability, activation
cost and final integrated tree-feature acceptance remain unverified. See the
[setup acceptance summary](07-development-setup.md#generated-forest-current-acceptance-2026-10-17)
for the retained commands, evidence and next implementation boundary.

### Ordinary-camera wood pickup (2026-10-17)

Local candidate selection and the server interaction handler share the same
visibility query. The original short camera trace keeps priority. If it misses,
a second trace extends by the camera-to-pawn distance (bounded to 500 cm camera
offset), and accepts only its first blocking hit when that actor is a live wood
drop inside the pawn's physical interaction range. For this fallback the
transaction receives pawn-to-drop distance, so the third-person boom does not
consume pickup reach. Existing server drop eligibility, independent physical
reach, inventory capacity, exactly-once reward and sparse-write checks still
apply. Standing trees, trunks and all non-drop interactions retain their short
trace behavior. No client-supplied hit, distance, reward or new RPC is introduced.

### Sea-level river map classification audit — 2026-10-18

Detailed minimap and expanded-map tiles interpolate final carved terrain on the
shared 125 cm triangles. Their signed sea depth is the shared
`SeaLevelWorldHeight` (zero) minus interpolated terrain height; positive depth
selects water independently of source biome or narrow RiverWeight. Regional
water depth remains a separate signed interpolation for lake/stream surfaces.
Do not clamp individual vertex depths before interpolation: dry bank vertices
must retain their negative contribution at the shore. Expanded overview tiles
use final regional height against the same sea constant, plus regional water.
Both expanded paths select Ocean water texture when classified wet.

Source tracing and signed-depth formula checks confirmed this classification;
no rendered preview, compiled sampler or peer execution was performed here.
Coarse overview pixels can miss channels between sample positions; this audit
does not add river widening or guarantee visibility at every zoom.

### Runtime tool progression and catalogue-only profiles — 2026-10-23

Station acquisition and upgrade entries are generated only for profiles that
map to a supported `EKalmalaToolKind` lifecycle definition. Registered
catalogue-only profiles for later or unimplemented items remain outside runtime
station progression until that lifecycle mapping exists; they cannot invalidate
progression for already supported tools.


## A9 gemstone mining runtime — 2026-10-23

After accepted A8/I4, the server enables the two enemy gem sources: generated
Elderwood Moon Bears select one Moonstone at 1/50, and generated Mossy Mire
Mirelings select one Dusk Diamond at 1/100. The server authenticates each stable
spawn ID against the current generated descriptor, biome and species. The
optional gem is persisted in the credited owner's pending reward payload;
capacity rejection retains that payload without rerolling, and a successful gem
claim is removed durably before its inventory publication. No client supplies
a gem, roll or quantity, and general material inventory remains session-only.

A distinct existing scroll location can commit its own discovery while reusing
an already learned, allowlisted effect entitlement. The same location still
rejects duplicate claims; no second learned-effect record or scroll stack is
created. This preserves the four existing scroll routes and their owner-bound
persistence without new scrolls or save-format changes.

Gemstone nodes use equipped Basic pickaxe attacks, never interaction mining. The server validates the pick family/tier, range, health and condition. Each of three accepted hits resolves its 10% smash before extraction; condition and sparse population schema-2 hit/outcome writes precede presentation. Failed writes do not publish node progress and the deterministic per-hit roll is stable on retry. Partial hits, shattered rocks, loose gems and consumed claims reconstruct from sparse state. A loose gem grants exactly one matching catalogue item after capacity validation; full packs retain it. Relevant peers receive node progress/outcome, while inventory stays owner-only. Inventory/gear remain session-only; this adds no general pack restoration. Source generation is enabled separately after its acquisition entry is complete.


A9 world treasure is attached to existing seeded PointOfInterest discovery actors in Elderwood, Shimmering Lakes and Thunder Mountains, as explicitly approved on 2026-10-23. Server-generated schema-2 treasure records retain even empty results and one shared consumed flag. The actor retains its original discovery route and adds a chest with an open/claimed cue; no new location, player-storage entry, item or buildable is created. Claiming the entire loot validates canonical descriptor, authority/range/living owner and inventory capacity, then commits the consumed save state before publishing removal. Full-pack and persistence failures retain availability. Enabled source entries control only initial generation, never reroll an existing chest; Lakes rolls are domain separated. Lake-water-dependent Rillstone placements are currently suppressed by the compiled basin policy. General material inventory remains session-only.

A9 source audit found Frost Salt mineral deposits existed only as contracts/fixtures. The user approved an additional bounded one-deposit-per-60-m-key pass on 2026-10-23. It uses 32 distinct deterministic candidates, dry slope-safe Tundra terrain and 250 cm separation. Ordinary candidates and frostmoss stay unchanged. Ordinary population multipliers apply before retaining this separately budgeted slot. A single 1% selection replaces it with Star Opal; the other 99% uses the existing Frost Salt mining/yield contract.

Opal Helm now uses owner-only replicated equipped Level and a server-only same-pawn GUID level ledger. Its Inventory actions submit only the equipped GUID to throttled server Forge maintenance. A visible same-world Forge within 2.5 m is required; paid upgrades match the target item level to the effective Forge level, while free repair accepts that Forge without a target-level gate. Authored costs double with current level through the existing item-level cap 10; existing station attachments currently raise a Forge only to level two, so higher authored tiers retain that existing station prerequisite. Armor/maximum condition grow by 1/10 per level and upgraded condition is restored. Head-slot wear and damage read that equipped level. Levels/wear are not saved or transferred across pawn ownership; this does not add a general gear-save system. Other wearables remain level one through existing actions.

Ordinary third-person interaction extends the camera visibility query for loose gemstones and unclaimed world treasure using the same bounded camera-offset rule as wood drops. The server still checks physical pawn-to-target reach and blocking visibility. This changes camera reach only; intact gemstone mining remains the normal equipped-pick combat route.
