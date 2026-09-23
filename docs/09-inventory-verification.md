# Inventory verification

After the editor build in `07-development-setup.md`, run `Scripts/Verify-InventoryReconnect.ps1`. It keeps one revision-4 seed-418 listen server alive across two separate client processes using the same temporary client user directory. Both clients start with conflicting world identity and must receive the server identity.

Each visit requires the owning client to receive seven wood, reject local grant/consume calls, display its own read-only pack, and observe empty simulated-remote contents after owner replication. The server must report one successful inventory and harvest fixture for the host and each new client pawn (three total). Its inventory fixture checks an empty initial pack before granting ten wood and consuming three, so the second visit proves the current pawn-lifetime reset behavior. It does not claim inventory restoration: production reconnects currently lose carried contents. Persistent inventory remains necessary for the final M2 saved-camp gate.

The harvest fixture checks server-selected Wood/Stone/Fibre, full-stack rejection followed by successful retry, distance rejection, uninitialized descriptors, duplicate/depleted interaction rejection, and a single sparse-delta callback per successful grant. Its M7 source fixture additionally checks wrong-tool and full-stack rejection without changing owner tool condition or node state, then accepts the catalogue-selected Wood grant, spends one FieldHatchet condition point, and records one depletion callback. Fixtures use isolated temporary actors and do not write a world-save slot or tool condition. The runner terminates only its own peer processes and retains all three logs in the printed temporary directory.

`Kalmala.Gameplay.Food.CampfireProcessing` verifies the first food transaction through the live recipe and status components. Boar/deer meat roast only at an accessible nearby lit hearth with positive heat; missing, unlit, smouldering, malformed-batch, and client-role attempts leave ingredients unchanged. The owning player's food request carries only a food identity, consumes one catalogue item atomically, and applies the fixed 120-second `State.Food.SteadyMeal` modifier. Duplicate or forged consumption preserves both inventory and effect timer; expiry opens the single meal slot again. Food stacks and effects remain pawn-lifetime, with no save-schema change.

Also run these headless automations with the memory-cache and temporary user/log flags documented in `07-development-setup.md`:

- `Kalmala.Gameplay.Inventory.Catalogue`
- `Kalmala.Gameplay.Inventory.NetworkContract`
- `Kalmala.Gameplay.HarvestNode.AuthorityAndDepletion`
- `Kalmala.Gameplay.Interaction.ServerOnlyRangeValidation`

The network contract inspects the compiled RPC signature: interaction intent carries no client-selected target, item, quantity, or outcome; neither the inventory nor harvest node declares a server mutation RPC. Catalogue and authority fixtures check malformed values and invalid calls, while the live peers check real owner-only replication. This is not a malformed-packet fuzz test or a test of simultaneous physical harvest input through the interaction trace. No runtime authority, replication, save schema, or normal gameplay behavior changes in this verification increment.
