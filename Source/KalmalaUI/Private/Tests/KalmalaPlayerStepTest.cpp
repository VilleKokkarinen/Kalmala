#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaPlayerStepWidget.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPlayerStepTest, "Kalmala.UI.PlayerStep.Privacy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaPlayerStepTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("The player label is generic"), FString(UKalmalaPlayerStepWidget::PlayerLabel()), FString(TEXT("Current player")));
    const FString Available(UKalmalaPlayerStepWidget::IdentityMessage(true));
    const FString Unavailable(UKalmalaPlayerStepWidget::IdentityMessage(false));
    TestTrue(TEXT("Both messages explain server authentication"),
        Available.Contains(TEXT("server")) && Unavailable.Contains(TEXT("server authentication")));
    TestTrue(TEXT("An unavailable identity is clearly explained"), Unavailable.Contains(TEXT("not available")));
    TestFalse(TEXT("The availability message does not claim server authentication succeeded"), Available.Contains(TEXT("authenticated")));
    return true;
}
#endif
