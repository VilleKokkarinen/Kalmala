# Garden Plot icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original
transparent build-menu artwork. The 1254×1254 RGBA source is prepared to a
64×64 texture and imported as `GardenPlotKit` in isolated batch AB. The image
was visually checked at 64×64. No reference image or existing artwork was
used.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/GardenPlotKit.png`
- `Content/Kalmala/UI/Source/Icons/GardenPlotKit.png`
- `Content/Kalmala/UI/Icons/Items/GardenPlotKit.uasset`

Prepare, import and validate with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AB
Scripts/Import-CatalogueIconBatch.ps1 -Batch AB
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id GardenPlotKit
```

The manifest registers this as a `buildable` icon for the `GardenPlot` recipe.
Its `GardenPlot` recipe identity resolves to the `GardenPlotKit` texture. This
is presentation for a directly placed object and does not create an inventory
item or a portable plot kit.

Exact generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy build-menu icon. Primary request: create an isolated GardenPlotKit icon. Subject: a single compact square garden bed made from low, rough dark-brown timber edging around rich, freshly tilled dark soil; the bed is empty and ready for planting, with no crop or sprout. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark-brown contour, restrained painted highlights, and earthy wood-and-soil colors; clear silhouette at 64-pixel icon scale. Composition: one centered square plot seen from a slight three-quarter overhead angle, filling about 78 percent of a square canvas with clean transparent padding. Constraints: genuinely transparent background; original artwork; no reference image, plants, seeds, tools, loose stones, text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, people, or extra objects.
