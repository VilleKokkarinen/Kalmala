#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaSkillProgressionComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaSkillProgressionReplicationTest,
    "Kalmala.Gameplay.Progression.ReplicationContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaSkillProgressionReplicationTest::RunTest(const FString& Parameters)
{
    FKalmalaSkillProgressionLedger Ledger;
    Ledger.Initialize();

    TestTrue(TEXT("A fresh server ledger can produce a valid peer presentation"),
        UKalmalaSkillProgressionComponent::BuildPeerPresentation(Ledger).IsValid());

    for (int32 Award = 0; Award < 4; ++Award)
    {
        TestTrue(TEXT("Accepted server awards remain available to the replication seam"),
            Ledger.AwardExperienceFromServer(EKalmalaSkill::Gathering, true, true, 25));
    }

    const FKalmalaSkillPeerPresentationState Presentation =
        UKalmalaSkillProgressionComponent::BuildPeerPresentation(Ledger);
    TestTrue(TEXT("Peer presentation remains valid after a derived unlock"), Presentation.IsValid());
    TestEqual(TEXT("Peer presentation exposes the highest derived level"), Presentation.HighestLevel, uint8(2));
    TestEqual(TEXT("Peer presentation exposes only the derived unlock mask"), Presentation.HighestUnlockMask,
        static_cast<uint8>(EKalmalaSkillUnlock::FirstTier));

    const FKalmalaSkillState* Gathering = Ledger.Find(EKalmalaSkill::Gathering);
    if (TestNotNull(TEXT("The server ledger retains detailed owner state"), Gathering))
    {
        TestEqual(TEXT("Owner detail retains experience"), Gathering->Experience, 100);
        TestEqual(TEXT("Owner detail retains the derived level"), Gathering->Level, 2);
        TestEqual(TEXT("Owner detail retains the derived unlocks"), Gathering->UnlockMask,
            static_cast<uint8>(EKalmalaSkillUnlock::FirstTier));
    }

    TestTrue(TEXT("The peer view contains no per-skill detail or experience field"),
        Presentation.HighestLevel == 2 && Presentation.HighestUnlockMask != 0);

    const FKalmalaSkillPeerPresentationState InvalidPresentation{
        2,
        static_cast<uint8>(EKalmalaSkillUnlock::Mastery)};
    TestFalse(TEXT("A client-authored mismatched unlock mask fails closed"), InvalidPresentation.IsValid());

    return true;
}

#endif
