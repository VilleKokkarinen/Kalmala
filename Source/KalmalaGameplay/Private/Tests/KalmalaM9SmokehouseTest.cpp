#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaConstructionActor.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlacementPreview.h"
#include "KalmalaRecipeCatalogue.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaM9SmokehouseTest, "Kalmala.Gameplay.M9.Smokehouse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaM9SmokehouseTest::RunTest(const FString& Parameters)
{
    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
    const UKalmalaRecipeCatalogue* Recipes = UKalmalaRecipeCatalogue::Get();
    TestTrue(TEXT("Updated catalogues validate"), Items->IsValidCatalogue() && Recipes->IsValidCatalogue());
    TestNull(TEXT("Smokehouse is removed from the item catalogue"), Items->FindItem(TEXT("Smokehouse")));
    TestNull(TEXT("Smokehouse construction recipe is removed"), Recipes->Find(TEXT("Smokehouse")));
    TestFalse(TEXT("Smokehouse construction cannot be placed"), FKalmalaPlacementPreview::IsSupportedKit(TEXT("SmokehouseKit")));
    TestFalse(TEXT("Smokehouse cannot satisfy a crafting recipe station"), AKalmalaConstructionActor::IsCraftingStationKit(TEXT("SmokehouseKit")));
    const FKalmalaRecipe* Smoking = Recipes->Find(TEXT("SmokeBoarMeat"));
    if (TestNotNull(TEXT("Open smoke-frame recipe remains available"), Smoking))
    {
        TestTrue(TEXT("Smoking uses the Smoke Frame only"), Smoking->RequiredStation.Contains(TEXT("SmokeFrameKit"))
            && Smoking->RequiredStation.Num() == 1);
        TestEqual(TEXT("Smoking charges raw fuel instead of a Fuel item"), Smoking->FuelPerServing, 1);
    }
    TestNull(TEXT("Raised chest variant is removed from the item catalogue"), Items->FindItem(TEXT("RaisedStorage")));
    TestNull(TEXT("Raised chest recipe is removed"), Recipes->Find(TEXT("RaisedStorage")));
    const FKalmalaItemDefinition* Chest = Items->FindItem(TEXT("StorageKit"));
    if (TestNotNull(TEXT("The normal chest remains available"), Chest))
        TestEqual(TEXT("Normal storage is presented as Chest"), Chest->DisplayName, FString(TEXT("Chest")));
    const FKalmalaRecipe* ChestRecipe = Recipes->Find(TEXT("Storage"));
    if (TestNotNull(TEXT("The normal chest uses its one remaining recipe"), ChestRecipe))
        TestEqual(TEXT("The normal chest recipe produces Storage"), ChestRecipe->Output, FName(TEXT("StorageKit")));
    return true;
}
#endif
