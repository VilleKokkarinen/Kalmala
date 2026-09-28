#include "KalmalaPlacementPreview.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaConstructionActor.h"
#include "GameFramework/Pawn.h"

bool FKalmalaPlacementPreview::IsSupportedKit(const FName ItemId)
{
    if (ItemId == TEXT("ForgeKit")) return true;
    return ItemId == TEXT("CampfireKit") || ItemId == TEXT("WorkbenchKit") || ItemId == TEXT("StorageKit")
        || ItemId == TEXT("CookingRackKit") || ItemId == TEXT("CauldronKit") || ItemId == TEXT("SmokeFrameKit")
        || ItemId == TEXT("DryingLineKit")
        || ItemId == TEXT("WorkbenchToolRackKit") || ItemId == TEXT("ForgeAnvilKit") || ItemId == TEXT("GrindingStoneKit")
        || ItemId == TEXT("FloorKit") || ItemId == TEXT("WallKit") || ItemId == TEXT("RoofKit");
}

bool FKalmalaPlacementPreview::IsSessionOnlyKit(const FName ItemId)
{
    return ItemId == TEXT("WorkbenchToolRackKit") || ItemId == TEXT("ForgeAnvilKit")
        || ItemId == TEXT("DryingLineKit");
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
        || (FKalmalaRegionalGeneration::Sample(Config, Surface).bHasWater))
    {
        Result.Message = TEXT("Preview invalid: dry ground is required");
        return Result;
    }

    const bool bIsStationAttachment = ItemId == TEXT("WorkbenchToolRackKit") || ItemId == TEXT("ForgeAnvilKit");
    const bool bIsCompactConstruction = bIsStationAttachment || ItemId == TEXT("GrindingStoneKit")
        || ItemId == TEXT("DryingLineKit");
    const FVector PlacementExtent = bIsCompactConstruction
        ? AKalmalaConstructionActor::GetCollisionExtent(ItemId) : FVector(54, 54, 56);
    const float PlacementHalfHeight = PlacementExtent.Z + (bIsCompactConstruction ? 2.0f : 0.0f);
    Result.Location = Ground.ImpactPoint + FVector(0, 0, PlacementHalfHeight);
    const float PreviewRadius = bIsCompactConstruction
        ? FMath::Max3(PlacementExtent.X, PlacementExtent.Y, PlacementExtent.Z) : 54.0f;
    if (World->OverlapBlockingTestByChannel(Result.Location, FQuat::Identity, ECC_Pawn,
        FCollisionShape::MakeSphere(PreviewRadius), Query))
    {
        Result.Message = TEXT("Preview invalid: clear more space");
        return Result;
    }
    Result.bIsValid = true;
    Result.Message = TEXT("Preview valid locally: server will recheck before placement");
    return Result;
}
