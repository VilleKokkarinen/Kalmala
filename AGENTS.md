# Kalmala autonomous-agent rules

Read this file, `BACKLOG.md`, `PROGRESS.md`, and relevant documentation in `docs/` before every implementation task. The project brief, architecture, game design, roadmap, and decision log remain the source of truth.

Re-read relevant context if repository state changes during a run.

## Autonomous run protocol

Only one autonomous development run may modify the Kalmala repository at a time.

Before changing anything, determine whether another autonomous Kalmala run is actively working on the same workspace or repository state. If so, stop immediately without modifying files.

Ignore stale processes, completed runs, and unrelated development activity.

For each run:

1. Find the earliest incomplete roadmap milestone in `BACKLOG.md`.
2. Within that milestone, find the first incomplete top-level parent task.
3. Select the first unchecked, unblocked child task.
4. If the parent has no children, select the parent itself.
5. Implement exactly that one increment.
6. Perform only lightweight development checks unless this increment completes the milestone.
7. Update `BACKLOG.md` and `PROGRESS.md`.
8. Commit only the changes created by this run.
9. Stop. Do not start another backlog increment in the same run.

Do not skip an eligible task in favor of later work.

Make concrete progress. If the selected task is large, reduce the work to the smallest integrated increment that legitimately advances it while preserving the intended direction.

Stop only for:

- a true concurrent-run or handoff conflict;
- an external dependency or permission requiring specific user action;
- an error that genuinely prevents further progress;
- a milestone-final verification failure that cannot be safely resolved.

Do not voluntarily defer implementation merely because it is difficult or inconvenient.

## Development verification policy

During normal development increments, optimize for implementation speed.

Do **not** run expensive verification after every task, including:

- full Unreal builds;
- full project builds;
- full test suites;
- packaging;
- broad integration tests;
- expensive editor/headless verification.

Instead, perform only lightweight checks needed to catch obvious mistakes when practical, such as:

- code/static inspection;
- syntax or parser checks;
- formatting or lint checks;
- narrow validators;
- very fast compilation checks for directly affected code;
- other quick project-specific sanity checks.

Lightweight checks are not a substitute for milestone-final verification.

A normal child increment may be committed without a full build or full test suite.

If no useful lightweight automated check exists, inspect the changed code carefully and record that full verification is deferred.

## Milestone-final verification

Full verification is performed only when the selected increment completes the **final remaining implementation task in the current roadmap milestone**.

When that happens, the same run becomes the milestone-final verification run.

Before marking the milestone complete:

1. inspect the combined changes and handoffs for the milestone;
2. run all project-prescribed verification applicable to the milestone;
3. diagnose failures;
4. fix bugs, compile errors, integration mistakes, regressions, or other defects introduced during milestone development;
5. rerun the relevant failing checks after each repair;
6. rerun the required full verification after repairs;
7. continue until verification succeeds or a genuine blocker prevents safe completion.

Milestone-final verification includes, where applicable:

- full Unreal/project builds;
- required automation or test suites;
- integration tests;
- Unreal headless/editor validation;
- validators and static checks;
- other verification required by `docs/07-development-setup.md`.

Do not broaden repair work into unrelated features or later milestones.

For clearly pre-existing failures, document evidence showing that they were not introduced by the current milestone. Do not modify unrelated systems solely to make an unrelated pre-existing failure pass.

Do not mark the milestone complete if required milestone-final verification fails.

## Backlog completion semantics

A checked task `[x]` means its implementation work is complete.

Individual child tasks may be checked after implementation and lightweight development checks; they do not require a full project build or full test suite.

A top-level parent may be checked when its implementation criteria and all child tasks are complete.

The roadmap milestone itself must not be considered complete until milestone-final verification succeeds.

If the final implementation task in a milestone has been completed but milestone-final verification fails, record the milestone as awaiting verification or blocked rather than presenting it as successfully complete.

## Safety and repository rules

- Preserve all pre-existing working-tree changes.
- Never discard, overwrite, revert, stage, or commit changes that were not made during the current run.
- Keep implementation narrowly scoped to the selected backlog increment, except for fixes required by milestone-final verification.
- Do not force-push, rebase, reset, delete assets, alter saved-data schemas, install plugins/dependencies, access paid services, publish builds, or modify CI/release configuration without explicit user direction.
- Do not modify `Binaries/`, `Intermediate/`, `Saved/`, or `DerivedDataCache/`.
- Keep gameplay/network authority server-side.
- Validate all client-controlled item IDs, targets, quantities, damage values, and interaction requests on the server.
- Use original code, assets, names, lore, and level content only. Do not copy copyrighted or marketplace content.
- Stop and report rather than guess when a choice changes the product's platform, business model, visual identity, online-service commitment, or vertical-slice scope.

## Windows path-length safety

All paths created, copied, moved, renamed, generated, temporarily produced, or referenced by autonomous work must remain below the traditional Windows `MAX_PATH` limit of 260 characters.

Consider the full absolute path, including the workspace or worktree root, before creating or moving files.

Prefer short filenames, shallow directory structures, existing short project paths, and short temporary/output paths.

Do not rely on Windows long-path support being enabled.

If a required operation cannot be performed without exceeding the limit, stop before making the unsafe change and report the exact path involved.

## Unreal build access

When invoking Unreal Engine builds or UnrealBuildTool, ensure the process has normal read/write access to:

`%LOCALAPPDATA%\UnrealBuildTool`

Do not run Unreal build commands in an environment that prevents UnrealBuildTool from using that location.

## Backlog and progress updates

After every successful development increment:

- check the selected task only when its implementation criteria are actually satisfied;
- update parent state only when its implementation criteria and child tasks are complete;
- update `PROGRESS.md` accurately;
- commit the increment.

`PROGRESS.md` must record:

- work completed;
- files changed;
- lightweight checks performed, if any;
- whether full verification remains deferred;
- observable impact;
- networking/authority assessment where applicable;
- known limitations or remaining scope;
- the next eligible task.

When a run performs milestone-final verification, also record:

- that verification was milestone-final;
- full verification commands actually run;
- results;
- failures discovered;
- fixes made;
- rerun results;
- any pre-existing failures;
- final milestone status.

Do not claim builds, tests, verification, work, or impact that did not occur.

Update relevant documentation when implementation changes an established contract, workflow, architecture, or externally meaningful behavior.

## Commit rules

Normal development increments may be committed after implementation and reasonable lightweight inspection. Full project verification is intentionally deferred until milestone completion.

Before every commit:

1. inspect all worktree changes;
2. stage only files belonging to the current increment or milestone-final repair;
3. inspect the staged diff;
4. confirm every staged change belongs to the current run;
5. confirm no committed path violates the 260-character Windows path limit.

Use a concise commit message.

Never stage or commit:

- pre-existing user changes;
- unrelated worktree changes;
- unintended generated files;
- changes belonging to another run.

During milestone-final verification, commit fixes only after the repaired state has passed the relevant verification.

## Main-checkout handoff synchronization

When autonomous development runs inside a Git worktree, ensure handoff state is visible in the main checkout at:

`E:\dev\Kalmala\`

Synchronize only:

- `BACKLOG.md`
- `PROGRESS.md`

For each file:

1. inspect the main-checkout version before modifying it;
2. if it has no user changes, synchronize the worktree version;
3. if it contains non-overlapping user edits, preserve them and apply only this run's backlog/progress changes;
4. if there is a genuine content conflict, overwrite neither version and report the conflict.

Never synchronize implementation files, generated files, or other worktree changes.

Never discard, stage, commit, or overwrite pre-existing changes in the main checkout.

Handoff synchronization must not alter the implementation commit or cause main-checkout user changes to enter it.

The same Windows path-length rules apply during handoff synchronization.

## Definition of done

### Normal development increment

A normal increment is done for the current run when:

- exactly one selected backlog increment has been implemented;
- reasonable lightweight checks or careful inspection have been performed;
- `BACKLOG.md` reflects the implementation state accurately;
- `PROGRESS.md` records that full verification is deferred;
- only current-run changes are committed;
- required handoff synchronization is complete.

The run then stops.

### Final task in a milestone

When the selected increment is the final remaining implementation task in the milestone, the run is not done until:

- the final increment is implemented;
- milestone-final verification has been run;
- milestone-related bugs and verification failures have been repaired where safely possible;
- required verification succeeds;
- backlog and documentation state are finalized;
- `PROGRESS.md` contains the complete verification handoff;
- all run-owned changes are committed;
- required handoff synchronization is complete.

If milestone-final verification cannot be made to pass safely, do not claim the milestone is complete. Report the exact blocker, failing verification, repairs attempted, and required next action.
