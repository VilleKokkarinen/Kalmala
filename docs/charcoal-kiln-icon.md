# Charcoal Kiln build-menu icon provenance

The Charcoal Kiln construction icon was generated with the built-in image
generation tool on 2026-10-20. It is an original transparent PNG for the
construction-only `CharcoalKilnKit` identity. The existing preparation script
creates its centered 64×64 version and the icon importer creates the runtime
Texture2D in batch K.

Files:

- `Content/Kalmala/UI/Source/IconOriginals/CharcoalKilnKit.png`
- `Content/Kalmala/UI/Source/Icons/CharcoalKilnKit.png`
- `Content/Kalmala/UI/Icons/Items/CharcoalKilnKit.uasset`

The icon uses the existing batch workflow:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch K
Scripts/Import-CatalogueIconBatch.ps1 -Batch K
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id CharcoalKilnKit
```

Generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy build-menu icon for the placeable Charcoal Kiln. Primary request: create one isolated charcoal kiln icon. Subject: a compact squat stone charcoal kiln, a hand-built rounded-square masonry chamber with a small dark arched loading mouth and a short capped stone vent; make the silhouette clearly read as a kiln structure, not a forge or cooking pot. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights, earthy stone gray and soot-brown colors, readable at 64 pixels. Composition: one centered object filling about 78 percent of a square canvas, slight three-quarter view, clean transparent padding. Lighting: soft warm highlight on stone edges, no glow. Materials: rough fitted stones with a few soot marks, no loose fuel or props. Constraints: original artwork, genuinely transparent background, no text, letters, labels, UI, border, ground, cast shadow, watermark, logo, people, fire, or extra objects.
