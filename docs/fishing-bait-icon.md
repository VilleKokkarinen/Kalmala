# Fishing Bait icon provenance

Generated with the built-in image-generation tool as original transparent
inventory artwork. The icon shows a tight cluster of five golden-brown bait
pellets with pale chanterelle flecks.

- Original: `Content/Kalmala/UI/Source/IconOriginals/FishingBait.png`
- Prepared: `Content/Kalmala/UI/Source/Icons/FishingBait.png`
- Imported: `Content/Kalmala/UI/Icons/Items/FishingBait.uasset`
- Runtime: `/Game/Kalmala/UI/Icons/Items/FishingBait.FishingBait`
- Batch: AB

## Generation prompt

Use case: stylized-concept. Asset type: original Kalmala survival-fantasy
inventory icon. Primary request: create a new, isolated FishingBait item icon.
Subject: a compact cluster of five small rounded bait pellets made from ground
golden-brown chanterelle mushroom, with a few subtle pale cream mushroom flecks
embedded in the surfaces. Style/medium: tactile hand-painted game inventory
artwork with broad painted facets, a bold dark-brown contour, restrained
highlights, and earthy colors, matching the established Kalmala small-item
icon treatment. Composition/framing: one centered cluster filling about 78
percent of a square canvas, clear and readable at 64 pixels, with clean
transparent padding. Lighting/mood: soft neutral studio light. Constraints:
original artwork; genuinely transparent background; no text, label, UI, border,
ground, soil, cast shadow, glow, hook, line, fish, worms, leaves, basket,
container, watermark, logo, or extra objects.

Fishing Bait is crafted through Inventory Craft by spending one Forest
Chanterelle for five. Its registered stack limit is 50 and weight is 0.02 kg
per item. It has no equipment profile, upgrades, or attack data; catching fish
is a separate A7 interaction task.

Prepare and import with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AB
Scripts/Import-CatalogueIconBatch.ps1 -Batch AB
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id FishingBait
```
