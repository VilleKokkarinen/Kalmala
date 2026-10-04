#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaRecipeRequirements.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaToolProgressionContract.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRecipeRequirementsTest, "Kalmala.UI.Crafting.Requirements",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaRecipeRequirementsTest::RunTest(const FString& Parameters)
{
    for (const auto& Recipe : UKalmalaRecipeCatalogue::Get()->Recipes)
    {
        const FString Text = FKalmalaRecipeRequirements::Describe(Recipe, nullptr, -1, TEXT("Waiting for pack"));
        TestTrue(TEXT("No fabricated skill lock"), Text.Contains(TEXT("Skill level: no recipe requirement.")));
        TestTrue(TEXT("First supplied reason is explicit"), Text.Contains(TEXT("Unavailable: Waiting for pack")));
        if (UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(Recipe.Output))
        {
            TestTrue(TEXT("Direct build needs carried hammer"), Text.Contains(TEXT("carried Construction Hammer level 1")));
            TestTrue(TEXT("No station for direct build"), Text.Contains(TEXT("Station: none; build in place.")));
            if (Recipe.Output == TEXT("CampfireKit")) TestTrue(TEXT("Ignition alternative remains explicit"), Text.Contains(TEXT("Ignition: one raw Wood, Lightwood, Densewood or Coal")));
        }
        if (Recipe.RequiredStation.Contains(TEXT("CookingRackKit")) || Recipe.RequiredStation.Contains(TEXT("CauldronKit"))
            || Recipe.RequiredStation.Contains(TEXT("FryingPanKit")))
            TestTrue(TEXT("Heat described even without a nearby station"), Text.Contains(TEXT("positive heat within 2.5 m of both")));
        if (FKalmalaToolProgressionContract::IsStationAttachmentKit(Recipe.Output))
            TestTrue(TEXT("Attachment station placement included"), Text.Contains(TEXT("within 1.25 m of its matching station")));
    }
    // Presentation-only fixture: exercise supported reusable-tool metadata without adding a recipe.
    FKalmalaRecipe Fixture;
    Fixture.RequiredTool = TEXT("Wood");
    Fixture.RequiredStation = {TEXT("WorkbenchKit"), TEXT("ForgeKit")};
    auto* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Transient authoritative fixture world"), World)) return false;
    auto* Inventory = NewObject<UKalmalaInventoryComponent>(World->SpawnActor<AActor>());
    FString Text = FKalmalaRecipeRequirements::Describe(Fixture, Inventory, 1, TEXT("Need a tool"));
    TestTrue(TEXT("Alternative station semantics"), Text.Contains(TEXT("any one")) && Text.Contains(TEXT(" or ")));
    TestTrue(TEXT("Missing reusable tool is truthful"), Text.Contains(TEXT("(not consumed) — Missing")));
    TestTrue(TEXT("Authoritative fixture grant"), Inventory->TryGrantFromServer(TEXT("Wood"), 1));
    Text = FKalmalaRecipeRequirements::Describe(Fixture, Inventory, 1, FString());
    TestTrue(TEXT("Tool owner refresh"), Text.Contains(TEXT("(not consumed) — Present")));
    TestTrue(TEXT("Ready preview does not promise acceptance"), Text.Contains(TEXT("server rechecks every request")));
    TestEqual(TEXT("Formatting cannot spend the reusable tool"), Inventory->GetQuantity(TEXT("Wood")), 1);
    Fixture.bEnabled = false;
    TestTrue(TEXT("Disabled cannot show ready even with empty supplied reason"),
        FKalmalaRecipeRequirements::Describe(Fixture, Inventory, 1, FString()).Contains(TEXT("Unavailable: Recipe unavailable")));
    Fixture.Output = TEXT("FloorKit");
    // Current direct-build identity is Floor (the catalogue owns the exact ID).
    const auto* Floor = UKalmalaRecipeCatalogue::Get()->Find(TEXT("Floor"));
    if (TestNotNull(TEXT("Existing direct floor recipe"), Floor))
    {
        TestTrue(TEXT("Missing carried hammer"), FKalmalaRecipeRequirements::Describe(*Floor, Inventory, 0, TEXT("Need your carried Construction Hammer")).Contains(TEXT("level 1 — Missing")));
        TestTrue(TEXT("Unknown tool state is pending"), FKalmalaRecipeRequirements::Describe(*Floor, nullptr, -1, TEXT("Waiting for pack")).Contains(TEXT("Waiting for your tool state")));
    }
    World->DestroyWorld(false);
    return true;
}
#endif
