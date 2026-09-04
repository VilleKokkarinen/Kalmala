#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaBiomeClassifier.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldPlayerStartResolver.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaWorldPlayerStartResolverTest,
    "Kalmala.World.GeneratedPlayerStart.Determinism",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaWorldPlayerStartResolverTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 418;
    Config.GeneratorRevision = 1;

    const FTransform FirstStart = FKalmalaWorldPlayerStartResolver::ResolveStartTransform(Config);
    const FTransform RepeatedStart = FKalmalaWorldPlayerStartResolver::ResolveStartTransform(Config);
    TestTrue(TEXT("The same identity resolves to the same player start"), FirstStart.Equals(RepeatedStart));

    const FVector FirstLocation = FirstStart.GetLocation();
    const FKalmalaWorldFieldSample FirstSample = FKalmalaWorldFieldSampler::Sample(Config, FVector2D(FirstLocation.X, FirstLocation.Y));
    TestTrue(TEXT("The start is selected from a Meadow candidate"), FKalmalaBiomeClassifier::Classify(FirstSample) == EKalmalaBiome::Meadows);
    TestEqual(
        TEXT("The start uses the shared terrain height plus pawn clearance"),
        static_cast<double>(FirstLocation.Z),
        static_cast<double>(FKalmalaTerrainHeightSampler::SampleHeight(Config, FVector2D(FirstLocation.X, FirstLocation.Y)) + 120.0f),
        0.01);
    TestTrue(
        TEXT("The shared terrain surface normal is normalized"),
        FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, FVector2D(FirstLocation.X, FirstLocation.Y)).IsNormalized());
    const FVector2D PatchOrigin(FirstLocation.X, FirstLocation.Y);
    const FVector2D EastPatchCenter = FKalmalaTerrainPatchLayout::GetPatchCenter(PatchOrigin, 1, 0);
    TestEqual(
        TEXT("Adjacent terrain patches are separated by one continuous patch width"),
        static_cast<double>(EastPatchCenter.X - PatchOrigin.X),
        static_cast<double>(FKalmalaTerrainPatchLayout::PatchSize),
        0.01);
    TestEqual(
        TEXT("Generated start belongs to the origin terrain patch"),
        FKalmalaTerrainPatchLayout::GetPatchCoordinate(PatchOrigin, PatchOrigin),
        FIntPoint::ZeroValue);
    TestEqual(
        TEXT("A position beyond the east patch boundary activates patch one"),
        FKalmalaTerrainPatchLayout::GetPatchCoordinate(PatchOrigin, PatchOrigin + FVector2D(FKalmalaTerrainPatchLayout::PatchSize * 0.51f, 0.0f)),
        FIntPoint(1, 0));

    Config.WorldSeed = 419;
    const FTransform DifferentSeedStart = FKalmalaWorldPlayerStartResolver::ResolveStartTransform(Config);
    TestFalse(TEXT("A different seed resolves to a different player start"), FirstStart.Equals(DifferentSeedStart));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaSurfaceWaterCoverageTest,
    "Kalmala.World.SurfaceWater.Coverage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaSurfaceWaterCoverageTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 418;
    Config.GeneratorRevision = 1;

    int32 SubmergedSampleCount = 0;
    for (int32 Y = -24000; Y <= 24000; Y += 1500)
    {
        for (int32 X = -24000; X <= 24000; X += 1500)
        {
            if (FKalmalaTerrainHeightSampler::SampleHeight(Config, FVector2D(X, Y)) <= FKalmalaTerrainHeightSampler::SeaLevelWorldHeight)
            {
                ++SubmergedSampleCount;
            }
        }
    }

    TestTrue(TEXT("The shared elevation field contains deterministic submerged terrain samples"), SubmergedSampleCount > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaShimmeringLakeCoverageTest,
    "Kalmala.World.ShimmeringLakes.Coverage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaShimmeringLakeCoverageTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 418;
    Config.GeneratorRevision = 1;

    int32 LakeWaterSampleCount = 0;
    for (int32 Y = -48000; Y <= 48000; Y += 500)
    {
        for (int32 X = -48000; X <= 48000; X += 500)
        {
            LakeWaterSampleCount += FKalmalaShimmeringLakeSampler::IsWater(Config, FVector2D(X, Y)) ? 1 : 0;
        }
    }

    TestTrue(TEXT("The shared fields contain deterministic Shimmering Lakes water samples"), LakeWaterSampleCount > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaWorldPopulationLayoutTest,
    "Kalmala.World.PopulationLayout.Determinism",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaWorldPopulationLayoutTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 418;
    Config.GeneratorRevision = 1;

    const FIntPoint SpatialKey = FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(6500.0f, -5500.0f));
    TestEqual(TEXT("World positions map to a deterministic invisible spatial key"), SpatialKey, FIntPoint(1, -1));

    for (const EKalmalaWorldPopulationKind Kind : { EKalmalaWorldPopulationKind::Wildlife, EKalmalaWorldPopulationKind::HarvestNode, EKalmalaWorldPopulationKind::Hazard })
    {
        const uint64 FirstSeed = FKalmalaWorldPopulationLayout::DeriveSpatialSeed(Config, SpatialKey, Kind);
        TestEqual(TEXT("The same identity, spatial key, and content kind produce the same seed"), FirstSeed, FKalmalaWorldPopulationLayout::DeriveSpatialSeed(Config, SpatialKey, Kind));
        TestTrue(TEXT("Every population budget is non-negative"), FKalmalaWorldPopulationLayout::GetSpawnBudget(Config, SpatialKey, Kind) >= 0);
        const TArray<FKalmalaWorldPopulationSpawn> FirstSpawns = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(Config, SpatialKey, Kind);
        const TArray<FKalmalaWorldPopulationSpawn> RepeatedSpawns = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(Config, SpatialKey, Kind);
        TestEqual(TEXT("Each spatial key produces its bounded spawn budget"), FirstSpawns.Num(), FKalmalaWorldPopulationLayout::GetSpawnBudget(Config, SpatialKey, Kind));
        TestEqual(TEXT("Repeated spatial layouts produce the same number of spawn descriptors"), FirstSpawns.Num(), RepeatedSpawns.Num());
        for (int32 SpawnIndex = 0; SpawnIndex < FirstSpawns.Num(); ++SpawnIndex)
        {
            TestEqual(TEXT("Repeated spatial layouts preserve spawn seeds"), FirstSpawns[SpawnIndex].SpawnSeed, RepeatedSpawns[SpawnIndex].SpawnSeed);
            TestEqual(TEXT("Repeated spatial layouts preserve sparse-delta identifiers"), FKalmalaWorldPopulationLayout::GetPersistentSpawnId(FirstSpawns[SpawnIndex]), FKalmalaWorldPopulationLayout::GetPersistentSpawnId(RepeatedSpawns[SpawnIndex]));
            TestTrue(TEXT("Spawn descriptors remain within their invisible spatial key"), FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(FirstSpawns[SpawnIndex].Location)) == SpatialKey);
        }
    }

    FKalmalaWorldGenerationConfig DifferentConfig = Config;
    DifferentConfig.WorldSeed = 419;
    TestNotEqual(
        TEXT("Different content kinds use independent spatial seeds"),
        FKalmalaWorldPopulationLayout::DeriveSpatialSeed(Config, SpatialKey, EKalmalaWorldPopulationKind::Wildlife),
        FKalmalaWorldPopulationLayout::DeriveSpatialSeed(Config, SpatialKey, EKalmalaWorldPopulationKind::HarvestNode));
    TestNotEqual(
        TEXT("A different world seed changes the spatial seed"),
        FKalmalaWorldPopulationLayout::DeriveSpatialSeed(Config, SpatialKey, EKalmalaWorldPopulationKind::Wildlife),
        FKalmalaWorldPopulationLayout::DeriveSpatialSeed(DifferentConfig, SpatialKey, EKalmalaWorldPopulationKind::Wildlife));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaWorldPopulationSaveGameTest,
    "Kalmala.World.PopulationSaveGame.SparseDeltas",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaWorldPopulationSaveGameTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 418;
    Config.GeneratorRevision = 1;
    UKalmalaWorldPopulationSaveGame* SaveGame = NewObject<UKalmalaWorldPopulationSaveGame>();
    SaveGame->InitializeForWorld(Config);
    const FString SpawnId = TEXT("1/1/-1/1234");
    const FString WildlifeSpawnId = TEXT("0/1/-1/5678");
    const FString HazardSpawnId = TEXT("2/1/-1/9012");
    TestTrue(TEXT("A sparse delta save belongs to its initialized world identity"), SaveGame->MatchesWorld(Config));
    TestFalse(TEXT("An untouched generated node has no saved depletion delta"), SaveGame->IsHarvested(SpawnId));
    SaveGame->MarkHarvested(SpawnId);
    TestTrue(TEXT("A harvested node is recorded as one sparse delta"), SaveGame->IsHarvested(SpawnId));
    TestFalse(TEXT("An untouched generated wildlife spawn has no defeated delta"), SaveGame->IsDefeated(WildlifeSpawnId));
    SaveGame->MarkDefeated(WildlifeSpawnId);
    SaveGame->MarkDefeated(HazardSpawnId);
    TestTrue(TEXT("A defeated generated wildlife spawn is recorded as a sparse delta"), SaveGame->IsDefeated(WildlifeSpawnId));
    TestTrue(TEXT("A defeated generated hazard spawn is recorded as a sparse delta"), SaveGame->IsDefeated(HazardSpawnId));

    TArray<uint8> SerializedSave;
    TestTrue(TEXT("The sparse delta container serializes without writing a slot"), UGameplayStatics::SaveGameToMemory(SaveGame, SerializedSave));
    UKalmalaWorldPopulationSaveGame* ReloadedSave = Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(SerializedSave));
    TestNotNull(TEXT("The serialized sparse delta container reloads as its expected type"), ReloadedSave);
    if (ReloadedSave != nullptr)
    {
        TestTrue(TEXT("The reloaded container retains its immutable world identity"), ReloadedSave->MatchesWorld(Config));
        TestTrue(TEXT("The reloaded container retains only the harvested sparse delta"), ReloadedSave->IsHarvested(SpawnId));
        TestTrue(TEXT("The reloaded container retains the defeated wildlife sparse delta"), ReloadedSave->IsDefeated(WildlifeSpawnId));
        TestTrue(TEXT("The reloaded container retains the defeated hazard sparse delta"), ReloadedSave->IsDefeated(HazardSpawnId));
    }

    const FString SlotName = TEXT("KalmalaPopulationSaveGameAutomation");
    TestTrue(TEXT("The sparse delta container saves to a local test slot"), UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, 0));
    UKalmalaWorldPopulationSaveGame* SlotReloadedSave = Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
    TestNotNull(TEXT("The local test slot reloads as its expected type"), SlotReloadedSave);
    if (SlotReloadedSave != nullptr)
    {
        TestTrue(TEXT("The local test slot retains its immutable world identity"), SlotReloadedSave->MatchesWorld(Config));
        TestTrue(TEXT("The local test slot retains the harvested sparse delta"), SlotReloadedSave->IsHarvested(SpawnId));
        TestTrue(TEXT("The local test slot retains the defeated wildlife sparse delta"), SlotReloadedSave->IsDefeated(WildlifeSpawnId));
        TestTrue(TEXT("The local test slot retains the defeated hazard sparse delta"), SlotReloadedSave->IsDefeated(HazardSpawnId));
    }

    Config.WorldSeed = 419;
    TestFalse(TEXT("A different seed cannot reuse this population delta save"), SaveGame->MatchesWorld(Config));
    return true;
}

#endif
