#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCraftingSubsystem.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaPlacementPreview.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRecipeBrowseTest, "Kalmala.UI.Crafting.LocalBrowsing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaRecipeBrowseTest::RunTest(const FString& Parameters)
{
    auto* Widget = NewObject<UKalmalaCraftingWidget>();
    const auto& Recipes = UKalmalaRecipeCatalogue::Get()->Recipes;
    const auto BuildIndices = Widget->GetVisibleRecipeIndices();
    int32 ExpectedBuilds = 0;
    for (int32 Index = 0; Index < Recipes.Num(); ++Index)
    {
        if (!FKalmalaPlacementPreview::IsSupportedKit(Recipes[Index].GetOutputIdentity())) continue;
        ++ExpectedBuilds;
        TestTrue(TEXT("Default Build view includes every supported placeable output"), BuildIndices.Contains(Index));
    }
    TestEqual(TEXT("Default Build view excludes non-placeable production recipes"), BuildIndices.Num(), ExpectedBuilds);
    TestTrue(TEXT("Build view starts populated"), !BuildIndices.IsEmpty());
    for (const int32 Index : BuildIndices)
        TestTrue(TEXT("Build view contains only supported placeables"), Widget->GetBuildBrowseGroup(Recipes[Index].GetOutputIdentity()) != 0);

    const auto FindRecipe = [&Recipes](const FName RecipeId) -> const FKalmalaRecipe*
    {
        return Recipes.FindByPredicate([RecipeId](const FKalmalaRecipe& Recipe) { return Recipe.RecipeId == RecipeId; });
    };
    for (const FName RecipeId : { FName(TEXT("Campfire")), FName(TEXT("Workbench")), FName(TEXT("Forge")),
             FName(TEXT("WorkbenchToolRack")), FName(TEXT("ForgeAnvil")), FName(TEXT("FryingPanRecipe")),
             FName(TEXT("GrindingStone")) })
    {
        const FKalmalaRecipe* Recipe = FindRecipe(RecipeId);
        if (TestNotNull(TEXT("Existing bootstrap/station placement recipe"), Recipe))
        {
            const int32 RecipeIndex = Recipes.IndexOfByPredicate([RecipeId](const FKalmalaRecipe& Candidate)
                { return Candidate.RecipeId == RecipeId; });
            TestTrue(TEXT("Bootstrap, station, and attachment output stays placeable"),
                BuildIndices.Contains(RecipeIndex));
        }
    }
    const FKalmalaRecipe* Workbench = FindRecipe(TEXT("Workbench"));
    const FKalmalaRecipe* Forge = FindRecipe(TEXT("Forge"));
    const FKalmalaRecipe* Floor = FindRecipe(TEXT("Floor"));
    const FKalmalaRecipe* Pan = FindRecipe(TEXT("FryingPanRecipe"));
    const FKalmalaRecipe* Stone = FindRecipe(TEXT("GrindingStone"));
    const FKalmalaRecipe* ToolRack = FindRecipe(TEXT("WorkbenchToolRack"));
    const FKalmalaRecipe* Anvil = FindRecipe(TEXT("ForgeAnvil"));
    if (TestNotNull(TEXT("Bootstrap Workbench recipe"), Workbench))
        TestTrue(TEXT("Bootstrap kit production remains available from Build"), Widget->CanBuildMenuCraftRecipe(*Workbench));
    if (TestNotNull(TEXT("Bootstrap Forge recipe"), Forge))
        TestTrue(TEXT("Forge construction remains available from Build"), Widget->CanBuildMenuCraftRecipe(*Forge));
    if (TestNotNull(TEXT("Direct floor recipe"), Floor))
        TestTrue(TEXT("Direct construction retains its material action"), Widget->CanBuildMenuCraftRecipe(*Floor));
    if (TestNotNull(TEXT("Forge-required Frying Pan recipe"), Pan))
        TestFalse(TEXT("Forge-required production stays in its service menu"), Widget->CanBuildMenuCraftRecipe(*Pan));
    if (TestNotNull(TEXT("Workbench-required Grinding Stone recipe"), Stone))
        TestFalse(TEXT("Workbench-required production stays in its service menu"), Widget->CanBuildMenuCraftRecipe(*Stone));
    if (TestNotNull(TEXT("Workbench Tool Rack attachment recipe"), ToolRack))
        TestFalse(TEXT("Tool Rack production stays in its service menu"), Widget->CanBuildMenuCraftRecipe(*ToolRack));
    if (TestNotNull(TEXT("Forge Anvil attachment recipe"), Anvil))
        TestFalse(TEXT("Anvil production stays in its service menu"), Widget->CanBuildMenuCraftRecipe(*Anvil));

    Widget->SetRecipeBrowse(TEXT("  zzz-no-build  "), 3, true);
    TestEqual(TEXT("No results stays inside Build scope"), Widget->GetVisibleRecipeIndices().Num(), 0);
    Widget->SetRecipeBrowse(TEXT(""), 3, true);
    const auto Sorted = Widget->GetVisibleRecipeIndices();
    for (int32 I = 1; I < Sorted.Num(); ++I)
        TestTrue(TEXT("Build names sort without modifying the catalogue"),
            Recipes[Sorted[I - 1]].DisplayName.Compare(Recipes[Sorted[I]].DisplayName, ESearchCase::IgnoreCase) <= 0);

    Widget->SetRecipeBrowse(TEXT(""), 3, false);
    TestTrue(TEXT("Catalogue order restores the original Build result"), Widget->GetVisibleRecipeIndices() == BuildIndices);
    int32 PartitionCount = 0;
    for (int32 Group = 4; Group <= 6; ++Group)
    {
        Widget->SetRecipeBrowse(TEXT(""), Group, true);
        const auto Members = Widget->GetVisibleRecipeIndices();
        TestTrue(TEXT("Every named Build group is populated"), !Members.IsEmpty());
        PartitionCount += Members.Num();
        for (int32 Index : Members)
            TestEqual(TEXT("Build group excludes unrelated outputs"), Widget->GetBuildBrowseGroup(Recipes[Index].GetOutputIdentity()), Group);
        if (!Members.IsEmpty())
        {
            Widget->SetRecipeBrowse(TEXT("  ") + Recipes[Members[0]].DisplayName.ToUpper() + TEXT("  "), Group, true);
            TestTrue(TEXT("Build name search intersects category"), Widget->GetVisibleRecipeIndices().Contains(Members[0]));
        }
        Widget->SetRecipeBrowse(TEXT("zzz-no-build"), Group, false);
        TestTrue(TEXT("Build category no-results stays empty"), Widget->GetVisibleRecipeIndices().IsEmpty());
    }
    TestEqual(TEXT("Named groups partition all placeable outputs"), PartitionCount, BuildIndices.Num());
    TestEqual(TEXT("Floor is structural"), Widget->GetBuildBrowseGroup(TEXT("FloorKit")), 4);
    TestEqual(TEXT("Workbench is a station"), Widget->GetBuildBrowseGroup(TEXT("WorkbenchKit")), 5);
    TestEqual(TEXT("Hearth is a camp utility"), Widget->GetBuildBrowseGroup(TEXT("CampfireKit")), 6);
    TestEqual(TEXT("Unknown output is excluded"), Widget->GetBuildBrowseGroup(TEXT("UnknownKit")), 0);
    Widget->SetRecipeBrowse(TEXT(""), 3, false);
    TestTrue(TEXT("Build browsing preserves full supported source order"), Widget->GetVisibleRecipeIndices() == BuildIndices);
    return true;
}
#endif
