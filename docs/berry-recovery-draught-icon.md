# Berry Recovery Draught icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as an original
transparent inventory icon. The final artwork is one hand-painted ruby-red
glass vial with a cork and a small berry cluster. It was prepared at 64×64 for
catalogue import batch Z.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/BerryRecoveryDraught.png`
- `Content/Kalmala/UI/Source/Icons/BerryRecoveryDraught.png`
- `Content/Kalmala/UI/Icons/Items/BerryRecoveryDraught.uasset`

Workflow:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch Z
Scripts/Import-CatalogueIconBatch.ps1 -Batch Z
Scripts/Validate-UIIcons.ps1 -Category Catalogue
```

The runtime mapping is `BerryRecoveryDraught` in
`Source/KalmalaUI/Private/KalmalaIconWidget.cpp`. The original prompt was:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Primary request: create one isolated Berry Recovery Draught icon for an inventory grid. Subject: a small squat glass potion vial containing rich ruby-red berry infusion, with a simple cork stopper and a restrained cluster of three tiny red berries with one muted green leaf near the neck; one object only. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors, matching the existing Kalmala 64-pixel item icon treatment. Composition/framing: one centered object filling about 78 percent of a square canvas, clear readable silhouette with clean transparent padding. Materials/textures: softly beveled glass, opaque red liquid, simple cork; broad painted facets rather than fine detail. Constraints: genuinely transparent background; original artwork; no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects.
