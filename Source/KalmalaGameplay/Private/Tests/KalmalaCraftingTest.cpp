#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaItemCatalogue.h"
#include "KalmalaRawFuelContract.h"
#include "KalmalaRecipeCatalogue.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaCraftingTransactionsTest, "Kalmala.Gameplay.Crafting.Transactions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaCraftingTransactionsTest::RunTest(const FString& Parameters)
{
    const UKalmalaRecipeCatalogue* Recipes = UKalmalaRecipeCatalogue::Get();
    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
    TestTrue(TEXT("Configured recipes validate"), Recipes->IsValidCatalogue());
    for (const FName Id : { FName(TEXT("Fuel")), FName(TEXT("Timber")), FName(TEXT("RaisedStorage")), FName(TEXT("Smokehouse")) })
        TestNull(TEXT("Removed recipe is not available"), Recipes->Find(Id));
    for (const FName Id : { FName(TEXT("Fuel")), FName(TEXT("ConstructionSupply")), FName(TEXT("RaisedStorage")), FName(TEXT("Smokehouse")) })
        TestNull(TEXT("Removed item is not available"), Items->FindItem(Id));

    for (const FKalmalaRecipe& Recipe : Recipes->Recipes)
        for (const FKalmalaInventoryStack& Ingredient : Recipe.Ingredients)
            TestFalse(FString::Printf(TEXT("%s consumes only catalogue materials"), *Recipe.RecipeId.ToString()),
                Ingredient.ItemId == TEXT("Fuel") || Ingredient.ItemId == TEXT("ConstructionSupply"));

    const FKalmalaRecipe* CookedBoar = Recipes->Find(TEXT("CookedBoarMeatRecipe"));
    if (!CookedBoar) CookedBoar = Recipes->Find(TEXT("RoastBoarMeat"));
    const FKalmalaRecipe* Broth = Recipes->Find(TEXT("SimmerBoarBroth"));
    if (CookedBoar)
    {
        TestEqual(TEXT("Cooked boar uses its raw meat as the only ingredient"), CookedBoar->Ingredients.Num(), 1);
        if (CookedBoar->Ingredients.Num() == 1)
            TestEqual(TEXT("Cooked boar consumes boar meat"), CookedBoar->Ingredients[0].ItemId, FName(TEXT("BoarMeat")));
        TestTrue(TEXT("Cooked boar produces its configured food output"), !CookedBoar->Output.IsNone());
        TestTrue(TEXT("Cooked boar recipe resolves to the Cooking Rack"), CookedBoar->RequiredStation.Contains(TEXT("CookingRackKit")));
    }
    if (Broth)
    {
        TestEqual(TEXT("Broth uses its meat as the only ingredient"), Broth->Ingredients.Num(), 1);
        if (Broth->Ingredients.Num() == 1)
            TestEqual(TEXT("Broth consumes boar meat"), Broth->Ingredients[0].ItemId, FName(TEXT("BoarMeat")));
        TestTrue(TEXT("Broth requires the cauldron station"), Broth->RequiredStation.Contains(TEXT("CauldronKit")));
    }

    struct FExpectedBuildCost { FName Id; TArray<FKalmalaInventoryStack> Costs; };
    const TArray<FExpectedBuildCost> DirectBuildCosts = {
        {TEXT("CampfireKit"), {{TEXT("Stone"),5},{TEXT("Wood"),3}}},
        {TEXT("FloorKit"), {{TEXT("Wood"),6},{TEXT("Fibre"),4}}},
        {TEXT("WallKit"), {{TEXT("Wood"),6},{TEXT("Fibre"),6}}},
        {TEXT("RoofKit"), {{TEXT("Wood"),6},{TEXT("Fibre"),8}}},
    };
    for (const FExpectedBuildCost& Expected : DirectBuildCosts)
    {
        TArray<FKalmalaInventoryStack> Actual;
        FString Reason;
        TestTrue(TEXT("Direct build recipe expands to raw materials"),
            UKalmalaRecipeCatalogue::BuildDirectMaterialCost(Expected.Id, Actual, Reason));
        TestEqual(TEXT("Direct build has its expected material count"), Actual.Num(), Expected.Costs.Num());
        for (int32 Index = 0; Index < Expected.Costs.Num() && Actual.IsValidIndex(Index); ++Index)
        {
            TestEqual(TEXT("Direct build material identity is correct"), Actual[Index].ItemId, Expected.Costs[Index].ItemId);
            TestEqual(TEXT("Direct build material quantity is correct"), Actual[Index].Quantity, Expected.Costs[Index].Quantity);
        }
    }

    TArray<FKalmalaInventoryStack> FuelCosts;
    FString Reason;
    const TArray<FKalmalaInventoryStack> MixedFuel = {{TEXT("Wood"),1},{TEXT("Coal"),1}};
    TestTrue(TEXT("A hearth refuel can draw from more than one raw material"),
        FKalmalaRawFuelContract::AddCosts(MixedFuel, 2, FuelCosts, Reason));
    TestEqual(TEXT("Wood participates directly in the exchange"), FuelCosts[0].ItemId, FName(TEXT("Wood")));
    TestEqual(TEXT("Coal participates directly in the exchange"), FuelCosts[1].ItemId, FName(TEXT("Coal")));
    TArray<FKalmalaInventoryStack> After;
    TestTrue(TEXT("A hearth refuel exchanges raw fuel without a recipe output"),
        UKalmalaInventoryComponent::BuildExchange({{TEXT("Wood"),1},{TEXT("Coal"),1}}, FuelCosts,
            NAME_None, 0, After, Reason));
    TestTrue(TEXT("The accepted raw fuel exchange empties its supplied stacks"), After.IsEmpty());
    TestTrue(TEXT("Raw fuel is not an inventory item"), Items->FindItem(TEXT("Fuel")) == nullptr
        && Items->FindItem(TEXT("Wood")) != nullptr && Items->FindItem(TEXT("Coal")) != nullptr);

    const TArray<FKalmalaInventoryStack> NoFuel;
    TArray<FKalmalaInventoryStack> UnchangedCosts = {{TEXT("BoarMeat"),1}};
    TestFalse(TEXT("Missing raw fuel rejects a processing batch"),
        FKalmalaRawFuelContract::AddCosts(NoFuel, 1, UnchangedCosts, Reason));
    TestEqual(TEXT("Failed raw-fuel selection leaves costs untouched"), UnchangedCosts.Num(), 1);

    auto* Invalid = NewObject<UKalmalaRecipeCatalogue>();
    Invalid->Recipes = Recipes->Recipes;
    Invalid->Recipes[0].Output = TEXT("Forged");
    TestFalse(TEXT("Unknown output fails closed"), Invalid->IsValidCatalogue());
    Invalid->Recipes = Recipes->Recipes;
    Invalid->Recipes[0].MaxBatch = 11;
    TestFalse(TEXT("Out-of-bound recipe batch fails closed"), Invalid->IsValidCatalogue());
    Invalid->Recipes = Recipes->Recipes;
    Invalid->Recipes[0].RequiredStation.Add(TEXT("Wood"));
    TestFalse(TEXT("A material cannot satisfy a station requirement"), Invalid->IsValidCatalogue());
    Invalid->Recipes = Recipes->Recipes;
    const FKalmalaRecipe Duplicate = Invalid->Recipes[0];
    Invalid->Recipes.Add(Duplicate);
    TestFalse(TEXT("Duplicate recipe IDs fail closed"), Invalid->IsValidCatalogue());

    const FKalmalaItemDefinition* Wood = Items->FindItem(TEXT("Wood"));
    if (Wood)
    {
        TArray<FKalmalaInventoryStack> Candidate;
        TestTrue(TEXT("Catalogue grant builds a scratch inventory candidate"),
            UKalmalaInventoryComponent::BuildGrant({{TEXT("Wood"),Wood->MaxStack - 1}}, TEXT("Wood"), 1, Candidate, Reason));
        TestEqual(TEXT("Grant reaches but does not exceed its stack limit"), Candidate[0].Quantity, Wood->MaxStack);
    }
    TestFalse(TEXT("Unknown item IDs cannot be stored"), UKalmalaInventoryComponent::BuildExchange(
        {{TEXT("Forged"),1}}, TArray<FKalmalaInventoryStack>(), NAME_None, 0, After, Reason));
    return true;
}
#endif
