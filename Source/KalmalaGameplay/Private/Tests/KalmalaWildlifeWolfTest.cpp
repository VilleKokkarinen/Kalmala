#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWolfPackContract.h"
#include "KalmalaInventoryComponent.h"
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

    const FString ClaimId = UKalmalaWorldPopulationSaveGame::MakeWolfPendingRewardClaimId(MemberIds[0]);
    FString DecodedSpawnId;
    TestTrue(TEXT("Wolf escrow uses a bounded species namespace"), !ClaimId.IsEmpty() && ClaimId.Len() <= 128);
    TestTrue(TEXT("Wolf reward claim decodes to the unique pack-member identity"),
        UKalmalaWorldPopulationSaveGame::TryGetWolfSpawnIdFromPendingRewardClaimId(ClaimId, DecodedSpawnId));
    TestEqual(TEXT("Wolf reward claim retains the exact pack-member identity"), DecodedSpawnId, MemberIds[0]);

    FKalmalaWorldGenerationConfig WorldConfig;
    WorldConfig.WorldSeed = 418;
    UKalmalaWorldPopulationSaveGame* Population = NewObject<UKalmalaWorldPopulationSaveGame>();
    TestNotNull(TEXT("Wolf reward escrow can be created"), Population);
    if (Population == nullptr) return false;
    Population->InitializeForWorld(WorldConfig);
    Population->MarkDefeated(MemberIds[0]);
    TestTrue(TEXT("A credited Wolf defeat queues its owner-bound baseline reward"),
        Population->TryQueuePendingWolfReward(MemberIds[0], TEXT("player:test")));
    TestTrue(TEXT("Retrying the Wolf reward for its owner is idempotent"),
        Population->TryQueuePendingWolfReward(MemberIds[0], TEXT("player:test")));
    TestFalse(TEXT("A Wolf reward cannot be reassigned to another owner"),
        Population->TryQueuePendingWolfReward(MemberIds[0], TEXT("player:other")));
    TestEqual(TEXT("The saved pending Wolf reward remains one two-fragment claim"),
        Population->GetPendingWildlifeRewards().Num(), 1);

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

    TArray<FKalmalaInventoryStack> Candidate;
    TArray<FKalmalaInventoryStack> EmptyInventory;
    FString FailureReason;
    TestTrue(TEXT("The two-fragment Wolf baseline fits an empty inventory"),
        UKalmalaInventoryComponent::BuildGrant(EmptyInventory, TEXT("BoneFragment"), 2, Candidate, FailureReason));
    const FKalmalaInventoryStack* BoneStack = Candidate.FindByPredicate(
        [](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("BoneFragment"); });
    TestTrue(TEXT("Wolf's accepted baseline is exactly two Bone Fragments"),
        BoneStack != nullptr && BoneStack->Quantity == 2);

    TestTrue(TEXT("A delivered Wolf claim can be removed by its stable ID"),
        Reloaded->RemovePendingWildlifeReward(ClaimId));
    TestTrue(TEXT("Wolf claim cleanup clears the pending row"), Reloaded->GetPendingWildlifeRewards().IsEmpty());
    return true;
}

#endif
