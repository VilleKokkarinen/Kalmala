#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCraftingSubsystem.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaPlacementPreview.h"
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
    Widget->SetRecipeBrowse(TEXT(""), 3, false);
    const auto Builds = Widget->GetVisibleRecipeIndices();
    int32 ExpectedBuilds = 0;
    for (const auto& Recipe : Recipes) if (FKalmalaPlacementPreview::IsSupportedKit(Recipe.Output)) ++ExpectedBuilds;
    TestEqual(TEXT("All builds includes exactly supported catalogue outputs"), Builds.Num(), ExpectedBuilds);
    TestTrue(TEXT("Builds are populated"), !Builds.IsEmpty());
    int32 PreviousGroup = 0;
    for (int32 Index : Builds)
    {
        const int32 Group = Widget->GetBuildBrowseGroup(Recipes[Index].Output);
        TestTrue(TEXT("All builds groups structural, stations then utilities"), Group >= PreviousGroup);
        PreviousGroup = Group;
    }
    int32 PartitionCount = 0;
    for (int32 Group = 4; Group <= 6; ++Group)
    {
        Widget->SetRecipeBrowse(TEXT(""), Group, true);
        const auto Members = Widget->GetVisibleRecipeIndices();
        TestTrue(TEXT("Every named build group is populated"), !Members.IsEmpty());
        PartitionCount += Members.Num();
        for (int32 Index : Members) TestEqual(TEXT("Group excludes unrelated outputs"), Widget->GetBuildBrowseGroup(Recipes[Index].Output), Group);
        if (!Members.IsEmpty())
        {
            Widget->SetRecipeBrowse(TEXT("  ") + Recipes[Members[0]].DisplayName.ToUpper() + TEXT("  "), Group, true);
            TestTrue(TEXT("Build name search intersects category"), Widget->GetVisibleRecipeIndices().Contains(Members[0]));
        }
        Widget->SetRecipeBrowse(TEXT("zzz-no-build"), Group, false);
        TestTrue(TEXT("Build no-results stays empty"), Widget->GetVisibleRecipeIndices().IsEmpty());
    }
    TestEqual(TEXT("Named groups partition all builds"), PartitionCount, Builds.Num());
    TestEqual(TEXT("Floor structural"), Widget->GetBuildBrowseGroup(TEXT("FloorKit")), 4);
    TestEqual(TEXT("Workbench station"), Widget->GetBuildBrowseGroup(TEXT("WorkbenchKit")), 5);
    TestEqual(TEXT("Hearth utility"), Widget->GetBuildBrowseGroup(TEXT("CampfireKit")), 6);
    TestEqual(TEXT("Unknown output excluded"), Widget->GetBuildBrowseGroup(TEXT("UnknownKit")), 0);
    Widget->SetRecipeBrowse(TEXT(""), 0, false);
    TestTrue(TEXT("Build browsing preserves complete source order"), Widget->GetVisibleRecipeIndices() == All);

    if (Builds.Num() >= 2 && !Cooking.IsEmpty())
    {
        int32 CraftedIndex = INDEX_NONE;
        for (int32 Index : All)
            if (Recipes[Index].ExperienceSkill != EKalmalaSkill::Cooking
                && Widget->GetBuildBrowseGroup(Recipes[Index].Output) == 0)
            {
                CraftedIndex = Index;
                break;
            }
        auto* OwnerA = NewObject<UKalmalaCraftingSubsystem>();
        auto* OwnerB = NewObject<UKalmalaCraftingSubsystem>();
        const FName OwnerABuild = Recipes[Builds[0]].RecipeId;
        const FName OwnerBBuild = Recipes[Builds.Last()].RecipeId;
        OwnerA->SetRecipeFavorite(OwnerABuild, true);
        OwnerB->SetRecipeFavorite(OwnerBBuild, true);
        OwnerA->SetRecipeActivityForTest(EKalmalaCraftingActionKind::BuiltPiece, OwnerABuild, 9, true);
        OwnerB->SetRecipeActivityForTest(EKalmalaCraftingActionKind::BuiltPiece, OwnerBBuild, 3, true);
        TestTrue(TEXT("Each local owner retains its own Favorite set"),
            OwnerA->IsRecipeFavorite(OwnerABuild) && !OwnerA->IsRecipeFavorite(OwnerBBuild)
            && OwnerB->IsRecipeFavorite(OwnerBBuild) && !OwnerB->IsRecipeFavorite(OwnerABuild));
        TestTrue(TEXT("Each local owner retains separate usage counts, ranks, and Recent IDs"),
            OwnerA->GetRecipeActivityCount(EKalmalaCraftingActionKind::BuiltPiece, OwnerABuild) == 9
            && OwnerA->GetRecipeActivityRank(EKalmalaCraftingActionKind::BuiltPiece, OwnerABuild) == 1
            && OwnerA->GetRecentRecipeActivity(EKalmalaCraftingActionKind::BuiltPiece) == OwnerABuild
            && OwnerA->GetRecipeActivityCount(EKalmalaCraftingActionKind::BuiltPiece, OwnerBBuild) == 0
            && OwnerB->GetRecipeActivityCount(EKalmalaCraftingActionKind::BuiltPiece, OwnerBBuild) == 3
            && OwnerB->GetRecipeActivityRank(EKalmalaCraftingActionKind::BuiltPiece, OwnerBBuild) == 1
            && OwnerB->GetRecentRecipeActivity(EKalmalaCraftingActionKind::BuiltPiece) == OwnerBBuild);
        TestTrue(TEXT("Recent cooking and crafting kinds remain separate local buckets"), CraftedIndex != INDEX_NONE
            && OwnerA->GetRecentRecipeActivity(EKalmalaCraftingActionKind::CookedRecipe).IsNone()
            && OwnerA->GetRecentRecipeActivity(EKalmalaCraftingActionKind::CraftedItem).IsNone());
    }
    else
    {
        AddError(TEXT("The active catalogue needs at least two build recipes and one cooking recipe for owner isolation coverage."));
    }
    return true;
}
#endif
