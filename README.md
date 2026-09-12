# Kalmala

Kalmala is an original cooperative survival-action RPG for Unreal Engine 5. It combines a systemic, destructible elemental world with Nordic-Finnic mythic survival, built around an art direction inspired by the visual language of the *Kalevala*.

The project is designed to be built primarily by AI agents. The documentation in [`docs/`](docs/README.md) is the source of truth for both human decisions and agent work.

## Project principles

- **Original world first.** Reference games inform high-level design lessons only; no copied assets, names, lore, levels, code, or distinctive content.
- **Playable early.** Every milestone produces a testable game loop.
- **Server-authoritative multiplayer.** The game must remain valid when played on a dedicated server.
- **Shelter earns progression.** Materials, weather, construction, and open-world exploration create a clear path from a temporary camp to durable structures and better gear.

## Start here

1. Read [`docs/00-project-brief.md`](docs/00-project-brief.md).
2. Follow the delivery sequence in [`docs/04-roadmap.md`](docs/04-roadmap.md).
3. Give an implementation agent the relevant prompt from [`docs/06-agent-prompts.md`](docs/06-agent-prompts.md).

## Development setup

The project targets the installed Unreal Engine 5.8.2 build at `C:\Program Files\Epic Games\UE_5.8`. Follow [`docs/07-development-setup.md`](docs/07-development-setup.md) to generate project files and build the editor and dedicated-server targets.

## Fast world-map PNGs

From the repository root, run `./Scripts/Export-WorldMaps.ps1 -Watch`. Edit and save [`Scripts/WorldMapPreview.params`](Scripts/WorldMapPreview.params) to refresh the master atlas, rotated land/water crop and biome map in `.cache/WorldMaps/`. Distances are in kilometres. The headless exporter stays warm, so parameter changes need neither a game launch nor a rebuild. Omit `-Watch` for one export; add `-Build` after changing C++ source. See [the export guide](docs/07-development-setup.md#fast-world-map-png-export) for controls and limits. Preview overrides never change gameplay or saves.
