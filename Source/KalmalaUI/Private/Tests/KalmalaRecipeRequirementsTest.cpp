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
        if (UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(Recipe.Output))
        {
            TestTrue(TEXT("Construction detail keeps actual placement requirements"),
                Text.Contains(TEXT("Build requirements"))
                    && Text.Contains(TEXT("Placement: clear, dry ground with a gentle slope")));
            TestTrue(TEXT("Construction detail shows its single first blocker"),
                Text.Contains(TEXT("Unavailable: Waiting for pack"))
                    && Text.Find(TEXT("Unavailable:"), ESearchCase::IgnoreCase, ESearchDir::FromStart)
                        == Text.Find(TEXT("Unavailable:"), ESearchCase::IgnoreCase, ESearchDir::FromEnd));
            TestFalse(TEXT("Construction detail omits generic requirement boilerplate"),
                Text.Contains(TEXT("Skill level:")) || Text.Contains(TEXT("Unlock:"))
                    || Text.Contains(TEXT("server rechecks")) || Text.Contains(TEXT("Rejected requests")));
            if (Recipe.Output == TEXT("CampfireKit"))
                TestTrue(TEXT("Hearth fuel and duration stay explicit"),
                    Text.Contains(TEXT("one raw Wood, Lightwood, Densewood, or Coal; starts with 60 seconds")));
        }
        else
            TestTrue(TEXT("Existing recipe requirement summary remains explicit"),
                Text.Contains(TEXT("Skill level: no recipe requirement."))
                    && Text.Contains(TEXT("Unavailable: Waiting for pack")));
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
        const FString MissingFloor = FKalmalaRecipeRequirements::Describe(
            *Floor, Inventory, 0, TEXT("Need your carried Construction Hammer"));
        TestTrue(TEXT("Missing carried hammer has one concise blocker"),
            MissingFloor.Contains(TEXT("Construction Hammer level 1: Missing."))
                && MissingFloor.Contains(TEXT("Unavailable: Need your carried Construction Hammer")));
        TestTrue(TEXT("Unknown tool state is pending"),
            FKalmalaRecipeRequirements::Describe(*Floor, nullptr, -1, TEXT("Waiting for pack"))
                .Contains(TEXT("Construction Hammer level 1: Waiting for your tool state.")));
        const FString AvailableFloor = FKalmalaRecipeRequirements::Describe(*Floor, Inventory, 1, TEXT("Ready"));
        TestTrue(TEXT("Available construction has no fabricated blocker"),
            AvailableFloor.Contains(TEXT("Construction Hammer level 1: Present."))
                && !AvailableFloor.Contains(TEXT("Unavailable:")));
        FKalmalaRecipe DisabledFloor = *Floor;
        DisabledFloor.bEnabled = false;
        const FString DisabledFloorText = FKalmalaRecipeRequirements::Describe(DisabledFloor, Inventory, 1, FString());
        TestTrue(TEXT("Disabled construction has one blocker"),
            DisabledFloorText.Contains(TEXT("Unavailable: Recipe unavailable"))
                && DisabledFloorText.Find(TEXT("Unavailable:"), ESearchCase::IgnoreCase, ESearchDir::FromStart)
                    == DisabledFloorText.Find(TEXT("Unavailable:"), ESearchCase::IgnoreCase, ESearchDir::FromEnd));
    }
    World->DestroyWorld(false);
    return true;
}
#endif
