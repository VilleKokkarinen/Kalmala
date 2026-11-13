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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaEnemyGemPayloadTest, "Kalmala.World.EnemyGemPayload",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaEnemyGemPayloadTest::RunTest(const FString& Parameters)
{
    int32 Diamonds = 0;
    for (uint64 Seed = 0; Seed < 10000; ++Seed)
    {
        Diamonds += FKalmalaGemstoneContract::SelectEnemyGem(false, EKalmalaBiome::MossyMire, Seed) == TEXT("DuskDiamond");
        TestTrue(TEXT("Bear cannot use the Mireling gem source"),
            FKalmalaGemstoneContract::SelectEnemyGem(true, EKalmalaBiome::MossyMire, Seed).IsNone());
        TestTrue(TEXT("Mireling outside Mire cannot yield Dusk Diamond"),
            FKalmalaGemstoneContract::SelectEnemyGem(false, EKalmalaBiome::ThunderMountains, Seed).IsNone());
    }
    TestTrue(TEXT("Dusk Diamond selection is near one percent"), Diamonds > 60 && Diamonds < 140);

    const FKalmalaWorldGenerationConfig Config;
    auto* Save = NewObject<UKalmalaWorldPopulationSaveGame>();
    Save->InitializeForWorld(Config);
    const FString MoonSpawn = TEXT("2/10/11/12");
    const FString MoonClaim = UKalmalaWorldPopulationSaveGame::MakeMoonBearPendingRewardClaimId(MoonSpawn);
    const FString DuskClaim = TEXT("mireling-gem|2/12/13/14");
    TestTrue(TEXT("Credited bear reward queues"), Save->TryQueuePendingMoonBearReward(MoonSpawn, TEXT("owner-a")));
    TestTrue(TEXT("Moonstone binds to its bear payload"), Save->SetPendingGemReward(MoonClaim, TEXT("Moonstone")));
    TestTrue(TEXT("Credited Mireling gem reward queues"), Save->TryQueuePendingWildlifeReward(DuskClaim, TEXT("owner-b")));
    TestTrue(TEXT("Dusk Diamond binds to its Mireling payload"), Save->SetPendingGemReward(DuskClaim, TEXT("DuskDiamond")));
    TestFalse(TEXT("Another owner cannot take a pending bear reward"), Save->TryQueuePendingMoonBearReward(MoonSpawn, TEXT("owner-b")));
    TestFalse(TEXT("Another owner cannot take a pending Mireling reward"), Save->TryQueuePendingWildlifeReward(DuskClaim, TEXT("owner-a")));
    TestFalse(TEXT("Moonstone cannot be attached to a Mireling"), Save->SetPendingGemReward(DuskClaim, TEXT("Moonstone")));
    TestFalse(TEXT("Dusk Diamond cannot be attached to a bear"), Save->SetPendingGemReward(MoonClaim, TEXT("DuskDiamond")));
    TestFalse(TEXT("Other gems cannot enter an enemy payload"), Save->SetPendingGemReward(MoonClaim, TEXT("StarOpal")));
    TestFalse(TEXT("An absent claim cannot receive a gem"), Save->SetPendingGemReward(TEXT("mireling-gem|absent"), TEXT("DuskDiamond")));

    TArray<uint8> Bytes;
    TestTrue(TEXT("Gem-bearing pending rewards serialize"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    auto* Restored = Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Gem-bearing pending rewards restore"), Restored)) return false;
    TestTrue(TEXT("Restored payload retains world identity"), Restored->MatchesWorld(Config));
    const auto& Pending = Restored->GetPendingWildlifeRewards();
    TestEqual(TEXT("Rejected changes did not create or remove rewards"), Pending.Num(), 2);
    TestTrue(TEXT("Moonstone and its credited owner survive rejection and reload"), Pending.ContainsByPredicate([&MoonClaim](const auto& Row)
        { return Row.PersistentSpawnId == MoonClaim && Row.PlayerIdentity == TEXT("owner-a") && Row.GemItemId == TEXT("Moonstone"); }));
    TestTrue(TEXT("Dusk Diamond and its credited owner survive rejection and reload"), Pending.ContainsByPredicate([&DuskClaim](const auto& Row)
        { return Row.PersistentSpawnId == DuskClaim && Row.PlayerIdentity == TEXT("owner-b") && Row.GemItemId == TEXT("DuskDiamond"); }));
    TestTrue(TEXT("Consumed bear claim can be removed"), Restored->RemovePendingWildlifeReward(MoonClaim));
    TestFalse(TEXT("Consumed bear claim cannot be removed twice"), Restored->RemovePendingWildlifeReward(MoonClaim));
    TestTrue(TEXT("Remaining pending reward serializes"), UGameplayStatics::SaveGameToMemory(Restored, Bytes));
    auto* AfterClaim = Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    TestTrue(TEXT("Claim removal survives reload without losing the other owner's gem"), AfterClaim && AfterClaim->MatchesWorld(Config)
        && AfterClaim->GetPendingWildlifeRewards().Num() == 1 && AfterClaim->GetPendingWildlifeRewards()[0].PersistentSpawnId == DuskClaim
        && AfterClaim->GetPendingWildlifeRewards()[0].GemItemId == TEXT("DuskDiamond"));

    auto* ScrollSave = NewObject<UKalmalaWorldPopulationSaveGame>();
    ScrollSave->InitializeForWorld(Config);
    const FString BossSpawn = TEXT("wildlife:mireling/2/12/13/14");
    const FString ScrollClaim = UKalmalaWorldPopulationSaveGame::MakeMirelingBossScrollPendingRewardClaimId(BossSpawn);
    FString DecodedBossSpawn;
    TestTrue(TEXT("Mireling scroll queue identity is derived from its persistent spawn ID"),
        !ScrollClaim.IsEmpty()
            && UKalmalaWorldPopulationSaveGame::TryGetMirelingBossSpawnIdFromPendingRewardClaimId(
                ScrollClaim, DecodedBossSpawn)
            && DecodedBossSpawn == BossSpawn);
    TestTrue(TEXT("Mireling scroll queue accepts the credited owner"),
        ScrollSave->TryQueuePendingMirelingBossScrollReward(BossSpawn, TEXT("owner-b")));
    TestFalse(TEXT("Mireling scroll queue cannot be reassigned to another owner"),
        ScrollSave->TryQueuePendingMirelingBossScrollReward(BossSpawn, TEXT("owner-a")));
    TestFalse(TEXT("Mireling scroll queue identities reject embedded claim delimiters"),
        !UKalmalaWorldPopulationSaveGame::MakeMirelingBossScrollPendingRewardClaimId(
            TEXT("wildlife:mireling|forged")).IsEmpty());
    TArray<uint8> ScrollBytes;
    TestTrue(TEXT("Pending Mireling scroll entitlement serializes in the population save"),
        UGameplayStatics::SaveGameToMemory(ScrollSave, ScrollBytes));
    auto* RestoredScrollSave = Cast<UKalmalaWorldPopulationSaveGame>(
        UGameplayStatics::LoadGameFromMemory(ScrollBytes));
    if (!TestNotNull(TEXT("Pending Mireling scroll entitlement reloads"), RestoredScrollSave)) return false;
    const FString ExpectedScrollClaim = ScrollClaim;
    TestTrue(TEXT("Mireling scroll entitlement retains its owner and no item payload"),
        RestoredScrollSave->MatchesWorld(Config)
            && RestoredScrollSave->GetPendingWildlifeRewards().ContainsByPredicate(
                [&ExpectedScrollClaim](const FKalmalaPendingWildlifeReward& Reward)
                {
                    return Reward.PersistentSpawnId == ExpectedScrollClaim
                        && Reward.PlayerIdentity == TEXT("owner-b") && Reward.GemItemId.IsNone();
                }));
    TestTrue(TEXT("Completed Mireling scroll entitlement can be removed from the pending queue"),
        RestoredScrollSave->RemovePendingWildlifeReward(ScrollClaim));
    auto* InvalidScrollSave = NewObject<UKalmalaWorldPopulationSaveGame>();
    InvalidScrollSave->InitializeForWorld(Config);
    TestTrue(TEXT("Generic pending queue can stage a deliberately malformed scroll record"),
        InvalidScrollSave->TryQueuePendingWildlifeReward(TEXT("mireling-scroll|"), TEXT("owner-b")));
    TestFalse(TEXT("Malformed Mireling scroll claim identities fail world-save validation"),
        InvalidScrollSave->MatchesWorld(Config));
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
