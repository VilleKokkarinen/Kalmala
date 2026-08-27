# Agent operating guide

## Before touching files

1. Read the project brief, relevant architecture/design documents, and decision log.
2. Inspect the existing project and preserve unrelated work.
3. State the smallest viable interpretation of the assigned outcome.
4. Identify affected multiplayer authority and persistence implications.

## Implementation rules

- Deliver small, integrated increments—not a speculative framework.
- Prefer C++ for authority, networking, data contracts, and reusable systems; expose intentional Blueprint extension points.
- Never trust client-provided item IDs, target actors, quantities, or damage values without server validation.
- Do not add marketplace assets, plugins, telemetry, external services, paid APIs, or copyrighted content without approval.
- Do not rename public assets or alter saved-data schemas without a migration plan.
- Keep a task-specific test map or automated test when it lowers regression risk.

## Definition of done

A task is complete only when it:

- builds or has a documented, concrete reason it cannot;
- has been exercised in the requested networking mode;
- has no known authority exploit introduced by the change;
- updates data/content documentation when its contract changes;
- reports files changed, verification run, limitations, and next recommended task.

## Handoff format

Use this exact concise structure at the end of each agent task:

```text
Outcome:
Changed:
Verification:
Multiplayer impact:
Known limits:
Next task:
```

## Stop and request direction when

- a choice changes the product’s platform, business model, visual identity, or online-service commitment;
- an external dependency, asset license, account, or paid service is required;
- a feature would expand the vertical-slice scope materially.

