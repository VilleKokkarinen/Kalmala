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
    TestEqual(TEXT("All thirteen current placed objects use material recipes"), BuildCount, 13);
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
