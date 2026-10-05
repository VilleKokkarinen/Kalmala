# M11 status parent and catalogue icons

The user requested the next three backlog items together on 2026-10-02,
with verification deferred until all three implementations were present.

The owner-local survival subsystem reads only its local controller's pawn.
It owns a transparent, non-focusable, hit-test-invisible hotbar at logical
top-right (-24,244), twelve units below the existing 208-unit minimap.
Entries wrap within 450 units, with six-unit gaps and wider cells for
100/125/150% local text. Empty states collapse; no background, border or
empty slots are drawn. Original monochrome line art and outlined text remain
readable over terrain without colour-dependent identification.

Stable order: Wet, Steady meal, Hot, Cold, the current support effect, then
weather. Status seconds come directly from owner replication; support and
weather use their replicated expiry/interval with synchronized server time.
Heat and cold say ongoing. Expired statuses disappear, duplicate status IDs
appear once, and an elapsed weather interval says awaiting update until the
server sends its replacement. Weather includes calm/active/high activity or
Storm; the interval timer describes weather, not an invented player debuff.

Escape > Status and weather details provides live wrapped, scrollable text
for category, intensity, magnitudes, source and recovery. This uses existing
modal focus and input restoration. Supported statuses do not stack. The
left HUD retains combat, discovery, support selection/cooldown/feedback,
inventory and preparation guidance. Its active Wet, meal and support rows
are removed; the old independent weather subsystem creates no badge. The
lower-left survival widget now shows only ocean travel information.
The optional colour-independent feedback panel retains action results and
nearby construction/hearth context; its duplicate Wet/active-support rows
are also removed. The ownership audit rejects status queries in either
left-side feedback source.

## Catalogue audit

The schema-4 JSON currently contains 42 inventory definitions and 19 recipe
outputs, of which 13 are direct-material construction entries. There are
six carried tool identities from the tool lifecycle catalogue. All 48
inventory/tool identities have explicit original icon assignments in
KalmalaIconWidget.cpp. Related materials and foods use authored monochrome
accents; root vegetables also have distinct silhouettes. Names, quantities,
tool levels/condition, recipe descriptions and requirements remain text.
Unknown IDs receive a question-mark fallback, never an invented assignment.

Construction JSON display IDs resolve through the existing loader to stable
runtime/save IDs (for example HearthRing to CampfireKit); icon mappings use
those canonical runtime IDs. The inventory renders icon/name rows and the
recipe/build selector renders the same icon for its selected canonical
output. All 19 selector entries therefore share the output's assignment.
No item, recipe, balance value, RPC, replicated field or save schema changes.
The slot-grid and selected-result preview consumers reuse these canonical
assignments; the selected-result preview is verified in
`docs/ui-recipe-preview/`.

## Status transition cues

The local status hotbar gives short ring-and-label cues for an authoritative
status start, a recognized refresh, and removal. Started, Refreshed, and Ended
use separate theme colours and a bounded configurable `StatusCueDuration`.
High contrast uses a white ring and keeps the words; the ring is decorative
feedback, not the only distinction. An ended status keeps its former icon row
for the cue interval with the word “Ended,” then disappears. The normal
server-derived timer returns after a start/refresh cue expires. No effect,
duration, countdown source, RPC, replication, or save data changes.

The first owner/weather snapshot is a silent baseline. Late initial fields are
folded in briefly, and pawn changes reset transition history, so reconnects and
owner replacement do not replay old effects. Countdown decreases are silent;
status remaining-time increases, support-effect expiry extensions, and a new
weather authority timestamp can cue a refresh. Repeated changes coalesce per
status. Reduced motion displays the same ring at a steady opacity until its
theme duration expires; the theme can also disable the pulse animation.

The focused `Kalmala.UI.StatusHotbar.Transitions` automation covers baselines,
start/refresh/end decisions, timer decreases versus authoritative extensions,
coalescing, expiry, and reduced motion. The rendered read-only fixture captures
start, refresh, and end labels for both owners; `-ReducedMotion` requires a
steady full-opacity cue. Its synthetic transitions replace only the fixture
snapshot after logging the real owner snapshot.

## Verification

After all three implementations, build the disposable editor mirror described
in 07-development-setup.md. Run the full Kalmala automation queue, including
Kalmala.UI.StatusHotbar.SnapshotAndLayout and
Kalmala.UI.CatalogueIcons.CompleteCoverage. Coverage enumerates runtime
catalogues, all tool definitions and all build/recipe outputs; it rejects
missing/duplicate assignments. Snapshot checks cover order, simultaneous
signals, duplicate IDs, replicated timers, ongoing labels and expiry.

Run Verify-StatusHotbar.ps1 from the mirror at 1024x768, 1280x720 and
2560x1080 with 100/125/150% text and high contrast. Its explicit development
flag substitutes only a UI snapshot, captures empty/populated/removed states
on both peers, and logs each actual local pawn snapshot before substitution.
It never mutates gameplay. Inspect screenshots as well as bounds assertions.
Run existing rendered crafting, settings/accessibility and owner inventory
regressions. Retain results and limitations in PROGRESS.md. The presentation
fixture does not itself prove real gameplay status timing or expiry; those
remain covered by existing server status/weather automations and peer tests.

Final parent-level gate passed: editor build; 101/101 full-queue automations;
owner inventory and real rain/Wet/recovery peers; rendered crafting and final
settings/accessibility regressions; documentation and negative duplicate-row
audits. Reviewed captures and selected assertions are retained in
`status-icons/`. Rendered cases cover 1024x768/150% high contrast,
1280x720/100% standard, and 2560x1080/125% high contrast; geometry automation
covers all nine resolution/text-scale pairs. No package or physical-input
walkthrough was performed, and full M11 acceptance remains a later task.
