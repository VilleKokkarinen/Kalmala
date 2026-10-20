# Smelter build-menu icon provenance

Generated with the built-in image-generation tool on 2026-10-20 as an original
transparent icon for the construction-only `SmelterKit` identity. The icon uses
the established small painted style and is distinguishable from the Charcoal
Kiln by its taller capped chimney and side tapping channel. Batch L prepared the
64×64 image and imported its runtime Texture2D.

Files:

- `Content/Kalmala/UI/Source/IconOriginals/SmelterKit.png`
- `Content/Kalmala/UI/Source/Icons/SmelterKit.png`
- `Content/Kalmala/UI/Icons/Items/SmelterKit.uasset`

Workflow:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch L
Scripts/Import-CatalogueIconBatch.ps1 -Batch L
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id SmelterKit
```

Generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy build-menu icon for a placeable metal Smelter. Primary request: create one isolated Smelter construction icon that is clearly distinct from a charcoal kiln. Subject: a compact squat stone smelting furnace with a broad blocky masonry base, a tall narrow capped chimney rising from the rear, and a small dark arched front firebox; add a short side tapping channel that reads as a smelter outlet, with no visible molten metal. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights, earthy cool stone gray and soot-brown colors, readable at 64 pixels. Composition: one centered object filling about 78 percent of a square canvas, slight three-quarter view, clean transparent padding. Lighting: soft neutral highlight on stone edges, no glow. Materials: rough fitted stones and dark iron outlet. Constraints: original artwork, genuinely transparent background, no text, letters, labels, UI, border, ground, cast shadow, watermark, logo, people, flames, sparks, loose ore, coal, ingots, or extra objects.
