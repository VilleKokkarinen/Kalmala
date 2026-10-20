#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaStorageSaveGame.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaDirectBuildTest, "Kalmala.Gameplay.Construction.NoPortableKits",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaDirectBuildTest::RunTest(const FString& Parameters)
{
    const auto* Items = UKalmalaItemCatalogue::Get();
    const auto* Recipes = UKalmalaRecipeCatalogue::Get();
    if (!TestTrue(TEXT("Separated item/buildable and recipe catalogues validate"),
        Items->IsValidCatalogue() && Recipes->IsValidCatalogue())) return false;
    TestFalse(TEXT("Placed-object metadata is retained separately"), Items->Buildables.IsEmpty());
    for (const FKalmalaItemDefinition& Item : Items->Items)
        TestFalse(TEXT("Inventory definitions cannot be construction kits"),
            Item.ItemId.ToString().EndsWith(TEXT("Kit")) || UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(Item.ItemId));

    int32 BuildCount = 0;
    for (const FKalmalaRecipe& Recipe : Recipes->Recipes)
    {
        if (Recipe.BuildableOutput.IsNone())
        {
            TestFalse(TEXT("No production recipe outputs a kit"), Recipe.Output.ToString().EndsWith(TEXT("Kit")));
            continue;
        }
        ++BuildCount;
        const FName Id = Recipe.BuildableOutput;
        TestTrue(*FString::Printf(TEXT("%s is a station-free direct material build"), *Recipe.RecipeId.ToString()),
            Recipe.Output.IsNone() && Recipe.RequiredStation.IsEmpty()
            && UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(Id));
        TestNull(TEXT("Construction object has no inventory definition"), Items->FindItem(Id));
        TestFalse(TEXT("Construction object cannot form a stack"), Items->IsValidStack(Id, 1));
        TestFalse(TEXT("Construction object cannot be added to a stack"), Items->CanAddToStack(Id, 0, 1));
        TArray<FKalmalaInventoryStack> Candidate;
        FString Reason;
        TestFalse(TEXT("Server grant builder refuses construction objects even in an empty pack"),
            UKalmalaInventoryComponent::BuildGrant({}, Id, 1, Candidate, Reason));
        FKalmalaInventoryStack SavedStack;
        SavedStack.ItemId = Id;
        SavedStack.Quantity = 1;
        SavedStack.InstanceGuid = FGuid::NewGuid();
        SavedStack.ItemGuids.Add(FGuid::NewGuid());
        TestFalse(TEXT("Storage/save validation rejects portable construction objects"),
            UKalmalaStorageSaveGame::IsValidStacks({SavedStack}));

        TArray<FKalmalaInventoryStack> Costs;
        TestTrue(TEXT("Every placement resolves an authored raw-material cost"),
            UKalmalaRecipeCatalogue::BuildDirectMaterialCost(Id, Costs, Reason));
        TestEqual(TEXT("Direct cost retains the authored ingredient count"), Costs.Num(), Recipe.Ingredients.Num());
        for (int32 Index = 0; Index < Costs.Num() && Recipe.Ingredients.IsValidIndex(Index); ++Index)
        {
            TestTrue(TEXT("Direct cost retains each authored material and quantity"),
                Costs[Index].ItemId == Recipe.Ingredients[Index].ItemId
                && Costs[Index].Quantity == Recipe.Ingredients[Index].Quantity);
            TestTrue(TEXT("Placement costs are ordinary inventory materials"), Items->IsValidStack(Costs[Index].ItemId, Costs[Index].Quantity));
        }
        int32 OutputCount = 0;
        TestFalse(TEXT("Direct builds cannot use ordinary production"),
            UKalmalaRecipeCatalogue::Scale(Recipe, 1, Candidate, OutputCount));
        FKalmalaRecipe ForgedProduction = Recipe;
        ForgedProduction.BuildableOutput = NAME_None;
        ForgedProduction.Output = Id;
        TestFalse(TEXT("Forging a portable-kit recipe does not bypass production validation"),
            UKalmalaRecipeCatalogue::Scale(ForgedProduction, 1, Candidate, OutputCount));
    }
    TestEqual(TEXT("All fifteen current placed objects use material recipes"), BuildCount, 15);
    const FKalmalaRecipe* Kiln = Recipes->Find(TEXT("CharcoalKiln"));
    TestNotNull(TEXT("Charcoal Kiln direct-build recipe is registered"), Kiln);
    if (Kiln)
    {
        TestEqual(TEXT("Charcoal Kiln recipe resolves to the stable construction identity"),
            Kiln->BuildableOutput, FName(TEXT("CharcoalKilnKit")));
        TestEqual(TEXT("Charcoal Kiln placement consumes only Stone"), Kiln->Ingredients.Num(), 1);
        if (Kiln->Ingredients.Num() == 1)
        {
            TestEqual(TEXT("Charcoal Kiln costs 20 Stone"), Kiln->Ingredients[0].ItemId, FName(TEXT("Stone")));
            TestEqual(TEXT("Charcoal Kiln Stone quantity"), Kiln->Ingredients[0].Quantity, 20);
        }
        TestNotNull(TEXT("Charcoal Kiln metadata is presentation-only"), Items->FindBuildable(TEXT("CharcoalKilnKit")));
        TestNull(TEXT("Charcoal Kiln is not an inventory item"), Items->FindItem(TEXT("CharcoalKilnKit")));
    }
    const FKalmalaRecipe* Smelter = Recipes->Find(TEXT("Smelter"));
    TestNotNull(TEXT("Smelter direct-build recipe is registered"), Smelter);
    if (Smelter)
    {
        TestEqual(TEXT("Smelter recipe resolves to the stable construction identity"),
            Smelter->BuildableOutput, FName(TEXT("SmelterKit")));
        TestEqual(TEXT("Smelter placement has two direct material costs"), Smelter->Ingredients.Num(), 2);
        if (Smelter->Ingredients.Num() == 2)
        {
            TestTrue(TEXT("Smelter costs 20 Stone and 20 Wood"),
                Smelter->Ingredients[0].ItemId == TEXT("Stone") && Smelter->Ingredients[0].Quantity == 20
                && Smelter->Ingredients[1].ItemId == TEXT("Wood") && Smelter->Ingredients[1].Quantity == 20);
        }
        TestNotNull(TEXT("Smelter metadata is presentation-only"), Items->FindBuildable(TEXT("SmelterKit")));
        TestNull(TEXT("Smelter is not an inventory item"), Items->FindItem(TEXT("SmelterKit")));
    }
    const FKalmalaRecipe* BronzeAlloy = Recipes->Find(TEXT("BronzeAlloy"));
    TestNotNull(TEXT("Forge Bronze alloy recipe is registered"), BronzeAlloy);
    if (BronzeAlloy)
    {
        TestEqual(TEXT("Bronze alloy outputs Bronze"), BronzeAlloy->Output, FName(TEXT("Bronze")));
        TestEqual(TEXT("Bronze alloy batch cap is five"), BronzeAlloy->MaxBatch, 5);
        TestTrue(TEXT("Bronze alloy uses two Copper and one Tin at the Forge"),
            BronzeAlloy->RequiredStation.Contains(TEXT("Forge")) && BronzeAlloy->Ingredients.Num() == 2
            && BronzeAlloy->Ingredients[0].ItemId == TEXT("Copper") && BronzeAlloy->Ingredients[0].Quantity == 2
            && BronzeAlloy->Ingredients[1].ItemId == TEXT("Tin") && BronzeAlloy->Ingredients[1].Quantity == 1);
    }
    for (const FKalmalaItemDefinition& Buildable : Items->Buildables)
        TestNotNull(TEXT("Buildable metadata is available only through its explicit lookup"), Items->FindBuildable(Buildable.ItemId));

    auto* ForgedItems = NewObject<UKalmalaItemCatalogue>();
    ForgedItems->Items = Items->Items;
    FKalmalaItemDefinition NewKit;
    NewKit.ItemId = TEXT("FuturePortableKit");
    NewKit.DisplayName = TEXT("Forbidden portable construction");
    NewKit.Description = TEXT("Must never be admitted as an inventory item.");
    ForgedItems->Items.Add(NewKit);
    TestFalse(TEXT("Future kit definitions fail catalogue validation"), ForgedItems->IsValidCatalogue());
    return true;
}
#endif
