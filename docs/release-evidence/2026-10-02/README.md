# M10 release evidence archive

Archived 2026-10-02 after completing the final M10 backlog goal. The evidence bundle is [M10-evidence.zip](M10-evidence.zip), 12,737,874 bytes, SHA-256 5440F663F6CC51C00BC5E66A9231FDFE8F369875143C27E53E5DD656E7BE6F66.

## What is included

- Unreal Engine 5.8.2 Win64 Development BuildCookRun log and package manifests.
- Full null-renderer package startup/map-load smoke log.
- SHA-256 inventory for the 48 built package files outside runtime Saved data.
- The 98-test automation log, M10 camp-tradeoff checks, M9 schema-2 migration/reconnect logs, and integrated ocean journey logs.
- Six host/client settings accessibility captures and six host/client world-map captures at 1024x768, 1280x720, and 2560x1080.
- Partial packaged host/client logs, explicitly labeled as control investigation rather than complete player acceptance.
- Point-in-time copies of the project and release contracts, migration policy, accessibility requirements, performance reports, and rebuild/runbook instructions.
- A per-file byte-count and SHA-256 manifest.

The zip contains 56 files. Its 55 evidence-manifest records were checked against the extracted zip contents, and all 48 package binary hashes matched the local package output. The longest tested extraction path is 104 characters. Save files, generated intermediate files, and package binaries are excluded.

## Build record

The archived BuildCookRun completed successfully in 296.16 seconds, compiled 358 actions, cooked 514 of 521 packages, and produced 48 non-Saved files totaling 970,637,056 bytes. The build-time longest package path was 140 characters. The package smoke log contains both required startup/map-load messages and no Error: lines; the recorded process remained alive for more than 20 seconds.

The smoke log also contains 21 warnings, including missing packaged audio cues and an invalid material index on the default skiff hull. These remain visible in the raw log and are release limitations, not claimed as resolved by this archive.

The binary package remains at C:\Users\Ville\AppData\Local\Temp\kca\Archive\Windows on this machine and is not committed. Later launches added runtime logs, settings, and save files under Saved, so that temporary directory is not a clean distribution archive. The checksum inventory excludes Saved data.

## Test and acceptance record

The full automation log records 98 successful tests, zero failures, and exit code 0. The wider M10 clean-profile rendered, authority, persistence, reconnect, travel, combat, construction, crafting, progression, weather, HUD, accessibility, and performance results are summarized in [PROGRESS.md](../../../PROGRESS.md) and the archived runbook reports.

The product owner reported completing the release-candidate fresh-player co-op, progression, and long-distance walkthrough manually. The agent did not observe that session; no per-step screenshots, timings, or logs were supplied. The included packaged peer logs document an earlier partial host/client control attempt only.

## Migration and accessibility

The archive includes the accepted M9 migration note and accessibility contract. Current save behavior remains schema 2 for construction and player discovery/tool state, with explicit schema-1 migration, exact world/player identity checks, and bounded records. The shared catalogue remains schema 4. No save, authority, or network contract changed during this archive task.

Rendered accessibility evidence includes host/client settings pages for audio, controls, and general settings. The documented six-capture run also checked modal backdrop pixels and text scaling. World-map captures cover the three listed aspect ratios.

## Rebuild and known limits

Follow the archived reference/docs/07-development-setup.md Windows Development package procedure: create a disposable short-path project mirror, run UE 5.8.2 BuildCookRun with -pak and -iostore, then launch from a new empty UserDir with a unique absolute log and verify engine initialization and prototype-map load for at least 20 seconds.

The build used a disposable project mirror under the local Temp directory. Its log identifies the project path but not an immutable source revision. Git HEAD at archive-task start was 76f8d39864270f480033d162ef6d7db84b76b0e3, with pre-existing dirty source and documentation changes preserved in the main checkout. No source patch or binary package is included, so a clean independent reproduction requires first preserving an immutable source snapshot.

Other recorded limitations: dedicated-server validation is unavailable with the installed Launcher engine; no game-managed save backup/restore path was exercised; the performance closeout accepts one-machine and simulated profiles without physical low/mid-tier coverage or approved numeric ceilings; extended packaged travel and long-session peak/growth measurements remain open. The manual walkthrough remains owner-reported.
