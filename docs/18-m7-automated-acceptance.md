# M7 automated progression and recovery acceptance

This M7 acceptance path needs no native computer-use session or targetable
Windows game window. Run `Scripts/Verify-M7AutomatedAcceptance.ps1` after a
successful `KalmalaEditor Win64 Development` build using the setup in
`07-development-setup.md`.

The script runs focused Unreal automation tests for biome content identity,
owner-only skill progression, tool lifecycle, crafting transactions, prepared
food, steady-meal effects, weather recovery, discovery privacy, survival-status
presentation, and prepared-food details. It then runs isolated host/client
scenarios for gathering and repair, rendered camp crafting, replicated weather
and camp recovery, a server-owned boar encounter, and optional discovery.
Each scenario starts with separate temporary user profiles and deterministic
server/client seeds; the joining peers begin with conflicting seeds so the
server world identity and relevant replication are checked without a manually
controlled client.

The rendered crafting scenario captures both peers at 1280×720 and verifies
that the capture and owner-local crafting presentation checks pass. The
automation tests assert text-plus-marker and status content directly. These
are functional and presentation-contract checks, not a claim that one player
completed the entire progression in a single unscripted journey. M7 does not
require Computer Use or physical keyboard/controller interaction. This suite is
the no-native-window host/client gate for the M7 post-content regression when
combined with the checked M6 regression slices and clean-profile package smoke.
It verifies process-level authority, replication, reconnect, and presentation
contracts; it does not pass physical input or the player-visible packaged
walkthrough. Keep those as explicit M6 limitations. The M5 player-facing
charter in `12-vertical-slice-runbook.md` remains unchanged.

The suite changes no gameplay, RPC, replication, authority, or save contract.
The existing server continues to own gathering, tool condition and repair,
inventory exchanges, food effects, weather, recovery, creature outcomes,
discovery claims, and rewards.
