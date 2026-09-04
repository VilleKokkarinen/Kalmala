#include "KalmalaShelterSampler.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

const FName FKalmalaShelterSampler::RoofTag(TEXT("KalmalaShelterRoof"));
const FName FKalmalaShelterSampler::WindbreakTag(TEXT("KalmalaShelterWindbreak"));

FKalmalaShelterSample FKalmalaShelterSampler::Sample(const UWorld* World, const AActor* Pawn, const float NaturalCover, const int32 WindDirectionDegrees)
{
    if (World == nullptr || Pawn == nullptr)
    {
        return Compose(NaturalCover, false, false);
    }

    const FVector ProbeOrigin = Pawn->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);
    const bool bHasRoof = HasTaggedBlockingGeometry(World, Pawn, ProbeOrigin, ProbeOrigin + FVector::UpVector * RoofProbeHeight, RoofTag);
    const FVector WindDirection = FRotator(0.0f, static_cast<float>(WindDirectionDegrees), 0.0f).Vector();
    const bool bHasWindbreak = HasTaggedBlockingGeometry(World, Pawn, ProbeOrigin, ProbeOrigin - WindDirection * WindbreakProbeDistance, WindbreakTag);
    return Compose(NaturalCover, bHasRoof, bHasWindbreak);
}

FKalmalaShelterSample FKalmalaShelterSampler::Compose(const float NaturalCover, const bool bHasRoof, const bool bHasWindbreak)
{
    FKalmalaShelterSample Result;
    Result.NaturalCoverShelter = FMath::Clamp(NaturalCover, 0.0f, 1.0f) * 0.30f;
    Result.bHasRoof = bHasRoof;
    Result.bHasWindbreak = bHasWindbreak;
    Result.RoofShelter = bHasRoof ? 0.45f : 0.0f;
    Result.WindbreakShelter = bHasWindbreak ? 0.35f : 0.0f;
    Result.Shelter = FMath::Clamp(Result.NaturalCoverShelter + Result.RoofShelter + Result.WindbreakShelter, 0.0f, 1.0f);
    return Result;
}

bool FKalmalaShelterSampler::HasTaggedBlockingGeometry(const UWorld* World, const AActor* IgnoredActor, const FVector& Start, const FVector& End, const FName RequiredTag)
{
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KalmalaShelterGeometry), false, IgnoredActor);
    TArray<FHitResult> Hits;
    World->LineTraceMultiByChannel(Hits, Start, End, ECC_Visibility, QueryParams);
    for (const FHitResult& Hit : Hits)
    {
        if (const AActor* HitActor = Hit.GetActor(); HitActor != nullptr && HitActor->ActorHasTag(RequiredTag))
        {
            return true;
        }
    }
    return false;
}
