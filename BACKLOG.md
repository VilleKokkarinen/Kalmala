# Autonomous development backlog

Documentation layout: former docs 08–47 are consolidated into the core docs (00–07). See [Game features](docs/game-features.md), [architecture contracts](docs/02-technical-architecture.md#consolidated-system-contracts), and [verification index](docs/07-development-setup.md#consolidated-verification-index). This user-directed cleanup does not close any milestone.

This is the short execution queue and completion index. The roadmap and feature contracts in docs/ remain authoritative for scope and acceptance criteria.

## Roadmap status

M0–M14 and world-generation Phases 1–9 are complete under their recorded acceptance scope and limitations. M13 integrated-main acceptance passed on 2026-10-11; M14 integrated-main acceptance passed on 2026-10-12. See [M14 integration evidence](docs/evidence/m14/main-integration.md).

## Completed roadmap log

- **M0 — Bootstrap:** project setup, prototype map, packaged build, and dedicated-server availability recorded.
- **M1 — Networked traversal and interaction:** replicated movement and server-validated interaction.
- **World generation, Phases 1–9:** deterministic world generation, survival, biome expansion, ocean travel, and expanded maps.
- **M2 — Survival camp loop:** gathering, crafting, construction, and camp persistence.
- **M3 — Elemental world prototype:** server-owned weather, wetness, exposure, and recovery.
- **M4 — Combat and support magic:** combat, creature encounters, discoveries, and support effects.
- **M5 — Vertical-slice finish:** completed under the recorded acceptance scope.
- **M6 — Production hardening and supported-session validation:** completed under the recorded release scope and limits.
- **M7 — Survival progression and readable gameplay UI:** completed.
- **M8 — Ocean and long-distance travel:** completed; dry-shore acceptance was waived.
- **M9 — Expanded biome content and encounter depth:** completed under the recorded scope.
- **M10 — Release completion and launch validation:** completed under the recorded scope and limits.
- **M11 — User experience and visual UI upgrades:** final acceptance passed 2026-10-06.
- **M12 — Inventory menu and cleaner gameplay HUD:** final acceptance passed 2026-10-09.
- **M13 — Revalidated UI on main:** final acceptance passed 2026-10-11; evidence in docs/evidence/m13/.
- **M14 — Expanded world map:** isolated acceptance passed 2026-10-10 on source 1a411c6; integrated-main build, acceptance matrix, and final 123/123 automation queue passed 2026-10-12. Evidence is in docs/evidence/m14/.

## Post-roadmap user-directed work

Current first-incomplete item: run the generated-forest simultaneous owned-pickup contention gate on current sources, then test client-visible full-pack rejection.

Open leaf checkboxes are sized as one focused automated-job increment. Mark a leaf complete only after its stated implementation or acceptance passes; leave it open if it needs a separate repair or another run.

- [ ] **2026-10-13 — Attack-driven tree harvesting:** use axe attacks to move trees from standing, through fallen trunk, to collectible wood.
  - [x] Integrate server-authoritative tree/trunk/drop harvesting for existing wood-resource actors, preserving tiers, yields and sparse depletion.
  - [ ] **Complete generated-forest harvesting and lifecycle.**
    - [x] Give generated trees stable seed/patch/candidate IDs; materialize undepleted candidates as patch-owned harvest actors and suppress their decorative counterparts.
    - [x] Persist successful pickup IDs and rebuild or evict patch actors from sparse state; eviction must not record depletion.
    - [x] Compile and run focused forest stage/lifecycle regressions and actual Meadow/Elderwood save-slot reload coverage. Four focused automations and the slot reload checks passed; command/log details remain in docs/07-development-setup.md.
    - [ ] **Complete current-source host/client delivery acceptance.**
      - [x] Verify host/client Trunk and Drop delivery for an exact generated-tree ID, with matching source suppression and replacement coverage.
      - [x] Verify trunk late join: a third peer receives the exact settled Trunk ID without cosmetic duplication.
      - [x] Verify drop late join: a third peer receives the exact Drop ID without cosmetic duplication.
      - [x] Verify normal exact-ID pickup grants once, writes depletion on the host, and leaves repeat, depleted, and second-player retries without extra rewards.
      - [x] Verify forged, trace-range, and physical-distance rejections preserve the live drop and grant nothing.
      - [x] Verify a remote-owned full-pack pickup request rejects without reward, preserves the drop, and restores the exact inventory snapshot.
      - [x] Verify ordinary-camera client targeting selects the exact live drop and completes one accepted pickup.
      - [x] Verify simultaneous pickup RPCs from competing owners grant at most once and leave the losing request with no reward.
      - [x] Verify a full pack through the ordinary client interaction path: the request is rejected, the drop remains live, and the owner receives the established rejection feedback.
    - [x] **Verify shared-patch departure and eviction.** With two players sharing a patch, confirm one departure retains the shared patch; the final departure evicts its actors without recording harvest.
    - [x] **Verify patch reactivation after eviction.** Collected IDs stay absent while uncollected generated trees materialize again.
    - [x] **Verify process-restart persistence.** Restart and load the real population slot; confirm the same collected IDs remain absent and uncollected trees return.
    - [x] **Reconcile generated-forest documentation.** Update feature, architecture and setup docs to match accepted behavior and remaining limits; remove superseded claims that generated forest trees are still decorative.
  - [ ] **Implement the server-authored fall transition.** Add replicated transition state/timing and late-join reconstruction; preserve the exact tree identity and server-owned hit, condition, yield and depletion decisions.
    - [x] Add and validate the authoritative fall/settled phase transition, including a focused regression for transition overlap and accepted-hit accounting.
    - [x] Add the original moving presentation and settled 3D trunk at the generated tree's position; retain the same persistent tree identity.
  - [ ] **Integrate settled-log targeting and pickup.**
    - [ ] Align collision, axe targeting and server range checks with the presented settled log; reject extra hits during transition.
    - [ ] Keep the wood bundle readable and collectible at the settled log; verify accepted pickup and full-pack/duplicate rejection without extra rewards.
  - [ ] **Run final tree-feature acceptance.**
    - [ ] Build the integrated editor source and run the focused tree-harvest automation group.
    - [ ] Run final exact-ID host/client stage, late-join, and pickup delivery acceptance on the integrated source.
    - [ ] Run final peer streaming, patch reactivation, and real population-slot restart acceptance.
    - [ ] Check the parent only after each final gate passes; leave any failed gate open until its repair and rerun succeed.

## M13 — Rebuild and revalidate M11/M12 UI on main

**Complete on main:** merge `e16b2f7` passed final acceptance on 2026-10-11. [Scope and criteria](docs/04-roadmap.md#m13--rebuild-and-revalidate-the-m11m12-ui-on-main) remain authoritative; unselected Settings candidates are not implemented features.

- [x] Confirm custody, lookup mappings and cooking for all 48 catalogue and nine status icons.
- [x] Revalidate the shared theme, accessibility, focus/input, local browsing, Favorites/ranks/Recent, notifications, recipe activity and Forge comparison.
- [x] Restore the 10×4 Inventory/crafting companion, contextual details, numbered hotbar and server-owned support-scroll behavior.
- [x] Reconcile HUD/service contexts, hammer Build browsing/placement and direct server-validated Campfire lighting.
- [x] Apply the minimap biome label, transparent Escape home and grouped local Settings presentation.
- [x] Verify complete icon rendering in editor peers and the Development package.
- [x] Pass the final editor build, 122/122 automation tests, rendered host/client matrix, input/privacy/reconnect checks and Development/Shipping smoke.

[Acceptance evidence](docs/evidence/m13/README.md) records exact commands, tested source, captures and scope limits. No M13 increment remains. The later Esc-border change has its own post-roadmap verification record above.

## M14 — Expanded world-map rework

**Complete:** accepted isolated source `1a411c6d2b76b3d9b4bd438b8449632fd5bac1f4`, integrated on main; final integrated-main build, full automation queue, and M14 acceptance passed 2026-10-12. [Scope and criteria](docs/04-roadmap.md#m14--expanded-world-map-rework) remain authoritative.

- [x] Render distinct original biome textures across supported zoom levels.
- [x] Preserve owner exploration fog without revealing hidden terrain through view changes or markers.
- [x] Provide owner-local marker selection and optional labels, including icon-only pins.
- [x] Relay server-validated right-click world pings globally for ten seconds, without saving them.
- [x] Remove map co-op controls/presence; keep map sharing and shared pins out of scope.
- [x] Cache bounded terrain/fog tiles across pan/zoom and reject stale async work.
- [x] Pass final editor build, 123/123 automation tests, four live map runners, viewport/scale/contrast/zoom/theme matrix and rendered/performance checks.
- [x] Complete the integrated-main milestone-final automation gate: UE 5.8 editor target rebuilt successfully after the test repairs; the full `Automation RunTests Kalmala` queue passed 123/123 with exit code 0. See [integrated-main verification and repairs](docs/evidence/m14/main-integration.md).

The accepted evidence includes 22 native PNGs, biome raster, exact commands, hashes and performance measurements. Recover the report and manifest from commit `9f11e09` with `git show 9f11e09:docs/evidence/m14/README.md` and `git show 9f11e09:docs/evidence/m14/acceptance.json`. Detailed verification and superseded retry history remain in [PROGRESS.md](PROGRESS.md) and Git history.

The M14 integrated-main verification is complete; no M14 increment remains. Map sharing remains deferred.
