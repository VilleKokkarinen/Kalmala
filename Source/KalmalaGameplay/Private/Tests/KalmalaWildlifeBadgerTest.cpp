#include "KalmalaWildlifeSpawn.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeBadgerContractTest,
    "Kalmala.Gameplay.Wildlife.Badger.SpawnAndClawReach",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWildlifeBadgerContractTest::RunTest(const FString& Parameters)
{
    int32 ExistingRosterSlots = 0;
    int32 BadgerSlots = 0;
    int32 ReservedHardSlots = 0;
    for (uint64 Seed = 0; Seed < 1000; ++Seed)
    {
        EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Mireling;
        const bool bEligible = AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(
            Seed, EKalmalaBiome::Elderwood, Archetype);
        const uint64 Slot = Seed % 10ull;
        if (Slot <= 4)
        {
            TestTrue(TEXT("Existing Elderwood candidate slots stay populated"), bEligible);
            TestFalse(TEXT("Existing roster slots do not select Badger"), Archetype == EKalmalaWildlifeArchetype::Badger);
            ++ExistingRosterSlots;
        }
        else if (Slot <= 8)
        {
            TestTrue(TEXT("Badger common slots stay populated"), bEligible);
            TestEqual(TEXT("Badger occupies the 40% common share"), Archetype, EKalmalaWildlifeArchetype::Badger);
            ++BadgerSlots;
        }
        else
        {
            TestFalse(TEXT("Unimplemented hard-enemy share remains empty"), bEligible);
            ++ReservedHardSlots;
        }

        if (AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(
                Seed, EKalmalaBiome::Meadows, Archetype))
        {
            TestFalse(TEXT("Badger remains Elderwood-only"), Archetype == EKalmalaWildlifeArchetype::Badger);
        }
    }

    TestEqual(TEXT("Elderwood existing roster slot count"), ExistingRosterSlots, 500);
    TestEqual(TEXT("Elderwood Badger slot count"), BadgerSlots, 400);
    TestEqual(TEXT("Reserved Elderwood hard slot count"), ReservedHardSlots, 100);
    TestEqual(TEXT("Badger warns within three metres"),
        AKalmalaWildlifeSpawn::BadgerAttackTriggerCentimetres, 300.0f);
    TestEqual(TEXT("Badger claws use the existing wildlife hit bound"),
        AKalmalaWildlifeSpawn::BadgerClawReachCentimetres, 180.0f);
    TestEqual(TEXT("Badger attack warning duration"),
        AKalmalaWildlifeSpawn::BadgerTelegraphSeconds, 0.5f);
    TestEqual(TEXT("Badger attack recovery duration"),
        AKalmalaWildlifeSpawn::BadgerRecoverySeconds, 0.8f);
    TestEqual(TEXT("Badger pursuit remains within 25 metres"),
        AKalmalaWildlifeSpawn::BadgerSpawnLeashCentimetres, 2500.0f);
    TestEqual(TEXT("Badger baseline Bone Fragment reward quantity is one"),
        AKalmalaWildlifeSpawn::GetDirectBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::Badger), 1);
    TestEqual(TEXT("Rat stays outside the direct reward path"),
        AKalmalaWildlifeSpawn::GetDirectBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::Rat), 0);
    TestEqual(TEXT("Giant Snake keeps its existing two-fragment reward"),
        AKalmalaWildlifeSpawn::GetDirectBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::GiantSnake), 2);
    TestEqual(TEXT("Unimplemented Moon Bear rewards remain disabled"),
        AKalmalaWildlifeSpawn::GetDirectBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::MoonBear), 0);
    TestTrue(TEXT("Server accepts an in-range claw"),
        AKalmalaWildlifeSpawn::IsBadgerClawInReach(true, true, 180.0f));
    TestFalse(TEXT("Claw reach rejects targets beyond 180 cm"),
        AKalmalaWildlifeSpawn::IsBadgerClawInReach(true, true, 180.01f));
    TestFalse(TEXT("Claw reach rejects negative distance"),
        AKalmalaWildlifeSpawn::IsBadgerClawInReach(true, true, -1.0f));
    TestFalse(TEXT("Claw reach rejects non-finite distance"),
        AKalmalaWildlifeSpawn::IsBadgerClawInReach(true, true, TNumericLimits<float>::QuietNaN()));
    TestFalse(TEXT("Client cannot author Badger damage reach"),
        AKalmalaWildlifeSpawn::IsBadgerClawInReach(false, true, 100.0f));
    TestFalse(TEXT("Invalid target cannot receive a Badger claw"),
        AKalmalaWildlifeSpawn::IsBadgerClawInReach(true, false, 100.0f));
    return true;
}
