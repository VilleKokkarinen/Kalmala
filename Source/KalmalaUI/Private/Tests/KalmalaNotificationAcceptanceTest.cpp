#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaSkillNotice.h"
#include "KalmalaNotificationSubsystem.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaNotificationAcceptanceTest, "Kalmala.UI.Notifications.CombinedPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaNotificationAcceptanceTest::RunTest(const FString& Parameters)
{
    FKalmalaSkillProgressionLedger Skills;
    Skills.Initialize();
    FKalmalaSkillNoticeQueue Queue;
    TestTrue(TEXT("Initial skill state establishes a silent baseline"), Queue.Observe(Skills.Skills, 4));

    TArray<FKalmalaItemGainReceipt> Gains{{1, TEXT("Wood"), 1}};
    TestTrue(TEXT("Existing gains establish a silent baseline"), Queue.ObserveGains(Gains, 4));
    TestTrue(TEXT("Existing discovery establishes a silent baseline"), Queue.ObserveDiscovery(
        7, EKalmalaDiscoveryFeedback::LandmarkFound, TEXT("Already known"), 4));
    TestEqual(TEXT("All three initial sources stay silent"), Queue.GetRows().Num(), 0);

    for (int32 Award = 0; Award < 4; ++Award)
        TestTrue(TEXT("Accepted skill action applies through the existing server contract"),
            Skills.AwardExperienceFromServer(EKalmalaSkill::Crafting, true, true, 25));
    TestTrue(TEXT("Reached level enters the shared queue"), Queue.Observe(Skills.Skills, 4));

    Gains.Add({2, TEXT("Wood"), 2});
    TestTrue(TEXT("Accepted item receipt enters the shared queue"), Queue.ObserveGains(Gains, 4));
    TestTrue(TEXT("Accepted discovery enters the shared queue"), Queue.ObserveDiscovery(
        8, EKalmalaDiscoveryFeedback::ScrollFound, TEXT("Scroll found: Field Notes"), 4));
    TestEqual(TEXT("Mixed queue respects the three-row bound"), Queue.GetRows().Num(), FKalmalaSkillNoticeQueue::MaxRows);
    TestTrue(TEXT("Rows retain skill, item, discovery order"),
        Queue.GetRows()[0].Skill == EKalmalaSkill::Crafting
        && Queue.GetRows()[0].ItemId.IsNone() && Queue.GetRows()[0].DiscoveryText.IsEmpty()
        && Queue.GetRows()[1].ItemId == FName(TEXT("Wood"))
        && Queue.GetRows()[2].DiscoveryText == TEXT("Scroll found: Field Notes"));

    auto* Widget = NewObject<UKalmalaNotificationWidget>();
    Widget->SetNotices(Queue.GetRows(), 150, 1);
    const FString Combined = Widget->GetPresentationText();
    TestTrue(TEXT("Combined text includes reached level"), Combined.Contains(TEXT("Crafting reached level 2")));
    TestTrue(TEXT("Combined text includes accepted item quantity"), Combined.Contains(TEXT("Gained 2 Wood")));
    TestTrue(TEXT("Combined text includes accepted discovery label"), Combined.Contains(TEXT("Scroll found: Field Notes")));
    TestFalse(TEXT("Combined notification remains passive at high contrast and 150 percent"), Widget->IsFocusable());

    Queue.Tick(2);
    TestTrue(TEXT("Repeated skill snapshot is accepted"), Queue.Observe(Skills.Skills, 4));
    TestTrue(TEXT("Repeated item receipt buffer is accepted"), Queue.ObserveGains(Gains, 4));
    TestTrue(TEXT("Repeated discovery serial is accepted"), Queue.ObserveDiscovery(
        8, EKalmalaDiscoveryFeedback::ScrollFound, TEXT("Scroll found: Field Notes"), 4));
    for (const FKalmalaSkillNotice& Row : Queue.GetRows())
        TestEqual(TEXT("Unchanged refresh does not renew any source"), Row.Remaining, 2.0f);

    Queue.Reset();
    TestTrue(TEXT("Reconnect skill state is silently baselined"), Queue.Observe(Skills.Skills, 4));
    TestTrue(TEXT("Reconnect gain receipts are silently baselined"), Queue.ObserveGains(Gains, 4));
    TestTrue(TEXT("Reconnect discovery is silently baselined"), Queue.ObserveDiscovery(
        8, EKalmalaDiscoveryFeedback::ScrollFound, TEXT("Scroll found: Field Notes"), 4));
    TestEqual(TEXT("Combined reconnect baseline never replays prior sources"), Queue.GetRows().Num(), 0);

    FKalmalaSkillNoticeQueue OtherOwner;
    OtherOwner.Observe(Skills.Skills, 4);
    OtherOwner.ObserveGains({}, 4);
    OtherOwner.ObserveDiscovery(0, EKalmalaDiscoveryFeedback::Unavailable, FString(), 4);
    TestEqual(TEXT("Independent owner starts with no peer-private notices"), OtherOwner.GetRows().Num(), 0);
    return true;
}
#endif
