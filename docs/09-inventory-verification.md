# Inventory verification

After the editor build in `07-development-setup.md`, run `Scripts/Verify-InventoryReconnect.ps1`. It keeps one revision-4 seed-418 listen server alive across two separate client processes using the same temporary client user directory. Both clients start with conflicting world identity and must receive the server identity.

Each visit requires the owning client to receive seven wood, reject local grant/consume calls, display its own read-only pack, and observe empty simulated-remote contents after owner replication. The server must report one successful inventory and harvest fixture for the host and each new client pawn (three total). Its inventory fixture checks an empty initial pack before granting ten wood and consuming three, so the second visit proves the current pawn-lifetime reset behavior. It does not claim inventory restoration: production reconnects currently lose carried contents. Persistent inventory remains necessary for the final M2 saved-camp gate.

The harvest fixture checks server-selected Wood/Stone/Fibre, full-stack rejection followed by successful retry, distance rejection, uninitialized descriptors, duplicate/depleted interaction rejection, and a single sparse-delta callback per successful grant. Fixtures use isolated temporary actors and do not write a world-save slot. The runner terminates only its own peer processes and retains all three logs in the printed temporary directory.

Also run these headless automations with the memory-cache and temporary user/log flags documented in `07-development-setup.md`:

- `Kalmala.Gameplay.Inventory.Catalogue`
- `Kalmala.Gameplay.Inventory.NetworkContract`
- `Kalmala.Gameplay.HarvestNode.AuthorityAndDepletion`
- `Kalmala.Gameplay.Interaction.ServerOnlyRangeValidation`

The network contract inspects the compiled RPC signature: interaction intent carries no client-selected target, item, quantity, or outcome; neither the inventory nor harvest node declares a server mutation RPC. Catalogue and authority fixtures check malformed values and invalid calls, while the live peers check real owner-only replication. This is not a malformed-packet fuzz test or a test of simultaneous physical harvest input through the interaction trace. No runtime authority, replication, save schema, or normal gameplay behavior changes in this verification increment.
