#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaGemstoneContract.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaGemstoneContractTest, "Kalmala.World.GemstoneContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaGemstoneContractTest::RunTest(const FString& Parameters)
{
    uint8 Hits = 0, Outcome = 0;
    int32 Success = 0;
    for (uint32 A = 0; A < 10; ++A) for (uint32 B = 0; B < 10; ++B) for (uint32 C = 0; C < 10; ++C)
    {
        Hits = Outcome = 0;
        for (uint32 Roll : {A, B, C})
        {
            uint8 NextHits = 99, NextOutcome = 99;
            if (FKalmalaGemstoneContract::Advance(Hits, Outcome, Roll, NextHits, NextOutcome))
            { Hits = NextHits; Outcome = NextOutcome; }
        }
        Success += Outcome == 2;
    }
    TestEqual(TEXT("Three independent per-hit rolls give 729/1000 extraction"), Success, 729);
    TestTrue(TEXT("Final hit can shatter before extraction"), FKalmalaGemstoneContract::Advance(2, 0, 0, Hits, Outcome));
    TestEqual(TEXT("Final smash has no gem"), Outcome, uint8(1));
    TestFalse(TEXT("Terminal rock cannot reroll"), FKalmalaGemstoneContract::Advance(Hits, Outcome, 9, Hits, Outcome));
    TestFalse(TEXT("Invalid roll cannot mutate"), FKalmalaGemstoneContract::Advance(0, 0, 10, Hits, Outcome));
    int32 Quartz = 0, Jade = 0;
    for (uint64 Seed = 0; Seed < 10000; ++Seed)
    {
        const FName Selected = FKalmalaGemstoneContract::SelectReplacement(TEXT("lakes-tin-shore-gravel"), EKalmalaBiome::ShimmeringLakes, Seed);
        Quartz += Selected == TEXT("gem-quartz-rock"); Jade += Selected == TEXT("gem-jade-rock");
        TestEqual(TEXT("Same seed retains the single selected occupant"), Selected,
            FKalmalaGemstoneContract::SelectReplacement(TEXT("lakes-tin-shore-gravel"), EKalmalaBiome::ShimmeringLakes, Seed));
        TestEqual(TEXT("Wrong biome cannot replace"), FKalmalaGemstoneContract::SelectReplacement(
            TEXT("lakes-tin-shore-gravel"), EKalmalaBiome::Meadows, Seed), FName(TEXT("lakes-tin-shore-gravel")));
    }
    TestTrue(TEXT("Quartz selection is near five percent"), Quartz > 400 && Quartz < 600);
    TestTrue(TEXT("Jade selection is near two percent"), Jade > 140 && Jade < 260);
    int32 Moonstones = 0;
    for (uint64 Seed = 0; Seed < 10000; ++Seed)
    {
        Moonstones += FKalmalaGemstoneContract::SelectEnemyGem(true, EKalmalaBiome::Elderwood, Seed) == TEXT("Moonstone");
        TestTrue(TEXT("Non-bear cannot yield Moonstone"), FKalmalaGemstoneContract::SelectEnemyGem(false, EKalmalaBiome::Elderwood, Seed).IsNone());
        TestTrue(TEXT("Wrong-biome bear cannot yield Moonstone"), FKalmalaGemstoneContract::SelectEnemyGem(true, EKalmalaBiome::Meadows, Seed).IsNone());
    }
    TestTrue(TEXT("Moonstone selection is near two percent"), Moonstones > 140 && Moonstones < 260);
    const FKalmalaWorldGenerationConfig Config;
    auto* Save = NewObject<UKalmalaWorldPopulationSaveGame>();
    Save->InitializeForWorld(Config);
    FKalmalaGemstoneState State;
    State.SpawnId = TEXT("1/2/3/456"); State.SourceId = TEXT("gem-quartz-rock"); State.Hits = 2;
    TestTrue(TEXT("Partial progress accepted"), Save->SetGemstone(State));
    TArray<uint8> Bytes;
    TestTrue(TEXT("Progress serializes"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    auto* Restored = Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Progress restores"), Restored)) return false;
    TestTrue(TEXT("World identity retained"), Restored->MatchesWorld(Config));
    TestTrue(TEXT("Partial hits survive reload"), Restored->FindGemstone(State.SpawnId) && Restored->FindGemstone(State.SpawnId)->Hits == 2);
    State.Hits = 3; State.Outcome = 2;
    TestTrue(TEXT("Loose gem state persists"), Save->SetGemstone(State));
    State.Outcome = 3;
    TestTrue(TEXT("Claimed state persists"), Save->SetGemstone(State));
    State.Hits = 1;
    TestFalse(TEXT("Invalid early extraction rejected"), Save->SetGemstone(State));
    return true;
}
#endif
