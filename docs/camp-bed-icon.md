# Camp Bed icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original,
transparent build-menu artwork. The retained 1254×1254 RGBA original was
prepared as a centered 64×64 RGBA texture and imported as `CampBedKit` in batch
AF. The first render contained an opaque checkerboard; an image edit removed
only that backdrop. The final source has transparent pixels and was visually
checked before preparation.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/CampBedKit.png`
- `Content/Kalmala/UI/Source/Icons/CampBedKit.png`
- `Content/Kalmala/UI/Icons/Items/CampBedKit.uasset`

The icon manifest maps the `CampBed` buildable and recipe to the `CampBedKit`
texture. It depicts a placed object, never a portable inventory item or kit.

Prepare, import, and validate with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AF
Scripts/Import-CatalogueIconBatch.ps1 -Batch AF
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id CampBedKit
```

Generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy build-menu icon. Primary request: create an isolated Camp Bed buildable icon. Subject: one compact hand-built timber bed, sized for one person, shown from a clear three-quarter angle with the full 2 m by 1 m footprint readable; sturdy wooden frame with four short legs, layered wooden slats, and a simple pale natural linen bedroll, no pillow, no blanket folds covering the frame. Style: small hand-painted game inventory artwork matching an earthy crafted survival-game catalogue, bold dark brown contour, broad tactile painted facets, restrained highlights, warm aged timber browns and muted cream linen. Composition: one centered object filling about 78 percent of a square canvas, clean transparent padding, silhouette legible at 64 pixels. Background: genuinely transparent. Constraints: original artwork, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, person, metal fittings, headboard, extra objects, or copied references.

Edit instruction:

> Preserve the illustrated timber bed unchanged: keep the same shape, four legs, slats, linen bedroll, angle, colors, contour, scale and framing. Remove only the gray checkerboard backdrop and make everything outside the bed silhouette genuinely transparent, with clean antialiased alpha edges. Do not draw any background pattern, shadow, floor, text, or extra object.
