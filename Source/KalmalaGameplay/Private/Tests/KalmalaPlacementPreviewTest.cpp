#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaPlacementPreview.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaConstructionSaveGame.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaRecipeCatalogue.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPlacementPreviewTest, "Kalmala.Gameplay.Construction.LocalPreview",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPlacementPreviewTest::RunTest(const FString& Parameters)
{
    for (const FName Kit : {FName(TEXT("CampfireKit")), FName(TEXT("WorkbenchKit")), FName(TEXT("StorageKit")),
        FName(TEXT("ReinforcedChestKit")),
        FName(TEXT("CookingRackKit")), FName(TEXT("CauldronKit")), FName(TEXT("FryingPanKit")), FName(TEXT("CharcoalKilnKit")), FName(TEXT("SmelterKit")),
        FName(TEXT("FloorKit")), FName(TEXT("WallKit")), FName(TEXT("RoofKit")), FName(TEXT("GardenPlotKit")),
        FName(TEXT("TimberBeamKit")), FName(TEXT("TimberStairsKit"))})
        TestTrue(TEXT("Camp and construction kit supports a local preview"), FKalmalaPlacementPreview::IsSupportedKit(Kit));
    TestFalse(TEXT("Removed raised chest cannot be previewed"), FKalmalaPlacementPreview::IsSupportedKit(TEXT("RaisedStorageKit")));
    for (const FName NotAKit : {FName(TEXT("Wood")), FName(TEXT("ConstructionSupply")), FName()})
        TestFalse(TEXT("Materials and ingredients do not create a placement preview"), FKalmalaPlacementPreview::IsSupportedKit(NotAKit));
    TestTrue(TEXT("Timber Beam joins the structural endpoint set"),
        AKalmalaConstructionActor::IsStructuralEndpointKit(TEXT("TimberBeamKit")));
    TestTrue(TEXT("Existing walls remain structural endpoints"),
        AKalmalaConstructionActor::IsStructuralEndpointKit(TEXT("WallKit")));
    TestFalse(TEXT("Furniture does not provide structural endpoints"),
        AKalmalaConstructionActor::IsStructuralEndpointKit(TEXT("StorageKit")));
    TestTrue(TEXT("Timber Stairs provide supported endpoints"),
        AKalmalaConstructionActor::IsStructuralEndpointKit(TEXT("TimberStairsKit")));
    TestTrue(TEXT("Timber Stairs occupy a 2 m by 1 m by 1 m envelope"),
        AKalmalaConstructionActor::GetCollisionExtent(TEXT("TimberStairsKit")) == FVector(100.0f, 50.0f, 50.0f));
    TestTrue(TEXT("Timber Beam collision is 2 m long and 0.15 m square"),
        AKalmalaConstructionActor::GetCollisionExtent(TEXT("TimberBeamKit")) == FVector(100.0f, 7.5f, 7.5f));
    const FRotator HorizontalBeamRotation = FKalmalaPlacementPreview::GetTimberBeamPlacementRotation(FRotator(20.0f, 42.0f, 0.0f));
    TestTrue(TEXT("Level view snaps a horizontal beam to a cardinal axis"),
        HorizontalBeamRotation.Pitch == 0.0f && HorizontalBeamRotation.Yaw == 0.0f);
    const FRotator VerticalBeamRotation = FKalmalaPlacementPreview::GetTimberBeamPlacementRotation(FRotator(-75.0f, 100.0f, 0.0f));
    TestTrue(TEXT("Steep view snaps a vertical beam to a cardinal axis"),
        VerticalBeamRotation.Pitch == 90.0f && VerticalBeamRotation.Yaw == 90.0f);
    const FRotator StairRotation = FKalmalaPlacementPreview::GetTimberStairsPlacementRotation(FRotator(0.0f, 132.0f, 0.0f));
    TestTrue(TEXT("Timber Stairs snap to the nearest cardinal direction"),
        StairRotation.Pitch == 0.0f && StairRotation.Yaw == 90.0f);
    const FKalmalaPlacementPreview NullPreview = FKalmalaPlacementPreview::Evaluate(nullptr, nullptr, TEXT("CampfireKit"));
    TestFalse(TEXT("Missing local presentation context cannot claim validity"), NullPreview.bIsValid);
    TestTrue(TEXT("Missing local context reports a textual reason"), NullPreview.Message.Contains(TEXT("waiting"), ESearchCase::IgnoreCase));
    const FKalmalaPlacementPreview Unsupported = FKalmalaPlacementPreview::Evaluate(nullptr, nullptr, TEXT("Wood"));
    TestFalse(TEXT("Unsupported material cannot claim validity"), Unsupported.bIsValid);
    TestTrue(TEXT("Unsupported material reports a textual reason"), Unsupported.Message.Contains(TEXT("select"), ESearchCase::IgnoreCase));

    TestTrue(TEXT("Garden Plot accepts level Meadows ground"),
        FKalmalaPlacementPreview::IsGardenPlotGroundAllowed(EKalmalaBiome::Meadows, 1.0f));
    TestTrue(TEXT("Garden Plot accepts level Elderwood ground"),
        FKalmalaPlacementPreview::IsGardenPlotGroundAllowed(EKalmalaBiome::Elderwood, 1.0f));
    TestTrue(TEXT("Garden Plot accepts level Shimmering Lakes ground"),
        FKalmalaPlacementPreview::IsGardenPlotGroundAllowed(EKalmalaBiome::ShimmeringLakes, 1.0f));
    TestFalse(TEXT("Garden Plot rejects other biomes"),
        FKalmalaPlacementPreview::IsGardenPlotGroundAllowed(EKalmalaBiome::ThunderMountains, 1.0f));
    TestFalse(TEXT("Garden Plot rejects slopes steeper than 15 degrees"),
        FKalmalaPlacementPreview::IsGardenPlotGroundAllowed(EKalmalaBiome::Meadows, 0.95f));
    TestFalse(TEXT("Garden Plot rejects invalid surface normals"),
        FKalmalaPlacementPreview::IsGardenPlotGroundAllowed(EKalmalaBiome::Meadows, NAN));

    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
    const UKalmalaRecipeCatalogue* Recipes = UKalmalaRecipeCatalogue::Get();
    TestNotNull(TEXT("Garden Plot is presentation-only buildable metadata"),
        Items ? Items->FindBuildable(TEXT("GardenPlotKit")) : nullptr);
    TestNull(TEXT("Garden Plot is not a portable inventory item"),
        Items ? Items->FindItem(TEXT("GardenPlotKit")) : nullptr);
    const FKalmalaRecipe* GardenRecipe = Recipes ? Recipes->Find(TEXT("GardenPlot")) : nullptr;
    TestNotNull(TEXT("Garden Plot direct-material recipe loads"), GardenRecipe);
    if (GardenRecipe)
    {
        TestEqual(TEXT("Garden Plot recipe maps to the placed object identity"),
            GardenRecipe->BuildableOutput, FName(TEXT("GardenPlotKit")));
        TestTrue(TEXT("Garden Plot does not create a portable output item"), GardenRecipe->Output.IsNone());
        TArray<FKalmalaInventoryStack> Cost;
        FString Reason;
        TestTrue(TEXT("Garden Plot direct-material costs resolve"),
            UKalmalaRecipeCatalogue::BuildDirectMaterialCost(TEXT("GardenPlotKit"), Cost, Reason));
        TestEqual(TEXT("Garden Plot has exactly two direct material costs"), Cost.Num(), 2);
        TestTrue(TEXT("Garden Plot costs 4 Wood and 1 Stone directly"), Cost.Num() == 2
            && Cost.ContainsByPredicate([](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("Wood") && Stack.Quantity == 4; })
            && Cost.ContainsByPredicate([](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("Stone") && Stack.Quantity == 1; }));
    }
    FKalmalaConstructionSaveRecord PlotRecord;
    PlotRecord.ConstructionId = TEXT("garden-plot-01");
    PlotRecord.KitId = TEXT("GardenPlotKit");
    PlotRecord.Transform = FTransform(FVector(100, 200, 300));
    TestTrue(TEXT("Garden Plot uses the existing persistent construction record"),
        UKalmalaConstructionSaveGame::IsValidRecord(PlotRecord));
    TestNotNull(TEXT("Timber Beam has presentation-only buildable metadata"),
        Items ? Items->FindBuildable(TEXT("TimberBeamKit")) : nullptr);
    TestNull(TEXT("Timber Beam is not a portable inventory item"),
        Items ? Items->FindItem(TEXT("TimberBeam")) : nullptr);
    const FKalmalaRecipe* BeamRecipe = Recipes ? Recipes->Find(TEXT("TimberBeam")) : nullptr;
    TestNotNull(TEXT("Timber Beam direct-material recipe loads"), BeamRecipe);
    if (BeamRecipe)
    {
        TestEqual(TEXT("Timber Beam recipe maps to the placed object identity"),
            BeamRecipe->BuildableOutput, FName(TEXT("TimberBeamKit")));
        TestTrue(TEXT("Timber Beam recipe does not create a portable output item"), BeamRecipe->Output.IsNone());
        TArray<FKalmalaInventoryStack> Cost;
        FString Reason;
        TestTrue(TEXT("Timber Beam direct-material cost resolves"),
            UKalmalaRecipeCatalogue::BuildDirectMaterialCost(TEXT("TimberBeamKit"), Cost, Reason));
        TestTrue(TEXT("Timber Beam costs exactly 2 Wood"), Cost.Num() == 1
            && Cost[0].ItemId == TEXT("Wood") && Cost[0].Quantity == 2);
    }
    TestNotNull(TEXT("Timber Stairs have presentation-only buildable metadata"),
        Items ? Items->FindBuildable(TEXT("TimberStairsKit")) : nullptr);
    TestNull(TEXT("Timber Stairs are not a portable inventory item"),
        Items ? Items->FindItem(TEXT("TimberStairs")) : nullptr);
    const FKalmalaRecipe* StairsRecipe = Recipes ? Recipes->Find(TEXT("TimberStairs")) : nullptr;
    TestNotNull(TEXT("Timber Stairs direct-material recipe loads"), StairsRecipe);
    if (StairsRecipe)
    {
        TestEqual(TEXT("Timber Stairs recipe maps to the placed object identity"),
            StairsRecipe->BuildableOutput, FName(TEXT("TimberStairsKit")));
        TestTrue(TEXT("Timber Stairs recipe does not create a portable output item"), StairsRecipe->Output.IsNone());
        TArray<FKalmalaInventoryStack> Cost;
        FString Reason;
        TestTrue(TEXT("Timber Stairs direct-material cost resolves"),
            UKalmalaRecipeCatalogue::BuildDirectMaterialCost(TEXT("TimberStairsKit"), Cost, Reason));
        TestTrue(TEXT("Timber Stairs cost exactly 6 Wood"), Cost.Num() == 1
            && Cost[0].ItemId == TEXT("Wood") && Cost[0].Quantity == 6);
    }
    FKalmalaConstructionSaveRecord StairsRecord;
    StairsRecord.ConstructionId = TEXT("timber-stairs-01");
    StairsRecord.KitId = TEXT("TimberStairsKit");
    StairsRecord.Transform = FTransform(FRotator::ZeroRotator, FVector(900.0f, 100.0f, 50.0f));
    TestTrue(TEXT("Timber Stairs use the current construction save format"),
        UKalmalaConstructionSaveGameV2::IsValidRecord(StairsRecord));
    FKalmalaConstructionSaveRecord BeamRecord;
    BeamRecord.ConstructionId = TEXT("timber-beam-01");
    BeamRecord.KitId = TEXT("TimberBeamKit");
    BeamRecord.Transform = FTransform(FRotator(90.0f, 90.0f, 0.0f), FVector(300.0f, 400.0f, 500.0f));
    FKalmalaConstructionSaveRecord HorizontalBeamRecord = BeamRecord;
    HorizontalBeamRecord.ConstructionId = TEXT("timber-beam-horizontal-01");
    HorizontalBeamRecord.Transform = FTransform(FRotator(0.0f, 0.0f, 0.0f), FVector(600.0f, 700.0f, 800.0f));
    TestTrue(TEXT("Timber Beam uses the current persistent construction format"),
        UKalmalaConstructionSaveGameV2::IsValidRecord(BeamRecord));
    TestTrue(TEXT("Horizontal Timber Beam uses the same persistent construction format"),
        UKalmalaConstructionSaveGameV2::IsValidRecord(HorizontalBeamRecord));
    FKalmalaWorldGenerationConfig World;
    World.WorldSeed = 418;
    UKalmalaConstructionSaveGameV2* CurrentSave = NewObject<UKalmalaConstructionSaveGameV2>(GetTransientPackage());
    CurrentSave->InitializeForWorld(World);
    TestTrue(TEXT("Schema 6 persists an empty Garden Plot construction"), CurrentSave->AddRecord(PlotRecord));
    TestTrue(TEXT("Schema 6 persists a vertical beam without a new state field"), CurrentSave->AddRecord(BeamRecord));
    TestTrue(TEXT("Schema 6 persists a horizontal beam without a new state field"), CurrentSave->AddRecord(HorizontalBeamRecord));
    UWorld* EndpointWorld = UWorld::CreateWorld(EWorldType::Game, false);
    if (TestNotNull(TEXT("Structural endpoint test world is available"), EndpointWorld))
    {
        AKalmalaConstructionActor* VerticalBeam = EndpointWorld->SpawnActor<AKalmalaConstructionActor>();
        if (TestNotNull(TEXT("Vertical Timber Beam construction spawns"), VerticalBeam))
        {
            VerticalBeam->InitializeFromServer(TEXT("TimberBeamKit"), TEXT("vertical-beam-endpoints"));
            VerticalBeam->SetActorTransform(BeamRecord.Transform);
            TArray<FVector> Endpoints;
            AKalmalaConstructionActor::GetStructuralEndpoints(VerticalBeam, Endpoints);
            TestEqual(TEXT("A Timber Beam exposes two structural endpoints"), Endpoints.Num(), 2);
            TestTrue(TEXT("Vertical beam endpoints follow its saved rotation"), Endpoints.Num() == 2
                && FMath::IsNearlyEqual(FMath::Min(Endpoints[0].Z, Endpoints[1].Z), 400.0f)
                && FMath::IsNearlyEqual(FMath::Max(Endpoints[0].Z, Endpoints[1].Z), 600.0f));
        }
        AKalmalaConstructionActor* Stairs = EndpointWorld->SpawnActor<AKalmalaConstructionActor>();
        if (TestNotNull(TEXT("Timber Stairs construction spawns"), Stairs))
        {
            Stairs->InitializeFromServer(TEXT("TimberStairsKit"), TEXT("timber-stairs-endpoints"));
            Stairs->SetActorTransform(StairsRecord.Transform);
            TArray<FVector> Endpoints;
            AKalmalaConstructionActor::GetStructuralEndpoints(Stairs, Endpoints);
            TestEqual(TEXT("Timber Stairs expose lower and upper support endpoints"), Endpoints.Num(), 2);
            TestTrue(TEXT("Timber Stairs endpoints span 2 m and 1 m"), Endpoints.Num() == 2
                && FMath::IsNearlyEqual(FVector::Dist(Endpoints[0], Endpoints[1]), 223.6068f, 0.1f)
                && FMath::IsNearlyEqual(FMath::Min(Endpoints[0].Z, Endpoints[1].Z), StairsRecord.Transform.GetLocation().Z - 50.0f)
                && FMath::IsNearlyEqual(FMath::Max(Endpoints[0].Z, Endpoints[1].Z), StairsRecord.Transform.GetLocation().Z + 50.0f));

            TArray<UBoxComponent*> BoxComponents;
            Stairs->GetComponents<UBoxComponent>(BoxComponents);
            int32 ActiveStepColliders = 0;
            for (const UBoxComponent* Box : BoxComponents)
            {
                if (Box && Box->GetFName().ToString().StartsWith(TEXT("StairsStepCollision")))
                {
                    ++ActiveStepColliders;
                    TestTrue(TEXT("Timber stair treads block walking"),
                        Box->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics
                        && Box->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);
                }
            }
            TestEqual(TEXT("Timber Stairs have five solid walkable treads"), ActiveStepColliders, 5);
        }
        EndpointWorld->DestroyWorld(false);
    }
    return true;
}
#endif
