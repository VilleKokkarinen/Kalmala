# Bone fragment icon provenance

Generated with the built-in image-generation tool on 2026-10-15. The existing
original `Stone.png` was a style reference only. No existing artwork was replaced.
The retained original is 1254x1254 RGBA with transparency; the prepared image is
64x64 RGBA. Batch H imports it as a UI Texture2D, preserving alpha.

- Original: `Content/Kalmala/UI/Source/IconOriginals/BoneFragment.png`
- Prepared: `Content/Kalmala/UI/Source/Icons/BoneFragment.png`
- Imported: `Content/Kalmala/UI/Icons/Items/BoneFragment.uasset`
- Runtime: `/Game/Kalmala/UI/Icons/Items/BoneFragment.BoneFragment`

## Generation prompt

Use case: stylized-concept. Generate a NEW original Bone fragment inventory item icon for Kalmala, using the supplied Stone icon ONLY as a rendering/style reference, replacing the subject completely. Subject: one short weathered animal long-bone fragment with one recognizable rounded joint end and a jagged broken hollow end, diagonally oriented from lower-left to upper-right. Dry ivory bone, warm cream highlights, muted tan weathering in cracks and porous broken edge, dark brown contour, tactile hand-painted survival-fantasy rendering matching the reference. Bold simple silhouette readable at 64x64. Centered isolated object filling approximately 78 percent of the square with clean transparent padding. True transparent background. No stones, no skull, no blood, no flesh, no ground, no cast shadow, no glow, no text, no logos, no border, no extra props.

## Reproduction and validation

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch H
Scripts/Import-CatalogueIconBatch.ps1 -Batch H
Scripts/Validate-UIIcons.ps1 -Category Catalogue
```

The original and prepared images were visually inspected. The content-only
Unreal importer loaded and validated the imported texture; all 63 ledger rows
passed the file validator. The icon mapping includes a dedicated bone vector
fallback, and the global icon automation now requires the imported BoneFragment
texture with every other mapped item. A gameplay editor build, global Unreal
automation and in-game rendering remain deferred. Existing missing material
icons and the duplicate shield recipe are separate unresolved issues.
