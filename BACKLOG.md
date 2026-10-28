# Development backlog

## Active queue

No active goals are currently listed. Completed goal records are retained under Completed post-roadmap work.

## Completed roadmap

| Milestone | Completed scope / acceptance |
| --- | --- |
| M0 — Bootstrap | Project setup, prototype map, packaged build and dedicated-server availability recorded. |
| M1 — Networked traversal and interaction | Replicated movement and server-validated interaction. |
| World generation, Phases 1–9 | Deterministic generation, survival, biome expansion, ocean travel and expanded maps. |
| M2 — Survival camp loop | Gathering, crafting, construction and camp persistence. |
| M3 — Elemental world prototype | Server-owned weather, wetness, exposure and recovery. |
| M4 — Combat and support magic | Combat, creature encounters, discoveries and support effects. |
| M5 — Vertical-slice finish | Complete under recorded acceptance scope. |
| M6 — Production hardening | Complete under recorded supported-session scope and limits. |
| M7 — Survival progression and gameplay UI | Complete. |
| M8 — Ocean and long-distance travel | Complete; dry-shore acceptance waived. |
| M9 — Expanded biome content | Complete under recorded scope. |
| M10 — Release completion and launch validation | Complete under recorded scope and limits. |
| M11 — User experience and visual UI | Final acceptance passed 2026-10-06. |
| M12 — Inventory menu and cleaner HUD | Final acceptance passed 2026-10-09. |
| M13 — Revalidated UI on main | Integrated-main acceptance passed 2026-10-11: 122/122 tests, rendered peers, input/privacy/reconnect and Development/Shipping smoke. [Evidence](docs/evidence/m13/README.md). |
| M14 — Expanded world map | Integrated-main acceptance passed 2026-10-12: editor build, 123/123 tests, live map runners, rendered/performance matrix. [Evidence](docs/evidence/m14/main-integration.md). |

## Completed post-roadmap work

### Meadows relief retune — 2026-10-27

- **Selected task:** Apply a Valheim-informed Meadows height adjustment after
  the user reported that the merged biome still looked flat.
- **Work completed:** Increased compiled defaults from 4 / 0.6 / 0.1 m to
  8 / 1.2 / 0.2 m for broad hills, rolling relief and floor detail; changed
  floor scale from 8 m to 12 m. Updated the focused default assertion and the
  current terrain contract. The prior settings remain in the historical
  BH25 profile record.
- **Checks:** Manually checked the defaults against the existing preview bounds
  and one-factor sweep ranges; `git diff --check` passed. No build, test suite,
  profile sweep or rendered review was run.
- **Status:** Source implementation is complete. Fresh combined-profile and
  rendered terrain acceptance remain pending.

### Meadows 30 m height follow-up — 2026-10-28

- **Selected task:** Raise Meadow relief headroom to reflect the user's
  observation of Valheim Meadows reaching roughly 30 m.
- **Work completed:** Set the broad Meadow amplitude to 20 m, move shoulder
  compression to a 30 m threshold, and expand accepted preview bounds to cover
  both values. Updated the preview parameter file and current tuning docs.
- **Checks:** Source and preview bounds were inspected; `git diff --check`
  passed. No build, automation, combined-profile sweep or rendered review was
  run.
- **Status:** Defaults and preview contracts are updated. Actual absolute
  height range and rendered terrain readability remain unverified.

### Valheim-informed forest and mountain follow-up — 2026-10-28

- **Selected task:** Compare the other Kalmala biome height functions with
  Valheim's corresponding terrain formulas and adjust the close analogs.
- **Work completed:** Raised shared Elderwood/Tundra broad relief from 4 m to
  20 m at the existing 1 km scale, matching Valheim Black Forest's nominal
  ±20 m detail contribution. Raised Thunder Mountain highland gain from 0.4
  to 1.0, matching the full one-for-one amplification of source excess above
  its threshold. Expanded preview bounds and the existing profile matrix to
  cover the adjusted settings. Kept the intentionally compressed Mire,
  mixed Lakes and master-constrained Ocean controls at their measured values.
- **Checks:** Compared the formulas and control units, inspected all current
  preview bounds and regression expectations, and ran `git diff --check`. No
  build, automation, profile sweep or rendered review was run.
- **Status:** Source settings and research notes are updated. Fresh profiles
  and rendered terrain acceptance remain pending for Elderwood/Tundra,
  Thunder Mountains, and Lakes after the Meadow change.

## Scope and history

The [roadmap](docs/04-roadmap.md) and [feature contracts](docs/game-features.md)
remain authoritative for scope; the [verification index](docs/07-development-setup.md#consolidated-verification-index)
records project checks. Detailed implementation history and retired checklists
remain in [PROGRESS.md](PROGRESS.md) and Git history. No acceptance status or
product scope changes as part of this cleanup.
