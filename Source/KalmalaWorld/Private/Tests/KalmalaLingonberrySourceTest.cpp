#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "KalmalaBiomeContentContract.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaLingonberrySourceContractTest,
    "Kalmala.World.Content.LingonberrySource",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaLingonberrySourceContractTest::RunTest(const FString& Parameters)
{
    const FName SourceId(TEXT("meadows-wild-lingonberry-shrub"));
    TestTrue(TEXT("The server-recognized Meadow shrub is a valid source"),
        FKalmalaBiomeContentContract::IsValidGatheringSourceId(SourceId));
    TestEqual(TEXT("The shrub resolves to its dedicated presentation"),
        FKalmalaBiomeContentContract::GetGatheringPresentationId(SourceId),
        FName(TEXT("lingonberry-shrub-harvest")));
    TestTrue(TEXT("The shrub uses no-tool hand gathering"),
        FKalmalaBiomeContentContract::IsHandGatherableSourceId(SourceId));
    TestFalse(TEXT("Unknown source IDs cannot enter the hand-gather route"),
        FKalmalaBiomeContentContract::IsHandGatherableSourceId(TEXT("forged-lingonberry-source")));
    return true;
}

#endif
