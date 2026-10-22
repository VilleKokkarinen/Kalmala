#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaWildlifeSpawn.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeGiantSnakeContractTest,
    "Kalmala.Gameplay.Wildlife.GiantSnake.ShoreLungeAndRewardClaim",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWildlifeGiantSnakeContractTest::RunTest(const FString& Parameters)
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
        EKalmalaWildlifeArchetype OtherArchetype = EKalmalaWildlifeArchetype::GiantSnake;
        if (AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(9, OtherBiome, OtherArchetype))
        {
            TestFalse(TEXT("Giant Snake remains exclusive to Shimmering Lakes"),
                OtherArchetype == EKalmalaWildlifeArchetype::GiantSnake);
        }
    }

    TestEqual(TEXT("Giant Snake has the reviewed hard-enemy health"),
        AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(EKalmalaWildlifeArchetype::GiantSnake), 250.0f);
    TestEqual(TEXT("Giant Snake has the reviewed scale armor"),
        AKalmalaWildlifeSpawn::GetArmorForArchetype(EKalmalaWildlifeArchetype::GiantSnake), 30.0f);
    const FKalmalaDamageChannels Attack = AKalmalaWildlifeSpawn::GetBaseAttackChannelsForArchetype(
        EKalmalaWildlifeArchetype::GiantSnake);
    TestEqual(TEXT("Giant Snake's powerful lunge deals 100 Stab"), Attack.Stab, 100.0f);
    TestEqual(TEXT("Giant Snake does not add Crush damage"), Attack.Crush, 0.0f);
    TestEqual(TEXT("Giant Snake telegraphs its lunge for one second"),
        AKalmalaWildlifeSpawn::GiantSnakeTelegraphSeconds, 1.0f);
    TestEqual(TEXT("Giant Snake recovers for 1.2 seconds"),
        AKalmalaWildlifeSpawn::GiantSnakeRecoverySeconds, 1.2f);
    TestEqual(TEXT("Giant Snake stays within the 25 metre leash"),
        AKalmalaWildlifeSpawn::SnakeSpawnLeashCentimetres, 2500.0f);
    TestEqual(TEXT("Giant Snake baseline reward contains two Bone Fragments"),
        AKalmalaWildlifeSpawn::GetBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::GiantSnake), 2);

    const FString SpawnId(TEXT("1/4/-8/123456"));
    const FString ClaimId = UKalmalaWorldPopulationSaveGame::MakeGiantSnakePendingRewardClaimId(SpawnId);
    FString DecodedSpawnId;
    TestTrue(TEXT("Giant Snake claims retain a namespaced stable identity"), !ClaimId.IsEmpty());
    TestTrue(TEXT("Giant Snake claim decodes its spawn identity"),
        UKalmalaWorldPopulationSaveGame::TryGetGiantSnakeSpawnIdFromPendingRewardClaimId(
            ClaimId, DecodedSpawnId));
    TestEqual(TEXT("Decoded identity matches the spawned wildlife"), DecodedSpawnId, SpawnId);
    TestTrue(TEXT("Empty Giant Snake spawn identities are rejected"),
        UKalmalaWorldPopulationSaveGame::MakeGiantSnakePendingRewardClaimId(FString()).IsEmpty());
    TestFalse(TEXT("Raw spawn identities do not decode as Giant Snake claims"),
        UKalmalaWorldPopulationSaveGame::TryGetGiantSnakeSpawnIdFromPendingRewardClaimId(
            SpawnId, DecodedSpawnId));

    FKalmalaWorldGenerationConfig WorldConfig;
    WorldConfig.WorldSeed = 418;
    UKalmalaWorldPopulationSaveGame* Population = NewObject<UKalmalaWorldPopulationSaveGame>();
    TestNotNull(TEXT("Giant Snake reward escrow can be created"), Population);
    if (Population == nullptr) return false;

    Population->InitializeForWorld(WorldConfig);
    Population->MarkDefeated(SpawnId);
    TestTrue(TEXT("A credited Giant Snake reward is queued for its owner"),
        Population->TryQueuePendingGiantSnakeReward(SpawnId, TEXT("player:test")));
    TestTrue(TEXT("Retrying the Giant Snake claim for its owner is idempotent"),
        Population->TryQueuePendingGiantSnakeReward(SpawnId, TEXT("player:test")));
    TestFalse(TEXT("A Giant Snake claim cannot be reassigned to another owner"),
        Population->TryQueuePendingGiantSnakeReward(SpawnId, TEXT("player:other")));
    TestEqual(TEXT("Only one pending Giant Snake claim is retained"),
        Population->GetPendingWildlifeRewards().Num(), 1);

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
        TestFalse(TEXT("A full pack rejects two Giant Snake Bone Fragments"),
            UKalmalaInventoryComponent::BuildGrant(FullPack, TEXT("BoneFragment"), 2,
                RejectedCandidate, FailureReason));
        TestEqual(TEXT("The full-pack rejection is capacity-related"),
            FailureReason, FString(TEXT("Pack output capacity reached")));
        TestEqual(TEXT("A full-pack failure retains the pending Giant Snake claim"),
            Population->GetPendingWildlifeRewards().Num(), 1);
    }

    TArray<uint8> Bytes;
    TestTrue(TEXT("The Giant Snake claim serializes in the existing population save"),
        UGameplayStatics::SaveGameToMemory(Population, Bytes));
    UKalmalaWorldPopulationSaveGame* Reloaded =
        Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    TestNotNull(TEXT("Giant Snake reward escrow reloads"), Reloaded);
    if (Reloaded == nullptr || Reloaded->GetPendingWildlifeRewards().IsEmpty()) return false;

    TestTrue(TEXT("Reload retains the defeated Giant Snake"), Reloaded->IsDefeated(SpawnId));
    TestTrue(TEXT("Reload retains the world identity"), Reloaded->MatchesWorld(WorldConfig));
    TestEqual(TEXT("Reload retains the Giant Snake claim identity"),
        Reloaded->GetPendingWildlifeRewards()[0].PersistentSpawnId, ClaimId);
    TestEqual(TEXT("Reload retains the credited owner"),
        Reloaded->GetPendingWildlifeRewards()[0].PlayerIdentity, FString(TEXT("player:test")));
    TestTrue(TEXT("Reloaded claim remains idempotent for its owner"),
        Reloaded->TryQueuePendingGiantSnakeReward(SpawnId, TEXT("player:test")));
    TestFalse(TEXT("Reloaded claim remains bound against owner reassignment"),
        Reloaded->TryQueuePendingGiantSnakeReward(SpawnId, TEXT("player:other")));

    if (FullPack.Num() == UKalmalaInventoryComponent::MaxSlots)
    {
        UKalmalaInventoryComponent* Capacity = NewObject<UKalmalaInventoryComponent>();
        TestNotNull(TEXT("Giant Snake retry capacity validator exists"), Capacity);
        if (Capacity != nullptr)
        {
            FullPack.RemoveAt(0, 2);
            TArray<FKalmalaInventoryStack> RetryCandidate;
            FString RetryReason;
            const bool bBuiltReward = UKalmalaInventoryComponent::BuildGrant(
                FullPack, TEXT("BoneFragment"), 2, RetryCandidate, RetryReason);
            TestTrue(TEXT("Both pending fragments fit after two cells are freed"), bBuiltReward);
            if (bBuiltReward)
            {
                TestTrue(TEXT("Two fragments and one carried tool fit after retry"),
                    Capacity->CanFitContents(RetryCandidate, 1));
                const FKalmalaInventoryStack* BoneStack = RetryCandidate.FindByPredicate(
                    [](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("BoneFragment"); });
                TestTrue(TEXT("Retry candidate contains both Bone Fragments"),
                    BoneStack != nullptr && BoneStack->Quantity == 2);
            }
            TestEqual(TEXT("Preparing a retry does not prematurely clear the claim"),
                Reloaded->GetPendingWildlifeRewards().Num(), 1);
        }
    }

    TestTrue(TEXT("Delivered Giant Snake claim can be removed by its stable identity"),
        Reloaded->RemovePendingWildlifeReward(ClaimId));
    TestTrue(TEXT("Delivered claim cleanup removes its pending row"),
        Reloaded->GetPendingWildlifeRewards().IsEmpty());
    return true;
}

#endif
