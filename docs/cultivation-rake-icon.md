# Cultivation Rake icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original
transparent inventory artwork. The generated 1254×1254 RGBA source is
prepared to a 64×64 transparent icon in batch AA and imported as the
`CultivationRake` Texture2D. The image was visually checked for its wooden
handle, bronze rake head and separated tines. No reference image or existing
artwork was used.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/CultivationRake.png`
- `Content/Kalmala/UI/Source/Icons/CultivationRake.png`
- `Content/Kalmala/UI/Icons/Items/CultivationRake.uasset`

Prepare, import and validate with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AA
Scripts/Import-CatalogueIconBatch.ps1 -Batch AA
Scripts/Validate-UIIcons.ps1 -Category Catalogue
```

The carried-tool profile is in `items/equipment/utility.json`; the Forge
acquisition is in `recipes/stations/forge.json`. It has no attack list or
level-up cost, and uses the existing free Workbench/Forge repair path. Plot
planting, harvesting and condition use remain part of A7 plot behavior.

Exact generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Primary request: create a new, isolated CultivationRake item icon. Subject: one compact hand-held garden rake with a long sturdy dark brown wooden handle and a small forged bronze-colored head with a straight crossbar and five short, clearly separated tines; it must read unmistakably as a cultivation rake. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition: one centered object filling about 78 percent of a square canvas, handle angled from lower left toward upper right with the rake head near the upper right, clean transparent padding. Background: genuinely transparent. Constraints: original artwork, no reference image, no soil, plants, leaves, text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects.
