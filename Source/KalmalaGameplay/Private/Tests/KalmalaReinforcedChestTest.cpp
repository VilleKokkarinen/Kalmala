#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaConstructionActor.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlacementPreview.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaStorageSaveGame.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaReinforcedChestContractTest,
    "Kalmala.Gameplay.Construction.ReinforcedChestContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaReinforcedChestContractTest::RunTest(const FString& Parameters)
{
    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
    const UKalmalaRecipeCatalogue* Recipes = UKalmalaRecipeCatalogue::Get();
    const FName ChestKit(TEXT("ReinforcedChestKit"));
    const FKalmalaItemDefinition* Chest = Items ? Items->FindBuildable(ChestKit) : nullptr;
    TestNotNull(TEXT("Reinforced Chest is registered as a placed buildable"), Chest);
    TestNull(TEXT("Reinforced Chest is not a portable inventory item"),
        Items ? Items->FindItem(ChestKit) : nullptr);
    TestNull(TEXT("Catalogue name does not create a portable chest item"),
        Items ? Items->FindItem(TEXT("ReinforcedChest")) : nullptr);

    const FKalmalaRecipe* Recipe = Recipes ? Recipes->Find(TEXT("ReinforcedChest")) : nullptr;
    TestNotNull(TEXT("Reinforced Chest has a registered Building Hammer recipe"), Recipe);
    if (Recipe)
    {
        TestEqual(TEXT("Recipe directly places the Reinforced Chest"), Recipe->BuildableOutput, ChestKit);
        TestTrue(TEXT("Recipe has no portable output or crafting station"), Recipe->Output.IsNone()
            && Recipe->RequiredStation.IsEmpty());
        TestEqual(TEXT("Recipe is bounded to one chest"), Recipe->MaxBatch, 1);
    }

    TArray<FKalmalaInventoryStack> Costs;
    FString Reason;
    TestTrue(TEXT("Direct material cost resolves from the authored build recipe"),
        UKalmalaRecipeCatalogue::BuildDirectMaterialCost(ChestKit, Costs, Reason));
    TestTrue(TEXT("Cost is exactly eight Wood and two Bronze"), Costs.Num() == 2
        && Costs.ContainsByPredicate([](const FKalmalaInventoryStack& Stack)
            { return Stack.ItemId == TEXT("Wood") && Stack.Quantity == 8; })
        && Costs.ContainsByPredicate([](const FKalmalaInventoryStack& Stack)
            { return Stack.ItemId == TEXT("Bronze") && Stack.Quantity == 2; }));

    TestTrue(TEXT("Reinforced Chest supports the server-rechecked build preview"),
        FKalmalaPlacementPreview::IsSupportedKit(ChestKit));
    TestFalse(TEXT("Preview requires a world and supported ground"),
        FKalmalaPlacementPreview::Evaluate(nullptr, nullptr, ChestKit).bIsValid);
    TestEqual(TEXT("Reinforced Chest occupies a 1.5 by 1 metre footprint and 0.9 metre height"),
        AKalmalaConstructionActor::GetCollisionExtent(ChestKit), FVector(75.0f, 50.0f, 45.0f));
    TestTrue(TEXT("Both chest forms share validated storage behavior"),
        AKalmalaConstructionActor::IsStorageKit(TEXT("StorageKit"))
            && AKalmalaConstructionActor::IsStorageKit(ChestKit));
    TestFalse(TEXT("A chest is not a structural support endpoint"),
        AKalmalaConstructionActor::IsStructuralEndpointKit(ChestKit));
    TestEqual(TEXT("Normal Chest capacity remains sixteen"),
        UKalmalaStorageSaveGame::GetCapacityForKit(TEXT("StorageKit")), 16);
    TestEqual(TEXT("Reinforced Chest capacity is twenty-four"),
        UKalmalaStorageSaveGame::GetCapacityForKit(ChestKit), 24);
    return true;
}
#endif
