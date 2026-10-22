#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
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
    const FString ClaimId = UKalmalaWorldPopulationSaveGame::MakeThunderSentinelPendingRewardClaimId(SpawnId, true);
    FString DecodedSpawnId;
    bool bDecodedHasThunderCore = false;
    TestTrue(TEXT("The durable claim fits the stable-ID limit"), !ClaimId.IsEmpty() && ClaimId.Len() <= 128);
    TestTrue(TEXT("The claim decodes the stable spawn ID"),
        UKalmalaWorldPopulationSaveGame::TryGetThunderSentinelSpawnIdFromPendingRewardClaimId(
            ClaimId, DecodedSpawnId, bDecodedHasThunderCore));
    TestEqual(TEXT("The decoded claim retains the defeated spawn ID"), DecodedSpawnId, SpawnId);
    TestTrue(TEXT("The claim stores the successful Thunder Core roll"), bDecodedHasThunderCore);
    TestTrue(TEXT("Empty spawn IDs are rejected"),
        UKalmalaWorldPopulationSaveGame::MakeThunderSentinelPendingRewardClaimId(FString(), true).IsEmpty());
    const FString NoCoreSpawnId(TEXT("-4/9/3/thundersentinel-118"));
    const FString NoCoreClaimId = UKalmalaWorldPopulationSaveGame::MakeThunderSentinelPendingRewardClaimId(
        NoCoreSpawnId, false);
    bool bDecodedHasNoCore = true;
    TestTrue(TEXT("A failed roll has a durable claim encoding"),
        UKalmalaWorldPopulationSaveGame::TryGetThunderSentinelSpawnIdFromPendingRewardClaimId(
            NoCoreClaimId, DecodedSpawnId, bDecodedHasNoCore));
    TestEqual(TEXT("The no-Core claim retains its spawn ID"), DecodedSpawnId, NoCoreSpawnId);
    TestFalse(TEXT("The claim stores the failed Thunder Core roll"), bDecodedHasNoCore);
    bool bLegacyHasCore = true;
    TestTrue(TEXT("An older unsuffixed Sentinel claim remains readable"),
        UKalmalaWorldPopulationSaveGame::TryGetThunderSentinelSpawnIdFromPendingRewardClaimId(
            FString(TEXT("thundersentinel-reward|")) + SpawnId, DecodedSpawnId, bLegacyHasCore));
    TestEqual(TEXT("The legacy claim retains the original spawn ID"), DecodedSpawnId, SpawnId);
    TestFalse(TEXT("Legacy claims remain baseline-only"), bLegacyHasCore);
    TestFalse(TEXT("A Goat claim cannot decode as a Sentinel claim"),
        UKalmalaWorldPopulationSaveGame::TryGetThunderSentinelSpawnIdFromPendingRewardClaimId(
            TEXT("hornedgoat-reward|1/2/3"), DecodedSpawnId, bDecodedHasThunderCore));
    TestFalse(TEXT("Unknown Sentinel claim suffixes are rejected"),
        UKalmalaWorldPopulationSaveGame::TryGetThunderSentinelSpawnIdFromPendingRewardClaimId(
            FString(TEXT("thundersentinel-reward|")) + SpawnId + TEXT("|unknown"),
            DecodedSpawnId, bDecodedHasThunderCore));

    FKalmalaWorldGenerationConfig WorldConfig;
    WorldConfig.WorldSeed = 431;
    UKalmalaWorldPopulationSaveGame* Population = NewObject<UKalmalaWorldPopulationSaveGame>();
    TestNotNull(TEXT("Sentinel reward escrow can be created"), Population);
    if (Population == nullptr) return false;
    Population->InitializeForWorld(WorldConfig);
    Population->MarkDefeated(SpawnId);
    TestTrue(TEXT("A credited defeat queues its stored Stone and Thunder Core outcome"),
        Population->TryQueuePendingThunderSentinelReward(SpawnId, TEXT("player:test"), true));
    TestTrue(TEXT("Retrying the same owner's claim retains its first roll"),
        Population->TryQueuePendingThunderSentinelReward(SpawnId, TEXT("player:test"), false));
    TestFalse(TEXT("A different player cannot take the pending reward"),
        Population->TryQueuePendingThunderSentinelReward(SpawnId, TEXT("player:other"), true));
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
    FString ReloadedSpawnId;
    bool bReloadedHasThunderCore = false;
    TestTrue(TEXT("Reload decodes the stored Thunder Core outcome"),
        UKalmalaWorldPopulationSaveGame::TryGetThunderSentinelSpawnIdFromPendingRewardClaimId(
            Reloaded->GetPendingWildlifeRewards()[0].PersistentSpawnId,
            ReloadedSpawnId, bReloadedHasThunderCore));
    TestTrue(TEXT("Reload retains the successful Thunder Core roll"), bReloadedHasThunderCore);

    TArray<FKalmalaInventoryStack> EmptyInventory;
    TArray<FKalmalaInventoryStack> WithStone;
    TArray<FKalmalaInventoryStack> Candidate;
    FString FailureReason;
    TestTrue(TEXT("The three-Stone Sentinel baseline fits an empty inventory"),
        UKalmalaInventoryComponent::BuildGrant(EmptyInventory, TEXT("Stone"), 3, WithStone, FailureReason));
    TestTrue(TEXT("The successful Thunder Core roll joins the same reward candidate"),
        UKalmalaInventoryComponent::BuildGrant(
            WithStone, TEXT("ThunderCore"), 1, Candidate, FailureReason));
    const FKalmalaInventoryStack* StoneStack = Candidate.FindByPredicate(
        [](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("Stone"); });
    const FKalmalaInventoryStack* CoreStack = Candidate.FindByPredicate(
        [](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("ThunderCore"); });
    TestTrue(TEXT("The accepted Sentinel baseline is exactly three Stone"),
        StoneStack != nullptr && StoneStack->Quantity == 3);
    TestTrue(TEXT("The successful material roll awards exactly one Thunder Core"),
        CoreStack != nullptr && CoreStack->Quantity == 1);

    TArray<FKalmalaInventoryStack> FullPack;
    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    if (Catalogue != nullptr)
    {
        for (const FKalmalaItemDefinition& Definition : Catalogue->Items)
        {
            if (Definition.ItemId.IsNone() || Definition.ItemId == TEXT("Stone")
                || Definition.ItemId == TEXT("ThunderCore") || Definition.MaxStack <= 0)
                continue;
            FKalmalaInventoryStack Stack;
            Stack.ItemId = Definition.ItemId;
            Stack.Quantity = 1;
            FullPack.Add(MoveTemp(Stack));
            if (FullPack.Num() == UKalmalaInventoryComponent::MaxSlots) break;
        }
    }
    TestEqual(TEXT("The capacity fixture fills all forty inventory cells"),
        FullPack.Num(), UKalmalaInventoryComponent::MaxSlots);
    if (FullPack.Num() == UKalmalaInventoryComponent::MaxSlots)
    {
        TArray<FKalmalaInventoryStack> OneCellPack = FullPack;
        OneCellPack.RemoveAt(0);
        TArray<FKalmalaInventoryStack> OneCellWithStone;
        TArray<FKalmalaInventoryStack> OneCellCandidate;
        const bool bOneCellStoneFits = UKalmalaInventoryComponent::BuildGrant(
            OneCellPack, TEXT("Stone"), 3, OneCellWithStone, FailureReason);
        const bool bOneCellCoreFits = bOneCellStoneFits && UKalmalaInventoryComponent::BuildGrant(
            OneCellWithStone, TEXT("ThunderCore"), 1, OneCellCandidate, FailureReason);
        TestFalse(TEXT("One free cell cannot accept the complete rolled bundle"), bOneCellCoreFits);
        TestEqual(TEXT("A capacity-limited Sentinel claim remains pending"),
            Reloaded->GetPendingWildlifeRewards().Num(), 1);

        TArray<FKalmalaInventoryStack> TwoCellPack = FullPack;
        TwoCellPack.RemoveAt(0, 2);
        TArray<FKalmalaInventoryStack> TwoCellWithStone;
        TArray<FKalmalaInventoryStack> TwoCellCandidate;
        const bool bTwoCellStoneFits = UKalmalaInventoryComponent::BuildGrant(
            TwoCellPack, TEXT("Stone"), 3, TwoCellWithStone, FailureReason);
        const bool bTwoCellCoreFits = bTwoCellStoneFits && UKalmalaInventoryComponent::BuildGrant(
            TwoCellWithStone, TEXT("ThunderCore"), 1, TwoCellCandidate, FailureReason);
        TestTrue(TEXT("The full rolled bundle fits after two cells are freed"), bTwoCellCoreFits);
        const FKalmalaInventoryStack* RetryCore = TwoCellCandidate.FindByPredicate(
            [](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("ThunderCore"); });
        TestTrue(TEXT("The retry candidate retains the one-Core reward"),
            RetryCore != nullptr && RetryCore->Quantity == 1);
    }
    return true;
}

#endif
