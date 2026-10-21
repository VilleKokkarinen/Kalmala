#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaConstructionActor.h"
#include "KalmalaConstructionSaveGame.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlayerModelComponent.h"
#include "KalmalaPlacementPreview.h"
#include "KalmalaRecipeCatalogue.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaCampBedContractTest,
    "Kalmala.Gameplay.Construction.CampBedContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaCampBedContractTest::RunTest(const FString& Parameters)
{
    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
    const UKalmalaRecipeCatalogue* Recipes = UKalmalaRecipeCatalogue::Get();
    TestTrue(TEXT("Camp Bed supports the ordinary construction preview"),
        FKalmalaPlacementPreview::IsSupportedKit(TEXT("CampBedKit")));
    TestTrue(TEXT("Camp Bed occupies a 2 m by 1 m footprint"),
        AKalmalaConstructionActor::GetCollisionExtent(TEXT("CampBedKit")) == FVector(100.0f, 50.0f, 22.0f));
    TestNotNull(TEXT("Camp Bed has presentation-only buildable metadata"),
        Items ? Items->FindBuildable(TEXT("CampBedKit")) : nullptr);
    TestNull(TEXT("Camp Bed is not a portable inventory item"),
        Items ? Items->FindItem(TEXT("CampBed")) : nullptr);

    const FKalmalaRecipe* Recipe = Recipes ? Recipes->Find(TEXT("CampBed")) : nullptr;
    TestNotNull(TEXT("Camp Bed direct-material recipe loads"), Recipe);
    if (Recipe)
    {
        TestEqual(TEXT("Camp Bed recipe maps to the placed object identity"),
            Recipe->BuildableOutput, FName(TEXT("CampBedKit")));
        TestTrue(TEXT("Camp Bed recipe has no portable output"), Recipe->Output.IsNone());
        TArray<FKalmalaInventoryStack> Cost;
        FString Reason;
        TestTrue(TEXT("Camp Bed direct material cost resolves"),
            UKalmalaRecipeCatalogue::BuildDirectMaterialCost(TEXT("CampBedKit"), Cost, Reason));
        TestTrue(TEXT("Camp Bed costs exactly 4 Wood, 2 Boar Hide, and 2 Deer Hide"), Cost.Num() == 3
            && Cost.ContainsByPredicate([](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("Wood") && Stack.Quantity == 4; })
            && Cost.ContainsByPredicate([](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("BoarHide") && Stack.Quantity == 2; })
            && Cost.ContainsByPredicate([](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("DeerHide") && Stack.Quantity == 2; }));
    }

    FKalmalaConstructionSaveRecord Record;
    Record.ConstructionId = TEXT("camp-bed-contract");
    Record.KitId = TEXT("CampBedKit");
    Record.Transform = FTransform(FVector(300.0f, 200.0f, 22.0f));
    TestTrue(TEXT("Camp Bed uses the existing construction save record"),
        UKalmalaConstructionSaveGameV2::IsValidRecord(Record));
    TestFalse(TEXT("Missing world data cannot make a bed placement valid"),
        FKalmalaPlacementPreview::Evaluate(nullptr, nullptr, TEXT("CampBedKit")).bIsValid);
    TestTrue(TEXT("Stationary local pose continues while health is unchanged"),
        UKalmalaPlayerModelComponent::ShouldContinueCampBedRestPose(true, 0.0f, 100.0f, 100.0f));
    TestFalse(TEXT("Remote presentation does not start or keep a local pose"),
        UKalmalaPlayerModelComponent::ShouldContinueCampBedRestPose(false, 0.0f, 100.0f, 100.0f));
    TestFalse(TEXT("Movement ends the local pose"),
        UKalmalaPlayerModelComponent::ShouldContinueCampBedRestPose(true, 5.1f, 100.0f, 100.0f));
    TestFalse(TEXT("Damage ends the local pose"),
        UKalmalaPlayerModelComponent::ShouldContinueCampBedRestPose(true, 0.0f, 99.0f, 100.0f));
    return true;
}
#endif
