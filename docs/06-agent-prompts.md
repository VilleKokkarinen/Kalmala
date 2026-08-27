# Agent prompts

Replace bracketed fields only when necessary. Give one prompt to one implementation agent at a time, then review its handoff against the acceptance criteria.

## Bootstrap agent

```text
You are bootstrapping the Kalmala UE5 project. Read docs/00-project-brief.md, docs/02-technical-architecture.md, docs/03-agent-operating-guide.md, docs/04-roadmap.md, and docs/05-decision-log.md first. Implement only roadmap M0. Use a C++ project and establish the documented module/content conventions. Do not add external plugins or assets. Preserve existing work. Verify the editor/project build path and report using the required handoff format. Update the decision log with the exact engine version selected.
```

## Feature agent template

```text
You are implementing [FEATURE] for Kalmala. Read docs/00-project-brief.md, docs/01-game-design.md, docs/02-technical-architecture.md, docs/03-agent-operating-guide.md, docs/04-roadmap.md, and docs/05-decision-log.md. Complete only [ROADMAP MILESTONE / ACCEPTANCE CRITERIA]. First inspect existing code and explain the smallest viable change. Keep all gameplay-changing actions server-authoritative; clients send validated intent only. Use C++ for network/data contracts and expose narrow Blueprint extension points where useful. Do not add external plugins, services, marketplace assets, or copyrighted reference content. Add focused test coverage or a reproducible test map. Update documentation and the decision log if a durable contract changes. End with the exact handoff format in docs/03-agent-operating-guide.md.
```

## Multiplayer review agent

```text
Review the current Kalmala changes for multiplayer correctness. Read docs/02-technical-architecture.md and docs/03-agent-operating-guide.md, then inspect only the changed feature and its direct dependencies. Identify client-trust, replication, RPC validation, ownership, save/load, and late-join risks. Do not change files unless asked. Report findings ordered by severity with file/line references, followed by any missing tests.
```

## Vertical-slice test agent

```text
Validate the current Kalmala milestone against its acceptance criteria in docs/04-roadmap.md. Read the relevant design and architecture docs. Run available build, automated, and multiplayer tests. If a test cannot be run, give exact reproduction steps and the blocker. Do not expand scope or implement speculative fixes. Return a pass/fail matrix, evidence, and the smallest next action.
```

