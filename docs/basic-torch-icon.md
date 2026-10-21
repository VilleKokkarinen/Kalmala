# Basic Torch icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original
transparent inventory artwork. The 1254×1254 RGBA source is prepared to a
64×64 transparent icon in batch W and imported as the `BasicTorch` Texture2D.
No existing artwork or outside reference was used.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/BasicTorch.png`
- `Content/Kalmala/UI/Source/Icons/BasicTorch.png`
- `Content/Kalmala/UI/Icons/Items/BasicTorch.uasset`

Prepare, import and validate with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch W
Scripts/Import-CatalogueIconBatch.ps1 -Batch W
Scripts/Validate-UIIcons.ps1 -Category Catalogue
```

Exact generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Primary request: create a new, isolated BasicTorch item icon. Subject: one simple hand-held wooden torch, a sturdy dark brown wooden shaft with a compact amber-wrapped flame bundle at the top and a small bright golden flame, unmistakably a basic carried torch; earthy warm palette with restrained amber highlights. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition: one centered object filling about 78 percent of a square canvas, shaft angled slightly from lower left to upper right, with clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects.

\n