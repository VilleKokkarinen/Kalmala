#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaSkillProgressionContract.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaSkillProgressionContractTest,
    "Kalmala.Gameplay.Progression.SkillContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaSkillProgressionContractTest::RunTest(const FString& Parameters)
{
    const TArray<EKalmalaSkill> AllowlistedSkills = FKalmalaSkillProgressionContract::GetAllowlistedSkills();
    TestEqual(TEXT("The first M7 skill set contains six bounded skills"), AllowlistedSkills.Num(), 6);
    TestFalse(TEXT("None is not an allowlisted skill"), FKalmalaSkillProgressionContract::IsKnownSkill(EKalmalaSkill::None));
    TestFalse(TEXT("Malformed enum values fail closed"), FKalmalaSkillProgressionContract::IsKnownSkill(static_cast<EKalmalaSkill>(255)));

    FKalmalaSkillProgressionLedger Ledger;
    Ledger.Initialize();
    TestTrue(TEXT("A new ledger contains only valid level-one skills"), Ledger.IsValid());
    for (const EKalmalaSkill Skill : AllowlistedSkills)
    {
        const FKalmalaSkillState* State = Ledger.Find(Skill);
        TestNotNull(TEXT("Every allowlisted skill has a server-owned state"), State);
        if (State)
        {
            TestEqual(TEXT("New skills start at zero experience"), State->Experience, 0);
            TestEqual(TEXT("New skills start at level one"), State->Level, 1);
            TestEqual(TEXT("New skills start without unlocks"), State->UnlockMask, uint8(0));
        }
    }

    TestTrue(TEXT("Only an accepted server action may award bounded experience"),
        FKalmalaSkillProgressionContract::IsExperienceAwardAllowed(true, true, EKalmalaSkill::Gathering, 25));
    TestFalse(TEXT("Client authority cannot award experience"),
        FKalmalaSkillProgressionContract::IsExperienceAwardAllowed(false, true, EKalmalaSkill::Gathering, 25));
    TestFalse(TEXT("An unaccepted action cannot award experience"),
        FKalmalaSkillProgressionContract::IsExperienceAwardAllowed(true, false, EKalmalaSkill::Gathering, 25));
    TestFalse(TEXT("Zero experience awards are rejected"),
        FKalmalaSkillProgressionContract::IsExperienceAwardAllowed(true, true, EKalmalaSkill::Gathering, 0));
    TestFalse(TEXT("Experience awards above the per-action bound are rejected"),
        FKalmalaSkillProgressionContract::IsExperienceAwardAllowed(true, true, EKalmalaSkill::Gathering, 26));
    TestFalse(TEXT("Malformed skills cannot receive experience"),
        FKalmalaSkillProgressionContract::IsExperienceAwardAllowed(true, true, static_cast<EKalmalaSkill>(255), 25));

    TestTrue(TEXT("The server can apply one accepted gathering award"),
        Ledger.AwardExperienceFromServer(EKalmalaSkill::Gathering, true, true, 25));
    const FKalmalaSkillState* Gathering = Ledger.Find(EKalmalaSkill::Gathering);
    if (TestNotNull(TEXT("Gathering state remains available after an award"), Gathering))
    {
        TestEqual(TEXT("The accepted award is recorded exactly"), Gathering->Experience, 25);
        TestEqual(TEXT("Level remains derived below the first threshold"), Gathering->Level, 1);
        TestEqual(TEXT("No unlock appears before the first threshold"), Gathering->UnlockMask, uint8(0));
    }

    for (int32 Award = 0; Award < 3; ++Award)
    {
        TestTrue(TEXT("Repeated accepted awards remain bounded and server-owned"),
            Ledger.AwardExperienceFromServer(EKalmalaSkill::Gathering, true, true, 25));
    }
    Gathering = Ledger.Find(EKalmalaSkill::Gathering);
    if (TestNotNull(TEXT("Gathering state reaches the first derived level"), Gathering))
    {
        TestEqual(TEXT("One hundred experience derives level two"), Gathering->Experience, 100);
        TestEqual(TEXT("Level two is derived from experience"), Gathering->Level, 2);
        TestEqual(TEXT("Level two grants only the first unlock tier"), Gathering->UnlockMask,
            static_cast<uint8>(EKalmalaSkillUnlock::FirstTier));
    }

    if (Gathering)
    {
        const FKalmalaSkillState BeforeRejectedAward = *Gathering;
        TestFalse(TEXT("A client or rejected action cannot mutate progression"),
            Ledger.AwardExperienceFromServer(EKalmalaSkill::Gathering, false, true, 25)
            || Ledger.AwardExperienceFromServer(EKalmalaSkill::Gathering, true, false, 25)
            || Ledger.AwardExperienceFromServer(EKalmalaSkill::Gathering, true, true, 26));
        Gathering = Ledger.Find(EKalmalaSkill::Gathering);
        if (TestNotNull(TEXT("Rejected awards leave the state available"), Gathering))
        {
            TestEqual(TEXT("Rejected awards do not change experience"), Gathering->Experience, BeforeRejectedAward.Experience);
            TestEqual(TEXT("Rejected awards do not change level"), Gathering->Level, BeforeRejectedAward.Level);
            TestEqual(TEXT("Rejected awards do not change unlocks"), Gathering->UnlockMask, BeforeRejectedAward.UnlockMask);
        }
    }

    for (int32 Award = 0; Award < 36; ++Award)
    {
        TestTrue(TEXT("Accepted awards advance progression until the cap"),
            Ledger.AwardExperienceFromServer(EKalmalaSkill::Gathering, true, true, 25));
    }
    Gathering = Ledger.Find(EKalmalaSkill::Gathering);
    if (TestNotNull(TEXT("Gathering state remains available at the cap"), Gathering))
    {
        TestEqual(TEXT("Experience is capped at the server maximum"), Gathering->Experience,
            FKalmalaSkillProgressionContract::MaxExperience);
        TestEqual(TEXT("Experience cap derives the maximum level"), Gathering->Level,
            FKalmalaSkillProgressionContract::MaxLevel);
        TestEqual(TEXT("Maximum level derives the mastery unlock"), Gathering->UnlockMask,
            static_cast<uint8>(EKalmalaSkillUnlock::FirstTier)
            | static_cast<uint8>(EKalmalaSkillUnlock::SecondTier)
            | static_cast<uint8>(EKalmalaSkillUnlock::Mastery));
        TestFalse(TEXT("A capped skill cannot be awarded more experience"),
            Ledger.AwardExperienceFromServer(EKalmalaSkill::Gathering, true, true, 25));
    }

    TestTrue(TEXT("Progression remains valid after capped awards"), Ledger.IsValid());
    TestTrue(TEXT("A different allowlisted skill progresses independently"),
        Ledger.AwardExperienceFromServer(EKalmalaSkill::Cooking, true, true, 10));
    const FKalmalaSkillState* Cooking = Ledger.Find(EKalmalaSkill::Cooking);
    if (TestNotNull(TEXT("Cooking state remains independent"), Cooking))
    {
        TestEqual(TEXT("Cooking keeps its own experience"), Cooking->Experience, 10);
        TestEqual(TEXT("Cooking keeps its own level"), Cooking->Level, 1);
    }

    return true;
}

#endif
