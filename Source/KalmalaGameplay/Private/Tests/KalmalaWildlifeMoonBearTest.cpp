#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeMoonBearContractTest,
    "Kalmala.Gameplay.Wildlife.MoonBear.HardSpawnPawAndRewardClaim",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWildlifeMoonBearContractTest::RunTest(const FString& Parameters)
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
            TestTrue(TEXT("Existing Elderwood slots remain eligible"), bEligible);
            TestFalse(TEXT("Moon Bear does not replace the existing roster"),
                Archetype == EKalmalaWildlifeArchetype::MoonBear);
            ++ExistingRosterSlots;
        }
        else if (Slot <= 8)
        {
            TestTrue(TEXT("Elderwood common slots remain eligible"), bEligible);
            TestEqual(TEXT("Badger retains the common share"), Archetype, EKalmalaWildlifeArchetype::Badger);
            ++BadgerSlots;
        }
        else
        {
            TestTrue(TEXT("The Elderwood hard slot is populated"), bEligible);
            TestEqual(TEXT("The hard slot selects Moon Bear"), Archetype, EKalmalaWildlifeArchetype::MoonBear);
            ++MoonBearSlots;
        }

        EKalmalaWildlifeArchetype OtherBiomeArchetype = EKalmalaWildlifeArchetype::Mireling;
        if (AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(
                Seed, EKalmalaBiome::Meadows, OtherBiomeArchetype))
        {
            TestFalse(TEXT("Moon Bear remains Elderwood-only"),
                OtherBiomeArchetype == EKalmalaWildlifeArchetype::MoonBear);
        }
    }

    TestEqual(TEXT("Elderwood retains half its slots for the existing roster"), ExistingRosterSlots, 500);
    TestEqual(TEXT("Elderwood keeps forty percent Badger slots"), BadgerSlots, 400);
    TestEqual(TEXT("Elderwood assigns ten percent to Moon Bear"), MoonBearSlots, 100);
    TestEqual(TEXT("Moon Bear paw reach matches the approved close-range value"),
        AKalmalaWildlifeSpawn::MoonBearPawReachCentimetres, 220.0f);
    TestEqual(TEXT("Moon Bear strike has the reviewed one-second warning"),
        AKalmalaWildlifeSpawn::MoonBearTelegraphSeconds, 1.0f);
    TestEqual(TEXT("Moon Bear strike has the reviewed 1.2-second recovery"),
        AKalmalaWildlifeSpawn::MoonBearRecoverySeconds, 1.2f);
    TestEqual(TEXT("Moon Bear pursuit remains within its 25-metre spawn leash"),
        AKalmalaWildlifeSpawn::MoonBearSpawnLeashCentimetres, 2500.0f);
    TestEqual(TEXT("Moon Bear baseline defeat reward is three Bone Fragments"),
        AKalmalaWildlifeSpawn::GetBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::MoonBear), 3);

    TestTrue(TEXT("The server accepts a paw hit at 220 cm"),
        AKalmalaWildlifeSpawn::IsMoonBearPawInReach(true, true, 220.0f));
    TestFalse(TEXT("The paw cannot hit beyond its close-range reach"),
        AKalmalaWildlifeSpawn::IsMoonBearPawInReach(true, true, 220.01f));
    TestFalse(TEXT("A client cannot author the paw hit"),
        AKalmalaWildlifeSpawn::IsMoonBearPawInReach(false, true, 100.0f));
    TestFalse(TEXT("An invalid target cannot receive the paw hit"),
        AKalmalaWildlifeSpawn::IsMoonBearPawInReach(true, false, 100.0f));
    TestFalse(TEXT("The paw reach rejects a negative distance"),
        AKalmalaWildlifeSpawn::IsMoonBearPawInReach(true, true, -1.0f));
    TestFalse(TEXT("The paw reach rejects non-finite distance"),
        AKalmalaWildlifeSpawn::IsMoonBearPawInReach(true, true, TNumericLimits<float>::QuietNaN()));

    const FString SpawnId(TEXT("1/4/-8/123456"));
    const FString ClaimId = UKalmalaWorldPopulationSaveGame::MakeMoonBearPendingRewardClaimId(SpawnId);
    FString DecodedSpawnId;
    TestTrue(TEXT("A stable Moon Bear spawn encodes a reward claim"), !ClaimId.IsEmpty());
    TestTrue(TEXT("The claim decodes its stable spawn identity"),
        UKalmalaWorldPopulationSaveGame::TryGetMoonBearSpawnIdFromPendingRewardClaimId(ClaimId, DecodedSpawnId));
    TestEqual(TEXT("Claim decoding preserves the spawn identity"), DecodedSpawnId, SpawnId);
    TestTrue(TEXT("Empty Moon Bear spawn identities are rejected"),
        UKalmalaWorldPopulationSaveGame::MakeMoonBearPendingRewardClaimId(FString()).IsEmpty());

    UKalmalaWorldPopulationSaveGame* Population = NewObject<UKalmalaWorldPopulationSaveGame>();
    TestNotNull(TEXT("Population save fixture exists"), Population);
    if (Population != nullptr)
    {
        FKalmalaWorldGenerationConfig World;
        World.WorldSeed = 418;
        Population->InitializeForWorld(World);
        Population->MarkDefeated(SpawnId);
        TestTrue(TEXT("The credited Moon Bear reward is queued for its owner"),
            Population->TryQueuePendingMoonBearReward(SpawnId, TEXT("player:test")));
        TestTrue(TEXT("Retrying the claim for the owner is idempotent"),
            Population->TryQueuePendingMoonBearReward(SpawnId, TEXT("player:test")));
        TestFalse(TEXT("Another owner cannot claim the defeated Moon Bear"),
            Population->TryQueuePendingMoonBearReward(SpawnId, TEXT("player:other")));
        TestEqual(TEXT("Only one reward claim is retained"), Population->GetPendingWildlifeRewards().Num(), 1);

        TArray<uint8> Bytes;
        TestTrue(TEXT("The queued Moon Bear reward serializes in the existing population save"),
            UGameplayStatics::SaveGameToMemory(Population, Bytes));
        UKalmalaWorldPopulationSaveGame* Reloaded =
            Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
        TestNotNull(TEXT("The queued Moon Bear reward reloads"), Reloaded);
        if (Reloaded != nullptr && !Reloaded->GetPendingWildlifeRewards().IsEmpty())
        {
            TestTrue(TEXT("Reload preserves the defeated spawn"), Reloaded->IsDefeated(SpawnId));
            TestEqual(TEXT("Reload preserves the stable claim ID"),
                Reloaded->GetPendingWildlifeRewards()[0].PersistentSpawnId, ClaimId);
            TestEqual(TEXT("Reload preserves the credited owner"),
                Reloaded->GetPendingWildlifeRewards()[0].PlayerIdentity, FString(TEXT("player:test")));
            TestTrue(TEXT("The claim remains idempotent for the same owner after reload"),
                Reloaded->TryQueuePendingMoonBearReward(SpawnId, TEXT("player:test")));
            TestFalse(TEXT("Reloaded claims cannot be reassigned"),
                Reloaded->TryQueuePendingMoonBearReward(SpawnId, TEXT("player:other")));
            TestEqual(TEXT("Reload retries retain a single claim"), Reloaded->GetPendingWildlifeRewards().Num(), 1);
            TestTrue(TEXT("The owner can clear the claim after delivery"), Reloaded->RemovePendingWildlifeReward(ClaimId));
            TestEqual(TEXT("Delivered claim cleanup leaves no pending rows"), Reloaded->GetPendingWildlifeRewards().Num(), 0);
        }
    }
    return true;
}
