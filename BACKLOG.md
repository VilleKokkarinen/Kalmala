# Autonomous development backlog

This file is the execution queue for Codex automations. Keep items small, ordered, and independently verifiable. Only start work in the earliest milestone whose acceptance criteria are not yet met.

## M0 — Bootstrap

- [ ] Verify that `KalmalaEditor Win64 Development` builds with the command in `docs/07-development-setup.md`; record the exact result in `PROGRESS.md`.
- [ ] Open the project in Unreal Editor and create the prototype map at `/Game/Kalmala/Maps/Prototype/L_Prototype`; document the result.
- [ ] Configure and verify a packaged development build launches after the prototype map exists.
- [ ] Confirm whether a server-capable Unreal 5.8 build is available; otherwise document the dedicated-server build blocker.

## M1 — Networked traversal and interaction

Do not begin until M0 acceptance criteria in `docs/04-roadmap.md` are met.

- [ ] Implement the smallest server-authoritative replicated character and camera setup needed for the two-player prototype map.
- [ ] Add a server-validated interaction trace and an interactable interface.
- [ ] Add a two-player test map flow and verify invalid client interactions are rejected.

## Later milestones

Use `docs/04-roadmap.md` as the source of truth. Add decomposed M2–M5 tasks here only after their preceding milestone acceptance criteria pass.
