# Root Stamina Draught icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original
transparent inventory artwork. The final icon is one green-gold glass vial
with a cork, a potato and an onion tied near its neck. It was prepared at
64×64 for catalogue import batch Z; no outside image was used as a reference.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/RootStaminaDraught.png`
- `Content/Kalmala/UI/Source/Icons/RootStaminaDraught.png`
- `Content/Kalmala/UI/Icons/Items/RootStaminaDraught.uasset`

Prepare and import with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch Z
Scripts/Import-CatalogueIconBatch.ps1 -Batch Z
Scripts/Validate-UIIcons.ps1 -Category Catalogue
```

The runtime mapping is `RootStaminaDraught` in
`Source/KalmalaUI/Private/KalmalaIconWidget.cpp`. The original prompt was:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Primary request: create one isolated Root Stamina Draught icon for an inventory grid. Subject: a small squat glass vial containing muted green-gold root-vegetable infusion, with a simple cork stopper and a restrained tiny potato and onion cue near the neck; one object only. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors, matching the existing Kalmala 64-pixel item icon treatment. Composition/framing: one centered object filling about 78 percent of a square canvas, clear readable silhouette with clean transparent padding. Materials/textures: softly beveled glass, opaque pale green-gold liquid, simple cork, tiny matte root-vegetable shapes; broad painted facets rather than fine detail. Constraints: genuinely transparent background; original artwork; no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects.

The original and 64×64 prepared PNGs were validated as transparent RGBA. The
batch importer created the matching Texture2D package and validated its
runtime mapping. In-game rendered appearance and the gameplay automation
remain for I2 milestone-final verification.
