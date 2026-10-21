# Planting Trowel icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original
transparent inventory artwork. The generated 1254×1254 RGBA source is prepared
to a 64×64 transparent icon in batch AA and imported as the `PlantingTrowel`
Texture2D. The image was visually checked for its wooden handle, bronze neck,
and single broad planting blade. No reference image or existing artwork was
used.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/PlantingTrowel.png`
- `Content/Kalmala/UI/Source/Icons/PlantingTrowel.png`
- `Content/Kalmala/UI/Icons/Items/PlantingTrowel.uasset`

Prepare, import and validate with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AA
Scripts/Import-CatalogueIconBatch.ps1 -Batch AA
Scripts/Validate-UIIcons.ps1 -Category Catalogue
```

The carried-tool profile is in `items/equipment/utility.json`; the Forge
acquisition is in `recipes/stations/forge.json`. It has no attack list or
level-up cost, and uses the existing free Workbench/Forge repair path. Planting,
harvesting, and condition use remain part of A7 plot behavior.

Exact generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Primary request: create a new, isolated PlantingTrowel item icon. Subject: one compact hand-held garden planting trowel with a sturdy dark brown wooden handle, a short bronze-colored neck, and a single broad, gently cupped, leaf-shaped bronze blade with a rounded tip; it must read clearly as a planting trowel, with one blade and no rake tines. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette at 64-pixel inventory scale. Composition: one centered object filling about 78 percent of a square canvas, handle angled from lower left toward upper right, blade near the upper right, clean transparent padding. Background: genuinely transparent. Constraints: original artwork, no reference image, no soil, plants, leaves, text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, or extra objects.
