#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeThunderSentinelContractTest,
    "Kalmala.Gameplay.Wildlife.ThunderSentinel.MountainSlamAndRewardClaim",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWildlifeThunderSentinelContractTest::RunTest(const FString& Parameters)
{
    int32 SentinelSlots = 0;
    for (uint64 Seed = 0; Seed < 1000; ++Seed)
    {
        EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Mireling;
        const bool bEligible = AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(
            Seed, EKalmalaBiome::ThunderMountains, Archetype);
        if (Seed % 10ull == 9ull)
        {
            TestTrue(TEXT("Thunder Sentinel occupies the rare hard slot"), bEligible);
            TestEqual(TEXT("The hard mountain slot selects Thunder Sentinel"), Archetype,
                EKalmalaWildlifeArchetype::ThunderSentinel);
            ++SentinelSlots;
        }
        else if (Seed % 10ull >= 5ull)
        {
            TestTrue(TEXT("Horned Goat retains the common mountain share"), bEligible);
            TestEqual(TEXT("Non-hard mountain wildlife remains Horned Goat"), Archetype,
                EKalmalaWildlifeArchetype::HornedGoat);
        }
        else
        {
            TestFalse(TEXT("The unassigned mountain half stays unspawned"), bEligible);
        }
    }
    TestEqual(TEXT("Thunder Sentinel receives ten percent of mountain candidates"), SentinelSlots, 100);

    for (const EKalmalaBiome Biome : { EKalmalaBiome::Meadows, EKalmalaBiome::Elderwood,
             EKalmalaBiome::ShimmeringLakes, EKalmalaBiome::MossyMire,
             EKalmalaBiome::FreezingTundra, EKalmalaBiome::Ocean })
    {
        for (uint64 Seed = 0; Seed < 10; ++Seed)
        {
            EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::ThunderSentinel;
            if (AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(Seed, Biome, Archetype))
                TestFalse(TEXT("Thunder Sentinel is exclusive to Thunder Mountains"),
                    Archetype == EKalmalaWildlifeArchetype::ThunderSentinel);
        }
    }

    TestEqual(TEXT("The hard stone creature retains 1250 health"),
        AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(EKalmalaWildlifeArchetype::ThunderSentinel), 1250.0f);
    TestEqual(TEXT("The hard stone creature retains 75 armor"),
        AKalmalaWildlifeSpawn::GetArmorForArchetype(EKalmalaWildlifeArchetype::ThunderSentinel), 75.0f);
    const FKalmalaDamageChannels Attack =
        AKalmalaWildlifeSpawn::GetBaseAttackChannelsForArchetype(EKalmalaWildlifeArchetype::ThunderSentinel);
    TestEqual(TEXT("The slam uses the reviewed 300 Crush profile"), Attack.Crush, 300.0f);
    TestEqual(TEXT("The baseline defeat reward contains three Stone"),
        AKalmalaWildlifeSpawn::GetStoneRewardQuantityForArchetype(
            EKalmalaWildlifeArchetype::ThunderSentinel), 3);
    TestEqual(TEXT("Other wildlife has no Thunder Sentinel Stone reward"),
        AKalmalaWildlifeSpawn::GetStoneRewardQuantityForArchetype(EKalmalaWildlifeArchetype::HornedGoat), 0);

    TestTrue(TEXT("The server accepts the inclusive 300 cm horizontal slam reach"),
        AKalmalaWildlifeSpawn::IsThunderSentinelSlamInReach(true, true, 300.0f));
    TestFalse(TEXT("The slam rejects targets beyond its horizontal reach"),
        AKalmalaWildlifeSpawn::IsThunderSentinelSlamInReach(true, true, 300.01f));
    TestFalse(TEXT("Clients cannot author Thunder Sentinel damage"),
        AKalmalaWildlifeSpawn::IsThunderSentinelSlamInReach(false, true, 100.0f));
    TestFalse(TEXT("Invalid targets cannot receive the slam"),
        AKalmalaWildlifeSpawn::IsThunderSentinelSlamInReach(true, false, 100.0f));
    TestFalse(TEXT("Negative target distances fail closed"),
        AKalmalaWildlifeSpawn::IsThunderSentinelSlamInReach(true, true, -1.0f));
    TestEqual(TEXT("The Sentinel warns for one second"),
        AKalmalaWildlifeSpawn::ThunderSentinelTelegraphSeconds, 1.0f);
    TestEqual(TEXT("The Sentinel takes a short 120 cm step"),
        AKalmalaWildlifeSpawn::ThunderSentinelStepDistanceCentimetres, 120.0f);
    TestEqual(TEXT("The slam recovery lasts 1.2 seconds"),
        AKalmalaWildlifeSpawn::ThunderSentinelRecoverySeconds, 1.2f);
    TestEqual(TEXT("The hard enemy remains within its 25 m leash"),
        AKalmalaWildlifeSpawn::ThunderSentinelSpawnLeashCentimetres, 2500.0f);
    TestTrue(TEXT("Dry traversable Thunder Mountains are valid habitat"),
        AKalmalaWildlifeSpawn::IsThunderMountainsSpawnHabitatAllowed(
            EKalmalaBiome::ThunderMountains, false, true));
    TestFalse(TEXT("Water is rejected as Sentinel habitat"),
        AKalmalaWildlifeSpawn::IsThunderMountainsSpawnHabitatAllowed(
            EKalmalaBiome::ThunderMountains, true, true));
    TestFalse(TEXT("Steep ground is rejected as Sentinel habitat"),
        AKalmalaWildlifeSpawn::IsThunderMountainsSpawnHabitatAllowed(
            EKalmalaBiome::ThunderMountains, false, false));
    TestFalse(TEXT("Other biomes are rejected as Sentinel habitat"),
        AKalmalaWildlifeSpawn::IsThunderMountainsSpawnHabitatAllowed(
            EKalmalaBiome::Elderwood, false, true));

    TestFalse(TEXT("Hard enemies cannot spawn inside 150 m of the generated start"),
        AKalmalaWildlifeSpawn::IsHardEnemySpawnAllowedByStart(true, true, 14999.0f));
    TestTrue(TEXT("The 150 m exclusion boundary is inclusive"),
        AKalmalaWildlifeSpawn::IsHardEnemySpawnAllowedByStart(true, true, 15000.0f));
    TestTrue(TEXT("Ordinary wildlife is not subject to hard-enemy exclusion"),
        AKalmalaWildlifeSpawn::IsHardEnemySpawnAllowedByStart(false, true, 0.0f));
    TestTrue(TEXT("A missing generated start does not exclude the hard enemy"),
        AKalmalaWildlifeSpawn::IsHardEnemySpawnAllowedByStart(true, false, 0.0f));

    const FString SpawnId(TEXT("-4/9/3/thundersentinel-117"));
    const FString ClaimId = UKalmalaWorldPopulationSaveGame::MakeThunderSentinelPendingRewardClaimId(SpawnId);
    FString DecodedSpawnId;
    TestTrue(TEXT("The durable claim fits the stable-ID limit"), !ClaimId.IsEmpty() && ClaimId.Len() <= 128);
    TestTrue(TEXT("The claim decodes the stable spawn ID"),
        UKalmalaWorldPopulationSaveGame::TryGetThunderSentinelSpawnIdFromPendingRewardClaimId(
            ClaimId, DecodedSpawnId));
    TestEqual(TEXT("The decoded claim retains the defeated spawn ID"), DecodedSpawnId, SpawnId);
    TestTrue(TEXT("Empty spawn IDs are rejected"),
        UKalmalaWorldPopulationSaveGame::MakeThunderSentinelPendingRewardClaimId(FString()).IsEmpty());
    TestFalse(TEXT("A Goat claim cannot decode as a Sentinel claim"),
        UKalmalaWorldPopulationSaveGame::TryGetThunderSentinelSpawnIdFromPendingRewardClaimId(
            TEXT("hornedgoat-reward|1/2/3"), DecodedSpawnId));

    FKalmalaWorldGenerationConfig WorldConfig;
    WorldConfig.WorldSeed = 431;
    UKalmalaWorldPopulationSaveGame* Population = NewObject<UKalmalaWorldPopulationSaveGame>();
    TestNotNull(TEXT("Sentinel reward escrow can be created"), Population);
    if (Population == nullptr) return false;
    Population->InitializeForWorld(WorldConfig);
    Population->MarkDefeated(SpawnId);
    TestTrue(TEXT("A credited defeat queues three Stone for its owner"),
        Population->TryQueuePendingThunderSentinelReward(SpawnId, TEXT("player:test")));
    TestTrue(TEXT("Retrying the same owner's claim is idempotent"),
        Population->TryQueuePendingThunderSentinelReward(SpawnId, TEXT("player:test")));
    TestFalse(TEXT("A different player cannot take the pending reward"),
        Population->TryQueuePendingThunderSentinelReward(SpawnId, TEXT("player:other")));
    TestEqual(TEXT("Only one Sentinel reward row is queued"), Population->GetPendingWildlifeRewards().Num(), 1);

    TArray<uint8> Bytes;
    TestTrue(TEXT("The Sentinel reward claim serializes with the population save"),
        UGameplayStatics::SaveGameToMemory(Population, Bytes));
    UKalmalaWorldPopulationSaveGame* Reloaded =
        Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    TestNotNull(TEXT("The pending Sentinel claim reloads"), Reloaded);
    if (Reloaded == nullptr || Reloaded->GetPendingWildlifeRewards().IsEmpty()) return false;
    TestTrue(TEXT("Reload retains the defeated spawn identity"), Reloaded->IsDefeated(SpawnId));
    TestEqual(TEXT("Reload retains the stable claim"),
        Reloaded->GetPendingWildlifeRewards()[0].PersistentSpawnId, ClaimId);
    TestEqual(TEXT("Reload retains the credited owner"),
        Reloaded->GetPendingWildlifeRewards()[0].PlayerIdentity, FString(TEXT("player:test")));
    return true;
}

#endif
