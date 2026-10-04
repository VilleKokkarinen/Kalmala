#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaSkillNotice.h"
#include "KalmalaNotificationSubsystem.h"
#include "KalmalaUITheme.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaSkillNoticeTest, "Kalmala.UI.Notifications.SkillLevels",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaSkillNoticeTest::RunTest(const FString& Parameters)
{
    FKalmalaSkillProgressionLedger Ledger;
    Ledger.Initialize();
    FKalmalaSkillNoticeQueue Queue;
    TestFalse(TEXT("Partial owner replication cannot establish baseline"), Queue.Observe({}, 4));
    TestTrue(TEXT("Complete owner baseline accepted"), Queue.Observe(Ledger.Skills, 4));
    TestEqual(TEXT("Initial snapshot is silent"), Queue.GetRows().Num(), 0);
    auto SetLevel = [&Ledger](EKalmalaSkill Skill, int32 Level)
    {
        auto* State = Ledger.Find(Skill);
        State->Experience = FKalmalaSkillProgressionContract::GetExperienceForLevel(Level);
        State->Level = Level;
        State->UnlockMask = FKalmalaSkillProgressionContract::GetUnlockMaskForLevel(Level);
    };
    // Award through the same authority contract used by gameplay.
    for (int32 Index = 0; Index < 4; ++Index)
        Ledger.AwardExperienceFromServer(EKalmalaSkill::Crafting, true, true, 25);
    Queue.Observe(Ledger.Skills, 4);
    TestEqual(TEXT("Accepted level increase produces one row"), Queue.GetRows().Num(), 1);
    TestEqual(TEXT("Row names the reached level"), Queue.GetRows()[0].Level, 2);
    Queue.Tick(2);
    Queue.Observe(Ledger.Skills, 4);
    TestEqual(TEXT("Repeated replication does not renew expiry"), Queue.GetRows()[0].Remaining, 2.f);
    Ledger.AwardExperienceFromServer(EKalmalaSkill::Crafting, true, true, 25);
    Queue.Observe(Ledger.Skills, 4);
    TestEqual(TEXT("XP within level stays silent"), Queue.GetRows()[0].Remaining, 2.f);
    auto Invalid = Ledger.Skills;
    Invalid[1] = Invalid[0];
    TestFalse(TEXT("Duplicate skill snapshot rejected atomically"), Queue.Observe(Invalid, 4));
    TestEqual(TEXT("Invalid snapshot preserves active feedback"), Queue.GetRows()[0].Remaining, 2.f);
    SetLevel(EKalmalaSkill::Crafting, 3);
    Queue.Observe(Ledger.Skills, 4);
    TestEqual(TEXT("Same skill burst coalesces"), Queue.GetRows().Num(), 1);
    TestEqual(TEXT("Coalescing retains latest level"), Queue.GetRows()[0].Level, 3);
    TestEqual(TEXT("New level renews bounded lifetime"), Queue.GetRows()[0].Remaining, 4.f);
    SetLevel(EKalmalaSkill::Gathering, 2); SetLevel(EKalmalaSkill::Woodcutting, 2);
    SetLevel(EKalmalaSkill::Mining, 2); SetLevel(EKalmalaSkill::Cooking, 2);
    Queue.Observe(Ledger.Skills, 4);
    TestEqual(TEXT("Burst remains bounded"), Queue.GetRows().Num(), 3);
    TestTrue(TEXT("Overflow evicts oldest row deterministically"), Queue.GetRows()[0].Skill == EKalmalaSkill::Woodcutting);
    auto* Widget = NewObject<UKalmalaNotificationWidget>();
    Widget->SetNotices(Queue.GetRows(), 150, 1);
    TestTrue(TEXT("Widget contains meaningful skill text"), Widget->GetPresentationText().Contains(TEXT("Cooking reached level 2")));
    TestFalse(TEXT("Passive notification cannot take focus"), Widget->IsFocusable());
    Widget->SetNotices({}, 100, 0);
    TestTrue(TEXT("Clearing removes stale presentation"), Widget->GetPresentationText().IsEmpty());
    Queue.Tick(4); Queue.Observe(Ledger.Skills, 4);
    TestEqual(TEXT("Expired snapshot never replays"), Queue.GetRows().Num(), 0);
    Queue.Reset(); Queue.Observe(Ledger.Skills, 4);
    TestEqual(TEXT("Reconnect high-level baseline is silent"), Queue.GetRows().Num(), 0);
    SetLevel(EKalmalaSkill::Crafting, 4); Queue.Observe(Ledger.Skills, 999);
    TestEqual(TEXT("Lifetime cannot exceed ten seconds"), Queue.GetRows()[0].Remaining, 10.f);
    SetLevel(EKalmalaSkill::Crafting, 1); Queue.Observe(Ledger.Skills, 4);
    TestEqual(TEXT("Progression reset clears session feedback"), Queue.GetRows().Num(), 0);
    FKalmalaSkillNoticeQueue OtherOwner; OtherOwner.Observe(Ledger.Skills, 4);
    SetLevel(EKalmalaSkill::Survival, 2); Queue.Observe(Ledger.Skills, 4);
    TestEqual(TEXT("Owner queues remain isolated"), OtherOwner.GetRows().Num(), 0);
    FConfigFile Config;
    Config.ProcessInputFileContents(TEXT("[Kalmala.UI.Theme]\nNotificationLifetime=6\n"), TEXT("Notices.ini"));
    TestEqual(TEXT("Theme controls lifetime"), FKalmalaUITheme::FromConfig(Config).NotificationLifetime, 6.f);
    Config.ProcessInputFileContents(TEXT("[Kalmala.UI.Theme]\nNotificationLifetime=999\n"), TEXT("Notices.ini"));
    TestEqual(TEXT("Unsafe theme duration uses default"), FKalmalaUITheme::FromConfig(Config).NotificationLifetime, 4.f);
    return true;
}
#endif
