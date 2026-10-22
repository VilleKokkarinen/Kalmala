#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWolfPackContract.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeWolfContractTest,
    "Kalmala.Gameplay.Wildlife.Wolf.TundraPackLungeAndRewardClaim",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWildlifeWolfContractTest::RunTest(const FString& Parameters)
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
            TestTrue(TEXT("Wolf occupies its two deterministic Tundra candidate slots"), bEligible);
            TestEqual(TEXT("Tundra Wolf slots select the Wolf archetype"), Archetype,
                EKalmalaWildlifeArchetype::Wolf);
            ++WolfSlots;
        }
        else if (Seed % 10ull == 7ull || Seed % 10ull == 8ull)
        {
            TestTrue(TEXT("Lynx occupies its two deterministic Tundra candidate slots"), bEligible);
            TestEqual(TEXT("Tundra Lynx slots select the Lynx archetype"), Archetype,
                EKalmalaWildlifeArchetype::Lynx);
            ++LynxSlots;
        }
        else if (Seed % 10ull == 9ull)
        {
            TestTrue(TEXT("Polar Bear owns the rare seeded Tundra share"), bEligible);
            TestEqual(TEXT("The final Tundra slot selects Polar Bear"), Archetype,
                EKalmalaWildlifeArchetype::PolarBear);
            ++PolarBearSlots;
        }
        else
        {
            TestFalse(TEXT("Unimplemented Tundra shares stay empty"), bEligible);
        }
    }
    TestEqual(TEXT("Wolf receives twenty percent of Tundra candidate slots"), WolfSlots, 200);
    TestEqual(TEXT("Lynx receives twenty percent of Tundra candidate slots"), LynxSlots, 200);
    TestEqual(TEXT("Polar Bear receives ten percent of Tundra candidate slots"), PolarBearSlots, 100);

    for (const EKalmalaBiome Biome : { EKalmalaBiome::Meadows, EKalmalaBiome::Elderwood,
             EKalmalaBiome::ShimmeringLakes, EKalmalaBiome::MossyMire,
             EKalmalaBiome::ThunderMountains, EKalmalaBiome::Ocean })
    {
        for (uint64 Seed = 0; Seed < 10; ++Seed)
        {
            EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Wolf;
            if (AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(Seed, Biome, Archetype))
            {
                TestTrue(TEXT("Wolf is exclusive to Freezing Tundra"),
                    Archetype != EKalmalaWildlifeArchetype::Wolf);
                TestTrue(TEXT("Lynx is exclusive to Freezing Tundra"),
                    Archetype != EKalmalaWildlifeArchetype::Lynx);
                TestTrue(TEXT("Polar Bear is exclusive to Freezing Tundra"),
                    Archetype != EKalmalaWildlifeArchetype::PolarBear);
            }
        }
    }

    TestEqual(TEXT("Wolf bite reach uses the user-reviewed 180 cm boundary"),
        AKalmalaWildlifeSpawn::WolfBiteReachCentimetres, 180.0f);
    TestTrue(TEXT("The Wolf bite boundary is inclusive on the server"),
        AKalmalaWildlifeSpawn::IsWolfBiteInReach(true, true, 180.0f));
    TestFalse(TEXT("The Wolf bite cannot hit beyond its reviewed reach"),
        AKalmalaWildlifeSpawn::IsWolfBiteInReach(true, true, 180.01f));
    TestFalse(TEXT("A client cannot apply a Wolf bite"),
        AKalmalaWildlifeSpawn::IsWolfBiteInReach(false, true, 100.0f));
    TestFalse(TEXT("An invalid target cannot receive a Wolf bite"),
        AKalmalaWildlifeSpawn::IsWolfBiteInReach(true, false, 100.0f));
    TestEqual(TEXT("Wolf uses the common half-second warning"),
        AKalmalaWildlifeSpawn::WolfTelegraphSeconds, 0.5f);
    TestEqual(TEXT("Wolf uses the common 0.8-second recovery"),
        AKalmalaWildlifeSpawn::WolfRecoverySeconds, 0.8f);
    TestEqual(TEXT("Wolf pursuit stays within the 25 m spawn leash"),
        AKalmalaWildlifeSpawn::WolfSpawnLeashCentimetres, 2500.0f);
    TestEqual(TEXT("Wolf baseline reward is two Bone Fragments"),
        AKalmalaWildlifeSpawn::GetBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::Wolf), 2);
    TestEqual(TEXT("Tundra reserves enough actor capacity for one complete pack"),
        FKalmalaWolfPackContract::GetActorBudgetWithPackRoom(1), FKalmalaWolfPackContract::MemberCount);
    TestEqual(TEXT("Tundra preserves an already sufficient wildlife actor cap"),
        FKalmalaWolfPackContract::GetActorBudgetWithPackRoom(4), 4);
    const FKalmalaDamageChannels Attack =
        AKalmalaWildlifeSpawn::GetBaseAttackChannelsForArchetype(EKalmalaWildlifeArchetype::Wolf);
    TestEqual(TEXT("Wolf bite retains its authored 100 Slash damage"), Attack.Slash, 100.0f);

    FKalmalaWorldPopulationSpawn PackSpawn;
    PackSpawn.Kind = EKalmalaWorldPopulationKind::Wildlife;
    PackSpawn.SpatialKey = FIntPoint(-4, 7);
    PackSpawn.SpawnSeed = 123456789ull;
    TArray<FString> MemberIds;
    FVector2D OffsetSum = FVector2D::ZeroVector;
    for (int32 MemberIndex = 0; MemberIndex < FKalmalaWolfPackContract::MemberCount; ++MemberIndex)
    {
        const FVector2D FirstOffset = FKalmalaWolfPackContract::GetMemberOffset(PackSpawn.SpawnSeed, MemberIndex);
        const FVector2D RepeatOffset = FKalmalaWolfPackContract::GetMemberOffset(PackSpawn.SpawnSeed, MemberIndex);
        TestTrue(TEXT("Pack formation offsets repeat for the same stable seed"),
            (FirstOffset - RepeatOffset).IsNearlyZero());
        TestTrue(TEXT("Every formation offset uses the reviewed formation radius"),
            FMath::IsNearlyEqual(FirstOffset.Size(), FKalmalaWolfPackContract::FormationRadiusCentimetres, 0.01f));
        OffsetSum += FirstOffset;

        const FString MemberId = FKalmalaWolfPackContract::GetMemberPersistentSpawnId(PackSpawn, MemberIndex);
        TestTrue(TEXT("Each pack member has a stable nonempty identity"), !MemberId.IsEmpty());
        for (const FString& ExistingMemberId : MemberIds)
            TestTrue(TEXT("Pack-member identities do not collide"), ExistingMemberId != MemberId);
        MemberIds.Add(MemberId);
    }
    TestTrue(TEXT("The three-member formation remains centered on its generated anchor"),
        OffsetSum.Size() < 0.01f);
    TestEqual(TEXT("Member identities are unique within one generated pack"),
        MemberIds.Num(), FKalmalaWolfPackContract::MemberCount);
    TestTrue(TEXT("Invalid pack-member indexes do not receive an identity"),
        FKalmalaWolfPackContract::GetMemberPersistentSpawnId(PackSpawn, 3).IsEmpty());

    const FString ClaimId = UKalmalaWorldPopulationSaveGame::MakeWolfPendingRewardClaimId(MemberIds[0], true);
    FString DecodedSpawnId;
    bool bDecodedHasWolfFang = false;
    TestTrue(TEXT("Wolf escrow uses a bounded species namespace"), !ClaimId.IsEmpty() && ClaimId.Len() <= 128);
    TestTrue(TEXT("Wolf reward claim decodes to the unique pack-member identity"),
        UKalmalaWorldPopulationSaveGame::TryGetWolfSpawnIdFromPendingRewardClaimId(
            ClaimId, DecodedSpawnId, bDecodedHasWolfFang));
    TestEqual(TEXT("Wolf reward claim retains the exact pack-member identity"), DecodedSpawnId, MemberIds[0]);
    TestTrue(TEXT("Wolf reward claim stores the successful fang roll"), bDecodedHasWolfFang);

    const FString NoFangClaimId =
        UKalmalaWorldPopulationSaveGame::MakeWolfPendingRewardClaimId(MemberIds[1], false);
    bool bDecodedNoWolfFang = true;
    TestTrue(TEXT("Wolf reward claim encodes a failed fang roll"),
        UKalmalaWorldPopulationSaveGame::TryGetWolfSpawnIdFromPendingRewardClaimId(
            NoFangClaimId, DecodedSpawnId, bDecodedNoWolfFang));
    TestEqual(TEXT("No-fang claim retains its member identity"), DecodedSpawnId, MemberIds[1]);
    TestFalse(TEXT("No-fang claim stores the failed roll"), bDecodedNoWolfFang);

    bool bLegacyWolfFang = true;
    TestTrue(TEXT("An older unsuffixed Wolf claim remains readable"),
        UKalmalaWorldPopulationSaveGame::TryGetWolfSpawnIdFromPendingRewardClaimId(
            FString(TEXT("wolf-reward|")) + MemberIds[2], DecodedSpawnId, bLegacyWolfFang));
    TestEqual(TEXT("Legacy Wolf claim retains its member identity"), DecodedSpawnId, MemberIds[2]);
    TestFalse(TEXT("Legacy Wolf claim does not invent a fang roll"), bLegacyWolfFang);
    TestFalse(TEXT("Unknown Wolf claim suffixes are rejected"),
        UKalmalaWorldPopulationSaveGame::TryGetWolfSpawnIdFromPendingRewardClaimId(
            FString(TEXT("wolf-reward|")) + MemberIds[0] + TEXT("|unknown"),
            DecodedSpawnId, bLegacyWolfFang));

    FKalmalaWorldGenerationConfig WorldConfig;
    WorldConfig.WorldSeed = 418;
    UKalmalaWorldPopulationSaveGame* Population = NewObject<UKalmalaWorldPopulationSaveGame>();
    TestNotNull(TEXT("Wolf reward escrow can be created"), Population);
    if (Population == nullptr) return false;
    Population->InitializeForWorld(WorldConfig);
    Population->MarkDefeated(MemberIds[0]);
    TestTrue(TEXT("A credited Wolf defeat queues its owner-bound rolled reward"),
        Population->TryQueuePendingWolfReward(MemberIds[0], TEXT("player:test"), true));
    TestTrue(TEXT("Retrying the Wolf reward for its owner is idempotent"),
        Population->TryQueuePendingWolfReward(MemberIds[0], TEXT("player:test"), false));
    TestFalse(TEXT("A Wolf reward cannot be reassigned to another owner"),
        Population->TryQueuePendingWolfReward(MemberIds[0], TEXT("player:other"), false));
    TestEqual(TEXT("The saved pending Wolf reward remains one claim"),
        Population->GetPendingWildlifeRewards().Num(), 1);
    TestTrue(TEXT("The saved pending Wolf reward retains its original fang outcome"),
        Population->GetPendingWildlifeRewards().Num() == 1
            && Population->GetPendingWildlifeRewards()[0].PersistentSpawnId == ClaimId);

    TArray<uint8> Bytes;
    TestTrue(TEXT("Wolf reward escrow serializes in the existing population save"),
        UGameplayStatics::SaveGameToMemory(Population, Bytes));
    UKalmalaWorldPopulationSaveGame* Reloaded =
        Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    TestNotNull(TEXT("Wolf reward escrow reloads"), Reloaded);
    if (Reloaded == nullptr || Reloaded->GetPendingWildlifeRewards().IsEmpty()) return false;
    TestTrue(TEXT("Reload retains the defeated pack-member identity"), Reloaded->IsDefeated(MemberIds[0]));
    TestTrue(TEXT("Reload retains the immutable world identity"), Reloaded->MatchesWorld(WorldConfig));
    TestEqual(TEXT("Reload retains the Wolf claim identity"),
        Reloaded->GetPendingWildlifeRewards()[0].PersistentSpawnId, ClaimId);
    TestEqual(TEXT("Reload retains the credited Wolf owner"),
        Reloaded->GetPendingWildlifeRewards()[0].PlayerIdentity, FString(TEXT("player:test")));

    TArray<FKalmalaInventoryStack> WithBone;
    TArray<FKalmalaInventoryStack> WithPelt;
    TArray<FKalmalaInventoryStack> Candidate;
    TArray<FKalmalaInventoryStack> EmptyInventory;
    FString FailureReason;
    TestTrue(TEXT("The two-fragment Wolf baseline fits an empty inventory"),
        UKalmalaInventoryComponent::BuildGrant(EmptyInventory, TEXT("BoneFragment"), 2, WithBone, FailureReason));
    TestTrue(TEXT("The guaranteed Wolf Pelt joins the same reward candidate"),
        UKalmalaInventoryComponent::BuildGrant(WithBone, TEXT("WolfPelt"), 1, WithPelt, FailureReason));
    TestTrue(TEXT("The rolled Wolf Fang joins the same reward candidate"),
        UKalmalaInventoryComponent::BuildGrant(WithPelt, TEXT("WolfFang"), 1, Candidate, FailureReason));
    const FKalmalaInventoryStack* BoneStack = Candidate.FindByPredicate(
        [](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("BoneFragment"); });
    const FKalmalaInventoryStack* PeltStack = Candidate.FindByPredicate(
        [](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("WolfPelt"); });
    const FKalmalaInventoryStack* FangStack = Candidate.FindByPredicate(
        [](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("WolfFang"); });
    TestTrue(TEXT("Wolf's accepted baseline is exactly two Bone Fragments"),
        BoneStack != nullptr && BoneStack->Quantity == 2);
    TestTrue(TEXT("Wolf's accepted material reward is exactly one Pelt"),
        PeltStack != nullptr && PeltStack->Quantity == 1);
    TestTrue(TEXT("Wolf's successful roll awards exactly one Fang"),
        FangStack != nullptr && FangStack->Quantity == 1);

    TArray<FKalmalaInventoryStack> NoFangCandidate;
    TestTrue(TEXT("The no-fang outcome keeps the guaranteed Wolf bundle"),
        UKalmalaInventoryComponent::BuildGrant(WithBone, TEXT("WolfPelt"), 1, NoFangCandidate, FailureReason));
    TestTrue(TEXT("A failed Wolf Fang roll does not grant a Fang"),
        NoFangCandidate.FindByPredicate(
            [](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("WolfFang"); }) == nullptr);

    TArray<FKalmalaInventoryStack> FullPack;
    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    if (Catalogue != nullptr)
    {
        for (const FKalmalaItemDefinition& Definition : Catalogue->Items)
        {
            if (Definition.ItemId.IsNone() || Definition.ItemId == TEXT("BoneFragment")
                || Definition.ItemId == TEXT("WolfPelt") || Definition.ItemId == TEXT("WolfFang")
                || Definition.MaxStack <= 0)
                continue;
            FKalmalaInventoryStack Stack;
            Stack.ItemId = Definition.ItemId;
            Stack.Quantity = 1;
            FullPack.Add(MoveTemp(Stack));
            if (FullPack.Num() == UKalmalaInventoryComponent::MaxSlots) break;
        }
    }
    TestEqual(TEXT("The Wolf capacity fixture fills all forty cells"),
        FullPack.Num(), UKalmalaInventoryComponent::MaxSlots);
    if (FullPack.Num() == UKalmalaInventoryComponent::MaxSlots)
    {
        TArray<FKalmalaInventoryStack> FullWithBone;
        FString FullReason;
        TestFalse(TEXT("A full pack defers the complete Wolf reward"),
            UKalmalaInventoryComponent::BuildGrant(
                FullPack, TEXT("BoneFragment"), 2, FullWithBone, FullReason));

        TArray<FKalmalaInventoryStack> NearlyFullPack = FullPack;
        NearlyFullPack.RemoveAt(0);
        TArray<FKalmalaInventoryStack> NearlyFullWithBone;
        TArray<FKalmalaInventoryStack> NearlyFullCandidate;
        FString NearlyFullReason;
        const bool bBonesFit = UKalmalaInventoryComponent::BuildGrant(
            NearlyFullPack, TEXT("BoneFragment"), 2, NearlyFullWithBone, NearlyFullReason);
        const bool bPeltFits = bBonesFit && UKalmalaInventoryComponent::BuildGrant(
            NearlyFullWithBone, TEXT("WolfPelt"), 1, NearlyFullCandidate, NearlyFullReason);
        TestTrue(TEXT("Two Bone Fragments fit the one free cell"), bBonesFit);
        TestFalse(TEXT("The Pelt waits when only the baseline fits"), bPeltFits);
        TestEqual(TEXT("Partial reward deferral retains the capacity reason"),
            NearlyFullReason, FString(TEXT("Pack output capacity reached")));
        TestEqual(TEXT("The owner-bound Wolf claim remains queued until the complete bundle fits"),
            Population->GetPendingWildlifeRewards().Num(), 1);

        TArray<FKalmalaInventoryStack> TwoCellPack = FullPack;
        TwoCellPack.RemoveAt(0, 2);
        TArray<FKalmalaInventoryStack> TwoCellWithBone;
        TArray<FKalmalaInventoryStack> TwoCellWithPelt;
        TArray<FKalmalaInventoryStack> TwoCellCandidate;
        FString TwoCellReason;
        const bool bTwoCellBonesFit = UKalmalaInventoryComponent::BuildGrant(
            TwoCellPack, TEXT("BoneFragment"), 2, TwoCellWithBone, TwoCellReason);
        const bool bTwoCellPeltFits = bTwoCellBonesFit && UKalmalaInventoryComponent::BuildGrant(
            TwoCellWithBone, TEXT("WolfPelt"), 1, TwoCellWithPelt, TwoCellReason);
        const bool bTwoCellFangFits = bTwoCellPeltFits && UKalmalaInventoryComponent::BuildGrant(
            TwoCellWithPelt, TEXT("WolfFang"), 1, TwoCellCandidate, TwoCellReason);
        TestTrue(TEXT("The two baseline items fit after two cells are freed"), bTwoCellPeltFits);
        TestFalse(TEXT("The Fang roll remains deferred when only baseline items fit"), bTwoCellFangFits);
        TestEqual(TEXT("Preparing the Wolf retry retains its durable claim"),
            Population->GetPendingWildlifeRewards().Num(), 1);

        TArray<FKalmalaInventoryStack> RetryPack = FullPack;
        RetryPack.RemoveAt(0, 3);
        TArray<FKalmalaInventoryStack> RetryWithBone;
        TArray<FKalmalaInventoryStack> RetryWithPelt;
        TArray<FKalmalaInventoryStack> RetryCandidate;
        FString RetryReason;
        const bool bRetryBonesFit = UKalmalaInventoryComponent::BuildGrant(
            RetryPack, TEXT("BoneFragment"), 2, RetryWithBone, RetryReason);
        const bool bRetryPeltFits = bRetryBonesFit && UKalmalaInventoryComponent::BuildGrant(
            RetryWithBone, TEXT("WolfPelt"), 1, RetryWithPelt, RetryReason);
        const bool bRetryFangFits = bRetryPeltFits && UKalmalaInventoryComponent::BuildGrant(
            RetryWithPelt, TEXT("WolfFang"), 1, RetryCandidate, RetryReason);
        TestTrue(TEXT("The complete Wolf bundle fits after three cells are freed"), bRetryFangFits);
        TestEqual(TEXT("Preparing a complete Wolf retry retains its durable claim"),
            Population->GetPendingWildlifeRewards().Num(), 1);
    }

    TestTrue(TEXT("A delivered Wolf claim can be removed by its stable ID"),
        Reloaded->RemovePendingWildlifeReward(ClaimId));
    TestTrue(TEXT("Wolf claim cleanup clears the pending row"), Reloaded->GetPendingWildlifeRewards().IsEmpty());
    return true;
}

#endif
