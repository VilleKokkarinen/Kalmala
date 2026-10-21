# Wearable gear icon provenance

The six original icons were generated with the built-in image-generation tool.
The committed Bronze Axe original was supplied as a style reference. Each source
is a transparent 1254×1254 RGBA PNG. Prepared 64×64 RGBA versions are imported
as Texture2D assets in batch G; no existing image was replaced.

Shared prompt details: a crisp hand-painted survival-fantasy game inventory
icon; bold dark contour; warm directional highlights; tactile wood or hide;
readable silhouette at 64×64; centered item filling about 78% of the square;
clean transparent padding; no text, logo, border, shadow, glow, extra props, or
duplicate objects. Every prompt used `Content/Kalmala/UI/Source/IconOriginals/BronzeAxe.png`
as the style reference.

| Identity | Prompt subject | Original PNG | Prepared PNG | Texture2D |
| --- | --- | --- | --- | --- |
| `WoodShield` | Compact round ash-plank shield with a bronze rim and leather straps; front view, angled slightly, no person. | `Content/Kalmala/UI/Source/IconOriginals/WoodShield.png` | `Content/Kalmala/UI/Source/Icons/WoodShield.png` | `Content/Kalmala/UI/Icons/Items/WoodShield.uasset` |
| `BoarHideHelmet` | Open-face cap from layered brown boar hide, visible stitching and short leather brow guard; three-quarter view, no person. | `Content/Kalmala/UI/Source/IconOriginals/BoarHideHelmet.png` | `Content/Kalmala/UI/Source/Icons/BoarHideHelmet.png` | `Content/Kalmala/UI/Icons/Items/BoarHideHelmet.uasset` |
| `BoarHideCape` | Short shoulder cape from warm brown boar hide, simple clasp and stitched edging; three-quarter view, no person. | `Content/Kalmala/UI/Source/IconOriginals/BoarHideCape.png` | `Content/Kalmala/UI/Source/Icons/BoarHideCape.png` | `Content/Kalmala/UI/Icons/Items/BoarHideCape.uasset` |
| `BoarHideTunic` | Sleeveless chest tunic from layered brown boar hide, simple leather ties and handmade stitching; front three-quarter view, no person. | `Content/Kalmala/UI/Source/IconOriginals/BoarHideTunic.png` | `Content/Kalmala/UI/Source/Icons/BoarHideTunic.png` | `Content/Kalmala/UI/Icons/Items/BoarHideTunic.uasset` |
| `BoarHideLeggings` | Pair of rugged brown boar-hide leg guards joined by leather ties, shown as the garment alone; front three-quarter view, no person. | `Content/Kalmala/UI/Source/IconOriginals/BoarHideLeggings.png` | `Content/Kalmala/UI/Source/Icons/BoarHideLeggings.png` | `Content/Kalmala/UI/Icons/Items/BoarHideLeggings.uasset` |
| `BoarHideBoots` | Matched pair of sturdy ankle-high brown boar-hide boots with leather soles and rough stitching; three-quarter view, no person. | `Content/Kalmala/UI/Source/IconOriginals/BoarHideBoots.png` | `Content/Kalmala/UI/Source/Icons/BoarHideBoots.png` | `Content/Kalmala/UI/Icons/Items/BoarHideBoots.uasset` |

To prepare or reimport these assets, run:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch G
Scripts/Import-CatalogueIconBatch.ps1 -Batch G
```

The canonical IDs and texture asset names are listed in
`Scripts/Data/catalogue-icon-manifest.csv`.

Bronze Helmet was generated with the built-in image-generation tool on
2026-10-21 as a standalone transparent 1254×1254 RGBA original. Its retained
original was prepared to 64×64 RGBA and imported through the single-item S
batch; the six initial wearable icons in batch G were not replaced.

| Identity | Prompt subject | Original PNG | Prepared PNG | Texture2D |
| --- | --- | --- | --- | --- |
| `BronzeHelmet` | Single rounded hammered-bronze cap with a short brow ridge and narrow boar-hide lining; three-quarter front view, no wearer. | `Content/Kalmala/UI/Source/IconOriginals/BronzeHelmet.png` | `Content/Kalmala/UI/Source/Icons/BronzeHelmet.png` | `Content/Kalmala/UI/Icons/Items/BronzeHelmet.uasset` |
| `BronzeTunic` | Sleeveless plated-bronze chest tunic over a dark boar-hide backing; front three-quarter view, no wearer. | `Content/Kalmala/UI/Source/IconOriginals/BronzeTunic.png` | `Content/Kalmala/UI/Source/Icons/BronzeTunic.png` | `Content/Kalmala/UI/Icons/Items/BronzeTunic.uasset` |

The prompt used the existing warm survival-fantasy inventory treatment: dark
contours, broad painted metal facets, warm highlights, centered silhouette,
transparent padding, and no text, props, glow, or background.

Bronze Tunic was generated with the built-in image-generation tool on
2026-10-21 as a standalone transparent 1254×1254 RGBA original. Its retained
original was prepared to 64×64 RGBA and imported through the single-item T
batch; the existing helmet and six batch-G wearable icons were not replaced.
The prompt requested one sleeveless plated-bronze chest tunic backed with dark
boar hide, centered in a warm hand-painted survival-fantasy inventory style,
with transparent padding and no wearer, text, props, or background.
