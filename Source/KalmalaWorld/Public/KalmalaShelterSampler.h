#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

/** Server-sampled shelter inputs. Geometry is supplied by constructed actors, never authored volumes. */
struct KALMALAWORLD_API FKalmalaShelterSample
{
    float NaturalCoverShelter = 0.0f;
    float RoofShelter = 0.0f;
    float WindbreakShelter = 0.0f;
    float Shelter = 0.0f;
    bool bHasRoof = false;
    bool bHasWindbreak = false;
};

/**
 * Samples only collision geometry explicitly tagged by server-owned construction.
 * Future placed roof and windbreak actors opt in with KalmalaShelterRoof and
 * KalmalaShelterWindbreak respectively; level-authored shelter volumes are not used.
 */
struct KALMALAWORLD_API FKalmalaShelterSampler
{
    static constexpr float RoofProbeHeight = 420.0f;
    static constexpr float WindbreakProbeDistance = 360.0f;
    static const FName RoofTag;
    static const FName WindbreakTag;

    static FKalmalaShelterSample Sample(const UWorld* World, const AActor* Pawn, const float NaturalCover, const int32 WindDirectionDegrees);
    static FKalmalaShelterSample Compose(const float NaturalCover, const bool bHasRoof, const bool bHasWindbreak);

private:
    static bool HasTaggedBlockingGeometry(const UWorld* World, const AActor* IgnoredActor, const FVector& Start, const FVector& End, const FName RequiredTag);
};
