#include "KalmalaPlacementPreview.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaWorldGenerationGameState.h"
#include "GameFramework/Pawn.h"

bool FKalmalaPlacementPreview::IsSupportedKit(const FName ItemId)
{
    return ItemId == TEXT("CampfireKit") || ItemId == TEXT("WorkbenchKit") || ItemId == TEXT("StorageKit")
        || ItemId == TEXT("FloorKit") || ItemId == TEXT("WallKit") || ItemId == TEXT("RoofKit");
}

FKalmalaPlacementPreview FKalmalaPlacementPreview::Evaluate(const UWorld* World, const APawn* Pawn, const FName ItemId)
{
    FKalmalaPlacementPreview Result;
    Result.Message = TEXT("Preview: select a camp or construction kit");
    if (!IsSupportedKit(ItemId)) return Result;
    Result.Message = TEXT("Preview unavailable: waiting for local terrain");
    if (!World || !Pawn || Pawn->GetActorLocation().ContainsNaN()) return Result;

    const FVector Forward = FRotator(0.0f, Pawn->GetActorRotation().Yaw, 0.0f).Vector();
    const FVector Probe = Pawn->GetActorLocation() + Forward * 165.0f;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(LocalConstructionPreview), false, Pawn);
    FHitResult Ground;
    Result.Message = TEXT("Preview invalid: face clear, dry, gently sloping ground");
    if (Probe.ContainsNaN() || !World->LineTraceSingleByChannel(Ground, Probe + FVector(0, 0, 100), Probe - FVector(0, 0, 350), ECC_Visibility, Query)
        || !Cast<AKalmalaGeneratedTerrainPatch>(Ground.GetActor()) || Ground.ImpactNormal.Z < 0.85f) return Result;

    const auto* State = World->GetGameState<AKalmalaWorldGenerationGameState>();
    if (!State || !State->GetWorldGenerationConfig().IsValid()) return Result;
    const auto& Config = State->GetWorldGenerationConfig();
    const FVector2D Surface(Ground.ImpactPoint);
    if (!FKalmalaWorldBounds::Contains(Config, Surface, 300)) { Result.Message = TEXT("Too close to the world edge"); return Result; }
    if (FKalmalaOceanSampler::Sample(Config, Surface).IsWater() || FKalmalaShimmeringLakeSampler::IsWater(Config, Surface)
        || (Config.GeneratorRevision >= 3 && FKalmalaRegionalGeneration::Sample(Config, Surface).bHasWater))
    {
        Result.Message = TEXT("Preview invalid: dry ground is required");
        return Result;
    }

    Result.Location = Ground.ImpactPoint + FVector(0, 0, 56);
    if (World->OverlapBlockingTestByChannel(Result.Location, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(54), Query))
    {
        Result.Message = TEXT("Preview invalid: clear more space");
        return Result;
    }
    Result.bIsValid = true;
    Result.Message = TEXT("Preview valid locally: server will recheck before placement");
    return Result;
}
