# Forest Chanterelle icon provenance

Generated with the built-in image-generation tool as an original transparent
inventory icon. The artwork shows three golden-orange chanterelle mushrooms
with wavy funnel caps, pale stems, and subtle forked ridges.

- Original: `Content/Kalmala/UI/Source/IconOriginals/ForestChanterelle.png`
- Prepared: `Content/Kalmala/UI/Source/Icons/ForestChanterelle.png`
- Imported: `Content/Kalmala/UI/Icons/Items/ForestChanterelle.uasset`
- Runtime: `/Game/Kalmala/UI/Icons/Items/ForestChanterelle.ForestChanterelle`
- Batch: Y

## Generation prompt

Use case: stylized-concept. Asset type: original Kalmala survival-fantasy
inventory icon. Primary request: create a new, isolated Forest Chanterelle
item icon. Subject: a small cluster of three wild chanterelle mushrooms,
recognizable by golden-orange wavy funnel caps, pale cream stems, and subtle
forked ridges beneath the caps. Style/medium: tactile hand-painted game
inventory artwork with broad painted facets, a bold dark-brown contour,
restrained highlights, and earthy colors, matching the established Kalmala
small-item icon treatment. Composition/framing: one centered mushroom cluster
filling about 78 percent of a square canvas, clear and readable at 64 pixels,
with clean transparent padding. Constraints: original artwork; genuinely
transparent background; no text, label, UI, border, ground, soil, cast shadow,
glow, leaves, berries, basket, watermark, logo, or extra objects.

The item is hand-gathered without a tool from a deterministic, server-selected
Elderwood patch. Each accepted depletion grants exactly two Forest
Chanterelles. Eating one uses the existing non-stacking Steady Meal effect.
The world patch is the acquisition route; there is no crafting recipe,
equipment profile, upgrade cost, or attack data.

Prepare and import the retained artwork with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch Y
Scripts/Import-CatalogueIconBatch.ps1 -Batch Y
Scripts/Validate-UIIcons.ps1 -Category Catalogue
```
