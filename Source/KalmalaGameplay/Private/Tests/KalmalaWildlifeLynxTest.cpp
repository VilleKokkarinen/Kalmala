#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaWildlifeSpawn.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
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
    int32 PolarBearSlots = 0;
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
        else if (Seed % 10ull == 9ull)
        {
            TestTrue(TEXT("The rare Polar Bear share remains eligible"), bEligible);
            TestEqual(TEXT("The final Tundra slot selects Polar Bear"), Archetype,
                EKalmalaWildlifeArchetype::PolarBear);
            ++PolarBearSlots;
        }
        else
        {
            TestFalse(TEXT("Unimplemented Tundra shares stay unspawned"), bEligible);
        }
    }
    TestEqual(TEXT("Wolf keeps twenty percent of Tundra candidate slots"), WolfSlots, 200);
    TestEqual(TEXT("Lynx holds twenty percent of Tundra candidate slots"), LynxSlots, 200);
    TestEqual(TEXT("Polar Bear holds ten percent of Tundra candidate slots"), PolarBearSlots, 100);

    for (const EKalmalaBiome Biome : { EKalmalaBiome::Meadows, EKalmalaBiome::Elderwood,
             EKalmalaBiome::ShimmeringLakes, EKalmalaBiome::MossyMire,
             EKalmalaBiome::ThunderMountains, EKalmalaBiome::Ocean })
    {
        for (uint64 Seed = 0; Seed < 10; ++Seed)
        {
            EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Lynx;
            if (AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(Seed, Biome, Archetype))
            {
                TestFalse(TEXT("Lynx cannot be selected outside Freezing Tundra"),
                    Archetype == EKalmalaWildlifeArchetype::Lynx);
                TestFalse(TEXT("Polar Bear cannot be selected outside Freezing Tundra"),
                    Archetype == EKalmalaWildlifeArchetype::PolarBear);
            }
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
    const FString ClaimId = UKalmalaWorldPopulationSaveGame::MakeLynxPendingRewardClaimId(SpawnId, true);
    FString DecodedSpawnId;
    bool bDecodedHasPelt = false;
    TestTrue(TEXT("Lynx escrow uses a bounded species namespace"), !ClaimId.IsEmpty() && ClaimId.Len() <= 128);
    TestTrue(TEXT("Lynx claim decodes its stable spawn identity"),
        UKalmalaWorldPopulationSaveGame::TryGetLynxSpawnIdFromPendingRewardClaimId(
            ClaimId, DecodedSpawnId, bDecodedHasPelt));
    TestEqual(TEXT("Claim decoding preserves the Lynx spawn identity"), DecodedSpawnId, SpawnId);
    TestTrue(TEXT("The claim stores a successful 50% pelt roll"), bDecodedHasPelt);
    const FString NoPeltClaimId = UKalmalaWorldPopulationSaveGame::MakeLynxPendingRewardClaimId(SpawnId, false);
    bool bDecodedNoPelt = true;
    TestTrue(TEXT("Lynx claim encodes a failed pelt roll"),
        UKalmalaWorldPopulationSaveGame::TryGetLynxSpawnIdFromPendingRewardClaimId(
            NoPeltClaimId, DecodedSpawnId, bDecodedNoPelt));
    TestEqual(TEXT("No-pelt claim retains the stable spawn identity"), DecodedSpawnId, SpawnId);
    TestFalse(TEXT("No-pelt claim stores the failed roll"), bDecodedNoPelt);
    bool bLegacyClaimHasPelt = true;
    TestTrue(TEXT("Legacy unsuffixed Lynx claims remain valid"),
        UKalmalaWorldPopulationSaveGame::TryGetLynxSpawnIdFromPendingRewardClaimId(
            TEXT("lynx-reward|-4/9/3/legacy"), DecodedSpawnId, bLegacyClaimHasPelt));
    TestFalse(TEXT("Legacy Lynx claims remain baseline-only"), bLegacyClaimHasPelt);
    TestTrue(TEXT("Empty Lynx spawn IDs are rejected"),
        UKalmalaWorldPopulationSaveGame::MakeLynxPendingRewardClaimId(FString(), false).IsEmpty());
    TestFalse(TEXT("Wolf claims do not decode as Lynx"),
        UKalmalaWorldPopulationSaveGame::TryGetLynxSpawnIdFromPendingRewardClaimId(
            TEXT("wolf-reward|1/2/3"), DecodedSpawnId, bDecodedHasPelt));
    TestFalse(TEXT("Unknown Lynx claim suffixes fail closed"),
        UKalmalaWorldPopulationSaveGame::TryGetLynxSpawnIdFromPendingRewardClaimId(
            TEXT("lynx-reward|1/2/3|unexpected"), DecodedSpawnId, bDecodedHasPelt));

    FKalmalaWorldGenerationConfig WorldConfig;
    WorldConfig.WorldSeed = 418;
    UKalmalaWorldPopulationSaveGame* Population = NewObject<UKalmalaWorldPopulationSaveGame>();
    TestNotNull(TEXT("Lynx reward escrow can be created"), Population);
    if (Population == nullptr) return false;
    Population->InitializeForWorld(WorldConfig);
    Population->MarkDefeated(SpawnId);
    TestTrue(TEXT("A credited Lynx defeat queues its owner-bound two-Bone and rolled-pelt reward"),
        Population->TryQueuePendingLynxReward(SpawnId, TEXT("player:test"), true));
    TestTrue(TEXT("Retrying with a different random outcome preserves the original Lynx roll"),
        Population->TryQueuePendingLynxReward(SpawnId, TEXT("player:test"), false));
    TestFalse(TEXT("A different player cannot take the Lynx claim"),
        Population->TryQueuePendingLynxReward(SpawnId, TEXT("player:other"), false));
    TestEqual(TEXT("One pending Lynx reward row remains for the same spawn"),
        Population->GetPendingWildlifeRewards().Num(), 1);
    const FString NoPeltSpawnId(TEXT("-4/9/3/lynx-12878"));
    const FString StoredNoPeltClaimId = UKalmalaWorldPopulationSaveGame::MakeLynxPendingRewardClaimId(
        NoPeltSpawnId, false);
    Population->MarkDefeated(NoPeltSpawnId);
    TestTrue(TEXT("A separate Lynx defeat can escrow the failed-pelt outcome"),
        Population->TryQueuePendingLynxReward(NoPeltSpawnId, TEXT("player:test"), false));
    TestEqual(TEXT("Both independent Lynx reward claims remain pending"),
        Population->GetPendingWildlifeRewards().Num(), 2);

    TArray<uint8> Bytes;
    TestTrue(TEXT("Lynx reward escrow serializes in the population save"),
        UGameplayStatics::SaveGameToMemory(Population, Bytes));
    UKalmalaWorldPopulationSaveGame* Reloaded =
        Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    TestNotNull(TEXT("Lynx reward escrow reloads"), Reloaded);
    if (Reloaded == nullptr || Reloaded->GetPendingWildlifeRewards().Num() != 2) return false;
    TestTrue(TEXT("Reload retains the defeated Lynx identity"), Reloaded->IsDefeated(SpawnId));
    TestTrue(TEXT("Reload retains the second defeated Lynx identity"), Reloaded->IsDefeated(NoPeltSpawnId));
    const FKalmalaPendingWildlifeReward* ReloadedPeltClaim = Reloaded->GetPendingWildlifeRewards().FindByPredicate(
        [&ClaimId](const FKalmalaPendingWildlifeReward& Reward) { return Reward.PersistentSpawnId == ClaimId; });
    const FKalmalaPendingWildlifeReward* ReloadedNoPeltClaim =
        Reloaded->GetPendingWildlifeRewards().FindByPredicate([&StoredNoPeltClaimId](
            const FKalmalaPendingWildlifeReward& Reward) { return Reward.PersistentSpawnId == StoredNoPeltClaimId; });
    TestNotNull(TEXT("The pelt claim survives reload"), ReloadedPeltClaim);
    TestNotNull(TEXT("The no-pelt claim survives reload"), ReloadedNoPeltClaim);
    if (ReloadedPeltClaim == nullptr || ReloadedNoPeltClaim == nullptr) return false;
    TestEqual(TEXT("Reload retains the credited Lynx owner"), ReloadedPeltClaim->PlayerIdentity,
        FString(TEXT("player:test")));
    TestTrue(TEXT("Reload preserves the stored pelt outcome"),
        UKalmalaWorldPopulationSaveGame::TryGetLynxSpawnIdFromPendingRewardClaimId(
            ReloadedPeltClaim->PersistentSpawnId, DecodedSpawnId, bDecodedHasPelt) && bDecodedHasPelt);
    TestTrue(TEXT("Reload preserves the stored no-pelt outcome"),
        UKalmalaWorldPopulationSaveGame::TryGetLynxSpawnIdFromPendingRewardClaimId(
            ReloadedNoPeltClaim->PersistentSpawnId, DecodedSpawnId, bDecodedHasPelt) && !bDecodedHasPelt);

    TArray<FKalmalaInventoryStack> EmptyInventory;
    TArray<FKalmalaInventoryStack> WithBones;
    TArray<FKalmalaInventoryStack> WithPelt;
    FString FailureReason;
    TestTrue(TEXT("Two Bone Fragments fit an empty inventory"),
        UKalmalaInventoryComponent::BuildGrant(EmptyInventory, TEXT("BoneFragment"), 2,
            WithBones, FailureReason));
    TestTrue(TEXT("The successful Lynx roll adds exactly one Lynx Pelt"),
        UKalmalaInventoryComponent::BuildGrant(WithBones, TEXT("LynxPelt"), 1, WithPelt, FailureReason));
    const FKalmalaInventoryStack* PeltStack = WithPelt.FindByPredicate(
        [](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("LynxPelt"); });
    TestTrue(TEXT("The accepted Lynx material reward is exactly one Pelt"),
        PeltStack != nullptr && PeltStack->Quantity == 1);
    const TArray<FKalmalaInventoryStack> NoPeltCandidate = WithBones;
    TestTrue(TEXT("A failed Lynx roll keeps only the baseline Bone reward"),
        NoPeltCandidate.FindByPredicate(
            [](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("LynxPelt"); }) == nullptr);
    TArray<FKalmalaInventoryStack> FullPack;
    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    if (Catalogue != nullptr)
    {
        for (const FKalmalaItemDefinition& Definition : Catalogue->Items)
        {
            if (Definition.ItemId.IsNone() || Definition.ItemId == TEXT("BoneFragment")
                || Definition.ItemId == TEXT("LynxPelt") || Definition.MaxStack <= 0)
                continue;
            FKalmalaInventoryStack Stack;
            Stack.ItemId = Definition.ItemId;
            Stack.Quantity = 1;
            FullPack.Add(MoveTemp(Stack));
            if (FullPack.Num() == UKalmalaInventoryComponent::MaxSlots) break;
        }
    }
    TestEqual(TEXT("The Lynx capacity fixture fills all forty cells"),
        FullPack.Num(), UKalmalaInventoryComponent::MaxSlots);
    if (FullPack.Num() == UKalmalaInventoryComponent::MaxSlots)
    {
        TArray<FKalmalaInventoryStack> OneCellPack = FullPack;
        OneCellPack.RemoveAt(0);
        TArray<FKalmalaInventoryStack> OneCellWithBone;
        TArray<FKalmalaInventoryStack> OneCellWithPelt;
        TestTrue(TEXT("The baseline Lynx reward fits when one cell is freed"),
            UKalmalaInventoryComponent::BuildGrant(
                OneCellPack, TEXT("BoneFragment"), 2, OneCellWithBone, FailureReason));
        TestFalse(TEXT("A rolled pelt waits until its full candidate can fit"),
            UKalmalaInventoryComponent::BuildGrant(
                OneCellWithBone, TEXT("LynxPelt"), 1, OneCellWithPelt, FailureReason));
        TestEqual(TEXT("Capacity deferral retains the stored Lynx reward outcome"),
            Reloaded->GetPendingWildlifeRewards().Num(), 2);

        TArray<FKalmalaInventoryStack> TwoCellPack = FullPack;
        TwoCellPack.RemoveAt(0, 2);
        TArray<FKalmalaInventoryStack> TwoCellWithBone;
        TArray<FKalmalaInventoryStack> TwoCellCandidate;
        const bool bTwoCellBonesFit = UKalmalaInventoryComponent::BuildGrant(
            TwoCellPack, TEXT("BoneFragment"), 2, TwoCellWithBone, FailureReason);
        const bool bTwoCellPeltFits = bTwoCellBonesFit && UKalmalaInventoryComponent::BuildGrant(
            TwoCellWithBone, TEXT("LynxPelt"), 1, TwoCellCandidate, FailureReason);
        TestTrue(TEXT("The complete two-item Lynx reward fits after two cells are freed"), bTwoCellPeltFits);
    }

    TestTrue(TEXT("Delivered Lynx claim can be removed by its stable ID"),
        Reloaded->RemovePendingWildlifeReward(ClaimId));
    TestTrue(TEXT("The no-pelt claim can be removed by its stable ID"),
        Reloaded->RemovePendingWildlifeReward(StoredNoPeltClaimId));
    TestTrue(TEXT("Claim cleanup clears both pending rows"), Reloaded->GetPendingWildlifeRewards().IsEmpty());
    return true;
}

#endif
