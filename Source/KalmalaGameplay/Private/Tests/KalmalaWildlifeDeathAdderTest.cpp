#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaWildlifeSpawn.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeDeathAdderContractTest,
    "Kalmala.Gameplay.Wildlife.DeathAdder.MireBiteAndRewardClaim",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWildlifeDeathAdderContractTest::RunTest(const FString& Parameters)
{
    int32 MirelingSlots = 0;
    int32 DeathAdderSlots = 0;
    int32 ReservedSlots = 0;
    for (uint64 Seed = 0; Seed < 1000; ++Seed)
    {
        EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Mireling;
        const bool bEligible = AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(
            Seed, EKalmalaBiome::MossyMire, Archetype);
        if (Seed % 10ull < 5ull)
        {
            TestTrue(TEXT("Mireling's half of Mossy Mire slots remains available"), bEligible);
            TestEqual(TEXT("The existing Mossy Mire share remains Mireling"), Archetype,
                EKalmalaWildlifeArchetype::Mireling);
            ++MirelingSlots;
        }
        else if (Seed % 10ull < 7ull)
        {
            TestTrue(TEXT("Both Death Adder candidate slots are available"), bEligible);
            TestEqual(TEXT("Death Adder occupies only its two reviewed slots"), Archetype,
                EKalmalaWildlifeArchetype::DeathAdder);
            ++DeathAdderSlots;
        }
        else
        {
            TestFalse(TEXT("Mosquito and Peat Husk slots stay empty until their own tasks"), bEligible);
            ++ReservedSlots;
        }
    }
    TestEqual(TEXT("Mireling retains fifty percent of Mossy Mire slots"), MirelingSlots, 500);
    TestEqual(TEXT("Death Adder receives twenty percent of Mossy Mire slots"), DeathAdderSlots, 200);
    TestEqual(TEXT("Other unfinished Mossy Mire shares remain unspawned"), ReservedSlots, 300);

    for (const EKalmalaBiome Biome : { EKalmalaBiome::Meadows, EKalmalaBiome::Elderwood,
             EKalmalaBiome::ShimmeringLakes, EKalmalaBiome::FreezingTundra,
             EKalmalaBiome::ThunderMountains, EKalmalaBiome::Ocean })
    {
        for (uint64 Seed = 0; Seed < 20; ++Seed)
        {
            EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::DeathAdder;
            if (AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(Seed, Biome, Archetype))
            {
                TestFalse(TEXT("Death Adder is never selected outside Mossy Mire"),
                    Archetype == EKalmalaWildlifeArchetype::DeathAdder);
            }
        }
    }
    TestTrue(TEXT("Dry Mossy Mire is eligible for its snake"),
        AKalmalaWildlifeSpawn::IsMossyMireSpawnHabitatAllowed(EKalmalaBiome::MossyMire, false));
    TestFalse(TEXT("Water rejects Death Adder spawn surfaces"),
        AKalmalaWildlifeSpawn::IsMossyMireSpawnHabitatAllowed(EKalmalaBiome::MossyMire, true));
    TestFalse(TEXT("Dry land outside Mossy Mire rejects Death Adder"),
        AKalmalaWildlifeSpawn::IsMossyMireSpawnHabitatAllowed(EKalmalaBiome::ShimmeringLakes, false));

    TestEqual(TEXT("Death Adder keeps 125 health"),
        AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(EKalmalaWildlifeArchetype::DeathAdder), 125.0f);
    TestEqual(TEXT("Death Adder keeps 20 armor"),
        AKalmalaWildlifeSpawn::GetArmorForArchetype(EKalmalaWildlifeArchetype::DeathAdder), 20.0f);
    const FKalmalaDamageChannels Bite =
        AKalmalaWildlifeSpawn::GetBaseAttackChannelsForArchetype(EKalmalaWildlifeArchetype::DeathAdder);
    TestEqual(TEXT("Its accepted bite uses 75 Stab"), Bite.Stab, 75.0f);
    TestEqual(TEXT("The coiled snake shares the reviewed short Viper warning"),
        AKalmalaWildlifeSpawn::ViperTelegraphSeconds, 0.5f);
    TestEqual(TEXT("The bite retreats after the short recovery"),
        AKalmalaWildlifeSpawn::ViperRecoverySeconds, 0.8f);
    TestEqual(TEXT("Bite reach stays close"), AKalmalaWildlifeSpawn::SnakeBiteReachCentimetres, 180.0f);
    TestEqual(TEXT("Death Adder applies ten seconds of major poison"),
        AKalmalaWildlifeSpawn::DeathAdderMajorPoisonDurationSeconds, 10.0f);
    TestEqual(TEXT("Major poison deals five damage per second"),
        UKalmalaPlayerStatusComponent::MajorPoisonDamagePerSecond, 5.0f);
    TestEqual(TEXT("Poison shares one one-second tick interval"),
        UKalmalaPlayerStatusComponent::PoisonTickIntervalSeconds, 1.0f);
    TestEqual(TEXT("Baseline Death Adder reward remains one Bone Fragment"),
        AKalmalaWildlifeSpawn::GetBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::DeathAdder), 1);

    const FString SpawnId(TEXT("-17/6/32/88421"));
    const FString ClaimId = UKalmalaWorldPopulationSaveGame::MakeDeathAdderPendingRewardClaimId(SpawnId);
    FString DecodedSpawnId;
    TestTrue(TEXT("Death Adder uses a namespaced persistent reward identity"), !ClaimId.IsEmpty());
    TestTrue(TEXT("The claim decodes to its stable spawn ID"),
        UKalmalaWorldPopulationSaveGame::TryGetDeathAdderSpawnIdFromPendingRewardClaimId(
            ClaimId, DecodedSpawnId));
    TestEqual(TEXT("Decoded identity matches the defeated spawn"), DecodedSpawnId, SpawnId);
    TestTrue(TEXT("Empty spawn identity is rejected"),
        UKalmalaWorldPopulationSaveGame::MakeDeathAdderPendingRewardClaimId(FString()).IsEmpty());
    TestFalse(TEXT("Unnamespaced claims do not decode as Death Adder"),
        UKalmalaWorldPopulationSaveGame::TryGetDeathAdderSpawnIdFromPendingRewardClaimId(
            SpawnId, DecodedSpawnId));

    FKalmalaWorldGenerationConfig WorldConfig;
    WorldConfig.WorldSeed = 418;
    UKalmalaWorldPopulationSaveGame* Population = NewObject<UKalmalaWorldPopulationSaveGame>();
    TestNotNull(TEXT("Death Adder reward escrow exists"), Population);
    if (Population == nullptr) return false;
    Population->InitializeForWorld(WorldConfig);
    Population->MarkDefeated(SpawnId);
    TestTrue(TEXT("Credited baseline reward is queued for its owner"),
        Population->TryQueuePendingDeathAdderReward(SpawnId, TEXT("player:test")));
    TestTrue(TEXT("Retry by the credited owner is idempotent"),
        Population->TryQueuePendingDeathAdderReward(SpawnId, TEXT("player:test")));
    TestFalse(TEXT("A different player cannot claim the defeated snake"),
        Population->TryQueuePendingDeathAdderReward(SpawnId, TEXT("player:other")));
    TestEqual(TEXT("One pending row remains per defeated snake"), Population->GetPendingWildlifeRewards().Num(), 1);

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
    TestEqual(TEXT("Full-pack retry fixture fills all forty cells"),
        FullPack.Num(), UKalmalaInventoryComponent::MaxSlots);
    if (FullPack.Num() == UKalmalaInventoryComponent::MaxSlots)
    {
        TArray<FKalmalaInventoryStack> Rejected;
        FString FailureReason;
        TestFalse(TEXT("Full pack defers the Bone Fragment grant"),
            UKalmalaInventoryComponent::BuildGrant(FullPack, TEXT("BoneFragment"), 1,
                Rejected, FailureReason));
        TestEqual(TEXT("Capacity rejection preserves the claim"), Population->GetPendingWildlifeRewards().Num(), 1);
        FullPack.RemoveAt(0, 2);
        TArray<FKalmalaInventoryStack> Retry;
        TestTrue(TEXT("The baseline reward can be prepared after capacity is freed"),
            UKalmalaInventoryComponent::BuildGrant(FullPack, TEXT("BoneFragment"), 1, Retry, FailureReason));
    }

    TArray<uint8> Bytes;
    TestTrue(TEXT("Claim survives population-save serialization"),
        UGameplayStatics::SaveGameToMemory(Population, Bytes));
    UKalmalaWorldPopulationSaveGame* Reloaded =
        Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    TestNotNull(TEXT("Saved reward claim reloads"), Reloaded);
    if (Reloaded != nullptr && !Reloaded->GetPendingWildlifeRewards().IsEmpty())
    {
        TestTrue(TEXT("Reload retains the defeated spawn"), Reloaded->IsDefeated(SpawnId));
        TestEqual(TEXT("Reload retains the owner-bound claim"),
            Reloaded->GetPendingWildlifeRewards()[0].PersistentSpawnId, ClaimId);
        TestEqual(TEXT("Reload retains its credited player"),
            Reloaded->GetPendingWildlifeRewards()[0].PlayerIdentity, FString(TEXT("player:test")));
        TestTrue(TEXT("Reloaded reward remains idempotent for its owner"),
            Reloaded->TryQueuePendingDeathAdderReward(SpawnId, TEXT("player:test")));
        TestFalse(TEXT("Reloaded reward cannot be reassigned"),
            Reloaded->TryQueuePendingDeathAdderReward(SpawnId, TEXT("player:other")));
    }
    return true;
}

#endif
