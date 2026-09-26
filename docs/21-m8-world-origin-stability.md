# M8 finite-world origin stability

The M8 ocean prototype keeps the existing finite 16 km radial world and does not enable world-origin rebasing. The representative two-peer skiff crossing must keep the engine world origin at (0,0,0) while the vessel moves between generated terrain patches.

## Verification

After the forced KalmalaEditor Win64 Development build, run:

    Scripts\Verify-OceanSkiffOriginStability.ps1 -Port 18169

The development-only world subsystem is created only when KalmalaOceanJourneyPeerTest is present. In each peer world, it waits for the locally controlled seated player, records the actual world origin and skiff location, then checks the origin every tick until the replicated skiff reaches its moored stop at least 239,000 cm from the start. The listen host must report the passenger seat and the remote client must report the helm seat. Both must record zero start/end origins and no intermediate origin change.

The runner also requires the existing server-authoritative seed-418 crossing, crosswind/calm weather results, patch-coordinate transition, the 25-patch cap, and replicated travel/stop observations. It uses separate temporary user directories and retains host/client logs. This accepts stable coordinates within the finite-world design; it does not enable or verify a rebased world.

## Authority and scope

The subsystem only observes UWorld::OriginLocation, local seat attachment, and replicated skiff movement. It adds no RPC, replicated field, generated world state, persistence data, or production behavior. Server authority for movement, weather, occupancy, bounds, and terrain streaming remains unchanged.

Remaining M8 work includes combined discovery/disembark/reconnect journey acceptance, physical helm input, rendered travel, and underway performance measurements. The existing actor, memory, connection, and moored frame-time samples remain snapshots rather than approved ceilings.
