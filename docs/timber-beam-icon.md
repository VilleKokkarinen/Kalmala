# Timber Beam icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original,
transparent build-menu artwork. The retained 1254×1254 RGBA original was
prepared to a 64×64 RGBA texture and imported as `TimberBeamKit` in isolated
batch AD. No reference image or existing artwork was used.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/TimberBeamKit.png`
- `Content/Kalmala/UI/Source/Icons/TimberBeamKit.png`
- `Content/Kalmala/UI/Icons/Items/TimberBeamKit.uasset`

The icon manifest registers the `TimberBeam` buildable and recipe identity to
the `TimberBeamKit` texture. This is presentation for a directly placed object,
not an inventory item or portable construction kit.

Prepare, import, and validate with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AD
Scripts/Import-CatalogueIconBatch.ps1 -Batch AD
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id TimberBeamKit
```

Generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy build-menu icon. Primary request: create a standalone icon for a Timber Beam buildable, a single hand-hewn squared timber beam with clear, sturdy support-beam silhouette. Subject: one 2 m long, 0.15 m square wooden structural beam, seen in a three-quarter diagonal view, with subtle end grain visible at both ends and broad carved facets. Style/medium: small hand-painted game inventory artwork, tactile broad facets, bold dark brown contour, restrained painted highlights, natural warm timber colors. Composition/framing: one centered beam filling about 78 percent of a square canvas, clear silhouette readable at 64 pixels. Materials/textures: aged warm brown wood with a few broad grain cuts, no metal fittings. Constraints: genuinely transparent background; original artwork; no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, wall, door, extra objects, or reference artwork.
