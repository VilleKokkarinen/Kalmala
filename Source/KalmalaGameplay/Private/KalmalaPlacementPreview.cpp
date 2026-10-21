#include "KalmalaPlacementPreview.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaBiomeClassifier.h"
#include "GameFramework/Pawn.h"

bool FKalmalaPlacementPreview::IsSupportedKit(const FName ItemId)
{
    if (ItemId == TEXT("ForgeKit")) return true;
    return ItemId == TEXT("CampfireKit") || ItemId == TEXT("WorkbenchKit") || ItemId == TEXT("StorageKit")
        || ItemId == TEXT("CharcoalKilnKit") || ItemId == TEXT("SmelterKit")
        || ItemId == TEXT("CookingRackKit") || ItemId == TEXT("CauldronKit") || ItemId == TEXT("FryingPanKit")
        || ItemId == TEXT("WorkbenchToolRackKit") || ItemId == TEXT("ForgeAnvilKit") || ItemId == TEXT("GrindingStoneKit")
        || ItemId == TEXT("FloorKit") || ItemId == TEXT("WallKit") || ItemId == TEXT("RoofKit")
        || ItemId == TEXT("GardenPlotKit") || ItemId == TEXT("TimberDoorKit");
}

bool FKalmalaPlacementPreview::IsGardenPlotGroundAllowed(const EKalmalaBiome Biome, const float SurfaceNormalZ)
{
    constexpr float MinimumUpComponentFor15Degrees = 0.9659258263f;
    const bool bAllowedBiome = Biome == EKalmalaBiome::Meadows || Biome == EKalmalaBiome::Elderwood
        || Biome == EKalmalaBiome::ShimmeringLakes;
    return bAllowedBiome && FMath::IsFinite(SurfaceNormalZ)
        && SurfaceNormalZ >= MinimumUpComponentFor15Degrees && SurfaceNormalZ <= 1.0f;
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
    if (ItemId == TEXT("TimberDoorKit"))
    {
        Result.Message = TEXT("Preview invalid: target an existing Windbreak Wall segment");
        const FVector ViewLocation = Pawn->GetPawnViewLocation();
        const FVector ViewDirection = Pawn->GetViewRotation().Vector();
        if (ViewLocation.ContainsNaN() || ViewDirection.ContainsNaN()) return Result;
        FCollisionQueryParams DoorQuery(SCENE_QUERY_STAT(TimberDoorPreview), false, Pawn);
        FHitResult WallHit;
        if (!World->LineTraceSingleByChannel(WallHit, ViewLocation,
                ViewLocation + ViewDirection * 250.0f, ECC_Visibility, DoorQuery)) return Result;
        AKalmalaConstructionActor* Wall = Cast<AKalmalaConstructionActor>(WallHit.GetActor());
        if (!IsValid(Wall) || Wall->GetWorld() != World || Wall->GetConstructionKit() != TEXT("WallKit")
            || Wall->GetConstructionId().IsEmpty()
            || FVector::DistSquared(Pawn->GetActorLocation(), Wall->GetActorLocation()) > FMath::Square(200.0f)) return Result;
        Result.Location = Wall->GetActorLocation();
        Result.ReplacementWall = Wall;
        Result.bIsValid = true;
        Result.Message = TEXT("Preview valid: replaces this Windbreak Wall segment; server will recheck before payment");
        return Result;
    }
    const bool bGardenPlot = ItemId == TEXT("GardenPlotKit");

    const FVector Forward = FRotator(0.0f, Pawn->GetActorRotation().Yaw, 0.0f).Vector();
    const FVector Probe = Pawn->GetActorLocation() + Forward * 165.0f;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(LocalConstructionPreview), false, Pawn);
    FHitResult Ground;
    Result.Message = TEXT("Preview invalid: face clear, dry, gently sloping ground");
    if (Probe.ContainsNaN() || !World->LineTraceSingleByChannel(Ground, Probe + FVector(0, 0, 100), Probe - FVector(0, 0, 350), ECC_Visibility, Query)
        || !Cast<AKalmalaGeneratedTerrainPatch>(Ground.GetActor())
        || (!bGardenPlot && Ground.ImpactNormal.Z < 0.85f)) return Result;

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

    if (bGardenPlot)
    {
        const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, Surface));
        if (!IsGardenPlotGroundAllowed(Biome, Ground.ImpactNormal.Z))
        {
            Result.Message = TEXT("Preview invalid: Garden Plot needs dry Meadows, Elderwood, or Shimmering Lakes ground within 15 degrees");
            return Result;
        }
        const FVector SkyStart = Ground.ImpactPoint + FVector(0, 0, 25.0f);
        FHitResult Overhead;
        if (World->LineTraceSingleByChannel(Overhead, SkyStart, SkyStart + FVector(0, 0, 100000.0f), ECC_Visibility, Query))
        {
            Result.Message = TEXT("Preview invalid: Garden Plot needs open sky");
            return Result;
        }
        Query.AddIgnoredActor(Ground.GetActor());
    }
    else if (ItemId == TEXT("WallKit"))
        Query.AddIgnoredActor(Ground.GetActor());

    const bool bIsStationAttachment = ItemId == TEXT("WorkbenchToolRackKit") || ItemId == TEXT("ForgeAnvilKit");
    const bool bIsKiln = ItemId == TEXT("CharcoalKilnKit");
    const bool bIsSmelter = ItemId == TEXT("SmelterKit");
    const bool bNeedsProcessingClearance = bIsKiln || bIsSmelter;
    const bool bIsWallSegment = ItemId == TEXT("WallKit");
    const bool bIsCompactConstruction = bGardenPlot || bIsStationAttachment || bNeedsProcessingClearance
        || ItemId == TEXT("GrindingStoneKit") || ItemId == TEXT("FryingPanKit") || bIsWallSegment;
    const FVector PlacementExtent = bIsCompactConstruction
        ? AKalmalaConstructionActor::GetCollisionExtent(ItemId) : FVector(54, 54, 56);
    const float PlacementHalfHeight = PlacementExtent.Z
        + (bIsCompactConstruction && !bGardenPlot && !bIsWallSegment ? 2.0f : 0.0f);
    Result.Location = Ground.ImpactPoint + FVector(0, 0, PlacementHalfHeight);
    const float PreviewRadius = bIsCompactConstruction
        ? FMath::Max3(PlacementExtent.X, PlacementExtent.Y, PlacementExtent.Z) : 54.0f;
    const FCollisionShape PlacementShape = (bNeedsProcessingClearance || bGardenPlot || bIsWallSegment)
        ? FCollisionShape::MakeBox(PlacementExtent) : FCollisionShape::MakeSphere(PreviewRadius);
    if (World->OverlapBlockingTestByChannel(Result.Location, FQuat::Identity, ECC_Pawn, PlacementShape, Query))
    {
        Result.Message = TEXT("Preview invalid: clear more space");
        return Result;
    }
    if (bNeedsProcessingClearance)
    {
        const FVector OverheadCentre = Result.Location + FVector(0, 0, PlacementExtent.Z + 100.0f);
        const FCollisionShape OverheadClearance = FCollisionShape::MakeBox(
            FVector(PlacementExtent.X, PlacementExtent.Y, 100.0f));
        if (World->OverlapBlockingTestByChannel(OverheadCentre, FQuat::Identity, ECC_Pawn, OverheadClearance, Query))
        {
            Result.Message = bIsKiln ? TEXT("Preview invalid: Charcoal Kiln needs 2 m of overhead clearance")
                : TEXT("Preview invalid: Smelter needs 2 m of overhead clearance");
            return Result;
        }
        const FVector ApproachCentre = Result.Location - Forward * (PlacementExtent.X + 50.0f)
            + FVector(0, 0, 50.0f - PlacementExtent.Z);
        const FCollisionShape ApproachClearance = FCollisionShape::MakeBox(FVector(50.0f, 40.0f, 50.0f));
        const FQuat ApproachRotation = FRotator(0.0f, Pawn->GetActorRotation().Yaw, 0.0f).Quaternion();
        if (World->OverlapBlockingTestByChannel(ApproachCentre, ApproachRotation, ECC_Pawn,
            ApproachClearance, Query))
        {
            Result.Message = bIsKiln ? TEXT("Preview invalid: keep a clear 1 m approach to the Charcoal Kiln")
                : TEXT("Preview invalid: keep a clear 1 m approach to the Smelter");
            return Result;
        }
    }
    Result.bIsValid = true;
    Result.Message = TEXT("Preview valid locally: server will recheck before placement");
    return Result;
}
