# M11 status parent and catalogue icons

M12 final rendering requires each raster image to fill its scaled 64x64 icon
box. The overlay slot explicitly fills both axes; imported texture loading alone
does not prove painted size. `Verify-StatusHotbar.ps1` now requires the runtime
`Hotbar raster geometry` marker for all six populated icons on each peer in
every rendered matrix case. Earlier M12 captures with smaller default-sized
images are superseded by the final evidence in `docs/45-m12-acceptance.md`.
The group also reserves a 12-unit gap beyond the top-centre support cue's
actual right edge and wraps down when that reduces its available width.
The support panel sizes to its four glyph cards so they remain within the
panel. The rendered helper applies text scale/contrast to both cues and
requires a `Hotbar support separation` marker on each peer.

The user requested the next three backlog items together on 2026-10-02,
with verification deferred until all three implementations were present.

The owner-local survival subsystem reads only its local controller's pawn.
It owns a transparent, non-focusable, hit-test-invisible status group just left
of the configured minimap slot, sharing its top offset and leaving a 12-unit
horizontal gap. Compact 72-unit cells render 64x64 raster icons with no visible
names; a visually hidden text label keeps each status name available to
assistive technology. Finite player-effect timers are centred beneath the
icons. Cells have four-unit gaps and wrap downward within a 384-unit row cap,
clamped to the safe width left of the minimap. Empty states collapse; no
background, border or empty slots are drawn.

Stable order: Wet, Steady meal, Hot, Cold, the current support effect, then
Storm. The M12 rendering uses the imported 64x64 icons without visible names;
accessible names remain available to assistive technology. Only finite player
statuses and support effects have a centred m:ss timer below the icon, derived
from owner replication or synchronized server time. Hot, Cold and Storm have
no countdown, and weather intervals are not displayed as player timers.
Expired statuses disappear and duplicate status IDs appear once.

Escape > Status and weather details provides live wrapped, scrollable text
for category, intensity, magnitudes, source and recovery. It retains finite
player-effect timing but omits weather countdowns and ongoing labels for
untimed conditions. This uses existing modal focus and input restoration.
Supported statuses do not stack. The left HUD retains combat, discovery,
support selection/cooldown/feedback, inventory and preparation guidance. Its
active Wet, meal and support rows are removed; the old independent weather
subsystem creates no badge. The lower-left survival widget now shows only
ocean travel information.
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
signals, duplicate IDs, finite replicated m:ss timers, omitted untimed/weather
countdowns, and expiry.

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
02: four support effects; 03: Storm). Assistive names stay available in a
visually hidden text label and server-derived finite timing remains widget
text; names and timing are never baked into an image.

Keep generated originals under
`Content/Kalmala/UI/Source/IconOriginals/Status/<IconId>.png`. Prepare the
import PNGs under `Content/Kalmala/UI/Source/Icons/Status/<IconId>.png` with
`python Scripts/Prepare-StatusIconBatch.py --batch 01`; the script alpha-crops
visible artwork and centers it in a transparent 64x64 RGBA canvas with a
56x56 maximum artwork area. Validate each pinned ID with
`Scripts/Validate-StatusIcon.ps1 -Id <IconId>`. The complete set is imported
as Texture2D packages at `/Game/Kalmala/UI/Icons/Status/<IconId>.<IconId>`.
`FKalmalaStatusIconLibrary` maps each hotbar entry ID to its manifest image ID
and constructs only those nine supported object paths.

Batch 01 was generated with the built-in image generator and reviewed at
native size. Its prompt set asks for: a clear blue-gray water droplet with a
moisture sheen; a small wooden bowl of amber stew with root vegetables, one
herb leaf, and steam; a golden-orange sun with three heat ripples; and a
six-point icy frost crystal over a pale-blue shard. All four use a centered,
hand-painted survival-game inventory style, a thin charcoal contour, a warm
directional highlight, transparent margins, and no text, scenery, badges, or
UI border. The retained sources and prepared files pass the PNG dimension and
alpha validator and remain recognizable at 64x64.

Batch 02 was generated with the built-in image generator and reviewed at
native size. Its prompt set asks for: a luminous gold healing cross nestled
between two green leaves; a compact wooden buckler with a warm amber hearth
glow; one broad bear paw print with four clear toe pads; and a symmetrical
pair of branching deer antlers with one small sound ring. The same centered,
hand-painted inventory style, thin charcoal contour, warm directional
highlight, transparent margins, and no text, scenery, badges, or UI border
were used. All four retained 1254x1254 RGBA sources and prepared 64x64 RGBA
files pass the identity, dimension, alpha and path validator and remain
recognizable at native size.

Batch 03 was generated with the built-in image generator and reviewed at
native size. Its prompt asks for one compact dark slate-blue storm cloud with
a single bold amber lightning bolt, a thin charcoal contour, warm directional
highlight, transparent margins, and no rain, text, scenery, badges, or UI
border. The retained 1254x1254 RGBA source and prepared transparent 64x64 RGBA
image pass the identity, dimension, alpha and path validator and remain
recognizable at native size.

`Scripts/Import-StatusIconAssets.ps1` imports all nine prepared PNGs through a
temporary UE 5.8 content-only project and copies only their Texture2D packages
into the project. The importer verifies 64x64 dimensions, source alpha,
sRGB, UI texture group, no mipmaps, and non-streaming settings. Run
`Scripts/Validate-StatusIconSet.ps1` for manifest, entry-map, PNG, and package
coverage; `Kalmala.UI.StatusHotbar.StatusIconCoverage` checks the nine runtime
paths and texture loads after an affected editor build. Each current hotbar
entry renders its imported image at 64x64, retaining a vector fallback and a
visually hidden accessible name. A centred m:ss appears under finite
Wet/meal/support entries only; Hot, Cold and Storm have no timer, and the
separate Status and weather detail view omits weather countdowns and untimed
"ongoing" labels. The displayed finite values are rebuilt from the existing
owner snapshot and synchronized server time; the client does not decrement
them locally. No status timing, effect, authority, replication, or save
contract changed.
