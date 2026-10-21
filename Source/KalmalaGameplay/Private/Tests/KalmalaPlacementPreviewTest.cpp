#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaPlacementPreview.h"
#include "KalmalaConstructionSaveGame.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaRecipeCatalogue.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPlacementPreviewTest, "Kalmala.Gameplay.Construction.LocalPreview",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPlacementPreviewTest::RunTest(const FString& Parameters)
{
    for (const FName Kit : {FName(TEXT("CampfireKit")), FName(TEXT("WorkbenchKit")), FName(TEXT("StorageKit")),
        FName(TEXT("CookingRackKit")), FName(TEXT("CauldronKit")), FName(TEXT("FryingPanKit")), FName(TEXT("CharcoalKilnKit")), FName(TEXT("SmelterKit")),
        FName(TEXT("FloorKit")), FName(TEXT("WallKit")), FName(TEXT("RoofKit")), FName(TEXT("GardenPlotKit"))})
        TestTrue(TEXT("Camp and construction kit supports a local preview"), FKalmalaPlacementPreview::IsSupportedKit(Kit));
    TestFalse(TEXT("Removed raised chest cannot be previewed"), FKalmalaPlacementPreview::IsSupportedKit(TEXT("RaisedStorageKit")));
    for (const FName NotAKit : {FName(TEXT("Wood")), FName(TEXT("ConstructionSupply")), FName()})
        TestFalse(TEXT("Materials and ingredients do not create a placement preview"), FKalmalaPlacementPreview::IsSupportedKit(NotAKit));
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
    FKalmalaWorldGenerationConfig World;
    World.WorldSeed = 418;
    UKalmalaConstructionSaveGameV2* CurrentSave = NewObject<UKalmalaConstructionSaveGameV2>(GetTransientPackage());
    CurrentSave->InitializeForWorld(World);
    TestTrue(TEXT("Schema 6 persists an empty Garden Plot construction"), CurrentSave->AddRecord(PlotRecord));
    return true;
}
#endif
