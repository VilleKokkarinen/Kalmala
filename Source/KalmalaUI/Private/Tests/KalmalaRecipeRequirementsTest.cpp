#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaRecipeRequirements.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaToolProgressionContract.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRecipeRequirementsTest, "Kalmala.UI.Crafting.Requirements",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaRecipeRequirementsTest::RunTest(const FString& Parameters)
{
    for (const auto& Recipe : UKalmalaRecipeCatalogue::Get()->Recipes)
    {
        const FString Text = FKalmalaRecipeRequirements::Describe(Recipe, nullptr, -1, TEXT("Waiting for pack"));
        if (UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(Recipe.BuildableOutput))
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
            if (Recipe.BuildableOutput == TEXT("CampfireKit"))
                TestTrue(TEXT("Campfire fuel and duration stay explicit"),
                    Text.Contains(TEXT("Campfire fuel: one raw Wood, Lightwood, Densewood, or Coal; starts with 60 seconds")));
        }
        else
        {
            const FKalmalaItemDefinition* OutputItem = UKalmalaItemCatalogue::Get()->FindItem(Recipe.Output);
            const FString ResultName = OutputItem ? OutputItem->DisplayName : Recipe.Output.ToString();
            TestTrue(TEXT("Generic details include result and supported quantity"),
                Text.Contains(FString::Printf(TEXT("Result: %d %s per batch."), Recipe.OutputCount, *ResultName))
                    && Text.Contains(TEXT("Quantity: one batch per press")));
            if (Recipe.MaxBatch > 1)
                TestTrue(TEXT("Generic details show the supported maximum batch"),
                    Text.Contains(FString::Printf(TEXT("up to %d batches per request"), Recipe.MaxBatch)));
            TestTrue(TEXT("Generic details show at most one current blocker"),
                Text.Contains(TEXT("Unavailable:"))
                    && Text.Find(TEXT("Unavailable:"), ESearchCase::IgnoreCase, ESearchDir::FromStart)
                        == Text.Find(TEXT("Unavailable:"), ESearchCase::IgnoreCase, ESearchDir::FromEnd));
            TestFalse(TEXT("Generic details omit empty, no-lock, and request boilerplate"),
                Text.Contains(TEXT("Station: none")) || Text.Contains(TEXT("Tool: no reusable"))
                    || Text.Contains(TEXT("Skill level:")) || Text.Contains(TEXT("Unlock:"))
                    || Text.Contains(TEXT("Preview:")) || Text.Contains(TEXT("server rechecks"))
                    || Text.Contains(TEXT("Rejected requests preserve"))
                    || Text.Contains(TEXT("stack limit")));
        }
        if (Recipe.RequiredStation.Contains(TEXT("CookingRackKit")) || Recipe.RequiredStation.Contains(TEXT("CauldronKit"))
            || Recipe.RequiredStation.Contains(TEXT("FryingPanKit")))
            TestTrue(TEXT("Heat described even without a nearby station"), Text.Contains(TEXT("positive heat within 2.5 m of both")));
        if (FKalmalaToolProgressionContract::IsStationAttachmentKit(Recipe.GetOutputIdentity()))
            TestTrue(TEXT("Attachment station placement included"), Text.Contains(TEXT("within 1.25 m of its matching station")));
    }
    // Presentation-only fixture: exercise supported reusable-tool metadata without adding a recipe.
    FKalmalaRecipe Fixture;
    Fixture.RequiredTool = TEXT("Wood");
    Fixture.RequiredStation = {TEXT("WorkbenchKit"), TEXT("ForgeKit")};
    Fixture.Output = TEXT("Wood");
    Fixture.OutputCount = 2;
    Fixture.MaxBatch = 4;
    FString Text = FKalmalaRecipeRequirements::Describe(Fixture, nullptr, 1, TEXT("Need a tool"));
    TestTrue(TEXT("Alternative station semantics"), Text.Contains(TEXT("any one")) && Text.Contains(TEXT(" or ")));
    TestTrue(TEXT("Supported output count and maximum batch are explicit"),
        Text.Contains(TEXT("Result: 2 Wood per batch."))
            && Text.Contains(TEXT("up to 4 batches per request")));
    TestTrue(TEXT("Missing reusable tool is named and reported once"),
        Text.Contains(TEXT("Reusable tool: Wood (not consumed)."))
            && Text.Contains(TEXT("Unavailable: Need a tool"))
            && Text.Find(TEXT("Unavailable:"), ESearchCase::IgnoreCase, ESearchDir::FromStart)
                == Text.Find(TEXT("Unavailable:"), ESearchCase::IgnoreCase, ESearchDir::FromEnd));
    Text = FKalmalaRecipeRequirements::Describe(Fixture, nullptr, 1, TEXT("Ready"));
    TestTrue(TEXT("Ready summary omits a synthetic blocker"), !Text.Contains(TEXT("Unavailable:")));
    TestTrue(TEXT("Reusable tool is shown without state boilerplate"),
        Text.Contains(TEXT("Reusable tool: Wood (not consumed)."))
            && !Text.Contains(TEXT("Tool: Wood in your pack")));
    Fixture.bEnabled = false;
    TestTrue(TEXT("Disabled cannot show ready even with empty supplied reason"),
        FKalmalaRecipeRequirements::Describe(Fixture, nullptr, 1, FString()).Contains(TEXT("Unavailable: Recipe unavailable")));
    Fixture.Output = TEXT("FloorKit");
    // Current direct-build identity is Floor (the catalogue owns the exact ID).
    const auto* Floor = UKalmalaRecipeCatalogue::Get()->Find(TEXT("Floor"));
    if (TestNotNull(TEXT("Existing direct floor recipe"), Floor))
    {
        const FString MissingFloor = FKalmalaRecipeRequirements::Describe(
            *Floor, nullptr, 0, TEXT("Need your carried Construction Hammer"));
        TestTrue(TEXT("Missing carried hammer has one concise blocker"),
            MissingFloor.Contains(TEXT("Construction Hammer level 1: Missing."))
                && MissingFloor.Contains(TEXT("Unavailable: Need your carried Construction Hammer")));
        TestTrue(TEXT("Unknown tool state is pending"),
            FKalmalaRecipeRequirements::Describe(*Floor, nullptr, -1, TEXT("Waiting for pack"))
                .Contains(TEXT("Construction Hammer level 1: Waiting for your tool state.")));
        const FString AvailableFloor = FKalmalaRecipeRequirements::Describe(*Floor, nullptr, 1, TEXT("Ready"));
        TestTrue(TEXT("Available construction has no fabricated blocker"),
            AvailableFloor.Contains(TEXT("Construction Hammer level 1: Present."))
                && !AvailableFloor.Contains(TEXT("Unavailable:")));
        FKalmalaRecipe DisabledFloor = *Floor;
        DisabledFloor.bEnabled = false;
        const FString DisabledFloorText = FKalmalaRecipeRequirements::Describe(DisabledFloor, nullptr, 1, FString());
        TestTrue(TEXT("Disabled construction has one blocker"),
            DisabledFloorText.Contains(TEXT("Unavailable: Recipe unavailable"))
                && DisabledFloorText.Find(TEXT("Unavailable:"), ESearchCase::IgnoreCase, ESearchDir::FromStart)
                    == DisabledFloorText.Find(TEXT("Unavailable:"), ESearchCase::IgnoreCase, ESearchDir::FromEnd));
    }
    return true;
}
#endif
