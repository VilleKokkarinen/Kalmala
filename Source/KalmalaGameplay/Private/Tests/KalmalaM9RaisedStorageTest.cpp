#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaConstructionActor.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlacementPreview.h"
#include "KalmalaRecipeCatalogue.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaM9RaisedStorageTest, "Kalmala.Gameplay.M9.RaisedStorage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaM9RaisedStorageTest::RunTest(const FString& Parameters)
{
    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
    const UKalmalaRecipeCatalogue* Recipes = UKalmalaRecipeCatalogue::Get();
    TestTrue(TEXT("Updated catalogues validate"), Items->IsValidCatalogue() && Recipes->IsValidCatalogue());
    TestNull(TEXT("RaisedStorage has no item definition"), Items->FindItem(TEXT("RaisedStorage")));
    TestNull(TEXT("RaisedStorage has no recipe"), Recipes->Find(TEXT("RaisedStorage")));
    TestFalse(TEXT("Raised chest construction is no longer supported"), FKalmalaPlacementPreview::IsSupportedKit(TEXT("RaisedStorageKit")));
    TestFalse(TEXT("Only the normal chest is a storage construction"), AKalmalaConstructionActor::IsStorageKit(TEXT("RaisedStorageKit")));
    TestTrue(TEXT("Normal chest remains the supported storage construction"), AKalmalaConstructionActor::IsStorageKit(TEXT("StorageKit")));
    const FKalmalaItemDefinition* Chest = Items->FindItem(TEXT("StorageKit"));
    const FKalmalaRecipe* ChestRecipe = Recipes->Find(TEXT("Storage"));
    if (TestNotNull(TEXT("Normal chest is the only storage item"), Chest))
        TestEqual(TEXT("Normal chest holds the existing shared storage"), Chest->DisplayName, FString(TEXT("Chest")));
    if (TestNotNull(TEXT("Normal chest is built from raw materials"), ChestRecipe))
    {
        TestEqual(TEXT("Chest output is the normal storage construction"), ChestRecipe->Output, FName(TEXT("StorageKit")));
        TestFalse(TEXT("Chest recipe uses no Densewood tier gate"), ChestRecipe->Ingredients.ContainsByPredicate(
            [](const FKalmalaInventoryStack& Cost) { return Cost.ItemId == TEXT("Densewood"); }));
    }
    return true;
}
#endif
