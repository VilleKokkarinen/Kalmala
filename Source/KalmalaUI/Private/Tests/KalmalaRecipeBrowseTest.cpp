#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCraftingSubsystem.h"
#include "KalmalaRecipeCatalogue.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRecipeBrowseTest, "Kalmala.UI.Crafting.LocalBrowsing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaRecipeBrowseTest::RunTest(const FString& Parameters)
{
    auto* Widget = NewObject<UKalmalaCraftingWidget>();
    const auto& Recipes = UKalmalaRecipeCatalogue::Get()->Recipes;
    const auto All = Widget->GetVisibleRecipeIndices();
    TestEqual(TEXT("All retains the existing catalogue"), All.Num(), Recipes.Num());
    Widget->SetRecipeBrowse(TEXT("  zzz-no-recipe  "), 0, true);
    TestEqual(TEXT("No results is bounded"), Widget->GetVisibleRecipeIndices().Num(), 0);
    Widget->SetRecipeBrowse(TEXT(""), 2, true);
    const auto Cooking = Widget->GetVisibleRecipeIndices();
    TestTrue(TEXT("Cooking category is populated"), !Cooking.IsEmpty());
    for (int32 Index : Cooking) TestTrue(TEXT("Cooking uses existing metadata"), Recipes[Index].ExperienceSkill == EKalmalaSkill::Cooking);
    Widget->SetRecipeBrowse(TEXT(""), 1, false);
    const auto Other = Widget->GetVisibleRecipeIndices();
    TestEqual(TEXT("Categories partition the existing catalogue"), Other.Num() + Cooking.Num(), All.Num());
    for (int32 Index : Other) TestTrue(TEXT("Other excludes cooking"), Recipes[Index].ExperienceSkill != EKalmalaSkill::Cooking);
    if (!Cooking.IsEmpty())
    {
        Widget->SetRecipeBrowse(TEXT("  ") + Recipes[Cooking[0]].DisplayName.ToUpper() + TEXT("  "), 2, true);
        TestTrue(TEXT("Trimmed case-insensitive name query retains recipe"), Widget->GetVisibleRecipeIndices().Contains(Cooking[0]));
    }
    Widget->SetRecipeBrowse(TEXT(""), 0, true);
    const auto Sorted = Widget->GetVisibleRecipeIndices();
    for (int32 I = 1; I < Sorted.Num(); ++I) TestTrue(TEXT("Names sort without modifying catalogue"),
        Recipes[Sorted[I-1]].DisplayName.Compare(Recipes[Sorted[I]].DisplayName, ESearchCase::IgnoreCase) <= 0);
    Widget->SetRecipeBrowse(TEXT(""), 0, false);
    TestTrue(TEXT("Catalogue order restored exactly"), Widget->GetVisibleRecipeIndices() == All);
    return true;
}
#endif
