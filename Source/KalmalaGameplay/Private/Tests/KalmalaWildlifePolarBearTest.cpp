#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifePolarBearContractTest,
    "Kalmala.Gameplay.Wildlife.PolarBear.TundraChargeAndRewardClaim",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWildlifePolarBearContractTest::RunTest(const FString& Parameters)
{
    int32 PolarBearSlots = 0;
    for (uint64 Seed = 0; Seed < 1000; ++Seed)
    {
        EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Mireling;
        const bool bEligible = AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(
            Seed, EKalmalaBiome::FreezingTundra, Archetype);
        if (Seed % 10ull == 9ull)
        {
            TestTrue(TEXT("Polar Bear occupies the final Tundra candidate slot"), bEligible);
            TestEqual(TEXT("The hard slot selects Polar Bear"), Archetype,
                EKalmalaWildlifeArchetype::PolarBear);
            ++PolarBearSlots;
        }
        else if (Seed % 10ull == 5ull || Seed % 10ull == 6ull)
        {
            TestTrue(TEXT("Wolf retains its Tundra candidate share"), bEligible);
            TestEqual(TEXT("The Wolf share remains unchanged"), Archetype, EKalmalaWildlifeArchetype::Wolf);
        }
        else if (Seed % 10ull == 7ull || Seed % 10ull == 8ull)
        {
            TestTrue(TEXT("Lynx retains its Tundra candidate share"), bEligible);
            TestEqual(TEXT("The Lynx share remains unchanged"), Archetype, EKalmalaWildlifeArchetype::Lynx);
        }
        else
        {
            TestFalse(TEXT("The empty existing-roster half stays unspawned"), bEligible);
        }
    }
    TestEqual(TEXT("Polar Bear receives ten percent of Tundra candidate slots"), PolarBearSlots, 100);

    for (const EKalmalaBiome Biome : { EKalmalaBiome::Meadows, EKalmalaBiome::Elderwood,
             EKalmalaBiome::ShimmeringLakes, EKalmalaBiome::MossyMire,
             EKalmalaBiome::ThunderMountains, EKalmalaBiome::Ocean })
    {
        for (uint64 Seed = 0; Seed < 10; ++Seed)
        {
            EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::PolarBear;
            if (AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(Seed, Biome, Archetype))
                TestFalse(TEXT("Polar Bear is exclusive to Freezing Tundra"),
                    Archetype == EKalmalaWildlifeArchetype::PolarBear);
        }
    }

    TestEqual(TEXT("Polar Bear retains 750 health"),
        AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(EKalmalaWildlifeArchetype::PolarBear), 750.0f);
    TestEqual(TEXT("Polar Bear retains 60 armor"),
        AKalmalaWildlifeSpawn::GetArmorForArchetype(EKalmalaWildlifeArchetype::PolarBear), 60.0f);
    const FKalmalaDamageChannels Attack =
        AKalmalaWildlifeSpawn::GetBaseAttackChannelsForArchetype(EKalmalaWildlifeArchetype::PolarBear);
    TestEqual(TEXT("The committed charge uses 200 Crush damage"), Attack.Crush, 200.0f);
    TestEqual(TEXT("Credited Polar Bear defeats award three Bone Fragments"),
        AKalmalaWildlifeSpawn::GetBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::PolarBear), 3);

    TestTrue(TEXT("The charge accepts its inclusive 220 cm impact reach"),
        AKalmalaWildlifeSpawn::IsPolarBearChargeInReach(true, true, 220.0f));
    TestFalse(TEXT("A charge cannot hit beyond its impact reach"),
        AKalmalaWildlifeSpawn::IsPolarBearChargeInReach(true, true, 220.01f));
    TestFalse(TEXT("Clients cannot author a Polar Bear hit"),
        AKalmalaWildlifeSpawn::IsPolarBearChargeInReach(false, true, 100.0f));
    TestFalse(TEXT("An invalid target cannot receive the charge"),
        AKalmalaWildlifeSpawn::IsPolarBearChargeInReach(true, false, 100.0f));
    TestFalse(TEXT("Non-finite charge distances fail closed"),
        AKalmalaWildlifeSpawn::IsPolarBearChargeInReach(true, true, TNumericLimits<float>::QuietNaN()));
    TestEqual(TEXT("The charge warning follows the hard-enemy one-second timing"),
        AKalmalaWildlifeSpawn::PolarBearTelegraphSeconds, 1.0f);
    TestEqual(TEXT("The charge retreats before the 1.2-second hard recovery"),
        AKalmalaWildlifeSpawn::PolarBearRecoverySeconds, 1.2f);
    TestEqual(TEXT("The bear stays within its 25 m spawn leash"),
        AKalmalaWildlifeSpawn::PolarBearSpawnLeashCentimetres, 2500.0f);

    const FString SpawnId(TEXT("-4/9/3/polarbear-117"));
    const FString ClaimId = UKalmalaWorldPopulationSaveGame::MakePolarBearPendingRewardClaimId(SpawnId);
    FString DecodedSpawnId;
    TestTrue(TEXT("The durable Polar Bear claim fits the stable-ID limit"), !ClaimId.IsEmpty() && ClaimId.Len() <= 128);
    TestTrue(TEXT("The claim decodes the stable Polar Bear spawn ID"),
        UKalmalaWorldPopulationSaveGame::TryGetPolarBearSpawnIdFromPendingRewardClaimId(ClaimId, DecodedSpawnId));
    TestEqual(TEXT("The decoded reward retains the defeated spawn ID"), DecodedSpawnId, SpawnId);
    TestTrue(TEXT("Empty spawn IDs are rejected"),
        UKalmalaWorldPopulationSaveGame::MakePolarBearPendingRewardClaimId(FString()).IsEmpty());
    TestFalse(TEXT("A Wolf claim cannot decode as Polar Bear"),
        UKalmalaWorldPopulationSaveGame::TryGetPolarBearSpawnIdFromPendingRewardClaimId(
            TEXT("wolf-reward|1/2/3"), DecodedSpawnId));

    FKalmalaWorldGenerationConfig WorldConfig;
    WorldConfig.WorldSeed = 419;
    UKalmalaWorldPopulationSaveGame* Population = NewObject<UKalmalaWorldPopulationSaveGame>();
    TestNotNull(TEXT("Polar Bear reward escrow can be created"), Population);
    if (Population == nullptr) return false;
    Population->InitializeForWorld(WorldConfig);
    Population->MarkDefeated(SpawnId);
    TestTrue(TEXT("A credited defeat queues its owner-bound three-Bone claim"),
        Population->TryQueuePendingPolarBearReward(SpawnId, TEXT("player:test")));
    TestTrue(TEXT("Retrying the same owner's claim is idempotent"),
        Population->TryQueuePendingPolarBearReward(SpawnId, TEXT("player:test")));
    TestFalse(TEXT("A different player cannot take the pending reward"),
        Population->TryQueuePendingPolarBearReward(SpawnId, TEXT("player:other")));
    TestEqual(TEXT("Only one Polar Bear reward row is queued"), Population->GetPendingWildlifeRewards().Num(), 1);

    TArray<uint8> Bytes;
    TestTrue(TEXT("Polar Bear reward escrow serializes with the population save"),
        UGameplayStatics::SaveGameToMemory(Population, Bytes));
    UKalmalaWorldPopulationSaveGame* Reloaded =
        Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    TestNotNull(TEXT("The Polar Bear reward escrow reloads"), Reloaded);
    if (Reloaded == nullptr || Reloaded->GetPendingWildlifeRewards().IsEmpty()) return false;
    TestTrue(TEXT("Reload retains the defeated Polar Bear identity"), Reloaded->IsDefeated(SpawnId));
    TestEqual(TEXT("Reload retains the stable Polar Bear claim"),
        Reloaded->GetPendingWildlifeRewards()[0].PersistentSpawnId, ClaimId);
    TestEqual(TEXT("Reload retains the credited owner"),
        Reloaded->GetPendingWildlifeRewards()[0].PlayerIdentity, FString(TEXT("player:test")));
    return true;
}

#endif
