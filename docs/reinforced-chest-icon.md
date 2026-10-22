# Reinforced Chest icon provenance

Generated with the built-in image-generation tool on 2026-10-22 as original,
transparent build-menu artwork. The retained 1254×1254 RGBA original is
prepared to a centered 64×64 RGBA texture and imported as `ReinforcedChestKit`
in isolated batch AN. No reference image or existing artwork was used.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/ReinforcedChestKit.png`
- `Content/Kalmala/UI/Source/Icons/ReinforcedChestKit.png`
- `Content/Kalmala/UI/Icons/Items/ReinforcedChestKit.uasset`

Prepare, import, and validate with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AN
Scripts/Import-CatalogueIconBatch.ps1 -Batch AN
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id ReinforcedChestKit
```

The icon manifest registers this as a `buildable` identity for the
`ReinforcedChest` recipe. It depicts a placed storage object, not a portable
inventory item or kit.

Generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy build-menu icon for a placed structure. Primary request: create a distinct ReinforcedChestKit icon that reads as a sturdy storage chest reinforced with bronze. Subject: one squat rectangular hand-built timber chest, closed, viewed from a clear three-quarter angle; visible lid, front latch, and broad bronze corner bands plus two bronze reinforcement straps across the front. Style/medium: small hand-painted game catalogue artwork matching an earthy crafted survival game, tactile broad painted facets, bold dark brown contour, restrained highlights, warm aged wood and muted bronze. Composition/framing: one centered chest filling about 78 percent of a square canvas, clean silhouette readable at 64 pixels, transparent padding. Background: genuinely transparent. Constraints: original artwork, no reference image, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, people, key, extra objects, or portable-kit packaging. Ensure transparency rather than a checkerboard pattern.
