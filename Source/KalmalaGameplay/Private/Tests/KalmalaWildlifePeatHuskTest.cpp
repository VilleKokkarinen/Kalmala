#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifePeatHuskContractTest,
    "Kalmala.Gameplay.Wildlife.PeatHusk.MireSweepAndRewardClaim",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWildlifePeatHuskContractTest::RunTest(const FString& Parameters)
{
    int32 MirelingSlots = 0;
    int32 DeathAdderSlots = 0;
    int32 MosquitoSlots = 0;
    int32 PeatHuskSlots = 0;
    for (uint64 Seed = 0; Seed < 1000; ++Seed)
    {
        EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Mireling;
        const bool bEligible = AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(
            Seed, EKalmalaBiome::MossyMire, Archetype);
        TestTrue(TEXT("Every reviewed Mossy Mire slot has an implemented candidate"), bEligible);
        switch (Seed % 10ull)
        {
        case 0: case 1: case 2: case 3: case 4:
            TestEqual(TEXT("Mireling keeps half the Mire slots"), Archetype,
                EKalmalaWildlifeArchetype::Mireling);
            ++MirelingSlots;
            break;
        case 5: case 6:
            TestEqual(TEXT("Death Adder keeps two Mire slots"), Archetype,
                EKalmalaWildlifeArchetype::DeathAdder);
            ++DeathAdderSlots;
            break;
        case 7: case 8:
            TestEqual(TEXT("Mosquito swarm keeps two Mire slots"), Archetype,
                EKalmalaWildlifeArchetype::MosquitoSwarm);
            ++MosquitoSlots;
            break;
        default:
            TestEqual(TEXT("Peat Husk occupies the rare hard slot"), Archetype,
                EKalmalaWildlifeArchetype::PeatHusk);
            ++PeatHuskSlots;
            break;
        }
    }
    TestEqual(TEXT("Mireling share is fifty percent"), MirelingSlots, 500);
    TestEqual(TEXT("Death Adder share is twenty percent"), DeathAdderSlots, 200);
    TestEqual(TEXT("Mosquito share is twenty percent"), MosquitoSlots, 200);
    TestEqual(TEXT("Peat Husk share is ten percent"), PeatHuskSlots, 100);

    for (const EKalmalaBiome Biome : { EKalmalaBiome::Meadows, EKalmalaBiome::Elderwood,
             EKalmalaBiome::ShimmeringLakes, EKalmalaBiome::FreezingTundra,
             EKalmalaBiome::ThunderMountains, EKalmalaBiome::Ocean })
    {
        for (uint64 Seed = 0; Seed < 100; ++Seed)
        {
            EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::PeatHusk;
            if (AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(Seed, Biome, Archetype))
                TestFalse(TEXT("Peat Husk cannot be selected outside Mossy Mire"),
                    Archetype == EKalmalaWildlifeArchetype::PeatHusk);
        }
    }

    TestTrue(TEXT("Dry Mossy Mire permits the Peat Husk spawn"),
        AKalmalaWildlifeSpawn::IsMossyMireSpawnHabitatAllowed(EKalmalaBiome::MossyMire, false));
    TestFalse(TEXT("Water rejects a Peat Husk spawn"),
        AKalmalaWildlifeSpawn::IsMossyMireSpawnHabitatAllowed(EKalmalaBiome::MossyMire, true));
    TestFalse(TEXT("Other dry biomes reject a Peat Husk spawn"),
        AKalmalaWildlifeSpawn::IsMossyMireSpawnHabitatAllowed(EKalmalaBiome::Elderwood, false));
    TestEqual(TEXT("Peat Husk keeps 300 health"),
        AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(EKalmalaWildlifeArchetype::PeatHusk), 300.0f);
    TestEqual(TEXT("Peat Husk keeps 50 armor"),
        AKalmalaWildlifeSpawn::GetArmorForArchetype(EKalmalaWildlifeArchetype::PeatHusk), 50.0f);
    const FKalmalaDamageChannels Sweep =
        AKalmalaWildlifeSpawn::GetBaseAttackChannelsForArchetype(EKalmalaWildlifeArchetype::PeatHusk);
    TestEqual(TEXT("The slow sweep uses the reviewed 150 Crush"), Sweep.Crush, 150.0f);
    TestEqual(TEXT("The Peat Husk acquires targets within five metres"),
        AKalmalaWildlifeSpawn::PeatHuskAttackTriggerCentimetres, 500.0f);
    TestEqual(TEXT("The heavy arm closes to its 260 cm reach"),
        AKalmalaWildlifeSpawn::PeatHuskSweepReachCentimetres, 260.0f);
    TestEqual(TEXT("The sweep arc uses a 120-degree forward cone"),
        AKalmalaWildlifeSpawn::PeatHuskSweepMinimumForwardDot, 0.5f);
    TestEqual(TEXT("The sweep has a one-second warning"),
        AKalmalaWildlifeSpawn::PeatHuskTelegraphSeconds, 1.0f);
    TestEqual(TEXT("The heavy sweep recovers for 1.2 seconds"),
        AKalmalaWildlifeSpawn::PeatHuskRecoverySeconds, 1.2f);
    TestEqual(TEXT("The creature stays within its 25-metre leash"),
        AKalmalaWildlifeSpawn::PeatHuskSpawnLeashCentimetres, 2500.0f);
    TestEqual(TEXT("The creature approaches slowly"),
        AKalmalaWildlifeSpawn::PeatHuskApproachSpeedCentimetresPerSecond, 100.0f);

    TestTrue(TEXT("The server sweep accepts its maximum reach and 120-degree arc edge"),
        AKalmalaWildlifeSpawn::IsPeatHuskSweepInReach(true, true, 260.0f, 0.5f));
    TestFalse(TEXT("The sweep cannot hit outside 260 cm"),
        AKalmalaWildlifeSpawn::IsPeatHuskSweepInReach(true, true, 260.01f, 1.0f));
    TestFalse(TEXT("The sweep misses targets outside its forward arc"),
        AKalmalaWildlifeSpawn::IsPeatHuskSweepInReach(true, true, 100.0f, 0.49f));
    TestFalse(TEXT("A client cannot author a sweep hit"),
        AKalmalaWildlifeSpawn::IsPeatHuskSweepInReach(false, true, 100.0f, 1.0f));
    TestFalse(TEXT("An invalid target cannot receive a sweep"),
        AKalmalaWildlifeSpawn::IsPeatHuskSweepInReach(true, false, 100.0f, 1.0f));
    TestFalse(TEXT("A negative target distance fails closed"),
        AKalmalaWildlifeSpawn::IsPeatHuskSweepInReach(true, true, -1.0f, 1.0f));
    TestFalse(TEXT("A non-finite sweep distance fails closed"),
        AKalmalaWildlifeSpawn::IsPeatHuskSweepInReach(true, true,
            TNumericLimits<float>::QuietNaN(), 1.0f));

    const FString SpawnId(TEXT("-4/9/3/12877"));
    const FString ClaimId = UKalmalaWorldPopulationSaveGame::MakePeatHuskPendingRewardClaimId(SpawnId);
    FString DecodedSpawnId;
    TestTrue(TEXT("A Peat Husk defeat receives a namespaced claim"), !ClaimId.IsEmpty());
    TestTrue(TEXT("The claim decodes the stable spawn identity"),
        UKalmalaWorldPopulationSaveGame::TryGetPeatHuskSpawnIdFromPendingRewardClaimId(
            ClaimId, DecodedSpawnId));
    TestEqual(TEXT("Claim decoding preserves the spawn identity"), DecodedSpawnId, SpawnId);
    TestTrue(TEXT("Empty Peat Husk spawn IDs are rejected"),
        UKalmalaWorldPopulationSaveGame::MakePeatHuskPendingRewardClaimId(FString()).IsEmpty());
    TestFalse(TEXT("Other reward namespaces do not decode as Peat Husk"),
        UKalmalaWorldPopulationSaveGame::TryGetPeatHuskSpawnIdFromPendingRewardClaimId(
            TEXT("deathadder-reward|1/2/3"), DecodedSpawnId));

    UKalmalaWorldPopulationSaveGame* Population = NewObject<UKalmalaWorldPopulationSaveGame>();
    TestNotNull(TEXT("Population reward save fixture exists"), Population);
    if (Population != nullptr)
    {
        FKalmalaWorldGenerationConfig World;
        World.WorldSeed = 418;
        Population->InitializeForWorld(World);
        Population->MarkDefeated(SpawnId);
        TestTrue(TEXT("The Amber baseline claim is queued for its credited owner"),
            Population->TryQueuePendingPeatHuskReward(SpawnId, TEXT("player:test")));
        TestTrue(TEXT("Retrying for the same owner is idempotent"),
            Population->TryQueuePendingPeatHuskReward(SpawnId, TEXT("player:test")));
        TestFalse(TEXT("A different player cannot take the pending claim"),
            Population->TryQueuePendingPeatHuskReward(SpawnId, TEXT("player:other")));
        TestEqual(TEXT("Only one row remains for this defeated spawn"),
            Population->GetPendingWildlifeRewards().Num(), 1);

        TArray<uint8> Bytes;
        TestTrue(TEXT("The claim serializes with the existing population save"),
            UGameplayStatics::SaveGameToMemory(Population, Bytes));
        UKalmalaWorldPopulationSaveGame* Reloaded =
            Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
        TestNotNull(TEXT("The claim reloads"), Reloaded);
        if (Reloaded != nullptr && !Reloaded->GetPendingWildlifeRewards().IsEmpty())
        {
            TestTrue(TEXT("Reload preserves the defeated spawn"), Reloaded->IsDefeated(SpawnId));
            TestEqual(TEXT("Reload preserves the namespaced claim"),
                Reloaded->GetPendingWildlifeRewards()[0].PersistentSpawnId, ClaimId);
            TestEqual(TEXT("Reload preserves the credited owner"),
                Reloaded->GetPendingWildlifeRewards()[0].PlayerIdentity, FString(TEXT("player:test")));
            TestTrue(TEXT("The delivered claim can be removed after successful delivery"),
                Reloaded->RemovePendingWildlifeReward(ClaimId));
            TestEqual(TEXT("Successful delivery cleanup leaves no claim"),
                Reloaded->GetPendingWildlifeRewards().Num(), 0);
        }
    }
    return true;
}

#endif
