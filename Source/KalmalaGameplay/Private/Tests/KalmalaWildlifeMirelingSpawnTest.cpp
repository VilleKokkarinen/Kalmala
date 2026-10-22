#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaWildlifeSpawn.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeMirelingSpawnTest,
    "Kalmala.Gameplay.Wildlife.Mireling.MossyMireShareAndHabitat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWildlifeMirelingSpawnTest::RunTest(const FString& Parameters)
{
    int32 MeadowsBoar = 0;
    int32 MeadowsDeer = 0;
    int32 MeadowsRat = 0;
    int32 ElderwoodBoar = 0;
    int32 ElderwoodDeer = 0;
    int32 ElderwoodBadger = 0;
    int32 ElderwoodMoonBear = 0;
    int32 LakeBoar = 0;
    int32 LakeDeer = 0;
    int32 LakeViper = 0;
    int32 LakeGiantSnake = 0;
    int32 MirelingSlots = 0;
    int32 DeathAdderSlots = 0;
    int32 MosquitoSlots = 0;
    int32 PeatHuskSlots = 0;
    for (uint64 Seed = 0; Seed < 1000; ++Seed)
    {
        EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Mireling;
        TestTrue(TEXT("Meadows seed retains a wildlife candidate"),
            AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(Seed, EKalmalaBiome::Meadows, Archetype));
        TestTrue(TEXT("Meadows selects only Boar, Deer, or Rat"),
            Archetype == EKalmalaWildlifeArchetype::Boar || Archetype == EKalmalaWildlifeArchetype::Deer
                || Archetype == EKalmalaWildlifeArchetype::Rat);
        if (Archetype == EKalmalaWildlifeArchetype::Boar) ++MeadowsBoar;
        else if (Archetype == EKalmalaWildlifeArchetype::Deer) ++MeadowsDeer;
        else if (Archetype == EKalmalaWildlifeArchetype::Rat) ++MeadowsRat;

        TestTrue(TEXT("Elderwood seed retains a wildlife candidate"),
            AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(Seed, EKalmalaBiome::Elderwood, Archetype));
        TestTrue(TEXT("Elderwood selects only its reviewed roster"),
            Archetype == EKalmalaWildlifeArchetype::Boar || Archetype == EKalmalaWildlifeArchetype::Deer
                || Archetype == EKalmalaWildlifeArchetype::Badger
                || Archetype == EKalmalaWildlifeArchetype::MoonBear);
        if (Archetype == EKalmalaWildlifeArchetype::Boar) ++ElderwoodBoar;
        else if (Archetype == EKalmalaWildlifeArchetype::Deer) ++ElderwoodDeer;
        else if (Archetype == EKalmalaWildlifeArchetype::Badger) ++ElderwoodBadger;
        else if (Archetype == EKalmalaWildlifeArchetype::MoonBear) ++ElderwoodMoonBear;

        TestTrue(TEXT("Shimmering Lakes seed retains a wildlife candidate"),
            AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(Seed, EKalmalaBiome::ShimmeringLakes, Archetype));
        TestTrue(TEXT("Shimmering Lakes selects only its reviewed roster"),
            Archetype == EKalmalaWildlifeArchetype::Boar || Archetype == EKalmalaWildlifeArchetype::Deer
                || Archetype == EKalmalaWildlifeArchetype::Viper
                || Archetype == EKalmalaWildlifeArchetype::GiantSnake);
        if (Archetype == EKalmalaWildlifeArchetype::Boar) ++LakeBoar;
        else if (Archetype == EKalmalaWildlifeArchetype::Deer) ++LakeDeer;
        else if (Archetype == EKalmalaWildlifeArchetype::Viper) ++LakeViper;
        else if (Archetype == EKalmalaWildlifeArchetype::GiantSnake) ++LakeGiantSnake;

        const bool bMireEligible = AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(
            Seed, EKalmalaBiome::MossyMire, Archetype);
        if (Seed % 10ull < 5ull)
        {
            TestTrue(TEXT("Mossy Mire's first half selects its frequent Mireling"), bMireEligible);
            TestEqual(TEXT("The 50% Mossy Mire share is Mireling"), Archetype,
                EKalmalaWildlifeArchetype::Mireling);
            ++MirelingSlots;
        }
        else if (Seed % 10ull < 7ull)
        {
            TestTrue(TEXT("Death Adder occupies its two Mossy Mire slots"), bMireEligible);
            TestEqual(TEXT("Mossy Mire common slots select Death Adder"), Archetype,
                EKalmalaWildlifeArchetype::DeathAdder);
            ++DeathAdderSlots;
        }
        else if (Seed % 10ull < 9ull)
        {
            TestTrue(TEXT("Both dedicated Mosquito swarm slots are available"), bMireEligible);
            TestEqual(TEXT("Mosquito swarm owns its reviewed twenty percent share"), Archetype,
                EKalmalaWildlifeArchetype::MosquitoSwarm);
            ++MosquitoSlots;
        }
        else
        {
            TestTrue(TEXT("Peat Husk occupies the Mire hard-enemy slot"), bMireEligible);
            TestEqual(TEXT("The remaining slot selects Peat Husk"), Archetype,
                EKalmalaWildlifeArchetype::PeatHusk);
            ++PeatHuskSlots;
        }
    }

    TestEqual(TEXT("Meadows keeps its 50% Boar share"), MeadowsBoar, 500);
    TestEqual(TEXT("Meadows keeps its 40% Deer share"), MeadowsDeer, 400);
    TestEqual(TEXT("Meadows keeps its 10% Rat share"), MeadowsRat, 100);
    TestEqual(TEXT("Elderwood keeps 30% Boar"), ElderwoodBoar, 300);
    TestEqual(TEXT("Elderwood keeps 20% Deer"), ElderwoodDeer, 200);
    TestEqual(TEXT("Elderwood keeps its 40% Badger share"), ElderwoodBadger, 400);
    TestEqual(TEXT("Elderwood keeps its 10% Moon Bear share"), ElderwoodMoonBear, 100);
    TestEqual(TEXT("Shimmering Lakes keeps 30% Boar"), LakeBoar, 300);
    TestEqual(TEXT("Shimmering Lakes keeps 20% Deer"), LakeDeer, 200);
    TestEqual(TEXT("Shimmering Lakes keeps its 40% Viper share"), LakeViper, 400);
    TestEqual(TEXT("Shimmering Lakes keeps its 10% Giant Snake share"), LakeGiantSnake, 100);
    TestEqual(TEXT("Mireling occupies exactly half the Mossy Mire slots"), MirelingSlots, 500);
    TestEqual(TEXT("Death Adder occupies twenty percent of Mossy Mire slots"), DeathAdderSlots, 200);
    TestEqual(TEXT("Mosquito swarm occupies twenty percent of Mossy Mire slots"), MosquitoSlots, 200);
    TestEqual(TEXT("Peat Husk occupies ten percent of the Mossy Mire roster"), PeatHuskSlots, 100);

    for (const EKalmalaBiome OtherBiome : { EKalmalaBiome::Meadows, EKalmalaBiome::Elderwood,
             EKalmalaBiome::ShimmeringLakes, EKalmalaBiome::FreezingTundra,
             EKalmalaBiome::ThunderMountains, EKalmalaBiome::Ocean })
    {
        for (uint64 Seed = 0; Seed < 100; ++Seed)
        {
            EKalmalaWildlifeArchetype OtherArchetype = EKalmalaWildlifeArchetype::Mireling;
            if (AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(Seed, OtherBiome, OtherArchetype))
            {
                TestFalse(TEXT("Mireling cannot be selected outside Mossy Mire"),
                    OtherArchetype == EKalmalaWildlifeArchetype::Mireling);
                TestFalse(TEXT("Death Adder cannot be selected outside Mossy Mire"),
                    OtherArchetype == EKalmalaWildlifeArchetype::DeathAdder);
                TestFalse(TEXT("Mosquito swarm cannot be selected outside Mossy Mire"),
                    OtherArchetype == EKalmalaWildlifeArchetype::MosquitoSwarm);
                TestFalse(TEXT("Peat Husk cannot be selected outside Mossy Mire"),
                    OtherArchetype == EKalmalaWildlifeArchetype::PeatHusk);
            }
        }
    }

    TestTrue(TEXT("Dry Mossy Mire is a valid Mireling habitat"),
        AKalmalaWildlifeSpawn::IsMossyMireSpawnHabitatAllowed(EKalmalaBiome::MossyMire, false));
    TestFalse(TEXT("Water is not a valid Mireling spawn surface"),
        AKalmalaWildlifeSpawn::IsMossyMireSpawnHabitatAllowed(EKalmalaBiome::MossyMire, true));
    TestFalse(TEXT("Dry terrain in another biome is not a Mireling habitat"),
        AKalmalaWildlifeSpawn::IsMossyMireSpawnHabitatAllowed(EKalmalaBiome::Meadows, false));
    TestEqual(TEXT("The existing Mireling leaves baseline Bone Fragment loot unchanged"),
        AKalmalaWildlifeSpawn::GetBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::Mireling), 0);
    return true;
}

#endif
