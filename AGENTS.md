# Kalmala autonomous-agent rules

Read this file, `BACKLOG.md`, `PROGRESS.md`, and the relevant documentation in `docs/` before every implementation task. The project brief, architecture, game design, roadmap, and decision log remain the source of truth.

## Autonomous run protocol

1. Choose the first unchecked, unblocked task in `BACKLOG.md` within the earliest incomplete roadmap milestone.
2. Implement one small, integrated increment only. Do not expand scope or start a later milestone.
3. Run the relevant build, test, or Unreal headless/editor verification described in `docs/07-development-setup.md`.
4. Repair failures introduced by the run, within the run's time budget. If the same blocker persists after three attempts, mark the task `BLOCKED` with the exact evidence, update `PROGRESS.md`, and stop the run.
5. Update `PROGRESS.md` using the specified handoff format.

## Safety and repository rules

- Preserve all pre-existing working-tree changes. Never discard, overwrite, stage, or commit changes that were not made during the current run.
- Stage and commit only files changed during the current run, using a concise commit message. Do not commit if required verification fails.
- Do not force-push, rebase, reset, delete assets, alter saved-data schemas, install plugins/dependencies, access paid services, publish builds, or modify CI/release configuration without explicit user direction.
- Do not modify `Binaries/`, `Intermediate/`, `Saved/`, or `DerivedDataCache/`.
- Keep gameplay/network authority server-side. Validate all client-controlled item IDs, targets, quantities, damage values, and interaction requests on the server.
- Use original code, assets, names, lore, and level content only. Do not copy copyrighted material or marketplace content.
- Stop and report—not guess—when a choice changes the product's platform, business model, visual identity, online-service commitment, or vertical-slice scope.

## Definition of done

Every completed task must have a passing build or documented concrete blocker, an appropriate networking/authority assessment, updated documentation for changed contracts, and a `PROGRESS.md` handoff containing files changed, verification, limitations, and the next task.
