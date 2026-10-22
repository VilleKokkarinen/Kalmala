#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaWildlifeSpawn.h"
#include "KalmalaPlayerStatusComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeMosquitoContractTest,
    "Kalmala.Gameplay.Wildlife.MosquitoSwarm.MireContactAndCounters",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWildlifeMosquitoContractTest::RunTest(const FString& Parameters)
{
    int32 MirelingSlots = 0;
    int32 DeathAdderSlots = 0;
    int32 MosquitoSlots = 0;
    int32 ReservedPeatSlots = 0;
    for (uint64 Seed = 0; Seed < 1000; ++Seed)
    {
        EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Mireling;
        const bool bEligible = AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(
            Seed, EKalmalaBiome::MossyMire, Archetype);
        if (Seed % 10ull < 5ull)
        {
            TestTrue(TEXT("Mireling retains its fifty-percent share"), bEligible);
            TestEqual(TEXT("The first five slots remain Mirelings"), Archetype,
                EKalmalaWildlifeArchetype::Mireling);
            ++MirelingSlots;
        }
        else if (Seed % 10ull < 7ull)
        {
            TestTrue(TEXT("Death Adder retains two Mire slots"), bEligible);
            TestEqual(TEXT("Slots five and six remain Death Adders"), Archetype,
                EKalmalaWildlifeArchetype::DeathAdder);
            ++DeathAdderSlots;
        }
        else if (Seed % 10ull < 9ull)
        {
            TestTrue(TEXT("Mosquito swarm receives two Mire slots"), bEligible);
            TestEqual(TEXT("Slots seven and eight select the swarm"), Archetype,
                EKalmalaWildlifeArchetype::MosquitoSwarm);
            ++MosquitoSlots;
        }
        else
        {
            TestFalse(TEXT("Peat Husk remains the reserved hard slot"), bEligible);
            ++ReservedPeatSlots;
        }
    }
    TestEqual(TEXT("Mireling slot count"), MirelingSlots, 500);
    TestEqual(TEXT("Death Adder slot count"), DeathAdderSlots, 200);
    TestEqual(TEXT("Mosquito slot count"), MosquitoSlots, 200);
    TestEqual(TEXT("Peat Husk slot count remains reserved"), ReservedPeatSlots, 100);

    for (const EKalmalaBiome Biome : { EKalmalaBiome::Meadows, EKalmalaBiome::Elderwood,
             EKalmalaBiome::ShimmeringLakes, EKalmalaBiome::FreezingTundra,
             EKalmalaBiome::ThunderMountains, EKalmalaBiome::Ocean })
    {
        for (uint64 Seed = 0; Seed < 100; ++Seed)
        {
            EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::MosquitoSwarm;
            if (AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(Seed, Biome, Archetype))
                TestFalse(TEXT("Mosquito swarm never selects outside Mossy Mire"),
                    Archetype == EKalmalaWildlifeArchetype::MosquitoSwarm);
        }
    }

    TestEqual(TEXT("One gameplay swarm has 25 shared health"),
        AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(EKalmalaWildlifeArchetype::MosquitoSwarm), 25.0f);
    TestEqual(TEXT("The swarm has no armor"),
        AKalmalaWildlifeSpawn::GetArmorForArchetype(EKalmalaWildlifeArchetype::MosquitoSwarm), 0.0f);
    const FKalmalaDamageChannels Bite = AKalmalaWildlifeSpawn::GetBaseAttackChannelsForArchetype(
        EKalmalaWildlifeArchetype::MosquitoSwarm);
    TestEqual(TEXT("Each accepted contact bite uses five Stab"), Bite.Stab, 5.0f);
    TestEqual(TEXT("The swarm bites every 0.2 seconds"), AKalmalaWildlifeSpawn::MosquitoBiteIntervalSeconds, 0.2f);
    TestEqual(TEXT("Weak poison lasts ten seconds"), AKalmalaWildlifeSpawn::MosquitoWeakPoisonDurationSeconds, 10.0f);
    TestEqual(TEXT("Weak poison remains the shared one-damage rate"),
        UKalmalaPlayerStatusComponent::WeakPoisonDamagePerSecond, 1.0f);
    TestTrue(TEXT("A server-accepted bite reaches exactly one metre"),
        AKalmalaWildlifeSpawn::IsMosquitoBiteInReach(true, true, 100.0f));
    TestFalse(TEXT("The swarm cannot bite beyond one metre"),
        AKalmalaWildlifeSpawn::IsMosquitoBiteInReach(true, true, 100.1f));
    TestFalse(TEXT("A client cannot resolve a swarm bite"),
        AKalmalaWildlifeSpawn::IsMosquitoBiteInReach(false, true, 50.0f));

    TestTrue(TEXT("A visible active torch repels within three metres"),
        AKalmalaWildlifeSpawn::IsMosquitoFireRepelling(true, true, 300.0f,
            AKalmalaWildlifeSpawn::MosquitoTorchAvoidanceCentimetres));
    TestTrue(TEXT("A visible active campfire repels within five metres"),
        AKalmalaWildlifeSpawn::IsMosquitoFireRepelling(true, true, 500.0f,
            AKalmalaWildlifeSpawn::MosquitoCampfireAvoidanceCentimetres));
    TestFalse(TEXT("An inactive or smouldering fire does not repel"),
        AKalmalaWildlifeSpawn::IsMosquitoFireRepelling(true, false, 100.0f,
            AKalmalaWildlifeSpawn::MosquitoCampfireAvoidanceCentimetres));
    TestFalse(TEXT("A client flame flag cannot repel a server swarm"),
        AKalmalaWildlifeSpawn::IsMosquitoFireRepelling(false, true, 100.0f,
            AKalmalaWildlifeSpawn::MosquitoCampfireAvoidanceCentimetres));
    TestEqual(TEXT("The swarm creates no baseline Bone Fragment reward"),
        AKalmalaWildlifeSpawn::GetBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::MosquitoSwarm), 0);
    return true;
}

#endif
