#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaMenuFlow.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaMenuFlowTest, "Kalmala.UI.MenuFlow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaMenuFlowTest::RunTest(const FString& Parameters)
{
    FKalmalaMenuFlow Flow;
    uint64 First = 0, Second = 0, Duplicate = 99;
    int32 TravelCalls = 0;
    TestTrue(TEXT("Inactive until explicitly activated"), Flow.GetScreen() == EKalmalaMenuScreen::Inactive);
    TestTrue(TEXT("Open menu and join"), Flow.Activate() && Flow.Navigate(EKalmalaMenuScreen::Join));
    TestTrue(TEXT("Start connection"), Flow.TryBegin(EKalmalaMenuRequest::Connect, First) && First != 0);
    TestFalse(TEXT("Duplicate submission rejected"), Flow.TryBegin(EKalmalaMenuRequest::Connect, Duplicate));
    TestTrue(TEXT("Rejected submission clears output"), Duplicate == 0);
    TestFalse(TEXT("Navigation locked while pending"), Flow.Navigate(EKalmalaMenuScreen::Home));
    TestTrue(TEXT("Cancel restores origin"), Flow.Cancel(First) && Flow.GetScreen() == EKalmalaMenuScreen::Join);
    TestTrue(TEXT("Retry uses a new token"), Flow.TryBegin(EKalmalaMenuRequest::Connect, Second) && Second > First);
    TestFalse(TEXT("Old callback cannot dispatch"), Flow.TryCommitTravel(First, [&] { ++TravelCalls; }));
    TestFalse(TEXT("Old callback cannot fail the retry"), Flow.CompleteFailure(First));
    TestTrue(TEXT("Retry remains connecting"), Flow.GetScreen() == EKalmalaMenuScreen::Connecting);
    TestTrue(TEXT("Current callback dispatches once"), Flow.TryCommitTravel(Second, [&]
    {
        ++TravelCalls;
        TestFalse(TEXT("Reentrant callback cannot dispatch"), Flow.TryCommitTravel(Second, [&] { ++TravelCalls; }));
        TestFalse(TEXT("Committed travel cannot be canceled"), Flow.Cancel(Second));
    }));
    TestFalse(TEXT("Duplicate callback cannot dispatch"), Flow.TryCommitTravel(Second, [&] { ++TravelCalls; }));
    TestTrue(TEXT("Exactly one dispatch"), TravelCalls == 1);
    TestTrue(TEXT("Success enters gameplay"), Flow.CompleteSuccess(Second) && Flow.GetScreen() == EKalmalaMenuScreen::Gameplay);
    TestFalse(TEXT("Late failure cannot change gameplay"), Flow.CompleteFailure(Second));
    Flow.Shutdown();
    TestTrue(TEXT("Reactivate and start launch"), Flow.Activate() && Flow.Navigate(EKalmalaMenuScreen::WorldDetails)
        && Flow.TryBegin(EKalmalaMenuRequest::Launch, First) && First > Second);
    Flow.Shutdown();
    TestFalse(TEXT("Teardown invalidates callback"), Flow.TryCommitTravel(First, [&] { ++TravelCalls; }));
    TestTrue(TEXT("Teardown leaves inactive"), Flow.GetScreen() == EKalmalaMenuScreen::Inactive && TravelCalls == 1);
    TestTrue(TEXT("Reactivation preserves token sequence"), Flow.Activate() && Flow.Navigate(EKalmalaMenuScreen::Join)
        && Flow.TryBegin(EKalmalaMenuRequest::Connect, Second) && Second > First);
    TestFalse(TEXT("Old callback cannot alter reactivated flow"), Flow.CompleteFailure(First));
    TestTrue(TEXT("Failure restores origin"), Flow.CompleteFailure(Second) && Flow.GetScreen() == EKalmalaMenuScreen::Join);
    TestTrue(TEXT("Can retry after failure"), Flow.TryBegin(EKalmalaMenuRequest::Connect, Second));
    TestTrue(TEXT("Synchronous completion is safe"), Flow.TryCommitTravel(Second, [&]
    {
        ++TravelCalls;
        TestTrue(TEXT("Reentrant success completes once"), Flow.CompleteSuccess(Second));
        TestFalse(TEXT("Reentrant failure is stale"), Flow.CompleteFailure(Second));
    }));
    TestTrue(TEXT("Synchronous completion retains gameplay"), Flow.GetScreen() == EKalmalaMenuScreen::Gameplay && TravelCalls == 2);
    FKalmalaMenuFlow Other;
    TestTrue(TEXT("A separate local flow starts inactive"), Other.GetScreen() == EKalmalaMenuScreen::Inactive);
    return true;
}
#endif
