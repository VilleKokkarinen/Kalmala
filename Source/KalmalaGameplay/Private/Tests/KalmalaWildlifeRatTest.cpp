#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeRatContractTest,
    "Kalmala.Gameplay.Wildlife.Rat.SpawnReachAndRewardIdentity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWildlifeRatContractTest::RunTest(const FString& Parameters)
{
    int32 RatSlots = 0;
    for (uint64 Seed = 0; Seed < 1000; ++Seed)
    {
        EKalmalaWildlifeArchetype MeadowsArchetype = EKalmalaWildlifeArchetype::Mireling;
        TestTrue(TEXT("Every Meadows candidate remains eligible"),
            AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(Seed, EKalmalaBiome::Meadows, MeadowsArchetype));
        if (Seed % 10ull == 9ull)
        {
            ++RatSlots;
            TestEqual(TEXT("Every tenth deterministic Meadows slot selects Rat"),
                MeadowsArchetype, EKalmalaWildlifeArchetype::Rat);
        }
        else
        {
            TestEqual(TEXT("Other Meadows slots retain the existing seeded roster"),
                MeadowsArchetype, AKalmalaWildlifeSpawn::GetArchetypeForSpawnSeed(Seed));
        }

        for (const EKalmalaBiome Biome : { EKalmalaBiome::Elderwood, EKalmalaBiome::ShimmeringLakes,
            EKalmalaBiome::MossyMire, EKalmalaBiome::FreezingTundra, EKalmalaBiome::ThunderMountains })
        {
            EKalmalaWildlifeArchetype OtherBiomeArchetype = EKalmalaWildlifeArchetype::Rat;
            if (AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(Seed, Biome, OtherBiomeArchetype))
                TestTrue(TEXT("Rat is Meadows-only"), OtherBiomeArchetype != EKalmalaWildlifeArchetype::Rat);
        }
    }
    TestEqual(TEXT("Meadows uses exactly ten percent Rat candidate slots"), RatSlots, 100);

    TestTrue(TEXT("The approved 100 cm impact boundary is inclusive"),
        AKalmalaWildlifeSpawn::IsRatBiteInReach(true, true, 100.0f));
    TestFalse(TEXT("A bite beyond 100 cm is rejected"),
        AKalmalaWildlifeSpawn::IsRatBiteInReach(true, true, 100.01f));
    TestFalse(TEXT("A non-authority cannot apply a Rat bite"),
        AKalmalaWildlifeSpawn::IsRatBiteInReach(false, true, 50.0f));
    TestFalse(TEXT("An invalid target cannot receive a Rat bite"),
        AKalmalaWildlifeSpawn::IsRatBiteInReach(true, false, 50.0f));
    TestFalse(TEXT("A non-finite bite distance is rejected"),
        AKalmalaWildlifeSpawn::IsRatBiteInReach(true, true, TNumericLimits<float>::QuietNaN()));
    TestEqual(TEXT("The approach trigger remains three metres"),
        AKalmalaWildlifeSpawn::RatAttackTriggerCentimetres, 300.0f);
    TestEqual(TEXT("Rat uses the common half-second telegraph"),
        AKalmalaWildlifeSpawn::RatTelegraphSeconds, 0.5f);
    TestEqual(TEXT("Rat uses the common 0.8-second recovery"),
        AKalmalaWildlifeSpawn::RatRecoverySeconds, 0.8f);
    TestEqual(TEXT("Rat pursuit is bounded to 25 metres from its spawn"),
        AKalmalaWildlifeSpawn::RatSpawnLeashCentimetres, 2500.0f);

    UKalmalaWorldPopulationSaveGame* Population = NewObject<UKalmalaWorldPopulationSaveGame>();
    TestNotNull(TEXT("Pending reward escrow can be created"), Population);
    if (Population != nullptr)
    {
        FKalmalaWorldGenerationConfig World;
        World.WorldSeed = 418;
        Population->InitializeForWorld(World);
        Population->MarkDefeated(TEXT("wildlife:test-rat"));
        TestTrue(TEXT("A credited spawn reward is queued"),
            Population->TryQueuePendingWildlifeReward(TEXT("wildlife:test-rat"), TEXT("player:test")));
        TestTrue(TEXT("Retrying the same stable claim is idempotent"),
            Population->TryQueuePendingWildlifeReward(TEXT("wildlife:test-rat"), TEXT("player:test")));
        TestFalse(TEXT("A stable spawn claim cannot be reassigned"),
            Population->TryQueuePendingWildlifeReward(TEXT("wildlife:test-rat"), TEXT("player:other")));
        TestEqual(TEXT("One pending row remains for one spawn identity"),
            Population->GetPendingWildlifeRewards().Num(), 1);

        TArray<uint8> Serialized;
        TestTrue(TEXT("Pending Rat rewards serialize in the existing population save"),
            UGameplayStatics::SaveGameToMemory(Population, Serialized));
        UKalmalaWorldPopulationSaveGame* Reloaded =
            Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(Serialized));
        TestNotNull(TEXT("The pending reward save reloads"), Reloaded);
        if (Reloaded != nullptr && !Reloaded->GetPendingWildlifeRewards().IsEmpty())
        {
            TestTrue(TEXT("Reload retains the defeated spawn claim"),
                Reloaded->IsDefeated(TEXT("wildlife:test-rat")));
            TestTrue(TEXT("Reload retains the immutable world identity"), Reloaded->MatchesWorld(World));
            TestEqual(TEXT("Reload retains the credited spawn identity"),
                Reloaded->GetPendingWildlifeRewards()[0].PersistentSpawnId, FString(TEXT("wildlife:test-rat")));
            TestEqual(TEXT("Reload retains its credited owner"),
                Reloaded->GetPendingWildlifeRewards()[0].PlayerIdentity, FString(TEXT("player:test")));
        }
        TestTrue(TEXT("A delivered claim can be removed"),
            Population->RemovePendingWildlifeReward(TEXT("wildlife:test-rat")));
        TestEqual(TEXT("Delivery clears its escrow row"),
            Population->GetPendingWildlifeRewards().Num(), 0);
    }

    return true;
}

#endif
