# Gathered hearths and camp crafting

Every new character carries a Construction Hammer, shown in the right hand. Press **B** or the controller **View / special-left** button to open its **Build and craft** menu. The menu no longer repeats the current keyboard/controller binding beside its action labels; current mappings remain visible and remappable in Options > Controls. `CraftMenu` is an ordinary Unreal action mapping in `Config/DefaultInput.ini`, so projects and local input configurations can remap the entry point. The menu supports mouse buttons, Tab/button focus, Up/Down or D-pad selection, Enter/A to craft or build the selected hearth, floor, wall, or roof, Y to build/place, P for a local preview, X to refuel, RB to light, and Escape/B to close. Opening blocks movement/look; closing restores them and the previous cursor visibility. Settings closes the menu before opening, and map/the menu cannot open over another modal. Text, selection arrows, ingredient counts and explicit unavailable reasons convey state independently of colour. The panel scales with viewport DPI and scrolls at small resolutions.

## Recipes and transactions

The server-local `KalmalaRecipeCatalogue` loads these original recipes from schema-version-4 `Content/Data/GameCatalogues.json` alongside the item definitions. Unreal stages the `Content/Data` directory through UFS for packaged builds. `RequiredStation` is an array; any listed visible same-world station within 250 cm satisfies that recipe. The optional `RequiredTool` names a catalogue item that must be present in the server-owned pack and is never consumed by the recipe exchange. Food recipes use only their listed ingredients. A usable lit hearth with positive heat must be within 250 cm of both player and station for Cooking Rack, Cauldron, and Frying Pan recipes; schema 4 has no Smoke Frame station or recipe. Hearth fuel burns continuously by server time; a cooking batch adds no fuel debit. Recipe IDs, ingredient and tool IDs, duplicate definitions, quantities, output stack limits and batch arithmetic are validated before use; malformed or missing JSON fails closed. Disabled recipes are locked server-side.

Looking at a Cooking rack, Hearth cauldron, Frying pan, or Workbench displays a remappable **Interact** prompt. Pressing **E** (the default) sends the existing server-validated interaction; the server confirms the target and sends the station identity to its owner. The Cooking Rack opens in the shared themed station-context shell with only Cooked boar meat and Cooked deer meat. The Cauldron uses the same shell with only Meat stew and Root vegetable soup. The Frying Pan uses it with only Roasted root vegetables and Deer and rutabaga roast. Each cooking selection shows its food description, one-batch ingredient counts, one batch per press and the five-batch request limit, plus current hearth-heat availability and a Cook action. Tool, construction, eating, storage, and unrelated recipe controls are hidden in all three menus. Forge Craft remains the production path for making a Frying Pan from five Iron, and the B Construction Hammer menu remains the Build path for placing the pan. Workbench opens its Craft section with only recipes requiring that station or producing its matching Tool Rack attachment, plus the existing Bronze Axe tool operation. It shows ingredient counts, station requirements, effective Workbench level, and whether the Tool Rack is attached. The shell binds to the exact server-accepted actor and stable construction ID and closes when the target is destroyed, leaves range, or the local pawn changes. These filters and status are presentation only: each Cook request still uses the existing server path, which independently resolves and validates the nearby station, live hearth heat, and atomic ingredient exchange. The B Construction Hammer menu continues to show the full build catalogue and placeable items.

Interacting with a placed Chest opens the same themed shell in **Store**. Its two owner-local selectors show the current pack and that accepted chest's contents, including item names, descriptions, icons, and counts. The summary shows occupied pack/chest stacks against their 16-slot limits; unavailable actions identify full stack or slot capacity. Store and Take each request one item by its selected ID through the existing RPCs. The server rechecks its active chest's visibility/range and current pack, writes the chest candidate before publishing the pack change, and keeps the chest snapshot owner-only. Closing, leaving range, changing pawn, or destroying the chest clears the view and closes the context. The older CraftMenu storage controls remain available until the later Build-menu cleanup increment.

| Recipe | Ingredients per unit | Station | Maximum batch |
| --- | --- | --- | --- |
| Hearth ring | 5 fieldstone + 3 splitwood; also consumes 1 raw fuel item to start with 60 seconds of fuel | Construction Hammer; clear ground ahead | 1 per placement |
| Joiner's bench | 9 splitwood + 6 reed fibre + 2 fieldstone | Handcrafted | 1 |
| Chest | 6 splitwood + 8 reed fibre | Handcrafted | 1 |
| Cooking rack | 9 splitwood + 8 reed fibre | Handcrafted | 1 |
| Hearth cauldron | 9 splitwood + 6 reed fibre + 3 fieldstone | Handcrafted | 1 |
| Timber floor | 6 splitwood + 4 reed fibre | Construction Hammer; valid ground | 1 per placement |
| Windbreak wall | 6 splitwood + 6 reed fibre | Construction Hammer; valid ground | 1 per placement |
| Reed roof | 6 splitwood + 8 reed fibre | Construction Hammer; valid ground | 1 per placement |
| Cooked boar meat | 1 BoarMeat | Visible Cooking Rack plus usable lit hearth with positive heat within 250 cm of both | 5 |
| Cooked deer meat | 1 DeerMeat | Visible Cooking Rack plus usable lit hearth with positive heat within 250 cm of both | 5 |
| Meat stew | 1 BoarMeat + 1 DeerMeat + 2 Carrot + 2 Potato | Visible Cauldron plus usable lit hearth with positive heat within 250 cm of both | 5 |
| Frying pan | 5 Iron | Visible same-world Forge within 250 cm; place the crafted pan with the Construction Hammer | 1 |
| Root vegetable soup | 1 Carrot + 1 Potato + 1 Rutabaga + 1 Onion | Visible Cauldron plus usable lit hearth with positive heat within 250 cm of both | 5 |
| Roasted root vegetables | 1 Carrot + 1 Potato + 1 Onion | Visible same-world Frying Pan plus usable lit hearth with positive heat within 250 cm of both | 5 |
| Deer and rutabaga roast | 1 DeerMeat + 1 Rutabaga + 1 Onion | Visible same-world Frying Pan plus usable lit hearth with positive heat within 250 cm of both | 5 |

Iron is defined as a catalogue material for the Frying pan recipe; no Iron gathering source is currently configured.

`Fuel`, `ConstructionSupply`, and `RaisedStorage` are retired item IDs. Old saved chest `Fuel` stacks normalize to Wood; each saved `ConstructionSupply` becomes 3 Wood and 2 Fibre. Storage save schema 1 is unchanged. The normal storage construction is presented as **Chest** and is the only chest variant. Raw Wood, Lightwood, Densewood, and Coal each add 60 seconds to a hearth and burn at one fuel second per elapsed server second. Coal is accepted as fuel when present, but the current world loot catalogue has no Coal source.

M9 free tool repair supersedes M7's material-paid repair and broken-tool
replacement routes. The owner selects any damaged carried tool in Camp
crafting; the server reads its condition and restores it for free at a visible
same-world Workbench or Forge within 250 cm. Repair spends no inventory and
awards no Crafting experience. The client submits only the tool ID. The M9
Workbench shell now provides a separate owner-local Repair section with tool
selection and current level/condition; it reuses this same ID-only request and
server validation. The M9 Grinding Stone `Repair All` action remains a later
increment; see `27-m9-carried-tool-inventory.md` for the current progression
boundary.

The panel shows the selected recipe and its position in the catalogue above
the longer hearth and status details; Previous and Next continue to navigate
the full catalogue. The selected-recipe detail spells out its batch-1 ingredient cost,
maximum batch, output quantity and per-stack limit, the output item description,
station and heat requirements, and the first unmet availability
reason. Previous item / Next item in the storage selector also show the
selected catalogue item description, including raw materials without crafting
outputs. A separate text block lists the owning player's six private skills,
their current level and experience toward the next level, and the nearest
locked recipe with its current/required experience; accepted recipes that
award that skill identify the experience earned per request. This block reads
only the local owning pawn's owner-only progression array, follows local text
scale and contrast, and adds no focus or input. The camp panel submits batch 1
per press. Rejected exchanges preserve ingredients and tool condition. Tool
status and repair actions show owner-only condition and the no-cost
Workbench/Forge station requirement in text.
Button help text explains crafting, preview, placement, fire, food, repair,
and storage actions. Keyboard and controller instructions, the `>` selection
marker, and plain-text requirements and failure feedback keep controls and
state understandable without colour alone. The panel has no private
processing-station inventory or fire authority: the server still resolves
stations, hearth heat, costs, and inventory exchanges through the existing
construction, hearth, and pack paths.

Hearths and accepted workbenches remain assembly stations for recipes that produce camp or station kits; hearths need not be burning. The Construction Hammer builds the hearth ring, floor, wall, and roof directly, so they need neither an assembly station nor a kit stack. The hearth consumes one raw Wood, Lightwood, Densewood, or Coal during placement to start with 60 seconds of fuel. Its `CampfireKit` ID remains an internal placement identity; normal crafting cannot produce that item. Recipes may list one or more required stations; the server selects a visible same-world instance within 250 cm and never accepts a client-selected station. Remaining camp-kit stacks stay bounded to 5 and station attachments to 10.

The Field Hatchet replacement recipe is available only after the hatchet reaches zero condition. The server validates its existing tool definition, visible same-world workbench, and listed materials, then atomically pays 2 splitwood, 2 fieldstone, and 1 reed fibre before restoring its owner-only condition to 24/24. Rejected station, material, duplicate, or malformed requests change neither inventory nor condition. The existing repair action remains available for worn tools. Accepted replacement awards one fixed 10 Crafting experience after payment; no tool item or save field is created.

The Stone Pick replacement follows the same server transaction and visibility rules, with 2 splitwood, 3 fieldstone, and 1 reed fibre. It is available only at zero condition, pays before restoring the existing owner-only 20/20 condition, and awards one fixed 10 Crafting experience after acceptance. A worn pick can still be repaired for its matching gathered Stone instead. Missing workbench/material, duplicate, batch, and malformed attempts leave inventory and condition unchanged; no tool item or save field is created.

The Reed Knife replacement uses 1 splitwood, 1 fieldstone, and 2 reed fibre at the same visible same-world workbench. It is available only at zero condition, pays before restoring the existing owner-only 16/16 condition, and awards one fixed 10 Crafting experience after acceptance. Repair with gathered Fibre remains available for a worn knife. Intact-tool, missing-workbench/material, duplicate, batch, and malformed attempts leave inventory and condition unchanged; no tool item or save field is created.

### Current catalogue food processing and meal use

Schema 4 defines six food recipes: CookedBoarMeatRecipe and
CookedDeerMeatRecipe use the Cooking Rack; MeatStewRecipe and
RootVegetableSoupRecipe use the Cauldron; RoastedRootVegetablesRecipe and
DeerRootRoastRecipe use the Frying Pan. Their outputs are CookedBoarMeat,
CookedDeerMeat, MeatStew, RootVegetableSoup, RoastedRootVegetables, and
DeerRootRoast. Every recipe has a five-serving batch cap and each output stack
is capped at twenty. Recipe ingredients and station requirements are listed
in the catalogue table above and in docs/28-m9-camp-equipment-recipes.md.

For each recipe, the server selects a visible same-world station and requires
a usable lit hearth with finite positive heat within 250 cm of both player and
station. Missing access, heat, ingredients, valid batch, or output capacity
rejects the whole inventory exchange. Accepted requests award one fixed 10
Cooking experience after the exchange succeeds, regardless of batch size.
Recipes have no Cooking-level lock. Hearth fuel burns at the normal
one-fuel-second-per-second rate; a serving batch charges no separate fuel or
processing timer.

The current schema-4 catalogue has no Smoke Frame, smoke recipes,
RoastedFieldMeat, or SmokedFieldMeat. HearthBroth remains a catalogue item
with no production recipe; it is still recognized by the existing transient
steady-meal effect. Meal use is server-authoritative, consumes one item only
when the slot is free, and grants 120 seconds of 10% lower stamina cost.
Inventory exposes the same action without a station requirement. The server
revalidates the allowlisted item, owner pack quantity, and free status slot;
rejected or duplicate use preserves both item count and timer. Food is optional,
and food inventory and the effect add no save field.
## Build menu and local placement preview

Choose a construction entry in the hammer menu and press **P** or select **Preview placement**. The local presentation probes generated collision 165 cm ahead and reports a readable valid/invalid result for terrain availability, slope, water, and nearby pawn blocking. It supports hearth, workbench, Forge, Chest, cooking rack, cauldron, Frying Pan, floor, wall, and roof constructions; ingredients and raw materials do not offer a placement preview. The preview does not spawn or reserve an actor, alter inventory, send an RPC, or write save data. It is only a local aid: the later server request revalidates every placement condition.

The hammer build menu places a hearth ring, floor, wall, and roof directly from materials. A hearth costs 5 Stone and 3 Wood, plus one raw Wood, Lightwood, Densewood, or Coal that starts it with 60 seconds; floors cost 6 Wood and 4 Fibre, walls cost 6 Wood and 6 Fibre, and roofs cost 6 Wood and 8 Fibre. All construction recipes consume raw materials directly. Players no longer craft or carry the four direct-build items, and no intermediate `Fuel` or `ConstructionSupply` item is produced. Other camp constructions also consume their listed raw-material recipes.

For a hearth ring, the client sends the existing payload-free placement intent. The server requires the owner's carried Construction Hammer, selects the JSON-backed raw recipe cost and one raw fuel item, derives the probe from the owning pawn, validates immutable world identity, range, generated-terrain collision, ground support/slope, water, overlap and the session limit, then allocates the hearth before atomically consuming materials. For floors, walls, and roofs, the client submits only the existing canonical construction identity; the server derives position and yaw from the pawn, validates placement and save capacity, then allocates the replicated construction actor before atomically consuming raw materials. If actor creation or persistence fails, the original inventory candidate is restored. Stable `CampfireKit`, `FloorKit`, `WallKit`, and `RoofKit` runtime identities remain for construction placement and save compatibility; these are not catalogue items. A client supplies no transform, rotation, collision result, world identity, cost, fuel selection, or spawn state.

Every accepted generic construction actor receives a server-generated opaque construction ID that replicates with its construction identity. `UKalmalaConstructionSaveGameV2` schema 2 stores at most 128 records overall, including no more than 32 station-attachment records, under the exact immutable `WorldSeed`/`GeneratorRevision`; duplicate IDs, malformed records, overflow, and mismatched worlds fail closed. Before a generic placement is committed, the authoritative game mode confirms that the identity-scoped camp save has room. For a placement write, the server clones the validated schema-2 candidate or explicitly migrates a matching schema-1 slot, adds the server-created construction or attachment record, and revalidates every record and bound. It synchronously saves the candidate before replacing the in-memory save object. An invalid or mismatched existing slot is left untouched, and any failed write destroys the actor and restores the exact pre-placement inventory candidate, whether payment used a kit or direct raw materials. On a matching-world listen-server restart, the server restores only validated records as replicated actors before players join; station levels are derived from those restored attachments. Hearths and carried inventory remain outside this construction save. Storage contents use the separate bounded container below.

Accepted floor, windbreak wall, and roof constructions resolve their original procedural piece presentation and collision footprint from the replicated construction identity. The server creates and persists the actor, and collision is `240 x 240 x 24 cm` for floors, `240 x 24 x 220 cm` for windbreaks, and `264 x 264 x 32 cm` for roofs. Only server-accepted `WallKit` actors receive `KalmalaShelterWindbreak`, and only server-accepted `RoofKit` actors receive `KalmalaShelterRoof`; the existing server-only shelter sampler relies on those tags after its visibility trace. Clients derive the same collision/presentation from the replicated identity but cannot set tags, geometry, transforms, or shelter outcomes through an RPC.

The replicated pawn crafting component accepts a recipe ID and integer batch through an owning-player server RPC. It finds any required nearby usable station itself; clients supply no station, ingredient costs, output ID, fuel selection, or reward count. It rejects missing or distant stations. A new inventory exchange builds a validated scratch pack, applies all costs and output-capacity checks there, then publishes the entire array once on the game thread. Failed exchanges change nothing; a fully consumed ingredient stack can free an output slot. Requests on one player are rate-limited to one action every 0.2 server seconds. Distinct players use separate private inventories; their requests can overlap without sharing a mutable pack. Detailed stacks and request feedback remain owner-only.

## Workbench and storage

Place a Joiner's bench, Forge, or Chest through the existing paid construction path. Their original procedural silhouettes and conservative collision bounds derive from the normally replicated construction identity; none adds shelter tags. The Joiner's bench and Forge each start at tool-station level 1. The Forge recipe consumes its former timber-supply value as 15 Wood and 10 Fibre, plus 6 Stone. The hammer builds floor, wall, and roof directly without requiring a bench. Crafting interactions at a bench or Forge still use the existing server view trace and require the station within 250 cm and visible from the authoritative pawn's eyes.

In Build and craft, use **Inspect nearby chest**, **Previous item** / **Next item**, then **Store one** / **Take one**. These focusable controls retain ordinary Tab/button and controller navigation. The selected item name, authorized chest stacks, empty/unavailable states, and result text do not depend on colour. The UI reads only its owning pawn's crafting component. The normal traced interaction can also inspect a chest before opening the menu.

`ServerOpenStorage` selects the nearest visible, reachable chest on the server; no client target or stable ID is accepted. `ServerDepositStorage(ItemId)` and `ServerWithdrawStorage(ItemId)` request exactly one item. Each request shares the 0.2-second crafting cooldown and rechecks authority, controlled pawn, same world, saved construction identity/transform, 250 cm range, line of sight, valid catalogue item, available source count, unique stacks, and destination capacity. Chests are shared among connected players who can reach them. Only the inspecting pawn's owner receives a bounded content snapshot; the shared actor never replicates contents. Every action reloads the current server container, and the server refreshes or clears the view every 0.25 seconds. Closing the menu clears it explicitly. Invalid or out-of-range requests cannot address remote storage.

`KalmalaStorageSaveGame` is a separate schema-1 slot, `KalmalaStorage_<seed>_<revision>`, with at most 128 construction IDs and 16 unique catalogue-bounded stacks per chest. It changes no existing save schema. Loading validates the entire container, exact immutable world identity, IDs, and stacks; an existing invalid/incompatible slot disables storage without overwriting it. Only a matching registered `StorageKit` construction may access a record; newly placed or older empty chests begin empty. The server validates scratch copies of both containers, saves the candidate chest container synchronously, then publishes the pack on the same game-thread operation. Failed validation or save leaves both live containers unchanged. Sequential requests always read fresh contents, so stale owner snapshots cannot withdraw an item twice.

Carried inventory remains pawn-lifetime: this is persisted chest content, not player inventory persistence or crash-proof distributed inventory storage. Reconnect resets carried items as before. Chests have no removal/refund, ownership locks, material wetness, or movable-placement operation. The later complete M2 persisted-camp scenario remains unchecked.

## Paid placement and fuel

Gather 5 fieldstone and 3 splitwood, carry one raw fuel item, face clear ground, then choose **Build hearth**. The server uses its pawn's position and facing to probe ground 165 cm ahead. It checks the authoritative immutable world identity, nearby generated collision, slope, water, overlap, a 250 cm range, available ingredients and a 32-hearth session limit. The client supplies no position or transform. The server allocates a deferred native hearth, atomically consumes the raw stone and wood plus one Wood, Lightwood, Densewood, or Coal, then initializes and finishes spawning it. Failed allocation or validation does not consume ingredients. This deliberately small placement action has no movable ghost or general construction contract yet.

Each paid hearth starts unlit with 60 seconds of fuel. **Light hearth**, or the existing traced interaction, lights only a usable nearby unlit hearth with positive fuel and less than 90% fuel wetness. **Add raw fuel** consumes exactly one Wood, Lightwood, Densewood, or Coal for 60 seconds, only when that entire amount fits under the 300-second cap. Refuelling never resets wetness. Lit fires consume one fuel second per elapsed server second; at zero they extinguish and contribute no warmth. Unlit fires retain unused fuel. The default empty actor cannot light for free.

Existing server weather wetting, drying, extinguishing and effective warmth rules remain in use. When rain multiplied by wind reaches the existing 0.65 Highly Active storm threshold, exposed fuel wetting ramps linearly up to 25% faster at full storm intensity. An accepted roof removes rain input; an accepted windbreak removes wind input and its added storm surge. Server traces for `KalmalaShelterRoof` and `KalmalaShelterWindbreak` suppress direct rain and wind respectively; untagged geometry never claims protection. Natural gameplay shelter tags must still be attached only by accepted construction. The local crafting panel presents a read-only, colour-independent nearby-hearth summary with separate State, Fuel, Fuel condition, Rain protection, Wind protection, and Access lines. It explicitly reports `LIT`/`EXTINGUISHED`, remaining seconds, whether fuel is dry enough to light or too wet, and whether each weather protection is active or exposed. A locally generated original stone ring and the existing warm light provide world presentation.

Hearths are shared within the session by default. Trusted server code may set owner-only access; crafting, lighting and refuelling enforce it. No client sharing setter or private-inventory inspection is exposed. Lighting/refuelling RPCs take no target, fuel wetness, warmth, duration or lit-state value. Terrain, sparse generated-world saves and player save schemas are unchanged. Hearths and carried materials currently last only for the session/pawn; this does not complete M2's later persisted-camp acceptance gate.

## M12 Forge Craft section

An accepted Forge interaction opens the shared station-context shell in Craft.
The section includes only Forge-required recipes and the matching Forge anvil
attachment recipe. Selected recipe details continue to show catalogue material
costs, station requirements, and current server-reported availability. A
station status line reads effective Forge level and whether its anvil is
attached from the exact owner-visible accepted actor. This status is
presentation only: crafting still uses the existing recipe request and the
server independently resolves a visible same-world Forge and rechecks the
recipe, materials, inventory exchange, and station prerequisites. The older
CraftMenu remains available.

## M12 Forge Upgrade section

The Forge context has a separate Upgrade section for the authored Bronze Axe
level-one to Iron Axe level-two progression. Its comparison shows the carried
Bronze Axe's level and condition and the Iron Axe's starting level and full
condition, followed by the Forge, Crafting level/tier, previous-tool, and
material requirements with carried counts. The first unmet requirement is
shown, and Upgrade is enabled only while the owner-visible state reports the
existing progression as ready. The action reuses `ServerProgressTool` with the
Iron Axe tool ID; the server still selects a visible same-world Forge and
validates level, skill unlock, carried Bronze Axe, materials, and persistence
before applying the atomic exchange. The accepted Forge's effective level and
anvil state remain shown from its exact owner-visible context. No station ID,
cost, outcome, RPC, replication, or save-schema change is introduced.

## M12 Forge Repair section

Forge also offers a separate Repair section using the owner's carried-tool
condition inspector. Its tool selection is independent from Craft's recipe
selection and Upgrade's Bronze-to-Iron comparison. Repair submits only the
selected tool ID through the existing free-repair request. The server reads
the current carried record and keeps its existing visible same-world Workbench
or Forge range check at 250 cm; it does not accept a client target, condition,
cost, or repair result. The local menu remains bound to the exact accepted
Forge actor and closes when that context becomes invalid, while server repair
may still use another qualifying nearby station as before.

## Verification

Run the focused `Kalmala.Gameplay.Storage` tests for save round-trip/identity/bounds, transfer conservation and capacity, server RPC payloads, and owner-only replication. Run `Scripts/Verify-Storage.ps1` for a fresh temporary host/client followed by a restart of the same host user directory. Both owners place paid chests/workbenches and check direct floor-build availability from Wood and Fibre, reject distance/obstruction/unknown-item transfers, preserve contents on a simulated write failure, then issue real owner deposit/withdraw RPCs. Both must retain exactly three saved wood per chest and one private carried wood; client-local mutations fail, simulated peers have no chest snapshot, and closing clears the owner view. Restart must restore the same two chest IDs and exact contents. The fixture uses temporary grants and flying stationary pawns; it does not complete the later gather/build/weather camp scenario or rendered UI acceptance.

Build `KalmalaEditor Win64 Development -WaitMutex -NoHotReload -Force -MaxParallelActions=4` with UnrealBuildTool local-cache access, as in `07-development-setup.md`.

Run the focused headless automations `Kalmala.Gameplay.Crafting`, `Kalmala.Gameplay.Crafting.NetworkContract`, `Kalmala.Gameplay.Construction.LocalPreview`, `Kalmala.Gameplay.Construction.SaveContract`, `Kalmala.Gameplay.Inventory` and `Kalmala.Gameplay.Campfire` with the documented temporary user/log paths and memory DDC. The crafting network contract locks `ServerCraft` to recipe identity plus batch only and requires placement, refuelling, and lighting intents to remain payload-free. The local-preview test verifies the complete camp/construction-kit scope and fail-closed textual feedback without a local presentation context. The save contract covers memory round-trip, identity mismatch, duplicate/cap rejection, and non-finite record rejection. Transaction coverage includes required recipes, invalid batches, overflow, duplicate/invalid definitions, missing ingredients, full output stacks, full slot counts, reclaimed slots and repeated attempts without duplication or partial consumption.

Run `Scripts/Verify-Crafting.ps1` (or `-Rendered` for retained 1280x720 host/client menu screenshots). It waits for two server pawns before starting the launch-gated fixtures. Both players exercise paid placement against real generated collision, overlap rejection without payment, full/missing ingredients, disabled and unknown recipes, distant/locked stations, raw-fuel capacity/exhaustion, actual roof/windbreak traces, rain extinguishing, and wet-lighting rejection. Both owning peers then send real crafting RPCs, including immediate duplicates, insufficient-resource attempts, forged recipes and extreme batches, and an insufficient-resource placement request. Each owner must end with exactly two Workbenches and no leftover materials. The client must observe two dry-lit and two rain-extinguished fire snapshots matching the server state. The menu check verifies cost text, focusability and movement restoration; Workbench Craft scope must report only the Bronze Axe tool option, Grinding Stone, and Tool Rack, with their existing prerequisites. Fixtures allocate temporary actors and use separate temporary user directories; they add no save format or normal-play grants.

The same crafting presentation check verifies owner-only skill rows, experience
to the next level, the nearest skill-gated recipe unlock, and the accepted
recipe experience source. `-Rendered` retains host/client captures for layout
review; it does not replace scaled-font inspection at every viewport size.

Run `Scripts/Verify-ConstructionPersistence.ps1` after the focused construction contracts. Its first listen-server/client launch uses the ordinary hammer placement path to reject invalid or unpaid builds and accept two floors paid with raw Wood and Fibre. It compares the client's unique replicated IDs with the two paid server IDs. It then restarts the same temporary host user directory and requires the exact original two IDs to restore. The restarted crafting fixture also pays raw materials for two new floors: all four IDs must be distinct and the reconnecting client's ID set must equal the restored-plus-new server set. Repeated replication log lines cannot substitute for distinct actors or matching identities. The client submits no transform, rotation, preview result, construction ID, save record, or authoritative world data. The scenario's extra test actors and save slot are confined to its temporary user directory.

Retain `Scripts/Verify-InventoryReconnect.ps1` and `Scripts/Verify-CampChoices.ps1` as regressions for private inventory and existing weather/exposure recovery. The old camp-choice/exposure fixtures now explicitly grant and consume test fuel before lighting; normal play receives no free ingredients. Dedicated-server verification still requires a server-capable engine distribution. These checks are not malformed-packet fuzzing or assistive-technology certification.

The development-only crafting fixture first tries eight headings from the current pawn position. If restored construction leaves no valid hearth site there, it probes at most 24 nearby terrain positions within 18 m, testing eight headings at each. Each attempt still uses the ordinary server validation and payment path. This bounded test relocation neither removes restored pieces nor changes normal player movement, placement rules, or saves. Failure reports the original position and rejection reason.

Known verification defect (2026-09-11): Kalmala.Gameplay.Construction.GeometryAlignment currently fails because floor/wall/roof visual centres are offset -44/+54/+248 cm from their blocking box centres. Extent-only ShelterPieces coverage did not detect this. Shelter and movement-collision acceptance remains incomplete until geometry alignment and the peer scenario pass; see PROGRESS.md for retained evidence.

### Shelter geometry repair 2026-09-11

Floor, wall and roof procedural solids now use the collision-centred actor origin and the shared kit collision extent. This removes the obsolete -44/+54/+248 cm presentation offsets without changing placement, persisted transforms, collision, RPCs or server shelter sampling. GeometryAlignment and four related construction/shelter tests pass; live host/client movement acceptance remains pending.

### Construction shelter sampling regression 2026-09-11

`Kalmala.Gameplay.Construction.ShelterSampling` creates actual initialized construction actors in a transient collision world and calls the production shelter sampler. It verifies roof plus upwind wall protection, roof-only protection after wind reversal, loss of protection outside geometry, and rejection of blocking floor geometry as roof or windbreak. Pawn-sized capsule sweeps must hit each floor/wall/roof solid without starting in penetration. Run with `Kalmala.Gameplay.Construction` and `Kalmala.World.EnvironmentalExposure.ShelterComposition` using the documented headless flags after a forced editor build. This covers physics queries, not live Character Movement or network acceptance. Production exposure remains calculated by server GameMode and published through the authority-checked character setter; the sampler itself is a geometry-query utility, not an authority boundary. No removal/refund feature is included.

### Live construction movement regression 2026-09-11

After the forced editor build, run `Scripts/Verify-ConstructionMovement.ps1`. Its non-shipping `-KalmalaConstructionMovementTest` fixture waits for two players, spawns isolated elevated server-owned floor/windbreak actors per player, then teleports each pawn onto its fixture. Ordinary owning-player movement input must travel more than 50 cm, retain the floor as its grounded Character Movement base, and stop within 3 cm of the wall face minus capsule radius. The runner requires host-owner, remote-owner, and server-observed remote success plus exact remote player/floor/wall identity agreement and stopping-position agreement within 3 cm. The conflicting-seed client must receive the server identity. Tests use temporary user directories and add no camp save records, paid placement, grants, RPCs, or normal-play behavior. This bounded scenario covers floor/windbreak movement only; roof contact and live exposure comparisons remain required before shelter acceptance closes.

### Replicated roof movement coverage 2026-09-11

`Verify-ConstructionMovement.ps1` now also requires a server-created roof per fixture. After walking to the wall, each owning pawn calls ordinary Jump/StopJumping. Both the owner and authoritative server must observe airborne movement, a peak between 10 cm below and 1 cm above the ceiling-derived capsule-centre limit, and a grounded landing on the same floor. The runner compares exact remote player/roof identity and ceiling plus peak agreement within 10 cm. This tolerance accounts for frame-sampled peaks; the final run's owner/server remote peaks matched exactly. Observation starts before the local jump trigger because peer frame clocks differ. Simulated proxies are not used as movement authority evidence. The fixtures remain temporary elevated solids, not paid camps; no normal-play physics, RPC, saved data, or exposure contract changed. Live server-sampled shelter/exposure agreement remains the next acceptance gap.

### Live shelter and exposure agreement 2026-09-11

The same temporary two-player fixture now surrounds each elevated floor with a replicated roof and four actual server-created windbreak actors. During ordinary `GameMode` exposure updates, the production server sampler must detect both accepted tags and report at least 0.8 shelter; the runner compares the remote owner's wetness, warmth, and travel multiplier to its owning-client replicated exposure snapshot. The pieces are never registered, paid for, or saved. This adds no client-controlled shelter value, RPC, persistence record, gameplay grant, or refund/removal path.

### Persisted-camp scenario preflight 2026-09-11

`Scripts/Verify-PersistedCamp.ps1` runs the retained two-player hearth/crafting, storage/workbench, and construction shelter/exposure fixtures on adjacent ports. It is a preflight harness, not the M2 acceptance scenario: its three launches intentionally retain their own user directories and do not yet prove one shared gathered camp or its combined restart state. It fails on any constituent runner failure, giving the upcoming single-session scenario one stable entry point for its existing contracts.

### Shared persisted-camp hearth slice 2026-09-11

`Scripts/Verify-PersistedCampHearth.ps1` launches one listen server and one conflicting-seed client using `-KalmalaPersistedCampTest`. Each server-owned pawn harvests only its required Wood, Stone, and Fibre from initialized deterministic harvest-node descriptors through the ordinary server interaction path, then uses the production hammer and terrain-validated hearth placement path to consume 5 Stone, 3 Wood, and one raw fuel item directly. Each owner must receive the resulting empty private pack and a nearby replicated 60-second hearth; no client supplies a reward, quantity, recipe cost, placement transform, fire state, or world identity. These test-only nodes are destroyed after accepted harvest and deliberately do not claim the later sparse-delta/restart acceptance. The same one-session fixture must next add paid floor/wall/roof, workbench/chest use, shelter/weather agreement, and combined persistence/reconnect evidence before M2 can close.

### Shared persisted-camp construction attempt 2026-09-11

The same fixture now exercises the server-owned gather/craft/placement paths for a hearth, floor, wall, roof, workbench, and chest per player, followed by a server-validated one-wood chest deposit. The forced editor build passed and both server players logged successful paid builds. The runner intentionally remains failing: in the retained host/client run the joining client stopped advancing before it could publish its owning-player replicated fire, construction, and private storage snapshot. Do not use the two server build logs as a substitute for the required peer agreement. The next increment must diagnose that client tick/replication stall, then add explicit server-sampled shelter/weather comparison before the shared-scenario child can be checked.

### Shared persisted-camp weather and exposure evidence 2026-09-14

The same retained host/client fixture now selects a valid 120-second server storm (cycle 77, precipitation 0.75, north wind strength 1.0) only after each player completes the ordinary gathered, crafted, paid camp. It seeds a wet/cold state only through the authority-checked character setter; the existing GameMode exposure tick then samples the production environmental and shelter inputs and publishes the resulting replicated state. Both local owners must report the storm identity, private pack/fire/construction/storage state, and their changed wetness/warmth; only the server report includes its sampled shelter scalar. Clients neither select weather nor send exposure, shelter, construction, fire, inventory, or storage values. This validates shared-session weather/exposure replication, but does not replace the existing construction shelter-geometry regression or the pending combined save/load and reconnect proof.

### Shared persisted-camp restart attempt 2026-09-14

The first combined restart runner keeps one host save directory and is designed to compare exact construction IDs, owner-only chest contents, real generated harvest depletion, and an incompatible seed. Its initial shared-camp phase passed after a forced editor build, but the internal placement helper currently emits no stable construction ID telemetry. The runner stops before it can claim restart acceptance. Adding development-only report evidence is required; it must not add client-supplied IDs, alter save data, or weaken the identity checks.

Restart retry 2026-09-14: stable-ID test telemetry now exists, but the immediate retained peer run was environment-delayed by earlier local test editor processes before it reached its gameplay assertions. This does not count as restore evidence.

Restart progress 2026-09-14: the restart-only server fixture now reports ten restored constructions and a valid one-wood persisted chest. The outer runner must still complete its reconnecting-peer and sparse-delta/cross-world phases before this is acceptance evidence.

### Combined persisted-camp restart proof 2026-09-14

The final retained two-player run keeps one temporary host save directory. It proves the exact ten server-generated construction IDs from the gathered shared camp restore once each and replicate to a conflicting-seed reconnecting client, while both owners obtain a one-wood owner-only chest snapshot. A real generated harvest node is then consumed through the existing server path and remains absent after the next same-identity restart. Finally, a seed-419 server using that host directory restores no seed-418 construction. The runner and telemetry are development-only; construction IDs remain server-generated, chest content remains owner-only, and clients submit no identity, transform, save, weather, exposure, harvest, or storage state.

## M3 wetness and rain override (2026-09-14)

This section supersedes earlier references to continuous player wetness, warmth-derived wetness penalties, fuel wetness extinguishing, or rain damage below. `Wet` is only a server-owned player debuff with a reusable parameter definition: default maximum duration 120 seconds, unroofed-rain trigger 10 uninterrupted seconds, movement multiplier 0.92, and stamina-use multiplier 1.15. Standing in server-confirmed water applies it immediately. Rain applies it only when the server finds no accepted roof above the pawn. Reapplication cannot exceed 120 seconds; a nearby lit campfire with nonzero authoritative heat removes it. Surface moisture remains a grid material/fire input and is never a second player wetness system.

M3's authoritative defaults make this concrete. Each accepted floor, wall, workbench, and chest begins at 100 health; only those exposed to server-sampled precipitation lose `0.10 * precipitation` health per second, never below 50. Roofs never take rain wear. An overhead roof exists only if the server's 60 cm-up, 400 cm `ECC_Visibility` trace first hits an accepted `KalmalaShelterRoof`; neither client overlap nor an untagged object protects anything. Rain wear is transient in this slice—there is no repair, destruction, refund, collision change, or save delta.

A hearth's replicated state will replace the current boolean with `Extinguished`, `Lit`, or `Smouldering`. `Lit` means positive fuel and either roof protection or less than 0.05 precipitation, with normalized 1.0 heat in the existing 600 cm falloff. An already-lit, exposed hearth at or above 0.05 precipitation becomes `Smouldering`: it continues the normal one-second-per-second fuel burn but has zero heat/light. It returns to `Lit` automatically when roofed or dry again. `Extinguished` has no fuel (or has never passed the existing server lighting validation) and cannot self-light. Existing 60-second raw-fuel items and the 300-second cap remain the configuration defaults. All traces, health, fuel, weather inputs, state transitions, and replicated fields remain server-owned.

The three-state runtime is now implemented. Existing crafting verification checks replicated enum values alongside fuel and heat for both Lit and rain-Smouldering fires. Initial lighting still validates authority, access, distance, fuel and fuel condition; automatic reignition from Smouldering bypasses no client request because it is selected entirely by the server weather/roof update. Fuel wetness is retained as initial-lighting information and legacy telemetry, not a veto on active dry/roofed fires. Roof-driven reignition is covered by real collision in Hearth.RainState; the full live roof-building scenario remains later M3 acceptance.
