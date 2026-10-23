#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaGemstoneContract.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "KalmalaWorldPopulationLayout.h"
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
    FKalmalaTreasureState Treasure;
    Treasure.Id = TEXT("treasure:Poi:1:elderwood-quiet-clearing:2,3:0");
    TestTrue(TEXT("Empty treasure roll is retained"), Save->SetTreasure(Treasure));
    TestFalse(TEXT("Opening cannot reroll even an empty chest"), Save->SetTreasure(Treasure));
    Treasure.Items.Add(TEXT("Garnet"));
    Treasure.bClaimed = true;
    TestFalse(TEXT("Claim cannot change generated loot"), Save->SetTreasure(Treasure));
    Treasure.Items.Reset();
    TestTrue(TEXT("Empty chest can be marked opened once"), Save->SetTreasure(Treasure));
    TestFalse(TEXT("Competing second claim rejected"), Save->SetTreasure(Treasure));
    TestTrue(TEXT("Treasure claim serializes"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    Restored = Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    TestTrue(TEXT("Empty claim survives reload without reroll"), Restored && Restored->MatchesWorld(Config)
        && Restored->FindTreasure(Treasure.Id) && Restored->FindTreasure(Treasure.Id)->bClaimed);
    State.Hits = 1;
    TestFalse(TEXT("Invalid early extraction rejected"), Save->SetGemstone(State));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaGemstoneGenerationTest, "Kalmala.World.GemstoneGeneration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaGemstoneGenerationTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 418;
    int32 Replacements = 0;
    for (int32 Y = -16; Y <= 16; Y += 2) for (int32 X = -16; X <= 16; X += 2)
    {
        const FIntPoint Key(X, Y);
        const auto Before = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(Config, Key, EKalmalaWorldPopulationKind::HarvestNode, false);
        const auto After = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(Config, Key, EKalmalaWorldPopulationKind::HarvestNode);
        if (!TestEqual(TEXT("Gemstones never add candidates"), After.Num(), Before.Num())) return false;
        for (int32 Index = 0; Index < Before.Num(); ++Index)
        {
            TestEqual(TEXT("Replacement retains sparse identity"), FKalmalaWorldPopulationLayout::GetPersistentSpawnId(After[Index]),
                FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Before[Index]));
            TestTrue(TEXT("Replacement retains exact location and suitability"), After[Index].Location == Before[Index].Location);
            if (After[Index].ContentId == Before[Index].ContentId) continue;
            ++Replacements;
            TestTrue(TEXT("Changed slot is an enabled gemstone"), FKalmalaGemstoneContract::IsAcquisitionEnabled(After[Index].ContentId));
            const auto Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, FVector2D(Before[Index].Location)));
            const FName Original = Before[Index].ContentId;
            TestTrue(TEXT("Only approved resource/biome slots change"),
                (Original == TEXT("elderwood-copper-vein") && Biome == EKalmalaBiome::Elderwood)
                || (Original == TEXT("lakes-tin-shore-gravel") && Biome == EKalmalaBiome::ShimmeringLakes)
                || (Original == TEXT("mire-bog-iron") && Biome == EKalmalaBiome::MossyMire)
                || (Original == TEXT("mountains-slate-vein") && Biome == EKalmalaBiome::ThunderMountains)
                || (Original == TEXT("tundra-frost-salt-deposit") && Biome == EKalmalaBiome::FreezingTundra));
        }
    }
    TestTrue(TEXT("Seeded descriptor samples exercise real replacements"), Replacements > 0);
    return true;
}
#endif
