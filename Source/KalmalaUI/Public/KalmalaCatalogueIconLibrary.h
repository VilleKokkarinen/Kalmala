#pragma once

#include "CoreMinimal.h"

class UTexture2D;

/** Shared canonical item-to-texture lookup for catalogue UI. */
class KALMALAUI_API FKalmalaCatalogueIconLibrary final
{
public:
    /** Returns the deterministic imported object path for a known runtime ID. */
    static bool GetTextureObjectPath(FName CanonicalId, FSoftObjectPath& OutObjectPath);

    /** Loads an imported texture, or returns null while it is missing or unimported. */
    static UTexture2D* LoadTexture(FName CanonicalId);
};
