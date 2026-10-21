# I3 Garden Plot peer and rendered acceptance — 2026-10-21

## Reproduction

The isolated mirror was C:/Users/Ville/AppData/Local/Temp/kgp/Kalmala.uproject.
Its final incremental Win64 Development Editor build passed with:

    Build.bat KalmalaEditor Win64 Development -Project=C:/Users/Ville/AppData/Local/Temp/kgp/Kalmala.uproject -WaitMutex -NoHotReload -MaxParallelActions=4

The rendered two-peer check passed with:

    Scripts/Tests/Construction/Verify-GardenPlot.ps1 -Project C:/Users/Ville/AppData/Local/Temp/kgp/Kalmala.uproject -Rendered -Width 1280 -Height 720

The runner checked two server-owned placements, direct material cost, absence of a portable kit, host/client Carrot requests, seed and Planting Trowel payment, replicated crop state, and the seven Cultivation actions. Both views captured 1280×720 screenshots.

## Results

- Server owners 256 and 257 passed direct placement, Carrot Seed payment, Trowel wear, growing state and no-kit checks.
- Host owner 256 passed empty and growing Cultivation views; remote owner 257 passed the same views.
- The client confirmed world seed 418. The final server/client logs and incremental build output are retained beside the captures.
- The host and client captures show the Cultivate overlay with the growing status and all seven crop actions visible.

## Defects repaired

The compile pass first found a missing VerticalBox include in the UI fixture. The live run then exposed Garden Plot clearance overlapping its supporting terrain, the fixture's overly narrow actor lookup, a first-open context-section self-move that erased Cultivate, the host's existing request cooldown, and asynchronous client crop replication. The code and fixture were repaired, rebuilt, and the rendered two-peer check passed.

## Scope

This closes the outstanding I3 Garden Plot rendered and peer acceptance gate. Full A10 acceptance and project-wide verification remain deferred; A8 shelter rules review is still the next eligible backlog task.
