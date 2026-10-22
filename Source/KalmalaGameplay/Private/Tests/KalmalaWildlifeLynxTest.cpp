#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaWildlifeSpawn.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeLynxContractTest,
    "Kalmala.Gameplay.Wildlife.Lynx.TundraLeapAndRewardClaim",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWildlifeLynxContractTest::RunTest(const FString& Parameters)
{
    int32 WolfSlots = 0;
    int32 LynxSlots = 0;
    for (uint64 Seed = 0; Seed < 1000; ++Seed)
    {
        EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Mireling;
        const bool bEligible = AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(
            Seed, EKalmalaBiome::FreezingTundra, Archetype);
        if (Seed % 10ull == 5ull || Seed % 10ull == 6ull)
        {
            TestTrue(TEXT("The previous Tundra share remains eligible"), bEligible);
            TestEqual(TEXT("Wolf retains its two deterministic Tundra slots"), Archetype,
                EKalmalaWildlifeArchetype::Wolf);
            ++WolfSlots;
        }
        else if (Seed % 10ull == 7ull || Seed % 10ull == 8ull)
        {
            TestTrue(TEXT("Lynx owns both seeded Tundra slots"), bEligible);
            TestEqual(TEXT("Both Lynx slots select the Lynx archetype"), Archetype,
                EKalmalaWildlifeArchetype::Lynx);
            ++LynxSlots;
        }
        else
        {
            TestFalse(TEXT("Unimplemented Tundra shares stay unspawned"), bEligible);
        }
    }
    TestEqual(TEXT("Wolf keeps twenty percent of Tundra candidate slots"), WolfSlots, 200);
    TestEqual(TEXT("Lynx holds twenty percent of Tundra candidate slots"), LynxSlots, 200);

    for (const EKalmalaBiome Biome : { EKalmalaBiome::Meadows, EKalmalaBiome::Elderwood,
             EKalmalaBiome::ShimmeringLakes, EKalmalaBiome::MossyMire,
             EKalmalaBiome::ThunderMountains, EKalmalaBiome::Ocean })
    {
        for (uint64 Seed = 0; Seed < 10; ++Seed)
        {
            EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Lynx;
            if (AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(Seed, Biome, Archetype))
                TestFalse(TEXT("Lynx cannot be selected outside Freezing Tundra"),
                    Archetype == EKalmalaWildlifeArchetype::Lynx);
        }
    }

    TestEqual(TEXT("The Lynx leap has the user-reviewed 350 cm impact range"),
        AKalmalaWildlifeSpawn::LynxLeapImpactReachCentimetres, 350.0f);
    TestTrue(TEXT("The server accepts the inclusive maximum impact distance"),
        AKalmalaWildlifeSpawn::IsLynxLeapInReach(true, true, 350.0f));
    TestFalse(TEXT("The leap cannot hit beyond 350 cm"),
        AKalmalaWildlifeSpawn::IsLynxLeapInReach(true, true, 350.01f));
    TestFalse(TEXT("A client cannot author a Lynx hit"),
        AKalmalaWildlifeSpawn::IsLynxLeapInReach(false, true, 100.0f));
    TestFalse(TEXT("An invalid target cannot receive a Lynx hit"),
        AKalmalaWildlifeSpawn::IsLynxLeapInReach(true, false, 100.0f));
    TestFalse(TEXT("Negative target distance fails closed"),
        AKalmalaWildlifeSpawn::IsLynxLeapInReach(true, true, -1.0f));
    TestEqual(TEXT("The pounce uses a visible half-second warning"),
        AKalmalaWildlifeSpawn::LynxTelegraphSeconds, 0.5f);
    TestEqual(TEXT("The pounce animation runs for 0.24 seconds"),
        AKalmalaWildlifeSpawn::LynxLeapSeconds, 0.24f);
    TestEqual(TEXT("A landed leap recovers for the catalogue baseline"),
        AKalmalaWildlifeSpawn::LynxRecoverySeconds, 0.8f);
    TestEqual(TEXT("A missed leap receives the longer 1.2-second recovery"),
        AKalmalaWildlifeSpawn::LynxMissRecoverySeconds, 1.2f);
    TestEqual(TEXT("Lynx pursuit stays within the 25 m spawn leash"),
        AKalmalaWildlifeSpawn::LynxSpawnLeashCentimetres, 2500.0f);

    const FKalmalaDamageChannels Attack =
        AKalmalaWildlifeSpawn::GetBaseAttackChannelsForArchetype(EKalmalaWildlifeArchetype::Lynx);
    TestEqual(TEXT("The pounce retains the authored 75 Slash damage"), Attack.Slash, 75.0f);
    TestEqual(TEXT("The baseline reward is two Bone Fragments"),
        AKalmalaWildlifeSpawn::GetBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::Lynx), 2);

    const FString SpawnId(TEXT("-4/9/3/lynx-12877"));
    const FString ClaimId = UKalmalaWorldPopulationSaveGame::MakeLynxPendingRewardClaimId(SpawnId);
    FString DecodedSpawnId;
    TestTrue(TEXT("Lynx escrow uses a bounded species namespace"), !ClaimId.IsEmpty() && ClaimId.Len() <= 128);
    TestTrue(TEXT("Lynx claim decodes its stable spawn identity"),
        UKalmalaWorldPopulationSaveGame::TryGetLynxSpawnIdFromPendingRewardClaimId(ClaimId, DecodedSpawnId));
    TestEqual(TEXT("Claim decoding preserves the Lynx spawn identity"), DecodedSpawnId, SpawnId);
    TestTrue(TEXT("Empty Lynx spawn IDs are rejected"),
        UKalmalaWorldPopulationSaveGame::MakeLynxPendingRewardClaimId(FString()).IsEmpty());
    TestFalse(TEXT("Wolf claims do not decode as Lynx"),
        UKalmalaWorldPopulationSaveGame::TryGetLynxSpawnIdFromPendingRewardClaimId(
            TEXT("wolf-reward|1/2/3"), DecodedSpawnId));

    FKalmalaWorldGenerationConfig WorldConfig;
    WorldConfig.WorldSeed = 418;
    UKalmalaWorldPopulationSaveGame* Population = NewObject<UKalmalaWorldPopulationSaveGame>();
    TestNotNull(TEXT("Lynx reward escrow can be created"), Population);
    if (Population == nullptr) return false;
    Population->InitializeForWorld(WorldConfig);
    Population->MarkDefeated(SpawnId);
    TestTrue(TEXT("A credited Lynx defeat queues its owner-bound two-Bone reward"),
        Population->TryQueuePendingLynxReward(SpawnId, TEXT("player:test")));
    TestTrue(TEXT("Retrying the Lynx claim for its owner is idempotent"),
        Population->TryQueuePendingLynxReward(SpawnId, TEXT("player:test")));
    TestFalse(TEXT("A different player cannot take the Lynx claim"),
        Population->TryQueuePendingLynxReward(SpawnId, TEXT("player:other")));
    TestEqual(TEXT("One pending Lynx reward row remains"), Population->GetPendingWildlifeRewards().Num(), 1);

    TArray<uint8> Bytes;
    TestTrue(TEXT("Lynx reward escrow serializes in the population save"),
        UGameplayStatics::SaveGameToMemory(Population, Bytes));
    UKalmalaWorldPopulationSaveGame* Reloaded =
        Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    TestNotNull(TEXT("Lynx reward escrow reloads"), Reloaded);
    if (Reloaded == nullptr || Reloaded->GetPendingWildlifeRewards().IsEmpty()) return false;
    TestTrue(TEXT("Reload retains the defeated Lynx identity"), Reloaded->IsDefeated(SpawnId));
    TestTrue(TEXT("Reload retains the Lynx claim ID"),
        Reloaded->GetPendingWildlifeRewards()[0].PersistentSpawnId == ClaimId);
    TestEqual(TEXT("Reload retains the credited Lynx owner"),
        Reloaded->GetPendingWildlifeRewards()[0].PlayerIdentity, FString(TEXT("player:test")));

    TArray<FKalmalaInventoryStack> Candidate;
    TArray<FKalmalaInventoryStack> EmptyInventory;
    FString FailureReason;
    TestTrue(TEXT("Two Bone Fragments fit an empty inventory"),
        UKalmalaInventoryComponent::BuildGrant(EmptyInventory, TEXT("BoneFragment"), 2,
            Candidate, FailureReason));
    TestTrue(TEXT("Delivered Lynx claim can be removed by its stable ID"),
        Reloaded->RemovePendingWildlifeReward(ClaimId));
    TestTrue(TEXT("Claim cleanup clears the pending row"), Reloaded->GetPendingWildlifeRewards().IsEmpty());
    return true;
}

#endif
