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

## Same-voyage late join and authenticated restart

`Scripts/Verify-OceanSkiffIntegratedReconnectJourney.ps1` covers the same
seed-418 sailing/discovery trip with an underway late join, a host restart,
returning-owner reconnect, discovery replay rejection, and a second late join
after restart. The final live run passed at 242,482 cm, crossing the recorded
terrain-patch boundary and observing both crosswind and calm. The original
owner held the helm and host passenger seat; each kept the accepted Stone x1
reward until shutdown. After restart, the exact moored vessel position and
both authenticated seats restored, replay reported `AlreadyFound` without a
second reward, and neither late peer attached or received private reward
state.

This runner uses stable development-only test-provider identities. It does
not persist inventory: the claim ledger persists and blocks a duplicate grant,
while inventory returns to its fresh-process baseline. The fixture briefly
moves the reconnecting test character into the server-validated discovery
range for the replay attempt, then restores its helm position. This is
complementary to the earlier dry-shore landing attempt; this result does not
close that blocked dry-shore acceptance or the rendered player-facing M8 run.
