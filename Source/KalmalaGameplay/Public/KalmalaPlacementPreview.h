#pragma once

#include "CoreMinimal.h"
#include "KalmalaBiomeClassifier.h"

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
    /** Approved Garden Plot terrain: Meadows, Elderwood, or dry Shimmering Lakes terrain at <=15 degrees. */
    static bool IsGardenPlotGroundAllowed(EKalmalaBiome Biome, float SurfaceNormalZ);
    /** Schema-1 test data excludes M9 kits; current schema-4 slots store approved attachments directly. */
    static bool IsSessionOnlyKit(FName ItemId);
    static FKalmalaPlacementPreview Evaluate(const UWorld* World, const APawn* Pawn, FName ItemId);
};
