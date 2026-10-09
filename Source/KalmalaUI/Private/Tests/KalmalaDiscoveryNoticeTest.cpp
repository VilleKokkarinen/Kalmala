#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaSkillNotice.h"
#include "KalmalaNotificationSubsystem.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaDiscoveryNoticeTest, "Kalmala.UI.Notifications.Discoveries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaDiscoveryNoticeTest::RunTest(const FString& Parameters)
{
    FKalmalaSkillNoticeQueue Queue;
    TestTrue(TEXT("Initial discovery snapshot accepted"), Queue.ObserveDiscovery(
        7, EKalmalaDiscoveryFeedback::LandmarkFound, TEXT("Existing discovery"), 4));
    TestEqual(TEXT("Existing discovery is silently baselined"), Queue.GetRows().Num(), 0);
    Queue.ObserveDiscovery(7, EKalmalaDiscoveryFeedback::LandmarkFound, TEXT("Existing discovery"), 4);
    TestEqual(TEXT("Unchanged feedback does not replay"), Queue.GetRows().Num(), 0);

    Queue.ObserveDiscovery(8, EKalmalaDiscoveryFeedback::AlreadyFound, TEXT("Already discovered"), 4);
    TestEqual(TEXT("Already-found acknowledgement is presented"), Queue.GetRows().Num(), 1);
    TestEqual(TEXT("Already-found label is retained"), Queue.GetRows()[0].DiscoveryText, FString(TEXT("Already discovered")));
    Queue.ObserveDiscovery(9, EKalmalaDiscoveryFeedback::Unavailable, TEXT("Pack full"), 4);
    TestEqual(TEXT("Unavailable acknowledgement is presented"), Queue.GetRows().Num(), 2);
    TestEqual(TEXT("Unavailable label is retained"), Queue.GetRows()[1].DiscoveryText, FString(TEXT("Pack full")));
    Queue.ObserveDiscovery(10, EKalmalaDiscoveryFeedback::LandmarkFound, TEXT("Three-Run Rillstone found"), 4);
    TestEqual(TEXT("Accepted landmark remains in the bounded queue"), Queue.GetRows().Num(), FKalmalaSkillNoticeQueue::MaxRows);
    TestEqual(TEXT("Accepted server label is presented"), Queue.GetRows()[2].DiscoveryText, FString(TEXT("Three-Run Rillstone found")));
    FKalmalaSkillProgressionLedger Skills;
    Skills.Initialize();
    Queue.Observe(Skills.Skills, 4);
    TestEqual(TEXT("Skill baseline preserves discovery notices"), Queue.GetRows().Num(), 3);

    auto* Widget = NewObject<UKalmalaNotificationWidget>();
    Widget->SetNotices(Queue.GetRows(), 100, 0);
    TestTrue(TEXT("Widget presents discovery text"), Widget->GetPresentationText().Contains(TEXT("Three-Run Rillstone found")));
    TestFalse(TEXT("Discovery notification remains passive"), Widget->IsFocusable());
    Queue.Tick(2);
    Queue.ObserveDiscovery(10, EKalmalaDiscoveryFeedback::LandmarkFound, TEXT("Three-Run Rillstone found"), 4);
    TestEqual(TEXT("Refresh does not renew discovery lifetime"), Queue.GetRows()[2].Remaining, 2.f);

    Queue.ObserveDiscovery(11, EKalmalaDiscoveryFeedback::ScrollFound, TEXT("Scroll found: Field Notes"), 4);
    TestEqual(TEXT("Distinct accepted scroll is retained within row bound"), Queue.GetRows().Num(), FKalmalaSkillNoticeQueue::MaxRows);
    TestEqual(TEXT("Scroll copy stays owner-provided"), Queue.GetRows().Last().DiscoveryText, FString(TEXT("Scroll found: Field Notes")));
    Queue.ObserveDiscovery(12, EKalmalaDiscoveryFeedback::LandmarkFound, TEXT(""), 4);
    TestEqual(TEXT("Empty landmark label has safe fallback"), Queue.GetRows().Last().DiscoveryText, FString(TEXT("Discovery found")));
    Queue.ObserveDiscovery(13, EKalmalaDiscoveryFeedback::ScrollFound, FString::ChrN(80, TCHAR('x')), 4);
    TestEqual(TEXT("Discovery label stays bounded"), Queue.GetRows().Last().DiscoveryText.Len(), 48);
    TestEqual(TEXT("Mixed queue remains bounded"), Queue.GetRows().Num(), FKalmalaSkillNoticeQueue::MaxRows);

    Queue.Tick(4);
    Queue.ObserveDiscovery(13, EKalmalaDiscoveryFeedback::ScrollFound, FString::ChrN(80, TCHAR('x')), 4);
    TestEqual(TEXT("Expired feedback never replays"), Queue.GetRows().Num(), 0);
    Queue.Reset();
    Queue.ObserveDiscovery(13, EKalmalaDiscoveryFeedback::ScrollFound, TEXT("Already known"), 4);
    TestEqual(TEXT("Reconnect baseline suppresses existing discovery"), Queue.GetRows().Num(), 0);
    Queue.ObserveDiscovery(14, EKalmalaDiscoveryFeedback::AlreadyFound, TEXT("Already discovered"), 4);
    TestEqual(TEXT("New already-found acknowledgement remains visible"), Queue.GetRows().Num(), 1);
    Queue.ObserveDiscovery(15, EKalmalaDiscoveryFeedback::LandmarkFound, TEXT("Fresh discovery"), 4);
    TestEqual(TEXT("Later accepted event remains visible after prior acknowledgement"), Queue.GetRows().Num(), 2);

    FKalmalaSkillNoticeQueue OtherOwner;
    OtherOwner.ObserveDiscovery(15, EKalmalaDiscoveryFeedback::LandmarkFound, TEXT("Existing"), 4);
    TestEqual(TEXT("Other owner independently baselines"), OtherOwner.GetRows().Num(), 0);
    return true;
}
#endif
