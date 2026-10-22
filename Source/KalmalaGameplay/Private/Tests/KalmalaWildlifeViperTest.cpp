#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaWildlifeSpawn.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeViperContractTest,
    "Kalmala.Gameplay.Wildlife.Viper.ShoreLungeAndRewardClaim",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWildlifeViperContractTest::RunTest(const FString& Parameters)
{
    int32 ExistingRosterSlots = 0;
    int32 ViperSlots = 0;
    int32 GiantSnakeSlots = 0;
    for (uint64 Seed = 0; Seed < 1000; ++Seed)
    {
        EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Mireling;
        const bool bEligible = AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(
            Seed, EKalmalaBiome::ShimmeringLakes, Archetype);
        const uint64 Slot = Seed % 10ull;
        TestTrue(TEXT("Every Shimmering Lakes seed selects a wildlife candidate"), bEligible);
        if (Slot <= 4)
        {
            TestTrue(TEXT("Existing lake wildlife retains half the candidate slots"),
                Archetype == EKalmalaWildlifeArchetype::Boar || Archetype == EKalmalaWildlifeArchetype::Deer);
            ++ExistingRosterSlots;
        }
        else if (Slot <= 8)
        {
            TestEqual(TEXT("Viper occupies the common shoreline share"), Archetype,
                EKalmalaWildlifeArchetype::Viper);
            ++ViperSlots;
        }
        else
        {
            TestEqual(TEXT("Giant Snake occupies the rare hard-enemy slot"), Archetype,
                EKalmalaWildlifeArchetype::GiantSnake);
            ++GiantSnakeSlots;
        }

    }

    TestEqual(TEXT("Existing lake roster retains fifty percent"), ExistingRosterSlots, 500);
    TestEqual(TEXT("Viper receives forty percent"), ViperSlots, 400);
    TestEqual(TEXT("Giant Snake receives ten percent"), GiantSnakeSlots, 100);
    for (const EKalmalaBiome OtherBiome : { EKalmalaBiome::Meadows, EKalmalaBiome::Elderwood,
             EKalmalaBiome::MossyMire, EKalmalaBiome::FreezingTundra,
             EKalmalaBiome::ThunderMountains, EKalmalaBiome::Ocean })
    {
        EKalmalaWildlifeArchetype OtherArchetype = EKalmalaWildlifeArchetype::Viper;
        if (AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(5, OtherBiome, OtherArchetype))
        {
            TestFalse(TEXT("Viper remains exclusive to Shimmering Lakes"),
                OtherArchetype == EKalmalaWildlifeArchetype::Viper);
        }
    }
    TestEqual(TEXT("Viper's reviewed weak-poison duration is five seconds"),
        AKalmalaWildlifeSpawn::ViperWeakPoisonDurationSeconds, 5.0f);
    TestEqual(TEXT("Viper's server lunge warning matches the reviewed timing"),
        AKalmalaWildlifeSpawn::ViperTelegraphSeconds, 0.5f);
    TestEqual(TEXT("Viper's server lunge recovery matches the reviewed timing"),
        AKalmalaWildlifeSpawn::ViperRecoverySeconds, 0.8f);
    TestEqual(TEXT("Viper baseline reward remains one Bone Fragment"),
        AKalmalaWildlifeSpawn::GetBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::Viper), 1);

    const FString SpawnId(TEXT("1/4/-8/123456"));
    const FString ClaimId = UKalmalaWorldPopulationSaveGame::MakeViperPendingRewardClaimId(SpawnId);
    FString DecodedSpawnId;
    TestTrue(TEXT("Viper reward claims retain a namespaced stable identity"), !ClaimId.IsEmpty());
    TestTrue(TEXT("Viper reward claim decodes its spawn identity"),
        UKalmalaWorldPopulationSaveGame::TryGetViperSpawnIdFromPendingRewardClaimId(ClaimId, DecodedSpawnId));
    TestEqual(TEXT("Decoded Viper identity matches the spawned wildlife"), DecodedSpawnId, SpawnId);
    TestTrue(TEXT("Empty Viper spawn identities are rejected"),
        UKalmalaWorldPopulationSaveGame::MakeViperPendingRewardClaimId(FString()).IsEmpty());
    TestFalse(TEXT("Raw Rat claims do not decode as Viper"),
        UKalmalaWorldPopulationSaveGame::TryGetViperSpawnIdFromPendingRewardClaimId(SpawnId, DecodedSpawnId));

    FKalmalaWorldGenerationConfig WorldConfig;
    WorldConfig.WorldSeed = 418;
    UKalmalaWorldPopulationSaveGame* Population = NewObject<UKalmalaWorldPopulationSaveGame>();
    TestNotNull(TEXT("Viper reward escrow can be created"), Population);
    if (Population == nullptr) return false;

    Population->InitializeForWorld(WorldConfig);
    Population->MarkDefeated(SpawnId);
    TestTrue(TEXT("A credited Viper reward is queued for its owner"),
        Population->TryQueuePendingViperReward(SpawnId, TEXT("player:test")));
    TestTrue(TEXT("Retrying the Viper claim for its owner is idempotent"),
        Population->TryQueuePendingViperReward(SpawnId, TEXT("player:test")));
    TestFalse(TEXT("A Viper claim cannot be reassigned to another owner"),
        Population->TryQueuePendingViperReward(SpawnId, TEXT("player:other")));
    TestEqual(TEXT("Only one pending Viper claim is retained"), Population->GetPendingWildlifeRewards().Num(), 1);

    TArray<FKalmalaInventoryStack> FullPack;
    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    if (Catalogue != nullptr)
    {
        for (const FKalmalaItemDefinition& Definition : Catalogue->Items)
        {
            if (Definition.ItemId.IsNone() || Definition.ItemId == TEXT("BoneFragment") || Definition.MaxStack <= 0)
                continue;
            FKalmalaInventoryStack Stack;
            Stack.ItemId = Definition.ItemId;
            Stack.Quantity = 1;
            FullPack.Add(MoveTemp(Stack));
            if (FullPack.Num() == UKalmalaInventoryComponent::MaxSlots) break;
        }
    }
    TestEqual(TEXT("The full-pack fixture uses all forty inventory cells"),
        FullPack.Num(), UKalmalaInventoryComponent::MaxSlots);
    if (FullPack.Num() == UKalmalaInventoryComponent::MaxSlots)
    {
        TArray<FKalmalaInventoryStack> RejectedCandidate;
        FString FailureReason;
        TestFalse(TEXT("A full pack rejects the Viper Bone Fragment"),
            UKalmalaInventoryComponent::BuildGrant(FullPack, TEXT("BoneFragment"), 1,
                RejectedCandidate, FailureReason));
        TestEqual(TEXT("The full-pack rejection is capacity-related"),
            FailureReason, FString(TEXT("Pack output capacity reached")));
        TestEqual(TEXT("A full-pack failure retains the pending Viper claim"),
            Population->GetPendingWildlifeRewards().Num(), 1);
    }

    TArray<uint8> Bytes;
    TestTrue(TEXT("The Viper reward serializes in the existing population save"),
        UGameplayStatics::SaveGameToMemory(Population, Bytes));
    UKalmalaWorldPopulationSaveGame* Reloaded =
        Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    TestNotNull(TEXT("Viper reward escrow reloads"), Reloaded);
    if (Reloaded == nullptr || Reloaded->GetPendingWildlifeRewards().IsEmpty()) return false;

    TestTrue(TEXT("Reload retains the defeated Viper"), Reloaded->IsDefeated(SpawnId));
    TestTrue(TEXT("Reload retains the world identity"), Reloaded->MatchesWorld(WorldConfig));
    TestEqual(TEXT("Reload retains the Viper claim identity"),
        Reloaded->GetPendingWildlifeRewards()[0].PersistentSpawnId, ClaimId);
    TestEqual(TEXT("Reload retains the credited owner"),
        Reloaded->GetPendingWildlifeRewards()[0].PlayerIdentity, FString(TEXT("player:test")));
    TestTrue(TEXT("Reloaded claim remains idempotent for its owner"),
        Reloaded->TryQueuePendingViperReward(SpawnId, TEXT("player:test")));
    TestFalse(TEXT("Reloaded claim remains bound against owner reassignment"),
        Reloaded->TryQueuePendingViperReward(SpawnId, TEXT("player:other")));

    if (FullPack.Num() == UKalmalaInventoryComponent::MaxSlots)
    {
        UKalmalaInventoryComponent* Capacity = NewObject<UKalmalaInventoryComponent>();
        TestNotNull(TEXT("Viper retry capacity validator exists"), Capacity);
        if (Capacity != nullptr)
        {
            FullPack.RemoveAt(0, 2);
            TArray<FKalmalaInventoryStack> RetryCandidate;
            FString RetryReason;
            const bool bBuiltReward = UKalmalaInventoryComponent::BuildGrant(
                FullPack, TEXT("BoneFragment"), 1, RetryCandidate, RetryReason);
            TestTrue(TEXT("The pending Viper reward fits after two cells are freed"), bBuiltReward);
            if (bBuiltReward)
            {
                TestTrue(TEXT("The reward and one carried tool fit after retry"),
                    Capacity->CanFitContents(RetryCandidate, 1));
                const FKalmalaInventoryStack* BoneStack = RetryCandidate.FindByPredicate(
                    [](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("BoneFragment"); });
                TestTrue(TEXT("Retry candidate contains one Bone Fragment"),
                    BoneStack != nullptr && BoneStack->Quantity == 1);
            }
            TestEqual(TEXT("Preparing a retry does not prematurely clear the claim"),
                Reloaded->GetPendingWildlifeRewards().Num(), 1);
        }
    }

    TestTrue(TEXT("Delivered Viper claim can be removed by its stable claim identity"),
        Reloaded->RemovePendingWildlifeReward(ClaimId));
    TestTrue(TEXT("Delivered Viper claim cleanup removes its pending row"),
        Reloaded->GetPendingWildlifeRewards().IsEmpty());
    return true;
}

#endif
