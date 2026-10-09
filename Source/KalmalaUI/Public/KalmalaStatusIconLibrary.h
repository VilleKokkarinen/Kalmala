#pragma once

#include "CoreMinimal.h"

class UTexture2D;

/** Canonical imported status textures, separate from inventory/build identities. */
class KALMALAUI_API FKalmalaStatusIconLibrary
{
public:
    /** Maps a stable hotbar entry ID to its canonical status image ID. */
    static bool GetIconIdForEntry(FName EntryId, FName& OutIconId);

    /** Returns the deterministic imported texture path for a supported status image. */
    static bool GetTextureObjectPath(FName IconId, FSoftObjectPath& OutObjectPath);

    /** Loads a supported status texture, or returns null while it is missing. */
    static UTexture2D* LoadTexture(FName IconId);
};
