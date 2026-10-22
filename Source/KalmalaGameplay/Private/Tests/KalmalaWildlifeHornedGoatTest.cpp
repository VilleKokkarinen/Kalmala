#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeHornedGoatContractTest,
    "Kalmala.Gameplay.Wildlife.HornedGoat.MountainChargeKnockbackAndRewardClaim",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWildlifeHornedGoatContractTest::RunTest(const FString& Parameters)
{
    int32 GoatSlots = 0;
    for (uint64 Seed = 0; Seed < 1000; ++Seed)
    {
        EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Mireling;
        const bool bEligible = AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(
            Seed, EKalmalaBiome::ThunderMountains, Archetype);
        if (Seed % 10ull >= 5ull && Seed % 10ull <= 8ull)
        {
            TestTrue(TEXT("The four reserved mountain slots remain eligible"), bEligible);
            TestEqual(TEXT("Mountain slots select Horned Goat"), Archetype,
                EKalmalaWildlifeArchetype::HornedGoat);
            ++GoatSlots;
        }
        else
        {
            TestFalse(TEXT("Other mountain slots remain empty for the Sentinel or future wildlife"), bEligible);
        }
    }
    TestEqual(TEXT("Horned Goat occupies forty percent of the mountain candidates"), GoatSlots, 400);

    for (const EKalmalaBiome Biome : { EKalmalaBiome::Meadows, EKalmalaBiome::Elderwood,
             EKalmalaBiome::ShimmeringLakes, EKalmalaBiome::MossyMire,
             EKalmalaBiome::FreezingTundra, EKalmalaBiome::Ocean })
    {
        for (uint64 Seed = 0; Seed < 10; ++Seed)
        {
            EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::HornedGoat;
            if (AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(Seed, Biome, Archetype))
                TestFalse(TEXT("Horned Goat remains exclusive to Thunder Mountains"),
                    Archetype == EKalmalaWildlifeArchetype::HornedGoat);
        }
    }

    TestEqual(TEXT("Horned Goat retains 200 health"),
        AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(EKalmalaWildlifeArchetype::HornedGoat), 200.0f);
    TestEqual(TEXT("Horned Goat retains 50 armor"),
        AKalmalaWildlifeSpawn::GetArmorForArchetype(EKalmalaWildlifeArchetype::HornedGoat), 50.0f);
    const FKalmalaDamageChannels Attack =
        AKalmalaWildlifeSpawn::GetBaseAttackChannelsForArchetype(EKalmalaWildlifeArchetype::HornedGoat);
    TestEqual(TEXT("The horn charge uses 50 Crush damage"), Attack.Crush, 50.0f);
    TestEqual(TEXT("Credited defeats award two Bone Fragments"),
        AKalmalaWildlifeSpawn::GetBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::HornedGoat), 2);
    TestTrue(TEXT("Dry traversable Thunder Mountains are valid habitat"),
        AKalmalaWildlifeSpawn::IsThunderMountainsSpawnHabitatAllowed(EKalmalaBiome::ThunderMountains, false, true));
    TestFalse(TEXT("Goats cannot spawn in water"),
        AKalmalaWildlifeSpawn::IsThunderMountainsSpawnHabitatAllowed(EKalmalaBiome::ThunderMountains, true, true));
    TestFalse(TEXT("Goats cannot spawn on untraversable slopes"),
        AKalmalaWildlifeSpawn::IsThunderMountainsSpawnHabitatAllowed(EKalmalaBiome::ThunderMountains, false, false));
    TestFalse(TEXT("Goats cannot spawn outside Thunder Mountains"),
        AKalmalaWildlifeSpawn::IsThunderMountainsSpawnHabitatAllowed(EKalmalaBiome::Elderwood, false, true));

    TestEqual(TEXT("The territorial trigger is three metres"),
        AKalmalaWildlifeSpawn::HornedGoatAttackTriggerCentimetres, 300.0f);
    TestEqual(TEXT("The impact boundary is 180 cm"), AKalmalaWildlifeSpawn::HornedGoatHornReachCentimetres, 180.0f);
    TestEqual(TEXT("The windup is visibly telegraphed for half a second"),
        AKalmalaWildlifeSpawn::HornedGoatTelegraphSeconds, 0.5f);
    TestEqual(TEXT("The recovery lasts 0.8 seconds"), AKalmalaWildlifeSpawn::HornedGoatRecoverySeconds, 0.8f);
    TestEqual(TEXT("The maximum push is the approved 250 cm"), AKalmalaWildlifeSpawn::HornedGoatPushCentimetres, 250.0f);
    TestEqual(TEXT("The maximum stagger is the approved 0.75 seconds"),
        AKalmalaWildlifeSpawn::HornedGoatStaggerSeconds, 0.75f);
    TestEqual(TEXT("The Goat remains within its 25 m leash"),
        AKalmalaWildlifeSpawn::HornedGoatSpawnLeashCentimetres, 2500.0f);
    TestTrue(TEXT("A server hit accepts exactly 180 cm"),
        AKalmalaWildlifeSpawn::IsHornedGoatHornInReach(true, true, 180.0f));
    TestFalse(TEXT("The horn cannot hit beyond 180 cm"),
        AKalmalaWildlifeSpawn::IsHornedGoatHornInReach(true, true, 180.01f));
    TestFalse(TEXT("A client cannot author a horn hit"),
        AKalmalaWildlifeSpawn::IsHornedGoatHornInReach(false, true, 100.0f));
    TestFalse(TEXT("An invalid target cannot receive the horn hit"),
        AKalmalaWildlifeSpawn::IsHornedGoatHornInReach(true, false, 100.0f));
    TestFalse(TEXT("Non-finite impact distances fail closed"),
        AKalmalaWildlifeSpawn::IsHornedGoatHornInReach(true, true, TNumericLimits<float>::QuietNaN()));

    const FVector PushDirection(1.0f, 0.0f, 0.0f);
    TestTrue(TEXT("The approved server knockback values are accepted"),
        AKalmalaCharacter::IsWildlifeKnockbackAllowed(true, true, true, PushDirection, 250.0f, 0.75f));
    TestFalse(TEXT("Clients cannot author wildlife knockback"),
        AKalmalaCharacter::IsWildlifeKnockbackAllowed(false, true, true, PushDirection, 250.0f, 0.75f));
    TestFalse(TEXT("A dead target cannot receive wildlife knockback"),
        AKalmalaCharacter::IsWildlifeKnockbackAllowed(true, false, true, PushDirection, 250.0f, 0.75f));
    TestFalse(TEXT("An invalid wildlife source is rejected"),
        AKalmalaCharacter::IsWildlifeKnockbackAllowed(true, true, false, PushDirection, 250.0f, 0.75f));
    TestFalse(TEXT("A zero push direction is rejected"),
        AKalmalaCharacter::IsWildlifeKnockbackAllowed(true, true, true, FVector::ZeroVector, 250.0f, 0.75f));
    TestFalse(TEXT("Pushes above the approved cap are rejected"),
        AKalmalaCharacter::IsWildlifeKnockbackAllowed(true, true, true, PushDirection, 250.01f, 0.75f));
    TestFalse(TEXT("Staggers above the approved cap are rejected"),
        AKalmalaCharacter::IsWildlifeKnockbackAllowed(true, true, true, PushDirection, 250.0f, 0.751f));

    const FString SpawnId(TEXT("-4/9/3/goat-117"));
    const FString ClaimId = UKalmalaWorldPopulationSaveGame::MakeHornedGoatPendingRewardClaimId(SpawnId);
    FString DecodedSpawnId;
    TestTrue(TEXT("The durable Goat claim fits the stable-ID limit"), !ClaimId.IsEmpty() && ClaimId.Len() <= 128);
    TestTrue(TEXT("The claim decodes the stable Goat spawn ID"),
        UKalmalaWorldPopulationSaveGame::TryGetHornedGoatSpawnIdFromPendingRewardClaimId(ClaimId, DecodedSpawnId));
    TestEqual(TEXT("The decoded reward retains its defeated spawn ID"), DecodedSpawnId, SpawnId);
    TestTrue(TEXT("Empty Goat spawn IDs are rejected"),
        UKalmalaWorldPopulationSaveGame::MakeHornedGoatPendingRewardClaimId(FString()).IsEmpty());
    TestFalse(TEXT("A Polar Bear claim cannot decode as Goat"),
        UKalmalaWorldPopulationSaveGame::TryGetHornedGoatSpawnIdFromPendingRewardClaimId(
            TEXT("polarbear-reward|1/2/3"), DecodedSpawnId));

    FKalmalaWorldGenerationConfig WorldConfig;
    WorldConfig.WorldSeed = 419;
    UKalmalaWorldPopulationSaveGame* Population = NewObject<UKalmalaWorldPopulationSaveGame>();
    TestNotNull(TEXT("Goat reward escrow can be created"), Population);
    if (Population == nullptr) return false;
    Population->InitializeForWorld(WorldConfig);
    Population->MarkDefeated(SpawnId);
    TestTrue(TEXT("A credited defeat queues its owner-bound two-Bone claim"),
        Population->TryQueuePendingHornedGoatReward(SpawnId, TEXT("player:test")));
    TestTrue(TEXT("Retrying the same owner's claim is idempotent"),
        Population->TryQueuePendingHornedGoatReward(SpawnId, TEXT("player:test")));
    TestFalse(TEXT("A different player cannot take the pending reward"),
        Population->TryQueuePendingHornedGoatReward(SpawnId, TEXT("player:other")));
    TestEqual(TEXT("Only one Goat reward row is queued"), Population->GetPendingWildlifeRewards().Num(), 1);

    TArray<uint8> Bytes;
    TestTrue(TEXT("Goat reward escrow serializes with the population save"),
        UGameplayStatics::SaveGameToMemory(Population, Bytes));
    UKalmalaWorldPopulationSaveGame* Reloaded =
        Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    TestNotNull(TEXT("The Goat reward escrow reloads"), Reloaded);
    if (Reloaded == nullptr || Reloaded->GetPendingWildlifeRewards().IsEmpty()) return false;
    TestTrue(TEXT("Reload retains the defeated Goat identity"), Reloaded->IsDefeated(SpawnId));
    TestEqual(TEXT("Reload retains the stable Goat claim"),
        Reloaded->GetPendingWildlifeRewards()[0].PersistentSpawnId, ClaimId);
    TestEqual(TEXT("Reload retains the credited owner"),
        Reloaded->GetPendingWildlifeRewards()[0].PlayerIdentity, FString(TEXT("player:test")));
    return true;
}

#endif
