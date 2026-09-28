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
    {
        TestTrue(FString::Printf(TEXT("%s has a bounded fuel cost"), *Recipe.RecipeId.ToString()),
            Recipe.FuelPerServing >= 0 && Recipe.FuelPerServing <= 1);
        for (const FKalmalaInventoryStack& Ingredient : Recipe.Ingredients)
        {
            TestFalse(FString::Printf(TEXT("%s consumes only catalogue materials"), *Recipe.RecipeId.ToString()),
                Ingredient.ItemId == TEXT("Fuel") || Ingredient.ItemId == TEXT("ConstructionSupply"));
        }
    }
    const FKalmalaRecipe* Roast = Recipes->Find(TEXT("RoastBoarMeat"));
    const FKalmalaRecipe* Broth = Recipes->Find(TEXT("SimmerBoarBroth"));
    const FKalmalaRecipe* Smoke = Recipes->Find(TEXT("SmokeBoarMeat"));
    if (Roast) TestEqual(TEXT("Roasting has no extra raw-fuel cost"), Roast->FuelPerServing, 0);
    if (Broth)
    {
        TestEqual(TEXT("Broth uses one raw fuel unit per serving"), Broth->FuelPerServing, 1);
        TestTrue(TEXT("Broth requires the cauldron station"), Broth->RequiredStation.Contains(TEXT("CauldronKit")));
    }
    if (Smoke)
    {
        TestEqual(TEXT("Smoking has no Cooking-level requirement and charges raw fuel"), Smoke->FuelPerServing, 1);
        TestTrue(TEXT("Smoking requires only the open Smoke Frame"), Smoke->RequiredStation.Contains(TEXT("SmokeFrameKit"))
            && Smoke->RequiredStation.Num() == 1);
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

    TArray<FKalmalaInventoryStack> FuelCosts = {{TEXT("BoarMeat"),2}};
    FString Reason;
    const TArray<FKalmalaInventoryStack> MixedFuel = {{TEXT("Wood"),1},{TEXT("Coal"),1}};
    TestTrue(TEXT("One serving can draw raw fuel from more than one material"),
        FKalmalaRawFuelContract::AddCosts(MixedFuel, 2, FuelCosts, Reason));
    TestEqual(TEXT("Wood participates directly in the exchange"), FuelCosts[1].ItemId, FName(TEXT("Wood")));
    TestEqual(TEXT("Coal participates directly in the exchange"), FuelCosts[2].ItemId, FName(TEXT("Coal")));
    TArray<FKalmalaInventoryStack> After;
    TestTrue(TEXT("Food and mixed raw fuel commit as one transaction"),
        UKalmalaInventoryComponent::BuildExchange({{TEXT("BoarMeat"),2},{TEXT("Wood"),1},{TEXT("Coal"),1}},
            FuelCosts, TEXT("HearthBroth"), 2, After, Reason));
    const FKalmalaInventoryStack* BrothOutput = After.FindByPredicate(
        [](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("HearthBroth"); });
    if (TestNotNull(TEXT("Transaction contains its output"), BrothOutput))
        TestEqual(TEXT("Successful transaction outputs both servings"), BrothOutput->Quantity, 2);
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
    Invalid->Recipes[0].FuelPerServing = 2;
    TestFalse(TEXT("Out-of-bound raw-fuel requirement fails closed"), Invalid->IsValidCatalogue());
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
