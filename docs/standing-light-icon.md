# Standing Light icon provenance

The original transparent 1254×1254 RGBA Standing Light artwork and its
centered 64×64 RGBA preparation were already retained in the project. The
icon manifest now imports this placed-buildable identity in isolated batch AO
so RatTail's separate batch AG package is not rewritten. No inventory item or
portable kit is introduced.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/StandingLightKit.png`
- `Content/Kalmala/UI/Source/Icons/StandingLightKit.png`
- `Content/Kalmala/UI/Icons/Items/StandingLightKit.uasset`

The original generation prompt was not retained with the existing source and
is not reconstructed here. The artwork remains unchanged and is validated by
the shared icon workflow:

```powershell
Scripts/Import-CatalogueIconBatch.ps1 -Batch AO
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id StandingLightKit
```
