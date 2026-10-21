# Timber Door icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original
transparent build-menu artwork. The 1254×1254 RGBA source was prepared to a
64×64 texture and imported as `TimberDoorKit` in isolated batch AC. No reference
image or existing artwork was used.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/TimberDoorKit.png`
- `Content/Kalmala/UI/Source/Icons/TimberDoorKit.png`
- `Content/Kalmala/UI/Icons/Items/TimberDoorKit.uasset`

Prepare, import, and validate with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AC
Scripts/Import-CatalogueIconBatch.ps1 -Batch AC
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id TimberDoorKit
```

The prompt requested an original stylized TimberDoorKit build-menu icon: one
closed timber door centered in a short wall segment, on a transparent
background, without text or extra objects. The manifest registers the
`TimberDoor` buildable and recipe identity to the `TimberDoorKit` texture. This
is presentation for a directly placed object, not an inventory item or
portable construction kit.
