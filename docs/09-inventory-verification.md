# Inventory verification

After the editor build in `07-development-setup.md`, run `Scripts/Verify-InventoryReconnect.ps1`. It keeps one revision-4 seed-418 listen server alive across two separate client processes using the same temporary client user directory. Both clients start with conflicting world identity and must receive the server identity.

Each visit requires the owning client to receive seven wood, reject local grant/consume calls, display its own read-only pack, and observe empty simulated-remote contents after owner replication. The server must report one successful inventory and harvest fixture for the host and each new client pawn (three total). Its inventory fixture checks an empty initial pack before granting ten wood and consuming three, so the second visit proves the current pawn-lifetime reset behavior. It does not claim inventory restoration: production reconnects currently lose carried contents. Persistent inventory remains necessary for the final M2 saved-camp gate.

The harvest fixture checks server-selected Wood/Stone/Fibre, full-stack rejection followed by successful retry, distance rejection, uninitialized descriptors, duplicate/depleted interaction rejection, and a single sparse-delta callback per successful grant. Its M7 source fixture additionally checks wrong-tool and full-stack rejection without changing owner tool condition or node state, then accepts the catalogue-selected Wood grant, spends one FieldHatchet condition point, and records one depletion callback. Fixtures use isolated temporary actors and do not write a world-save slot or tool condition. The runner terminates only its own peer processes and retains all three logs in the printed temporary directory.

The same M7 fixture verifies repair against a transient Joiner's workbench: no-station and distant-station rejection, insufficient Wood with no change, one accepted repair to maximum FieldHatchet condition, full-condition rejection, and returned fixture materials. The owning client receives the repaired condition while its simulated remote pawn keeps condition private. It wears the Stone Pick through accepted server-selected mining nodes and the Reed Knife through accepted reed gathering, then verifies each zero-condition replacement at the workbench, exact configured payment, rejection with no mutation when the tool is intact, the station is missing, or materials are insufficient, a single 10-point Crafting award, duplicate rejection, and owner-only condition visibility. This tests server transaction helpers and owner-only replication, not physical rendered crafting input or saved condition.

`Kalmala.Gameplay.Food.CookingStationHeat` verifies the server-owned cooking-station heat rule and timed hearth fuel. Cooking Rack and cauldron recipes require their visible same-world station plus a usable Lit hearth with positive heat within range of both station and player. Recipe exchanges consume only the catalogue ingredients; serving count adds no fuel-item or direct hearth-fuel debit. The hearth continues to consume one fuel second per elapsed server second while Lit. Rejection for a missing station, missing or distant heat source, invalid batch, missing ingredients, or full output preserves the inventory. `Kalmala.Gameplay.Crafting.Transactions` separately verifies raw-fuel selection for hearth refuelling. The owning player's food request carries only an allowlisted food identity, consumes one catalogue item atomically, and applies the fixed 120-second `State.Food.SteadyMeal` modifier. Duplicate use and alternate-meal replacement preserve the corresponding food stacks and the partially elapsed status timer; forged consumption is also rejected. Expiry restores ordinary stamina cost and opens the meal slot again. Cooking experience awards remain post-transaction and server-owned. Food stacks, progression, and effects remain pawn-lifetime, with no save-schema change.

Also run these headless automations with the memory-cache and temporary user/log flags documented in `07-development-setup.md`:

- `Kalmala.Gameplay.Inventory.Catalogue`
- `Kalmala.Gameplay.Inventory.NetworkContract`
- `Kalmala.Gameplay.HarvestNode.AuthorityAndDepletion`
- `Kalmala.Gameplay.Interaction.ServerOnlyRangeValidation`

The network contract inspects the compiled RPC signature: interaction intent carries no client-selected target, item, quantity, or outcome; neither the inventory nor harvest node declares a server mutation RPC. Catalogue and authority fixtures check malformed values and invalid calls, while the live peers check real owner-only replication. This is not a malformed-packet fuzz test or a test of simultaneous physical harvest input through the interaction trace. No runtime authority, replication, save schema, or normal gameplay behavior changes in this verification increment.

Every item in the schema-version-4 JSON catalogue has a required, bounded
description. The catalogue test verifies no JSON property or value contains
"Kit", checks the removed recipe fields and items are absent, and rejects
player-facing item or recipe names that still contain "Kit". The crafting menu
displays the selected recipe output description and the description of the item
chosen by its Previous / Next item controls, so all catalogue descriptions can
be browsed without putting them into the inventory HUD. Clean JSON aliases map
to stable internal IDs for recipe, construction, and save compatibility.

The schema-version-4 item catalogue includes Carrot, Potato, Rutabaga (also called swede, or lanttu in Finnish), Onion, and a matching `CarrotSeed`, `PotatoSeed`, `RutabagaSeed`, and `OnionSeed` entry. Each seed description names the crop it is intended to grow. These definitions add no planting, crop growth, harvesting, recipe, replication, or save behavior.

`CookedDeerMeat` is the stable output item for `CookedDeerMeatRecipe` when that recipe is configured. This catalogue item does not add food consumption or a `SteadyMeal` effect.

The catalogue defines Iron and a placeable Frying pan made from five Iron at a
visible Forge. Roasted root vegetables and Deer and rutabaga roast require the
placed pan; all pan dishes need usable lit heat at both station and player.
Looking at a Cooking rack, cauldron, or pan shows the remappable Interact prompt,
and pressing E opens a recipe view filtered to that station after server
validation. Root vegetable soup uses the visible cauldron and the same station
heat rule. `Kalmala.Gameplay.Food.CookingStationHeat` covers accepted exchanges,
missing-station and missing-heat rejection, normal time-based hearth fuel
burn, and exact ingredient costs. The pan uses the existing persistent
construction record with no added schema field. Iron currently has no
configured gathering source. These cooked outputs remain outside the existing
meal-consumption allowlist and do not add SteadyMeal effects.
