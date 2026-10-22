#include "KalmalaWildlifeSpawn.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeBadgerContractTest,
    "Kalmala.Gameplay.Wildlife.Badger.SpawnAndClawReach",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWildlifeBadgerContractTest::RunTest(const FString& Parameters)
{
    int32 ExistingRosterSlots = 0;
    int32 BadgerSlots = 0;
    int32 MoonBearSlots = 0;
    for (uint64 Seed = 0; Seed < 1000; ++Seed)
    {
        EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Mireling;
        const bool bEligible = AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(
            Seed, EKalmalaBiome::Elderwood, Archetype);
        const uint64 Slot = Seed % 10ull;
        if (Slot <= 4)
        {
            TestTrue(TEXT("Existing Elderwood candidate slots stay populated"), bEligible);
            TestFalse(TEXT("Existing roster slots do not select Badger"), Archetype == EKalmalaWildlifeArchetype::Badger);
            ++ExistingRosterSlots;
        }
        else if (Slot <= 8)
        {
            TestTrue(TEXT("Badger common slots stay populated"), bEligible);
            TestEqual(TEXT("Badger occupies the 40% common share"), Archetype, EKalmalaWildlifeArchetype::Badger);
            ++BadgerSlots;
        }
        else
        {
            TestTrue(TEXT("Moon Bear fills the Elderwood hard-enemy share"), bEligible);
            TestEqual(TEXT("The stable hard slot selects Moon Bear"), Archetype, EKalmalaWildlifeArchetype::MoonBear);
            ++MoonBearSlots;
        }

        if (AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(
                Seed, EKalmalaBiome::Meadows, Archetype))
        {
            TestFalse(TEXT("Badger remains Elderwood-only"), Archetype == EKalmalaWildlifeArchetype::Badger);
        }
    }

    TestEqual(TEXT("Elderwood existing roster slot count"), ExistingRosterSlots, 500);
    TestEqual(TEXT("Elderwood Badger slot count"), BadgerSlots, 400);
    TestEqual(TEXT("Elderwood Moon Bear hard slot count"), MoonBearSlots, 100);
    TestEqual(TEXT("Badger warns within three metres"),
        AKalmalaWildlifeSpawn::BadgerAttackTriggerCentimetres, 300.0f);
    TestEqual(TEXT("Badger claws use the existing wildlife hit bound"),
        AKalmalaWildlifeSpawn::BadgerClawReachCentimetres, 180.0f);
    TestEqual(TEXT("Badger attack warning duration"),
        AKalmalaWildlifeSpawn::BadgerTelegraphSeconds, 0.5f);
    TestEqual(TEXT("Badger attack recovery duration"),
        AKalmalaWildlifeSpawn::BadgerRecoverySeconds, 0.8f);
    TestEqual(TEXT("Badger pursuit remains within 25 metres"),
        AKalmalaWildlifeSpawn::BadgerSpawnLeashCentimetres, 2500.0f);
    TestEqual(TEXT("Badger baseline Bone Fragment reward quantity is one"),
        AKalmalaWildlifeSpawn::GetBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::Badger), 1);
    TestEqual(TEXT("Rat baseline Bone Fragment reward quantity is one"),
        AKalmalaWildlifeSpawn::GetBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::Rat), 1);
    TestEqual(TEXT("Giant Snake keeps its existing two-fragment reward"),
        AKalmalaWildlifeSpawn::GetBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::GiantSnake), 2);
    TestEqual(TEXT("Moon Bear baseline Bone Fragment reward is three"),
        AKalmalaWildlifeSpawn::GetBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::MoonBear), 3);

    const FString BadgerSpawnId(TEXT("1/4/-8/123456"));
    const FString BadgerClaimId = UKalmalaWorldPopulationSaveGame::MakeBadgerPendingRewardClaimId(BadgerSpawnId);
    FString DecodedBadgerSpawnId;
    TestTrue(TEXT("Badger reward claims retain a namespaced stable identity"), !BadgerClaimId.IsEmpty());
    TestTrue(TEXT("Badger reward claims decode to the original spawn identity"),
        UKalmalaWorldPopulationSaveGame::TryGetBadgerSpawnIdFromPendingRewardClaimId(BadgerClaimId, DecodedBadgerSpawnId));
    TestEqual(TEXT("Decoded Badger identity matches the spawned wildlife"), DecodedBadgerSpawnId, BadgerSpawnId);
    TestFalse(TEXT("Legacy unprefixed Rat claims do not decode as Badger"),
        UKalmalaWorldPopulationSaveGame::TryGetBadgerSpawnIdFromPendingRewardClaimId(BadgerSpawnId, DecodedBadgerSpawnId));

    UKalmalaWorldPopulationSaveGame* Population = NewObject<UKalmalaWorldPopulationSaveGame>();
    TestNotNull(TEXT("Pending Badger reward escrow can be created"), Population);
    if (Population != nullptr)
    {
        FKalmalaWorldGenerationConfig World;
        World.WorldSeed = 418;
        Population->InitializeForWorld(World);
        Population->MarkDefeated(BadgerSpawnId);
        TestTrue(TEXT("A credited Badger reward is queued for its owner"),
            Population->TryQueuePendingBadgerReward(BadgerSpawnId, TEXT("player:test")));
        TestTrue(TEXT("Retrying the same Badger claim is idempotent"),
            Population->TryQueuePendingBadgerReward(BadgerSpawnId, TEXT("player:test")));
        TestFalse(TEXT("A Badger claim cannot be reassigned to another owner"),
            Population->TryQueuePendingBadgerReward(BadgerSpawnId, TEXT("player:other")));
        TestEqual(TEXT("One row is retained for the credited Badger"),
            Population->GetPendingWildlifeRewards().Num(), 1);

        TArray<FKalmalaInventoryStack> FullPack;
        const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
        if (Catalogue != nullptr)
        {
            for (const FKalmalaItemDefinition& Definition : Catalogue->Items)
            {
                if (Definition.ItemId.IsNone() || Definition.ItemId == TEXT("BoneFragment")
                    || Definition.ItemId == TEXT("BadgerFur") || Definition.ItemId == TEXT("BadgerMeat")
                    || Definition.MaxStack <= 0)
                    continue;
                FKalmalaInventoryStack Stack;
                Stack.ItemId = Definition.ItemId;
                Stack.Quantity = 1;
                FullPack.Add(MoveTemp(Stack));
                if (FullPack.Num() == UKalmalaInventoryComponent::MaxSlots) break;
            }
        }
        TestEqual(TEXT("The full-pack retry fixture uses all forty catalogue cells"),
            FullPack.Num(), UKalmalaInventoryComponent::MaxSlots);
        if (FullPack.Num() == UKalmalaInventoryComponent::MaxSlots)
        {
            TArray<FKalmalaInventoryStack> FullPackCandidate;
            FString GrantReason;
            TestFalse(TEXT("A full pack cannot accept the Badger Bone Fragment candidate"),
                UKalmalaInventoryComponent::BuildGrant(FullPack, TEXT("BoneFragment"), 1, FullPackCandidate, GrantReason));
            TestEqual(TEXT("The rejection is caused by pack cell capacity"),
                GrantReason, FString(TEXT("Pack output capacity reached")));
            TestEqual(TEXT("A full-pack rejection leaves the pending Badger claim queued"),
                Population->GetPendingWildlifeRewards().Num(), 1);
        }

        TArray<uint8> Serialized;
        TestTrue(TEXT("Pending Badger reward serializes in the existing population save"),
            UGameplayStatics::SaveGameToMemory(Population, Serialized));
        UKalmalaWorldPopulationSaveGame* Reloaded =
            Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(Serialized));
        TestNotNull(TEXT("Pending Badger reward save reloads"), Reloaded);
        if (Reloaded != nullptr && !Reloaded->GetPendingWildlifeRewards().IsEmpty())
        {
            TestTrue(TEXT("Reload retains the defeated Badger"), Reloaded->IsDefeated(BadgerSpawnId));
            TestTrue(TEXT("Reload retains the world identity"), Reloaded->MatchesWorld(World));
            TestEqual(TEXT("Reload retains the Badger claim identity"),
                Reloaded->GetPendingWildlifeRewards()[0].PersistentSpawnId, BadgerClaimId);
            TestEqual(TEXT("Reload retains the credited Badger owner"),
                Reloaded->GetPendingWildlifeRewards()[0].PlayerIdentity, FString(TEXT("player:test")));
            TestTrue(TEXT("Reloaded Badger claim remains idempotent for its credited owner"),
                Reloaded->TryQueuePendingBadgerReward(BadgerSpawnId, TEXT("player:test")));
            TestFalse(TEXT("Reloaded Badger claim cannot be reassigned to another owner"),
                Reloaded->TryQueuePendingBadgerReward(BadgerSpawnId, TEXT("player:other")));
            TestEqual(TEXT("Owner retries after reload keep one pending Badger claim"),
                Reloaded->GetPendingWildlifeRewards().Num(), 1);

            if (FullPack.Num() == UKalmalaInventoryComponent::MaxSlots)
            {
                UKalmalaInventoryComponent* Capacity = NewObject<UKalmalaInventoryComponent>();
                TestNotNull(TEXT("Inventory capacity validator can be created"), Capacity);
                if (Capacity != nullptr)
                {
                    TestFalse(TEXT("A full pack plus one carried tool exceeds capacity"),
                        Capacity->CanFitContents(FullPack, 1));
                    FullPack.RemoveAt(0, 4);
                    TestTrue(TEXT("Freeing four pack cells allows a carried tool and Badger reward bundle"),
                        Capacity->CanFitContents(FullPack, 1));

                    TArray<FKalmalaInventoryStack> WithBone;
                    TArray<FKalmalaInventoryStack> WithFur;
                    TArray<FKalmalaInventoryStack> RetryCandidate;
                    FString RetryReason;
                    const bool bBuiltBadgerBundle =
                        UKalmalaInventoryComponent::BuildGrant(FullPack, TEXT("BoneFragment"), 1, WithBone, RetryReason)
                        && UKalmalaInventoryComponent::BuildGrant(WithBone, TEXT("BadgerFur"), 1, WithFur, RetryReason)
                        && UKalmalaInventoryComponent::BuildGrant(WithFur, TEXT("BadgerMeat"), 1, RetryCandidate, RetryReason);
                    TestTrue(TEXT("The Badger reward bundle succeeds after cells are freed"), bBuiltBadgerBundle);
                    const auto HasOne = [&RetryCandidate](const FName ItemId)
                    {
                        const FKalmalaInventoryStack* Stack = RetryCandidate.FindByPredicate(
                            [ItemId](const FKalmalaInventoryStack& Candidate) { return Candidate.ItemId == ItemId; });
                        return Stack != nullptr && Stack->Quantity == 1;
                    };
                    TestTrue(TEXT("The retry bundle contains one Bone Fragment"), HasOne(TEXT("BoneFragment")));
                    TestTrue(TEXT("The retry bundle contains one Badger Fur"), HasOne(TEXT("BadgerFur")));
                    TestTrue(TEXT("The retry bundle contains one Badger Meat"), HasOne(TEXT("BadgerMeat")));
                    TestTrue(TEXT("The bundle and one carried tool fit together after retry"),
                        Capacity->CanFitContents(RetryCandidate, 1));
                    TestEqual(TEXT("The durable claim remains queued through candidate preparation"),
                        Reloaded->GetPendingWildlifeRewards().Num(), 1);
                }
            }

            TestTrue(TEXT("Retry claim can be cleared by its stable reward identity"),
                Reloaded->RemovePendingWildlifeReward(BadgerClaimId));
            TestEqual(TEXT("Cleared Badger claim leaves no pending reward"),
                Reloaded->GetPendingWildlifeRewards().Num(), 0);
        }
    }

    TestTrue(TEXT("Server accepts an in-range claw"),
        AKalmalaWildlifeSpawn::IsBadgerClawInReach(true, true, 180.0f));
    TestFalse(TEXT("Claw reach rejects targets beyond 180 cm"),
        AKalmalaWildlifeSpawn::IsBadgerClawInReach(true, true, 180.01f));
    TestFalse(TEXT("Claw reach rejects negative distance"),
        AKalmalaWildlifeSpawn::IsBadgerClawInReach(true, true, -1.0f));
    TestFalse(TEXT("Claw reach rejects non-finite distance"),
        AKalmalaWildlifeSpawn::IsBadgerClawInReach(true, true, std::numeric_limits<float>::quiet_NaN()));
    TestFalse(TEXT("Client cannot author Badger damage reach"),
        AKalmalaWildlifeSpawn::IsBadgerClawInReach(false, true, 100.0f));
    TestFalse(TEXT("Invalid target cannot receive a Badger claw"),
        AKalmalaWildlifeSpawn::IsBadgerClawInReach(true, false, 100.0f));
    return true;
}
