#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaInventorySubsystem.h"

#include "Misc/AutomationTest.h"

#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaInventoryPreparedFoodDetailsTest,
    "Kalmala.UI.Inventory.PreparedFoodDetails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaInventoryPreparedFoodDetailsTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Food details stay absent without a prepared food stack"),
        UKalmalaInventoryWidget::BuildPreparedFoodDetails(false, 42.0f).IsEmpty());

    const FString ReadyDetails = UKalmalaInventoryWidget::BuildPreparedFoodDetails(true, 0.0f);
    TestTrue(TEXT("Prepared food explains its bounded stamina benefit and duration"),
        ReadyDetails.Contains(TEXT("One serving grants Steady Meal: stamina cost −10% for 120 s")));
    TestTrue(TEXT("Prepared food states its no-stack/no-replacement rule"),
        ReadyDetails.Contains(TEXT("Meal effects do not stack or replace")));
    TestFalse(TEXT("Inactive meal has no fabricated timer"), ReadyDetails.Contains(TEXT("ACTIVE MEAL")));

    const FString ActiveDetails = UKalmalaInventoryWidget::BuildPreparedFoodDetails(true, 73.2f);
    TestTrue(TEXT("Active meal uses the supplied replicated remaining time"),
        ActiveDetails.Contains(TEXT("ACTIVE MEAL · stamina cost −10% · 74 s remaining")));
    TestTrue(TEXT("Active meal tells the player to wait for expiry"),
        ActiveDetails.Contains(TEXT("wait for expiry before another meal")));

    const FString InvalidTimerDetails = UKalmalaInventoryWidget::BuildPreparedFoodDetails(true, std::numeric_limits<float>::quiet_NaN());
    TestFalse(TEXT("Invalid timer data cannot create an active meal display"), InvalidTimerDetails.Contains(TEXT("ACTIVE MEAL")));
    return true;
}

#endif
