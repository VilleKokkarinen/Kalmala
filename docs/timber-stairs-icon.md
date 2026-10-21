# Timber Stairs icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original,
transparent build-menu artwork. The retained 1254×1254 RGBA original was
prepared to a centered 64×64 RGBA texture and imported as `TimberStairsKit` in
batch AE. The artwork depicts broad timber treads and side stringers; no
reference image or existing artwork was used.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/TimberStairsKit.png`
- `Content/Kalmala/UI/Source/Icons/TimberStairsKit.png`
- `Content/Kalmala/UI/Icons/Items/TimberStairsKit.uasset`

The icon manifest registers the `TimberStairs` buildable and recipe identity
to the `TimberStairsKit` texture. It represents a directly placed structure,
not an inventory item or portable construction kit.

Prepare, import, and validate with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AE
Scripts/Import-CatalogueIconBatch.ps1 -Batch AE
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id TimberStairsKit
```

Generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy build-menu icon. Primary request: create a standalone transparent icon for a Timber Stairs buildable. Subject: one compact timber staircase with a 2 m run, 1 m width, and 1 m rise, shown from a three-quarter angle so the stepped top profile, side stringers, and solid walkable treads are immediately readable. Style/medium: small hand-painted game inventory artwork, tactile broad facets, bold dark brown contour, restrained painted highlights, warm aged timber colors. Composition/framing: one centered staircase filling about 78 percent of a square canvas, clean silhouette readable at 64 pixels. Materials/textures: hand-hewn brown planks, broad carved wood grain marks, no metal fittings. Constraints: genuinely transparent background; original artwork; no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, ladder, rope, railings, wall, person, or extra objects.
