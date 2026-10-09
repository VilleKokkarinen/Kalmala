# M11 status parent and catalogue icons

The user requested the next three backlog items together on 2026-10-02,
with verification deferred until all three implementations were present.

The owner-local survival subsystem reads only its local controller's pawn.
It owns a transparent, non-focusable, hit-test-invisible hotbar at logical
top-right (-24,244), 24 units below the existing 208-unit minimap, whose
top/right inset is now 12 UI units.
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

The schema-4 JSON now contains 41 normal item definitions and 19 recipe
outputs, including 13 construction outputs. CampfireKit is one construction-
only output without a normal item row. There are six carried tool identities
from the tool lifecycle catalogue. All 48 canonical item/tool/construction
identities have explicit original line-art assignments in
KalmalaIconWidget.cpp. M12 pins raster-image identity, aliases, shared views,
and fixed batches in docs/43-catalogue-icon-manifest.md and
docs/catalogue-icon-manifest.csv. The manifest marks CampfireKit as
construction-only and excludes it from pack/storage views. Related materials
and foods use authored monochrome accents; root vegetables also have distinct
silhouettes. Names, quantities, tool levels/condition, recipe descriptions
and requirements remain text. Unknown IDs receive a question-mark fallback,
never an invented assignment. The M12 shared raster lookup accepts only these
canonical runtime IDs; missing/unimported textures keep their existing vector
icons, while unknown IDs keep the question-mark fallback.

Construction JSON display IDs resolve through the existing loader to stable
runtime/save IDs (for example HearthRing to CampfireKit); icon mappings use
those canonical runtime IDs. Inventory, tool, ingredient, storage, recipe,
and build surfaces reuse one identity's assignment instead of defining an
image per view. All 19 recipe outputs therefore share their output identity's
assignment. The manifest does not change gameplay catalogue or save IDs.
No item, recipe, balance value, RPC, replicated field or save schema changes.
The later slot grid and larger selected-result preview remain separate tasks.

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

## M12 active-only follow-on

The hotbar now includes only known positive-duration player statuses, the
existing qualifying Hot/Cold exposure states, a live supported owner effect,
and a current Storm. Storm uses the shared `HighlyActiveStormThreshold` of
0.65 and must be inside its replicated server-time interval; ordinary weather,
fog-only high activity, stale intervals, and future intervals stay hidden.
Expired or cleared entries disappear from the rebuilt local snapshot list, and
an empty list collapses the whole widget with zero calculated height. This is
presentation-only and adds no timer, gameplay threshold, replication, or save
state.

## M12 active-status raster icons

[`status-icon-manifest.csv`](status-icon-manifest.csv) pins the nine entries
emitted by `UKalmalaStatusHotbarWidget::BuildEntries`: Wet, Steady meal, Hot,
Cold, Mending, Hearth shield, Bear's vigor, Deer call, and Storm. `entry_id`
matches the existing hotbar entry identity; `icon_id` is the canonical image
name. The rows are fixed in three ordered batches (01: four player conditions;
02: four support effects; 03: Storm). Existing accessible names and
server-derived timing remain widget text and are never baked into an image.

Keep generated originals under
`Content/Kalmala/UI/Source/IconOriginals/Status/<IconId>.png`. Prepare the
import PNGs under `Content/Kalmala/UI/Source/Icons/Status/<IconId>.png` with
`python Scripts/Prepare-StatusIconBatch.py --batch 01`; the script alpha-crops
visible artwork and centers it in a transparent 64x64 RGBA canvas with a
56x56 maximum artwork area. Validate each pinned ID with
`Scripts/Validate-StatusIcon.ps1 -Id <IconId>`. The intended future Unreal
texture location is `/Game/Kalmala/UI/Icons/Status/<IconId>.<IconId>`.

Batch 01 was generated with the built-in image generator and reviewed at
native size. Its prompt set asks for: a clear blue-gray water droplet with a
moisture sheen; a small wooden bowl of amber stew with root vegetables, one
herb leaf, and steam; a golden-orange sun with three heat ripples; and a
six-point icy frost crystal over a pale-blue shard. All four use a centered,
hand-painted survival-game inventory style, a thin charcoal contour, a warm
directional highlight, transparent margins, and no text, scenery, badges, or
UI border. The retained sources and prepared files pass the PNG dimension and
alpha validator and remain recognizable at 64x64.

The batch is prepared only: no status texture `.uasset` has been imported and
the hotbar does not consume these images yet. Later goal-10 increments import
the complete pinned set and switch the local status presentation to these
images. No status timing, effect, authority, replication, or save contract is
changed by the assets.
