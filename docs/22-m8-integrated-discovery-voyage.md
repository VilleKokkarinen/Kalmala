# M8 integrated discovery voyage

Run `Scripts/Verify-OceanSkiffIntegratedJourney.ps1 -Port 18172` after a
forced `KalmalaEditor Win64 Development` build. The isolated seed-418 listen
host and conflicting-seed client require the server to choose an optional sea
discovery whose generated deep-water corridor continues for 2.4 km and reaches
a safe stopped-exit position. Both peers occupy the two server-selected seats,
claim the same canonical discovery for their authenticated player identities,
and retain the exact catalogue reward.

The development fixture submits bounded helm intent through the existing
server steering acceptance method, keeps the vessel on its generated route,
observes the GameMode-selected crosswind and calm intervals while underway,
and waits for the server-simulated stop before disembarking both occupants.
The host checks the route distance, terrain-patch transition, finite-world
bounds, active-patch cap, empty seats, saved-seat cleanup, discovery state,
and rewards. The remote owner checks its replicated weather observations,
reward and discovery feedback, disembark result, empty seats, and moored mode.

This is an integrated server-fixture increment, not the full M8 acceptance
run. It creates a development skiff directly rather than exercising the
generated-surface launch trace; the separate skiff journey check covers that
path and the remote steering RPC. Discovery is claimed at the voyage start,
and the safe exit is a qualifying open-water position rather than a dry-shore
landing. Late join, restart/reconnect in this same journey, physical helm
input, origin rebasing, rendered travel, and underway performance ceilings
remain outside this check. Existing server authority, authenticated discovery
ledger, and versioned travel saves remain in force; this fixture adds no
production RPC, gameplay save field, or persistent client input.

The peer runner retains both logs in its temporary output directory. It
requires the server's integrated-journey result, the helm owner's complete
replica result, server/client seed agreement, and the existing weather peer
checks for cycle 7001 crosswind and cycle 7002 calm.
