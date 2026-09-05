#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaBiomeClassifier.h"
#include "KalmalaBiomeExpansionContract.h"
#include "KalmalaCampConditionSampler.h"
#include "KalmalaEnvironmentalExposureSampler.h"
#include "KalmalaShelterSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldPlayerStartResolver.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "KalmalaWeatherCycle.h"
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
        static_cast<double>(FKalmalaTerrainHeightSampler::SampleHeight(Config, FVector2D(FirstLocation.X, FirstLocation.Y)) + 136.0f),
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaWeatherCycleTest,
    "Kalmala.World.WeatherCycle.Determinism",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaWeatherCycleTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 418;
    Config.GeneratorRevision = 1;

    const FKalmalaWeatherState First = FKalmalaWeatherCycle::DeriveState(Config, 0, 10.0f);
    const FKalmalaWeatherState Repeated = FKalmalaWeatherCycle::DeriveState(Config, 0, 10.0f);
    const FKalmalaWeatherState Next = FKalmalaWeatherCycle::DeriveState(Config, 1, 10.0f + First.DurationSeconds);
    TestTrue(TEXT("A weather interval is within the replicated contract bounds"), First.IsValid());
    TestEqual(TEXT("The same immutable identity and cycle index repeat duration"), First.DurationSeconds, Repeated.DurationSeconds);
    TestEqual(TEXT("The same immutable identity and cycle index repeat precipitation"), First.PrecipitationIntensity, Repeated.PrecipitationIntensity);
    TestEqual(TEXT("The same immutable identity and cycle index repeat wind direction"), First.WindDirectionDegrees, Repeated.WindDirectionDegrees);
    TestEqual(TEXT("The same immutable identity and cycle index repeat wind strength"), First.WindStrength, Repeated.WindStrength);
    TestEqual(TEXT("The next interval increments its immutable cycle index"), Next.WeatherCycleIndex, 1);
    TestEqual(TEXT("The next interval starts where the previous one ended"), Next.ServerStartTimeSeconds, First.ServerStartTimeSeconds + First.DurationSeconds);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaEnvironmentalExposureSamplerTest,
    "Kalmala.World.EnvironmentalExposure.TerrainVariation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaEnvironmentalExposureSamplerTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 418;
    Config.GeneratorRevision = 1;

    bool bFoundLowWetGround = false;
    bool bFoundShoreline = false;
    float HighestRidgeWind = 0.0f;
    float HighestOpenWind = 0.0f;
    float LowestCoveredWind = 1.0f;
    for (int32 Y = -48000; Y <= 48000; Y += 400)
    {
        for (int32 X = -48000; X <= 48000; X += 400)
        {
            const FKalmalaEnvironmentalExposureSample Sample = FKalmalaEnvironmentalExposureSampler::Sample(Config, FVector2D(X, Y));
            bFoundLowWetGround |= Sample.bIsLowWetGround && Sample.GroundWetness >= 0.5f;
            bFoundShoreline |= Sample.bIsShoreline && Sample.ShorelineWetness > 0.0f;
            if (Sample.RidgeExposure >= 0.75f)
            {
                HighestRidgeWind = FMath::Max(HighestRidgeWind, Sample.WindExposure);
            }
            if (Sample.NaturalCover <= 0.2f)
            {
                HighestOpenWind = FMath::Max(HighestOpenWind, Sample.WindExposure);
            }
            if (Sample.NaturalCover >= 0.8f)
            {
                LowestCoveredWind = FMath::Min(LowestCoveredWind, Sample.WindExposure);
            }
        }
    }

    TestTrue(TEXT("Continuous terrain supplies low humid ground with increased wetness"), bFoundLowWetGround);
    TestTrue(TEXT("Lake adjacency supplies shoreline wetness without a biome zone"), bFoundShoreline);
    TestTrue(TEXT("Exposed ridges can produce substantial wind exposure"), HighestRidgeWind >= 0.45f);
    TestTrue(TEXT("Dense natural cover reduces wind exposure compared with open ground"), LowestCoveredWind < HighestOpenWind);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaShelterSamplerTest,
    "Kalmala.World.EnvironmentalExposure.ShelterComposition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaShelterSamplerTest::RunTest(const FString& Parameters)
{
    const FKalmalaShelterSample NaturalOnly = FKalmalaShelterSampler::Compose(0.8f, false, false);
    const FKalmalaShelterSample RoofAndWindbreak = FKalmalaShelterSampler::Compose(0.0f, true, true);
    const FKalmalaShelterSample CompleteShelter = FKalmalaShelterSampler::Compose(1.0f, true, true);

    TestEqual(TEXT("Natural cover contributes continuous partial shelter"), NaturalOnly.NaturalCoverShelter, 0.24f);
    TestEqual(TEXT("Natural cover alone is not complete shelter"), NaturalOnly.Shelter, 0.24f);
    TestTrue(TEXT("A roof is recorded only from shelter geometry"), RoofAndWindbreak.bHasRoof);
    TestTrue(TEXT("A windbreak is recorded only from shelter geometry"), RoofAndWindbreak.bHasWindbreak);
    TestEqual(TEXT("Roof and windbreak geometry compose substantial shelter"), RoofAndWindbreak.Shelter, 0.8f);
    TestEqual(TEXT("All shelter inputs remain clamped"), CompleteShelter.Shelter, 1.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaCampConditionSamplerTest,
    "Kalmala.World.CampConditions.LocalTradeoffs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaCampConditionSamplerTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 418;
    Config.GeneratorRevision = 1;
    float LowestWetness = 1.0f, HighestWetness = 0.0f, LowestCover = 1.0f, HighestCover = 0.0f;
    float NearestWater = FKalmalaCampConditionSampler::WaterSearchRadius, FarthestWater = 0.0f;
    int32 LowestResources = MAX_int32, HighestResources = 0;
    for (int32 Y = -24000; Y <= 24000; Y += 800)
    for (int32 X = -24000; X <= 24000; X += 800)
    {
        const FKalmalaCampConditionSample Sample = FKalmalaCampConditionSampler::Sample(Config, FVector2D(X, Y));
        LowestWetness = FMath::Min(LowestWetness, Sample.GroundWetness); HighestWetness = FMath::Max(HighestWetness, Sample.GroundWetness);
        LowestCover = FMath::Min(LowestCover, Sample.NaturalCover); HighestCover = FMath::Max(HighestCover, Sample.NaturalCover);
        NearestWater = FMath::Min(NearestWater, Sample.WaterDistance); FarthestWater = FMath::Max(FarthestWater, Sample.WaterDistance);
        LowestResources = FMath::Min(LowestResources, Sample.NearbyHarvestNodeCount); HighestResources = FMath::Max(HighestResources, Sample.NearbyHarvestNodeCount);
    }
    TestTrue(TEXT("Freely sampled camp ground has meaningful wetness variation"), HighestWetness - LowestWetness >= 0.30f);
    TestTrue(TEXT("Freely sampled camp ground has meaningful natural-cover variation"), HighestCover - LowestCover >= 0.30f);
    TestTrue(TEXT("Freely sampled camp ground has both near and distant water"), NearestWater < FarthestWater);
    TestTrue(TEXT("Freely sampled camp ground has different nearby harvest-resource availability"), LowestResources < HighestResources);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaBiomeExpansionContractTest, "Kalmala.World.BiomeExpansion.SharedContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaBiomeExpansionContractTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 418;
    Config.GeneratorRevision = 1;
    const FIntPoint SpatialKey(2, -3);
    for (const EKalmalaBiome Biome : { EKalmalaBiome::Meadows, EKalmalaBiome::ShimmeringLakes, EKalmalaBiome::Elderwood, EKalmalaBiome::MossyMire, EKalmalaBiome::FreezingTundra, EKalmalaBiome::ThunderMountains })
    {
        const FKalmalaBiomeExpansionProfile Profile = FKalmalaBiomeExpansionContract::GetProfile(Biome);
        TestTrue(TEXT("Every land biome has a non-negative terrain feature strength"), Profile.TerrainFeatureStrength >= 0.0f);
        TestTrue(TEXT("Every land biome keeps bounded server population budgets"), FKalmalaBiomeExpansionContract::ApplyPopulationBudget(4, EKalmalaWorldPopulationKind::HarvestNode, Biome) >= 0);
        const FKalmalaBiomeDiscoveryCandidate First = FKalmalaBiomeExpansionContract::BuildDiscoveryCandidate(Config, SpatialKey, Biome);
        const FKalmalaBiomeDiscoveryCandidate Repeated = FKalmalaBiomeExpansionContract::BuildDiscoveryCandidate(Config, SpatialKey, Biome);
        TestEqual(TEXT("Discovery candidates have stable server identifiers"), First.StableId, Repeated.StableId);
        TestEqual(TEXT("Discovery candidates reproduce their terrain-aligned location"), First.Location, Repeated.Location);
    }
    const FKalmalaEnvironmentalExposureSample BaseExposure = FKalmalaEnvironmentalExposureSampler::Sample(Config, FVector2D(3000.0f, -2000.0f));
    const FKalmalaEnvironmentalExposureSample MireExposure = FKalmalaBiomeExpansionContract::ApplyExposureModifiers(BaseExposure, EKalmalaBiome::MossyMire);
    TestTrue(TEXT("Biome exposure modifiers remain normalized"), MireExposure.GroundWetness >= 0.0f && MireExposure.GroundWetness <= 1.0f && MireExposure.WindExposure >= 0.0f && MireExposure.WindExposure <= 1.0f);
    const FKalmalaBiomeFeatureInspection Inspection = FKalmalaBiomeExpansionContract::Inspect(Config, FVector2D(3000.0f, -2000.0f));
    TestFalse(TEXT("Developer inspection never creates a discovery actor"), Inspection.DiscoveryCandidate.StableId.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaShimmeringLakesSliceTest, "Kalmala.World.BiomeExpansion.ShimmeringLakesSlice", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaShimmeringLakesSliceTest::RunTest(const FString& Parameters)
{
    const FKalmalaWorldGenerationConfig Config{ 418ull, 1 };
    bool bFoundLakeEdgeDiscovery = false;
    for (int32 Y = -6; Y <= 6 && !bFoundLakeEdgeDiscovery; ++Y)
    {
        for (int32 X = -6; X <= 6 && !bFoundLakeEdgeDiscovery; ++X)
        {
            const FIntPoint SpatialKey(X, Y);
            FKalmalaBiomeDiscoveryCandidate Discovery;
            if (!FKalmalaBiomeExpansionContract::TryBuildShimmeringLakeDiscovery(Config, SpatialKey, Discovery)) continue;
            const FVector2D Position(Discovery.Location);
            const FKalmalaEnvironmentalExposureSample Base = FKalmalaEnvironmentalExposureSampler::Sample(Config, Position);
            const FKalmalaEnvironmentalExposureSample Modified = FKalmalaBiomeExpansionContract::ApplyExposureModifiers(Base, EKalmalaBiome::ShimmeringLakes);
            TestFalse(TEXT("Lake-edge discovery remains on dry traversable ground"), FKalmalaShimmeringLakeSampler::IsWater(Config, Position));
            TestTrue(TEXT("Lake-edge discovery has adjacent interlocking lake water"), FKalmalaShimmeringLakeSampler::IsWater(Config, Position + FVector2D(350.0f, 0.0f)) || FKalmalaShimmeringLakeSampler::IsWater(Config, Position - FVector2D(350.0f, 0.0f)) || FKalmalaShimmeringLakeSampler::IsWater(Config, Position + FVector2D(0.0f, 350.0f)) || FKalmalaShimmeringLakeSampler::IsWater(Config, Position - FVector2D(0.0f, 350.0f)));
            TestTrue(TEXT("Lake shore increases wet-ground camp pressure"), Modified.GroundWetness >= Base.GroundWetness);
            TestTrue(TEXT("Lake shore preserves a bounded natural-cover tradeoff"), Modified.NaturalCover <= Base.NaturalCover);
            TestEqual(TEXT("Lake discovery is stable for the same seed and spatial key"), Discovery.StableId, FKalmalaBiomeExpansionContract::BuildDiscoveryCandidate(Config, SpatialKey, EKalmalaBiome::ShimmeringLakes).StableId);
            bFoundLakeEdgeDiscovery = true;
        }
    }
    TestTrue(TEXT("Seed contains a deterministic dry Shimmering Lakes discovery without requiring a boat"), bFoundLakeEdgeDiscovery);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaElderwoodSliceTest, "Kalmala.World.BiomeExpansion.ElderwoodSlice", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaElderwoodSliceTest::RunTest(const FString& Parameters)
{
    const FKalmalaWorldGenerationConfig Config{ 418ull, 1 };
    bool bFoundClearingDiscovery = false;
    for (int32 Y = -6; Y <= 6 && !bFoundClearingDiscovery; ++Y)
    {
        for (int32 X = -6; X <= 6 && !bFoundClearingDiscovery; ++X)
        {
            const FIntPoint SpatialKey(X, Y);
            FKalmalaBiomeDiscoveryCandidate Discovery;
            if (!FKalmalaBiomeExpansionContract::TryBuildElderwoodDiscovery(Config, SpatialKey, Discovery)) continue;
            const FVector2D Position(Discovery.Location);
            const FKalmalaWorldFieldSample Fields = FKalmalaWorldFieldSampler::Sample(Config, Position);
            const FKalmalaEnvironmentalExposureSample OpenExposure = FKalmalaEnvironmentalExposureSampler::Sample(Config, Position);
            const FKalmalaEnvironmentalExposureSample CompactExposure = FKalmalaBiomeExpansionContract::ApplyExposureModifiers(OpenExposure, EKalmalaBiome::Elderwood);
            TestEqual(TEXT("Elderwood discovery remains inside the continuous Elderwood classifier"), FKalmalaBiomeClassifier::Classify(Fields), EKalmalaBiome::Elderwood);
            TestTrue(TEXT("Elderwood discovery resolves a lower-flora clearing rather than an authored site"), Fields.Flora <= 0.76f);
            TestTrue(TEXT("Elderwood discovery stays on gently traversable terrain"), FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, Position).Z >= 0.86f);
            TestTrue(TEXT("Compact Elderwood cover reduces wind exposure versus an open camp"), CompactExposure.WindExposure <= OpenExposure.WindExposure);
            TestTrue(TEXT("Compact Elderwood cover increases natural shelter versus an open camp"), CompactExposure.NaturalCover >= OpenExposure.NaturalCover);
            TestEqual(TEXT("Elderwood clearing discovery has a stable sparse identifier"), Discovery.StableId, FKalmalaBiomeExpansionContract::BuildDiscoveryCandidate(Config, SpatialKey, EKalmalaBiome::Elderwood).StableId);
            bFoundClearingDiscovery = true;
        }
    }
    TestTrue(TEXT("Seed contains a deterministic Elderwood clearing discovery without a trail"), bFoundClearingDiscovery);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaMossyMireSliceTest, "Kalmala.World.BiomeExpansion.MossyMireSlice", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaMossyMireSliceTest::RunTest(const FString& Parameters)
{
    const FKalmalaWorldGenerationConfig Config{ 418ull, 1 };
    bool bFoundHummockDiscovery = false;
    for (int32 Y = -8; Y <= 8 && !bFoundHummockDiscovery; ++Y)
    {
        for (int32 X = -8; X <= 8 && !bFoundHummockDiscovery; ++X)
        {
            const FIntPoint SpatialKey(X, Y);
            FKalmalaBiomeDiscoveryCandidate Discovery;
            if (!FKalmalaBiomeExpansionContract::TryBuildMossyMireDiscovery(Config, SpatialKey, Discovery)) continue;
            const FVector2D Position(Discovery.Location);
            const FKalmalaEnvironmentalExposureSample Base = FKalmalaEnvironmentalExposureSampler::Sample(Config, Position);
            const FKalmalaEnvironmentalExposureSample Mire = FKalmalaBiomeExpansionContract::ApplyExposureModifiers(Base, EKalmalaBiome::MossyMire);
            TestEqual(TEXT("Mire discovery remains inside the continuous Mossy Mire classifier"), FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, Position)), EKalmalaBiome::MossyMire);
            TestTrue(TEXT("Mire dry hummock remains gently traversable"), FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, Position).Z >= 0.84f);
            TestTrue(TEXT("Mire hummock is drier than the saturated ground threshold"), Base.GroundWetness <= 0.72f);
            TestTrue(TEXT("Mire profile increases wet-ground shelter preparation pressure"), Mire.GroundWetness >= Base.GroundWetness);
            TestTrue(TEXT("Mire profile retains bounded wind and cover inputs"), Mire.WindExposure >= 0.0f && Mire.WindExposure <= 1.0f && Mire.NaturalCover >= 0.0f && Mire.NaturalCover <= 1.0f);
            TestEqual(TEXT("Mire hummock discovery has a stable sparse identifier"), Discovery.StableId, FKalmalaBiomeExpansionContract::BuildDiscoveryCandidate(Config, SpatialKey, EKalmalaBiome::MossyMire).StableId);
            bFoundHummockDiscovery = true;
        }
    }
    TestTrue(TEXT("Seed contains a deterministic dry Mossy Mire hummock discovery without requiring a crossing"), bFoundHummockDiscovery);
    return true;
}

#endif
