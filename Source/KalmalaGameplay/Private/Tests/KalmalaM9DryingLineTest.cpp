#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaConstructionActor.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlacementPreview.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaSkillProgressionComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaM9DryingLineTest, "Kalmala.Gameplay.M9.DryingLine",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaM9DryingLineTest::RunTest(const FString& Parameters)
{
    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
    const UKalmalaRecipeCatalogue* Recipes = UKalmalaRecipeCatalogue::Get();
    TestTrue(TEXT("Current item and recipe catalogues validate"), Items->IsValidCatalogue() && Recipes->IsValidCatalogue());

    const FKalmalaItemDefinition* Line = Items->FindItem(TEXT("DryingLineKit"));
    if (TestNotNull(TEXT("Drying Line uses its stable runtime construction identity"), Line))
    {
        TestEqual(TEXT("Drying Line has a readable public name"), Line->DisplayName, FString(TEXT("Drying Line")));
        TestTrue(TEXT("Drying Line item stack is bounded to five"), Line->MaxStack == 5);
    }
    const FKalmalaItemDefinition* Dried = Items->FindItem(TEXT("DriedFieldMeat"));
    if (TestNotNull(TEXT("Dried field meat is a catalogue item"), Dried))
    {
        TestEqual(TEXT("Dried field meat stack matches other prepared meals"), Dried->MaxStack, 20);
        TestTrue(TEXT("Dried food description is bounded"), !Dried->Description.IsEmpty() && Dried->Description.Len() <= 180);
    }

    const FKalmalaRecipe* BuildLine = Recipes->Find(TEXT("DryingLine"));
    if (TestNotNull(TEXT("Drying Line construction recipe exists"), BuildLine))
    {
        TestEqual(TEXT("Construction recipe resolves to the stable line identity"), BuildLine->Output, FName(TEXT("DryingLineKit")));
        TestEqual(TEXT("Construction recipe remains one bounded placement"), BuildLine->MaxBatch, 1);
        TestTrue(TEXT("Construction uses a visible same-world Workbench"), BuildLine->RequiredStation.Contains(TEXT("WorkbenchKit"))
            && BuildLine->RequiredStation.Num() == 1);
        const TPair<FName, int32> ExpectedCosts[] = {
            {TEXT("Wood"), 6}, {TEXT("Fibre"), 7}, {TEXT("Densewood"), 1}
        };
        TestEqual(TEXT("Retired supplies are converted to three raw material stacks"),
            BuildLine->Ingredients.Num(), int32(UE_ARRAY_COUNT(ExpectedCosts)));
        for (const TPair<FName, int32>& Expected : ExpectedCosts)
        {
            const FKalmalaInventoryStack* Cost = BuildLine->Ingredients.FindByPredicate(
                [&Expected](const FKalmalaInventoryStack& Candidate) { return Candidate.ItemId == Expected.Key; });
            if (TestNotNull(FString::Printf(TEXT("Construction raw cost %s exists"), *Expected.Key.ToString()), Cost))
                TestEqual(TEXT("Construction raw cost has the accepted quantity"), Cost->Quantity, Expected.Value);
        }
    }

    const TPair<FName, FName> DryingRecipes[] = {
        {TEXT("DryBoarMeat"), TEXT("BoarMeat")}, {TEXT("DryDeerMeat"), TEXT("DeerMeat")}
    };
    for (const TPair<FName, FName>& Expected : DryingRecipes)
    {
        const FKalmalaRecipe* Recipe = Recipes->Find(Expected.Key);
        if (!TestNotNull(FString::Printf(TEXT("%s recipe exists"), *Expected.Key.ToString()), Recipe)) continue;
        TestEqual(TEXT("Dried recipes consume only the matching raw meat"), Recipe->Ingredients.Num(), 1);
        if (Recipe->Ingredients.Num() == 1)
        {
            TestEqual(TEXT("Drying recipe uses its species input"), Recipe->Ingredients[0].ItemId, Expected.Value);
            TestEqual(TEXT("Drying recipe consumes one meat per serving"), Recipe->Ingredients[0].Quantity, 1);
        }
        TestEqual(TEXT("Drying produces the shared prepared food item"), Recipe->Output, FName(TEXT("DriedFieldMeat")));
        TestTrue(TEXT("Drying requires its visible same-world station"), Recipe->RequiredStation.Contains(TEXT("DryingLineKit"))
            && Recipe->RequiredStation.Num() == 1);
        TestEqual(TEXT("Drying batch is capped at three servings"), Recipe->MaxBatch, 3);
        TestEqual(TEXT("Successful drying awards the existing Cooking skill"), Recipe->ExperienceSkill, EKalmalaSkill::Cooking);
        TestEqual(TEXT("Drying awards one bounded amount per accepted request"), Recipe->ExperienceAward, 10);

        TArray<FKalmalaInventoryStack> Costs;
        int32 OutputCount = 0;
        TestTrue(TEXT("Three servings scale inside the accepted batch bound"),
            UKalmalaRecipeCatalogue::Scale(*Recipe, 3, Costs, OutputCount));
        TestEqual(TEXT("Three-serving drying output remains three"), OutputCount, 3);
        TestFalse(TEXT("Four servings exceed the accepted batch bound"),
            UKalmalaRecipeCatalogue::Scale(*Recipe, 4, Costs, OutputCount));
    }

    TestTrue(TEXT("Drying Line is accepted as a crafting station"), AKalmalaConstructionActor::IsCraftingStationKit(TEXT("DryingLineKit")));
    TestTrue(TEXT("Drying Line uses the ordinary validated construction placement path"), FKalmalaPlacementPreview::IsSupportedKit(TEXT("DryingLineKit")));
    TestTrue(TEXT("Drying Line construction remains session-only before M9 migration"), FKalmalaPlacementPreview::IsSessionOnlyKit(TEXT("DryingLineKit")));
    TestEqual(TEXT("Session construction has a fixed world cap"), AKalmalaConstructionActor::MaxSessionDryingLines, 5);
    TestTrue(TEXT("Dried field meat uses the existing bounded Steady Meal effect"),
        UKalmalaPlayerStatusComponent::IsKnownFoodItem(TEXT("DriedFieldMeat")));
    return true;
}
#endif
