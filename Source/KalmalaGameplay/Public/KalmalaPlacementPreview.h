#pragma once

#include "CoreMinimal.h"

class APawn;
class UWorld;

/**
 * Local presentation probe for construction kits. It intentionally creates no
 * actor, reservation, RPC, inventory transaction, or saved state; the later
 * server placement request reruns its own authoritative validation.
 */
struct KALMALAGAMEPLAY_API FKalmalaPlacementPreview
{
    bool bIsValid = false;
    FVector Location = FVector::ZeroVector;
    FString Message;

    static bool IsSupportedKit(FName ItemId);
    /** New M9 kits stay out of schema-1 construction saves until migration is approved. */
    static bool IsSessionOnlyKit(FName ItemId);
    static FKalmalaPlacementPreview Evaluate(const UWorld* World, const APawn* Pawn, FName ItemId);
};
