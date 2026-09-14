#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaPlayerStatusComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPlayerWetStatusTest, "Kalmala.Gameplay.Status.Wet",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaPlayerWetStatusTest::RunTest(const FString& Parameters)
{
    TArray<FKalmalaPlayerStatusEntry> Entries;
    UKalmalaPlayerStatusComponent::ApplyWet(Entries);
    TestEqual(TEXT("Wet creates one bounded status entry"), Entries.Num(), 1);
    TestEqual(TEXT("Wet starts at its configured maximum"), Entries[0].RemainingSeconds, UKalmalaPlayerStatusComponent::WetMaximumSeconds);

    UKalmalaPlayerStatusComponent::Advance(Entries, 10.0f);
    TestEqual(TEXT("Server time advances remaining duration"), Entries[0].RemainingSeconds, 110.0f);
    UKalmalaPlayerStatusComponent::ApplyWet(Entries);
    TestEqual(TEXT("Reapplication clamps instead of stacking duration"), Entries[0].RemainingSeconds, UKalmalaPlayerStatusComponent::WetMaximumSeconds);

    FKalmalaPlayerStatusEntry Malformed;
    Malformed.StatusId = UKalmalaPlayerStatusComponent::WetStatusId;
    Malformed.RemainingSeconds = NAN;
    Entries.Add(Malformed);
    UKalmalaPlayerStatusComponent::Advance(Entries, -1.0f);
    TestEqual(TEXT("Malformed and duplicate status data fails closed"), Entries.Num(), 1);
    UKalmalaPlayerStatusComponent::Advance(Entries, 1000.0f);
    TestEqual(TEXT("Expired Wet is removed"), Entries.Num(), 0);
    TestEqual(TEXT("Rain trigger remains explicit"), UKalmalaPlayerStatusComponent::UnroofedRainTriggerSeconds, 10.0f);
    return true;
}

#endif
