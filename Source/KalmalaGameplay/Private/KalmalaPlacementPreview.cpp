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
        || ItemId == TEXT("CharcoalKilnKit")
        || ItemId == TEXT("CookingRackKit") || ItemId == TEXT("CauldronKit") || ItemId == TEXT("FryingPanKit")
        || ItemId == TEXT("WorkbenchToolRackKit") || ItemId == TEXT("ForgeAnvilKit") || ItemId == TEXT("GrindingStoneKit")
        || ItemId == TEXT("FloorKit") || ItemId == TEXT("WallKit") || ItemId == TEXT("RoofKit");
}

bool FKalmalaPlacementPreview::IsSessionOnlyKit(const FName ItemId)
{
    return ItemId == TEXT("WorkbenchToolRackKit") || ItemId == TEXT("ForgeAnvilKit");
}

FKalmalaPlacementPreview FKalmalaPlacementPreview::Evaluate(const UWorld* World, const APawn* Pawn, const FName ItemId)
{
    FKalmalaPlacementPreview Result;
    Result.Message = TEXT("Preview: select a camp or construction object");
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
    const bool bIsKiln = ItemId == TEXT("CharcoalKilnKit");
    const bool bIsCompactConstruction = bIsStationAttachment || bIsKiln
        || ItemId == TEXT("GrindingStoneKit") || ItemId == TEXT("FryingPanKit");
    const FVector PlacementExtent = bIsCompactConstruction
        ? AKalmalaConstructionActor::GetCollisionExtent(ItemId) : FVector(54, 54, 56);
    const float PlacementHalfHeight = PlacementExtent.Z + (bIsCompactConstruction ? 2.0f : 0.0f);
    Result.Location = Ground.ImpactPoint + FVector(0, 0, PlacementHalfHeight);
    const float PreviewRadius = bIsCompactConstruction
        ? FMath::Max3(PlacementExtent.X, PlacementExtent.Y, PlacementExtent.Z) : 54.0f;
    const FCollisionShape PlacementShape = bIsKiln
        ? FCollisionShape::MakeBox(PlacementExtent) : FCollisionShape::MakeSphere(PreviewRadius);
    if (World->OverlapBlockingTestByChannel(Result.Location, FQuat::Identity, ECC_Pawn, PlacementShape, Query))
    {
        Result.Message = TEXT("Preview invalid: clear more space");
        return Result;
    }
    if (bIsKiln)
    {
        const FVector OverheadCentre = Result.Location + FVector(0, 0, PlacementExtent.Z + 100.0f);
        const FCollisionShape OverheadClearance = FCollisionShape::MakeBox(
            FVector(PlacementExtent.X, PlacementExtent.Y, 100.0f));
        if (World->OverlapBlockingTestByChannel(OverheadCentre, FQuat::Identity, ECC_Pawn, OverheadClearance, Query))
        {
            Result.Message = TEXT("Preview invalid: Charcoal Kiln needs 2 m of overhead clearance");
            return Result;
        }
        const FVector ApproachCentre = Result.Location - Forward * (PlacementExtent.X + 50.0f)
            + FVector(0, 0, 50.0f - PlacementExtent.Z);
        const FCollisionShape ApproachClearance = FCollisionShape::MakeBox(FVector(50.0f, 40.0f, 50.0f));
        const FQuat ApproachRotation = FRotator(0.0f, Pawn->GetActorRotation().Yaw, 0.0f).Quaternion();
        if (World->OverlapBlockingTestByChannel(ApproachCentre, ApproachRotation, ECC_Pawn,
            ApproachClearance, Query))
        {
            Result.Message = TEXT("Preview invalid: keep a clear 1 m approach to the Charcoal Kiln");
            return Result;
        }
    }
    Result.bIsValid = true;
    Result.Message = TEXT("Preview valid locally: server will recheck before placement");
    return Result;
}
