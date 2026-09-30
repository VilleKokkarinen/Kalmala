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
    const FKalmalaRecipe* CookedDeer = Recipes->Find(TEXT("CookedDeerMeatRecipe"));
    const FKalmalaRecipe* MeatStew = Recipes->Find(TEXT("MeatStewRecipe"));
    if (TestNotNull(TEXT("Current cooked-boar recipe exists"), CookedBoar))
    {
        TestEqual(TEXT("Cooked boar uses its raw meat as the only ingredient"), CookedBoar->Ingredients.Num(), 1);
        if (CookedBoar->Ingredients.Num() == 1)
            TestEqual(TEXT("Cooked boar consumes boar meat"), CookedBoar->Ingredients[0].ItemId, FName(TEXT("BoarMeat")));
        TestEqual(TEXT("Cooked boar produces the current catalogue food output"), CookedBoar->Output, FName(TEXT("CookedBoarMeat")));
        TestTrue(TEXT("Cooked boar recipe resolves to the Cooking Rack"), CookedBoar->RequiredStation.Contains(TEXT("CookingRackKit")));
    }
    if (TestNotNull(TEXT("Current cooked-deer recipe exists"), CookedDeer))
    {
        TestEqual(TEXT("Cooked deer uses its raw meat as the only ingredient"), CookedDeer->Ingredients.Num(), 1);
        if (CookedDeer->Ingredients.Num() == 1)
            TestEqual(TEXT("Cooked deer consumes deer meat"), CookedDeer->Ingredients[0].ItemId, FName(TEXT("DeerMeat")));
        TestEqual(TEXT("Cooked deer produces the current catalogue food output"), CookedDeer->Output, FName(TEXT("CookedDeerMeat")));
        TestTrue(TEXT("Cooked deer recipe resolves to the Cooking Rack"), CookedDeer->RequiredStation.Contains(TEXT("CookingRackKit")));
    }
    if (TestNotNull(TEXT("Current meat-stew recipe exists"), MeatStew))
    {
        TestEqual(TEXT("Meat stew uses four catalogue ingredients"), MeatStew->Ingredients.Num(), 4);
        TestEqual(TEXT("Meat stew produces the current catalogue food output"), MeatStew->Output, FName(TEXT("MeatStew")));
        TestTrue(TEXT("Meat stew requires the cauldron station"), MeatStew->RequiredStation.Contains(TEXT("CauldronKit")));
        for (const TPair<FName, int32>& IngredientAndQuantity : {
            TPair<FName, int32>(TEXT("BoarMeat"), 1), TPair<FName, int32>(TEXT("DeerMeat"), 1),
            TPair<FName, int32>(TEXT("Carrot"), 2), TPair<FName, int32>(TEXT("Potato"), 2) })
        {
            const FKalmalaInventoryStack* Ingredient = MeatStew->Ingredients.FindByPredicate(
                [&IngredientAndQuantity](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == IngredientAndQuantity.Key; });
            TestNotNull(FString::Printf(TEXT("Meat stew includes %s"), *IngredientAndQuantity.Key.ToString()), Ingredient);
            if (Ingredient) TestEqual(TEXT("Meat stew uses its listed ingredient quantity"), Ingredient->Quantity, IngredientAndQuantity.Value);
        }
    }

    const FKalmalaRecipe* FryingPan = Recipes->Find(TEXT("FryingPanRecipe"));
    const FKalmalaRecipe* RootSoup = Recipes->Find(TEXT("RootVegetableSoupRecipe"));
    const FKalmalaRecipe* RoastedRoots = Recipes->Find(TEXT("RoastedRootVegetablesRecipe"));
    const FKalmalaRecipe* DeerRootRoast = Recipes->Find(TEXT("DeerRootRoastRecipe"));
    if (FryingPan)
    {
        TestEqual(TEXT("Frying pan recipe outputs its placeable station"), FryingPan->Output, FName(TEXT("FryingPanKit")));
        TestTrue(TEXT("Frying pan is forged at a Forge"), FryingPan->RequiredStation.Contains(TEXT("ForgeKit")));
        TestEqual(TEXT("Frying pan costs one material stack"), FryingPan->Ingredients.Num(), 1);
        if (FryingPan->Ingredients.Num() == 1)
        {
            TestEqual(TEXT("Frying pan costs iron"), FryingPan->Ingredients[0].ItemId, FName(TEXT("Iron")));
            TestEqual(TEXT("Frying pan costs five iron"), FryingPan->Ingredients[0].Quantity, 5);
        }
    }
    if (RootSoup)
    {
        TestEqual(TEXT("Root soup output is configured"), RootSoup->Output, FName(TEXT("RootVegetableSoup")));
        TestTrue(TEXT("Root soup uses a cauldron"), RootSoup->RequiredStation.Contains(TEXT("CauldronKit")));
        TestEqual(TEXT("Root soup uses four ingredients"), RootSoup->Ingredients.Num(), 4);
        for (const FName IngredientId : { FName(TEXT("Carrot")), FName(TEXT("Potato")), FName(TEXT("Rutabaga")), FName(TEXT("Onion")) })
        {
            const FKalmalaInventoryStack* Ingredient = RootSoup->Ingredients.FindByPredicate(
                [IngredientId](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == IngredientId; });
            TestNotNull(FString::Printf(TEXT("Root soup includes %s"), *IngredientId.ToString()), Ingredient);
            if (Ingredient) TestEqual(TEXT("Root soup takes one of each root"), Ingredient->Quantity, 1);
        }
    }
    for (const TPair<const FKalmalaRecipe*, FName>& Expected : {
        TPair<const FKalmalaRecipe*, FName>(RoastedRoots, TEXT("RoastedRootVegetables")),
        TPair<const FKalmalaRecipe*, FName>(DeerRootRoast, TEXT("DeerRootRoast")) })
    {
        if (!Expected.Key) continue;
        TestEqual(TEXT("Pan dish output is configured"), Expected.Key->Output, Expected.Value);
        TestTrue(TEXT("Pan dish requires the placed frying pan station"), Expected.Key->RequiredStation.Contains(TEXT("FryingPanKit")));
        TestTrue(TEXT("Pan dish does not require carrying a frying pan"), Expected.Key->RequiredTool.IsNone());
    }
    TestTrue(TEXT("No-tool recipes have no inventory tool requirement"),
        UKalmalaRecipeCatalogue::HasRequiredTool(NAME_None, {}));
    TestTrue(TEXT("A carried catalogue tool satisfies a generic tool requirement"),
        UKalmalaRecipeCatalogue::HasRequiredTool(TEXT("Iron"), {{TEXT("Iron"), 1}}));
    TestFalse(TEXT("A missing catalogue tool fails the generic tool requirement"),
        UKalmalaRecipeCatalogue::HasRequiredTool(TEXT("Iron"), {}));
    TestFalse(TEXT("A zero-count tool stack fails the generic tool requirement"),
        UKalmalaRecipeCatalogue::HasRequiredTool(TEXT("Iron"), {{TEXT("Iron"), 0}}));

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
    if (FKalmalaRecipe* PanRecipe = Invalid->Recipes.FindByPredicate(
        [](const FKalmalaRecipe& Candidate) { return Candidate.RecipeId == TEXT("RoastedRootVegetablesRecipe"); }))
        PanRecipe->RequiredTool = TEXT("UnknownTool");
    TestFalse(TEXT("Unknown reusable tools fail closed"), Invalid->IsValidCatalogue());
    Invalid->Recipes = Recipes->Recipes;
    if (FKalmalaRecipe* PanRecipe = Invalid->Recipes.FindByPredicate(
        [](const FKalmalaRecipe& Candidate) { return Candidate.RecipeId == TEXT("RoastedRootVegetablesRecipe"); }))
        PanRecipe->RequiredTool = TEXT("Carrot");
    TestFalse(TEXT("A reusable tool cannot also be consumed as an ingredient"), Invalid->IsValidCatalogue());
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
