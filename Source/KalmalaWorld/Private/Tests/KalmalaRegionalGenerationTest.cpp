#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaRegionalGeneration.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaWaterSurfaceMesh.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaWorldPlayerStartResolver.h"
#include "KalmalaGenerationPreview.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaTerrainPreviewControlTest,
    "Kalmala.World.Regional.PreviewHeightControls",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaTerrainPreviewControlTest::RunTest(const FString& Parameters)
{
    FKalmalaGenerationPreviewSettings Settings;
    TestTrue(TEXT("Documented default terrain controls are valid"), Settings.HasValidReliefControls());
    TestEqual(TEXT("Broad relief preview scale defaults to tuning"), Settings.ReliefBroadScale,
        FKalmalaRegionalTuning::ReliefBroadScale);
    TestEqual(TEXT("Meadow hill amplitude defaults to tuning"), Settings.MeadowHillAmplitude,
        FKalmalaRegionalTuning::MeadowHillAmplitude);
    TestEqual(TEXT("Meadow shoulder threshold defaults to tuning"), Settings.MeadowShoulderHeight,
        FKalmalaRegionalTuning::MeadowShoulderHeight);
    TestEqual(TEXT("Meadow shoulder scale defaults to tuning"), Settings.MeadowShoulderCompressionScale,
        FKalmalaRegionalTuning::MeadowShoulderCompressionScale);
    TestEqual(TEXT("Elderwood broad amplitude defaults to tuning"), Settings.ElderwoodBroadAmplitude,
        FKalmalaRegionalTuning::ElderwoodBroadAmplitude);
    TestEqual(TEXT("Elderwood middle amplitude defaults to tuning"), Settings.ElderwoodMiddleAmplitude,
        FKalmalaRegionalTuning::ElderwoodMiddleAmplitude);
    TestEqual(TEXT("Elderwood local amplitude defaults to tuning"), Settings.ElderwoodLocalAmplitude,
        FKalmalaRegionalTuning::ElderwoodLocalAmplitude);
    TestEqual(TEXT("Shimmering Lakes Meadow share defaults to tuning"), Settings.ShimmeringLakesMeadowShare,
        FKalmalaRegionalTuning::ShimmeringLakesMeadowShare);
    TestEqual(TEXT("Shimmering Lakes lowland floor defaults to tuning"), Settings.ShimmeringLakesLowlandFloor,
        FKalmalaRegionalTuning::ShimmeringLakesLowlandFloor);
    TestEqual(TEXT("Shimmering Lakes lowland ceiling defaults to tuning"), Settings.ShimmeringLakesLowlandCeiling,
        FKalmalaRegionalTuning::ShimmeringLakesLowlandCeiling);
    TestEqual(TEXT("Shimmering Lakes lowland compression defaults to tuning"), Settings.ShimmeringLakesLowlandCompression,
        FKalmalaRegionalTuning::ShimmeringLakesLowlandCompression);

    auto CheckRange = [this, &Settings](double FKalmalaGenerationPreviewSettings::*Control,
        const double Minimum, const double Maximum, const TCHAR* Label)
    {
        Settings = FKalmalaGenerationPreviewSettings{};
        Settings.*Control = Minimum;
        TestTrue(FString::Printf(TEXT("%s accepts its inclusive minimum"), Label), Settings.HasValidReliefControls());
        Settings.*Control = Maximum;
        TestTrue(FString::Printf(TEXT("%s accepts its inclusive maximum"), Label), Settings.HasValidReliefControls());
        Settings.*Control = Minimum - 0.01;
        TestFalse(FString::Printf(TEXT("%s rejects below its minimum"), Label), Settings.HasValidReliefControls());
        Settings = FKalmalaGenerationPreviewSettings{};
        Settings.*Control = Maximum + 0.01;
        TestFalse(FString::Printf(TEXT("%s rejects above its maximum"), Label), Settings.HasValidReliefControls());
    };

    CheckRange(&FKalmalaGenerationPreviewSettings::ReliefBroadScale, 50000.0, 150000.0, TEXT("Broad scale"));
    CheckRange(&FKalmalaGenerationPreviewSettings::ReliefMiddleScale, 8000.0, 25000.0, TEXT("Middle scale"));
    CheckRange(&FKalmalaGenerationPreviewSettings::ReliefLocalScale, 1000.0, 5000.0, TEXT("Local scale"));
    CheckRange(&FKalmalaGenerationPreviewSettings::MeadowHillScale, 10000.0, 25000.0, TEXT("Meadow hill scale"));
    CheckRange(&FKalmalaGenerationPreviewSettings::MeadowHillAmplitude, 400.0, 1000.0, TEXT("Meadow hill amplitude"));
    CheckRange(&FKalmalaGenerationPreviewSettings::MeadowShoulderHeight, 600.0, 1600.0, TEXT("Meadow shoulder height"));
    CheckRange(&FKalmalaGenerationPreviewSettings::MeadowShoulderCompressionScale, 1000.0, 6000.0, TEXT("Meadow shoulder compression scale"));
    CheckRange(&FKalmalaGenerationPreviewSettings::MeadowRollingScale, 3000.0, 7000.0, TEXT("Meadow rolling scale"));
    CheckRange(&FKalmalaGenerationPreviewSettings::MeadowRollingAmplitude, 60.0, 200.0, TEXT("Meadow rolling amplitude"));
    CheckRange(&FKalmalaGenerationPreviewSettings::MeadowFloorScale, 800.0, 2000.0, TEXT("Meadow floor scale"));
    CheckRange(&FKalmalaGenerationPreviewSettings::MeadowFloorAmplitude, 10.0, 35.0, TEXT("Meadow floor amplitude"));
    CheckRange(&FKalmalaGenerationPreviewSettings::ElderwoodBroadAmplitude, 400.0, 1600.0, TEXT("Elderwood broad amplitude"));
    CheckRange(&FKalmalaGenerationPreviewSettings::ElderwoodMiddleAmplitude, 60.0, 400.0, TEXT("Elderwood middle amplitude"));
    CheckRange(&FKalmalaGenerationPreviewSettings::ElderwoodLocalAmplitude, 10.0, 100.0, TEXT("Elderwood local amplitude"));
    CheckRange(&FKalmalaGenerationPreviewSettings::ShimmeringLakesMeadowShare, 0.35, 0.65, TEXT("Shimmering Lakes Meadow share"));
    CheckRange(&FKalmalaGenerationPreviewSettings::ShimmeringLakesLowlandFloor, 100.0, 500.0, TEXT("Shimmering Lakes lowland floor"));
    CheckRange(&FKalmalaGenerationPreviewSettings::ShimmeringLakesLowlandCeiling, 500.0, 2000.0, TEXT("Shimmering Lakes lowland ceiling"));
    CheckRange(&FKalmalaGenerationPreviewSettings::ShimmeringLakesLowlandCompression, 0.05, 0.35, TEXT("Shimmering Lakes lowland compression"));
    Settings = FKalmalaGenerationPreviewSettings{};
    Settings.ShimmeringLakesLowlandFloor = 500.0;
    Settings.ShimmeringLakesLowlandCeiling = 500.0;
    TestFalse(TEXT("Lakes lowland ceiling must stay above its floor"), Settings.HasValidReliefControls());

    Settings = FKalmalaGenerationPreviewSettings{};
    Settings.ReliefBroadScale = TNumericLimits<double>::QuietNaN();
    TestFalse(TEXT("Non-finite terrain controls are rejected"), Settings.HasValidReliefControls());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaBiomeHeightFunctionTest,
    "Kalmala.World.Regional.HeightFunctionBaselines",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaBiomeHeightFunctionTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    constexpr double SourceHeight = 1000.0;
    const double Details[] = {-1.0, 0.0, 1.0};
    const FKalmalaTerrainReliefSample NoSharedRelief;
    const double ExpectedMeadows[] = {960.0, 1000.0, 1040.0};
    const double ExpectedElderwood[] = {925.0, 940.0, 955.0};
    const double ExpectedMire[] = {925.0, 940.0, 955.0};

    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Details); ++Index)
    {
        TestEqual(TEXT("Meadows candidate preserves the prior sample"),
            G::ShapeMeadows(SourceHeight, Details[Index]), ExpectedMeadows[Index]);
        TestEqual(TEXT("Elderwood candidate preserves the prior local detail at zero shared relief"),
            G::ShapeElderwood(SourceHeight, Details[Index], NoSharedRelief, 900.0, 240.0, 40.0), ExpectedElderwood[Index]);
        TestEqual(TEXT("Mire candidate preserves the prior sample"),
            G::ShapeMire(SourceHeight, Details[Index]), ExpectedMire[Index]);
        TestEqual(TEXT("Tundra uses the Elderwood candidate at zero shared relief"),
            G::ShapeTundra(SourceHeight, Details[Index], NoSharedRelief, 900.0, 240.0, 40.0), ExpectedElderwood[Index]);
        TestEqual(TEXT("Tundra direct reference is exactly the Elderwood function"),
            G::ShapeTundra(SourceHeight, Details[Index], NoSharedRelief, 900.0, 240.0, 40.0),
            G::ShapeElderwood(SourceHeight, Details[Index], NoSharedRelief, 900.0, 240.0, 40.0));
    }

    const double MountainRegions[] = {0.0, 0.5, 1.0};
    const double ExpectedMountains[3][3] = {
        {840.0, 890.0, 940.0},
        {1000.0, 1050.0, 1100.0},
        {1160.0, 1210.0, 1260.0},
    };
    for (int32 DetailIndex = 0; DetailIndex < UE_ARRAY_COUNT(Details); ++DetailIndex)
    for (int32 RegionIndex = 0; RegionIndex < UE_ARRAY_COUNT(MountainRegions); ++RegionIndex)
    {
        TestEqual(TEXT("Mountain candidate preserves detail and regional offset"),
            G::ShapeMountains(SourceHeight, Details[DetailIndex], MountainRegions[RegionIndex]),
            ExpectedMountains[DetailIndex][RegionIndex]);
    }

    const FKalmalaTerrainReliefSample LowRelief{-1.0, -1.0, -1.0};
    const FKalmalaTerrainReliefSample HighRelief{1.0, 1.0, 1.0};
    const double ElderwoodLow = G::ShapeElderwood(SourceHeight, 0.0, LowRelief, 900.0, 240.0, 40.0);
    const double ElderwoodHigh = G::ShapeElderwood(SourceHeight, 0.0, HighRelief, 900.0, 240.0, 40.0);
    TestTrue(TEXT("Elderwood relief remains bounded by the three named amplitudes"),
        ElderwoodLow >= SourceHeight - 900.0 - 240.0 - 40.0 - 60.0
        && ElderwoodHigh <= SourceHeight + 900.0 + 240.0 + 40.0 - 60.0);
    TestEqual(TEXT("Elderwood preserves source-height differences without Meadows compression"),
        G::ShapeElderwood(SourceHeight + 500.0, 0.0, HighRelief, 900.0, 240.0, 40.0) - ElderwoodHigh,
        500.0);
    const double ElderwoodReliefRange = ElderwoodHigh - ElderwoodLow;
    const double MeadowReliefRange = G::ShapeMeadows(2000.0, 1.0, 600.0, 1.0, 120.0,
        1.0, 20.0, 1000.0, 3000.0)
        - G::ShapeMeadows(2000.0, -1.0, 600.0, -1.0, 120.0, -1.0, 20.0, 1000.0, 3000.0);
    TestTrue(TEXT("Elderwood keeps stronger high-ground relief than compressed Meadows"),
        ElderwoodReliefRange > MeadowReliefRange);

    const FKalmalaWorldGenerationConfig SeedConfigs[] = {{418ull}, {419ull}, {419ull}};
    const FVector2D SamplePositions[] = {
        FVector2D(91450.9, -205569.1), // Seed-418 Elderwood baseline.
        FVector2D(-205569.1, 91450.9), // Seed-419 Elderwood baseline.
        FVector2D(314997.6, 735515.4), // Seed-419 Elderwood/Tundra control.
    };
    for (int32 SampleIndex = 0; SampleIndex < UE_ARRAY_COUNT(SeedConfigs); ++SampleIndex)
    {
        const auto Shared = G::SampleSharedRelief(SeedConfigs[SampleIndex], SamplePositions[SampleIndex]);
        const double Detail = G::Noise(SeedConfigs[SampleIndex], 250, SamplePositions[SampleIndex], 0.00015);
        const float Source = (FKalmalaWorldFieldSampler::Sample(SeedConfigs[SampleIndex], SamplePositions[SampleIndex]).Elevation
            - FKalmalaRegionalTuning::SeaElevation) * 2000.0f;
        const double Elderwood = G::ShapeElderwood(Source, Detail, Shared, 900.0, 240.0, 40.0);
        const double Tundra = G::ShapeTundra(Source, Detail, Shared, 900.0, 240.0, 40.0);
        TestTrue(TEXT("Shared relief is finite at positive and negative world coordinates"),
            FMath::IsFinite(Shared.Broad) && FMath::IsFinite(Shared.Middle) && FMath::IsFinite(Shared.Local));
        TestEqual(TEXT("Tundra remains the same seeded Elderwood shape across seeds and coordinates"), Tundra, Elderwood);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaShimmeringLakesShapeTest,
    "Kalmala.World.Regional.ShimmeringLakesContinuousShape",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaShimmeringLakesShapeTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    using T = FKalmalaRegionalTuning;
    const double SourceSamples[] = {-100.0, 0.0, 199.0, 200.0, 250.0, 450.0, 700.0,
        950.0, 1200.0, 1300.0};
    double Previous = -TNumericLimits<double>::Max();
    for (const double Source : SourceSamples)
    {
        const double Compressed = G::CompressShimmeringLakesLowland(Source, T::WetlandHumidityFull,
            T::WetlandHumidityMinimum, T::WetlandHumidityFull, T::ShimmeringLakesLowlandFloor,
            T::ShimmeringLakesLowlandCeiling, T::ShimmeringLakesLowlandCompression);
        TestTrue(TEXT("Lowland transform remains finite"), FMath::IsFinite(Compressed));
        TestTrue(TEXT("Lowland compression is monotonic across both smooth band edges"), Compressed >= Previous);
        TestTrue(TEXT("Lowland compression never raises source height"), Compressed <= Source);
        TestTrue(TEXT("Lowland compression stays within the configured maximum reduction"),
            Compressed >= Source - FMath::Max(0.0, Source - T::ShimmeringLakesLowlandFloor) * 0.35 - 0.001);
        if (Source <= T::ShimmeringLakesLowlandFloor || Source >= T::ShimmeringLakesLowlandCeiling)
        {
            TestEqual(TEXT("Outside lowland band the source height is unchanged"), Compressed, Source);
        }
        else
        {
            TestTrue(TEXT("Compressed lowland remains above its floor"), Compressed >= T::ShimmeringLakesLowlandFloor);
        }
        Previous = Compressed;
    }

    TestEqual(TEXT("Humidity below the existing wetland threshold adds no lowland compression"),
        G::CompressShimmeringLakesLowland(700.0, T::WetlandHumidityMinimum - 0.01,
            T::WetlandHumidityMinimum, T::WetlandHumidityFull, T::ShimmeringLakesLowlandFloor,
            T::ShimmeringLakesLowlandCeiling, T::ShimmeringLakesLowlandCompression), 700.0);
    TestEqual(TEXT("Invalid lowland interval safely preserves source height"),
        G::CompressShimmeringLakesLowland(700.0, T::WetlandHumidityFull,
            T::WetlandHumidityMinimum, T::WetlandHumidityFull, 800.0, 700.0, 0.2), 700.0);
    TestEqual(TEXT("Invalid humidity interval safely preserves source height"),
        G::CompressShimmeringLakesLowland(700.0, T::WetlandHumidityFull,
            0.7, 0.6, T::ShimmeringLakesLowlandFloor, T::ShimmeringLakesLowlandCeiling, 0.2), 700.0);

    constexpr double SourceHeight = 700.0;
    constexpr double MeadowCandidate = 1100.0;
    constexpr double ElderwoodCandidate = 900.0;
    const double CompressedSource = G::CompressShimmeringLakesLowland(SourceHeight,
        T::WetlandHumidityFull, T::WetlandHumidityMinimum, T::WetlandHumidityFull,
        T::ShimmeringLakesLowlandFloor, T::ShimmeringLakesLowlandCeiling,
        T::ShimmeringLakesLowlandCompression);
    const double LakesCandidate = G::ShapeShimmeringLakes(SourceHeight, MeadowCandidate,
        ElderwoodCandidate, T::WetlandHumidityFull, T::WetlandHumidityMinimum,
        T::WetlandHumidityFull, T::ShimmeringLakesMeadowShare,
        T::ShimmeringLakesLowlandFloor, T::ShimmeringLakesLowlandCeiling,
        T::ShimmeringLakesLowlandCompression);
    const double RoundedShoulders = FMath::Lerp(ElderwoodCandidate, MeadowCandidate,
        T::ShimmeringLakesMeadowShare);
    TestEqual(TEXT("Lakes candidate mixes Meadow and Elderwood shoulders with only the lowland offset"),
        LakesCandidate, RoundedShoulders + (CompressedSource - SourceHeight));

    // Use the fixed former-basin site where Meadows, Lakes and Elderwood all
    // have continuous non-zero shares. Transfer a tiny share between adjacent
    // candidates and require the height to follow their weighted difference.
    const FKalmalaWorldGenerationConfig Config{418ull};
    const FVector2D FormerBasinPosition(100830.5, -143038.6);
    const FKalmalaWorldFieldSample Fields = FKalmalaWorldFieldSampler::Sample(Config, FormerBasinPosition);
    const FKalmalaRegionalSample WeightsSample = G::SampleInternal(Fields, true, true);
    TestTrue(TEXT("Former basin fixture has continuous Meadows, Lakes and Elderwood weights"),
        WeightsSample.Weights[0] > 0.0f && WeightsSample.Weights[1] > 0.0f && WeightsSample.Weights[2] > 0.0f);

    const auto& Preview = FKalmalaGenerationPreview::Get();
    const double Source = (Fields.Elevation - T::SeaElevation) * 2000.0;
    const double Detail = G::Noise(Config, 250, FormerBasinPosition, 0.00015);
    const double Hill = G::Noise(Config, T::MeadowHillSalt, FormerBasinPosition, 1.0 / Preview.MeadowHillScale);
    const double Rolling = G::Noise(Config, T::MeadowRollingSalt, FormerBasinPosition, 1.0 / Preview.MeadowRollingScale);
    const double Floor = G::Noise(Config, T::MeadowFloorSalt, FormerBasinPosition, 1.0 / Preview.MeadowFloorScale);
    const double Meadow = G::ShapeMeadows(Source, Hill, Preview.MeadowHillAmplitude,
        Rolling, Preview.MeadowRollingAmplitude, Floor, Preview.MeadowFloorAmplitude,
        Preview.MeadowShoulderHeight, Preview.MeadowShoulderCompressionScale);
    const auto SharedRelief = G::SampleSharedRelief(Config, FormerBasinPosition);
    const double Elderwood = G::ShapeElderwood(Source, Detail, SharedRelief,
        Preview.ElderwoodBroadAmplitude, Preview.ElderwoodMiddleAmplitude, Preview.ElderwoodLocalAmplitude);
    const double Lakes = G::ShapeShimmeringLakes(Source, Meadow, Elderwood, Fields.Humidity,
        Preview.WetlandHumidityMinimum, Preview.WetlandHumidityFull,
        Preview.ShimmeringLakesMeadowShare, Preview.ShimmeringLakesLowlandFloor,
        Preview.ShimmeringLakesLowlandCeiling, Preview.ShimmeringLakesLowlandCompression);
    const double Candidates[] = {Meadow, Lakes, Elderwood, G::ShapeMire(Source, Detail),
        G::ShapeTundra(Source, Detail, SharedRelief, Preview.ElderwoodBroadAmplitude,
            Preview.ElderwoodMiddleAmplitude, Preview.ElderwoodLocalAmplitude),
        G::ShapeMountains(Source, Detail, 0.0)};
    const float FullWeights[] = {WeightsSample.Weights[0], WeightsSample.Weights[1],
        WeightsSample.Weights[2], WeightsSample.Weights[3], WeightsSample.Weights[4],
        WeightsSample.Weights[5], WeightsSample.Weights[6]};
    const FKalmalaRegionalSample Integrated = G::SampleInternal(Fields, false, true);
    TestEqual(TEXT("Regional sampler uses the Meadows/Elderwood Lakes candidate blend"), Integrated.Height,
        G::BlendCandidateHeights(float(Source), Candidates, 0.0f, FullWeights));
    constexpr float WeightStep = 0.0001f;
    float NeighborWeights[7];
    FMemory::Memcpy(NeighborWeights, FullWeights, sizeof(FullWeights));
    NeighborWeights[0] += WeightStep;
    NeighborWeights[1] -= WeightStep;
    const float HeightBefore = G::BlendCandidateHeights(float(Source), Candidates, 0.0f, FullWeights);
    const float HeightAfter = G::BlendCandidateHeights(float(Source), Candidates, 0.0f, NeighborWeights);
    TestTrue(TEXT("Neighboring Meadows/Lakes weight transfer changes height continuously"),
        FMath::IsNearlyEqual(HeightAfter - HeightBefore, WeightStep * float(Meadow - Lakes), 0.02f));

    TestFalse(TEXT("Compiled analytic basin support remains disabled"), T::bEnableAnalyticBasins);
    const FKalmalaRegionalSample FormerBasin = G::Sample(Config, FormerBasinPosition);
    TestEqual(TEXT("Former basin receives no analytic basin support"), FormerBasin.BasinWeight, 0.0f);
    TestEqual(TEXT("Former basin receives no inland basin water level"), FormerBasin.WaterLevel, 0.0f);
    TestFalse(TEXT("Former basin remains dry without river water"), FormerBasin.bHasWater);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaBiomeHeightBlendTest,
    "Kalmala.World.Regional.ContinuousHeightBlend",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaBiomeHeightBlendTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    constexpr float Source = 1000.0f;
    const double Candidates[] = {1100.0, 1200.0, 900.0, 950.0, 1050.0, 1300.0};

    // These weights are already normalized land-biome shares multiplied by Land=0.75;
    // the ocean share completes the seven weights to one.
    const float InlandWeights[] = {0.375f, 0.375f, 0.0f, 0.0f, 0.0f, 0.0f, 0.25f};
    float WeightTotal = 0.0f;
    float LandWeightTotal = 0.0f;
    for (int32 Index = 0; Index < 7; ++Index)
    {
        WeightTotal += InlandWeights[Index];
        if (Index < 6) LandWeightTotal += InlandWeights[Index];
    }
    TestTrue(TEXT("Normalized land weights plus ocean weight sum to one"), FMath::IsNearlyEqual(WeightTotal, 1.0f));
    TestTrue(TEXT("Land weights retain one coast factor"), FMath::IsNearlyEqual(LandWeightTotal, 0.75f));

    const float NoOceanOffset = 0.0f;
    const float InlandHeight = G::BlendCandidateHeights(Source, Candidates, NoOceanOffset, InlandWeights);
    TestTrue(TEXT("Candidate deltas use the source base once"), FMath::IsNearlyEqual(InlandHeight, 1112.5f));

    const float CoastWeights[] = {0.2f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f};
    const float CoastHeight = G::BlendCandidateHeights(Source, Candidates, NoOceanOffset, CoastWeights);
    const float InlandRelief = InlandHeight - Source;
    const float CoastRelief = CoastHeight - Source;
    TestTrue(TEXT("Continuous coast attenuation scales the land blend once"),
        FMath::IsNearlyEqual(CoastRelief / InlandRelief, 0.4f / 0.75f, 0.0001f));

    const float NearTieWeights[] = {0.3751f, 0.3749f, 0.0f, 0.0f, 0.0f, 0.0f, 0.25f};
    const float NearTieHeight = G::BlendCandidateHeights(Source, Candidates, NoOceanOffset, NearTieWeights);
    TestTrue(TEXT("A small biome-weight change has no label-switch height step"),
        FMath::Abs(NearTieHeight - InlandHeight) < 0.05f);

    const float OceanOffset = -20.0f;
    TestTrue(TEXT("Ocean shaping remains weighted independently"),
        FMath::IsNearlyEqual(G::BlendCandidateHeights(Source, Candidates, OceanOffset, InlandWeights), 1107.5f));

    const FKalmalaWorldGenerationConfig Config{418ull};
    const FVector2D Position(-182120.2, 171177.3);
    const FKalmalaWorldFieldSample Fields = FKalmalaWorldFieldSampler::Sample(Config, Position);
    const FKalmalaRegionalSample Sample = G::SampleInternal(Fields, false, true);
    TestTrue(TEXT("Fixed Meadows/Elderwood transition has both candidate weights"),
        Sample.Weights[0] > 0.49f && Sample.Weights[2] > 0.49f);
    TestEqual(TEXT("Fixed transition does not use an ocean candidate"), Sample.Weights[6], 0.0f);

    const float SourceHeight = (Fields.Elevation - FKalmalaRegionalTuning::SeaElevation) * 2000.0f;
    const double Detail = G::Noise(Config, 250, Position, 0.00015);
    const auto& Preview = FKalmalaGenerationPreview::Get();
    const FKalmalaTerrainReliefSample ElderwoodRelief = G::SampleSharedRelief(Config, Position);
    constexpr double ElderwoodBroadAmplitude = FKalmalaRegionalTuning::ElderwoodBroadAmplitude;
    constexpr double ElderwoodMiddleAmplitude = FKalmalaRegionalTuning::ElderwoodMiddleAmplitude;
    constexpr double ElderwoodLocalAmplitude = FKalmalaRegionalTuning::ElderwoodLocalAmplitude;
    const double HillNoise = G::Noise(Config, FKalmalaRegionalTuning::MeadowHillSalt,
        Position, 1.0 / Preview.MeadowHillScale);
    const double RollingNoise = G::Noise(Config, FKalmalaRegionalTuning::MeadowRollingSalt,
        Position, 1.0 / Preview.MeadowRollingScale);
    const double MeadowFloorNoise = G::Noise(Config, FKalmalaRegionalTuning::MeadowFloorSalt,
        Position, 1.0 / Preview.MeadowFloorScale);
    const double TransitionCandidates[] = {
        G::ShapeMeadows(SourceHeight, HillNoise, Preview.MeadowHillAmplitude,
            RollingNoise, Preview.MeadowRollingAmplitude,
            MeadowFloorNoise, Preview.MeadowFloorAmplitude,
            Preview.MeadowShoulderHeight, Preview.MeadowShoulderCompressionScale),
        G::ShapeShimmeringLakes(SourceHeight,
            G::ShapeMeadows(SourceHeight, HillNoise, Preview.MeadowHillAmplitude,
                RollingNoise, Preview.MeadowRollingAmplitude, MeadowFloorNoise, Preview.MeadowFloorAmplitude,
                Preview.MeadowShoulderHeight, Preview.MeadowShoulderCompressionScale),
            G::ShapeElderwood(SourceHeight, Detail, ElderwoodRelief,
                ElderwoodBroadAmplitude, ElderwoodMiddleAmplitude, ElderwoodLocalAmplitude),
            Fields.Humidity, Preview.WetlandHumidityMinimum, Preview.WetlandHumidityFull,
            Preview.ShimmeringLakesMeadowShare, Preview.ShimmeringLakesLowlandFloor,
            Preview.ShimmeringLakesLowlandCeiling, Preview.ShimmeringLakesLowlandCompression),
        G::ShapeElderwood(SourceHeight, Detail, ElderwoodRelief,
            ElderwoodBroadAmplitude, ElderwoodMiddleAmplitude, ElderwoodLocalAmplitude),
        G::ShapeMire(SourceHeight, Detail),
        G::ShapeTundra(SourceHeight, Detail, ElderwoodRelief,
            ElderwoodBroadAmplitude, ElderwoodMiddleAmplitude, ElderwoodLocalAmplitude),
        G::ShapeMountains(SourceHeight, Detail, 0.0),
    };
    const float NoTransitionOceanOffset = 0.0f;
    const float ExpectedTransitionHeight = G::BlendCandidateHeights(
        SourceHeight, TransitionCandidates, NoTransitionOceanOffset, Sample.Weights);
    TestEqual(TEXT("Regional sampling blends candidates with its continuous biome weights"),
        Sample.Height, ExpectedTransitionHeight);
    TestEqual(TEXT("Height blending leaves classifier identity unchanged"), Sample.Biome, G::SampleBiome(Fields));

    const FVector2D ElderwoodTundraPosition(314997.6, 735515.4);
    const FKalmalaWorldGenerationConfig ElderwoodTundraConfig{419ull};
    const auto ElderwoodTundraFields = FKalmalaWorldFieldSampler::Sample(ElderwoodTundraConfig, ElderwoodTundraPosition);
    const auto ElderwoodTundraSample = G::SampleInternal(ElderwoodTundraFields, false, true);
    TestTrue(TEXT("Fixed Elderwood/Tundra control has both continuous biome weights"),
        ElderwoodTundraSample.Weights[2] > 0.4f && ElderwoodTundraSample.Weights[4] > 0.4f);
    const float ElderwoodTundraSource = (ElderwoodTundraFields.Elevation - FKalmalaRegionalTuning::SeaElevation) * 2000.0f;
    const auto ElderwoodTundraRelief = G::SampleSharedRelief(ElderwoodTundraConfig, ElderwoodTundraPosition);
    const double ElderwoodTundraDetail = G::Noise(ElderwoodTundraConfig, 250, ElderwoodTundraPosition, 0.00015);
    const double SharedElderwoodCandidate = G::ShapeElderwood(ElderwoodTundraSource, ElderwoodTundraDetail,
        ElderwoodTundraRelief, ElderwoodBroadAmplitude, ElderwoodMiddleAmplitude, ElderwoodLocalAmplitude);
    const double ElderwoodTundraCandidates[] = {
        ElderwoodTundraSource,
        ElderwoodTundraSource,
        SharedElderwoodCandidate,
        ElderwoodTundraSource,
        SharedElderwoodCandidate,
        ElderwoodTundraSource,
    };
    const float ElderwoodTundraWeightTotal = ElderwoodTundraSample.Weights[2] + ElderwoodTundraSample.Weights[4];
    const float ElderwoodTundraBlendWeights[] = {
        0.0f, 0.0f, ElderwoodTundraSample.Weights[2] / ElderwoodTundraWeightTotal,
        0.0f, ElderwoodTundraSample.Weights[4] / ElderwoodTundraWeightTotal, 0.0f, 0.0f,
    };
    TestTrue(TEXT("Elderwood/Tundra transition blends the identical shared candidate through both weights"),
        FMath::IsNearlyEqual(G::BlendCandidateHeights(ElderwoodTundraSource, ElderwoodTundraCandidates,
            0.0f, ElderwoodTundraBlendWeights), float(SharedElderwoodCandidate)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaMeadowShoulderCompressionTest,
    "Kalmala.World.Regional.MeadowShoulderCompression",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaMeadowShoulderCompressionTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    constexpr double ShoulderHeight = 1000.0;
    constexpr double CompressionScale = 3000.0;
    const double Inputs[] = {-500.0, 0.0, 500.0, ShoulderHeight, 1001.0, 1500.0, 2500.0, 4000.0, 1000000.0};
    double Previous = -TNumericLimits<double>::Max();
    for (const double Input : Inputs)
    {
        const double Output = G::CompressMeadowShoulder(Input, ShoulderHeight, CompressionScale);
        TestTrue(TEXT("Meadow compression remains finite"), FMath::IsFinite(Output));
        TestTrue(TEXT("Upper-shoulder compression is monotonic"), Output >= Previous);
        TestTrue(TEXT("Compression is bounded by its finite asymptote"), Output <= ShoulderHeight + CompressionScale);
        if (Input <= ShoulderHeight)
        {
            TestEqual(TEXT("Lower terrain and camp pockets are unchanged by the shoulder curve"), Output, Input);
        }
        else
        {
            TestTrue(TEXT("Upper shoulders are smoothly reduced"), Output < Input && Output > ShoulderHeight);
        }
        Previous = Output;
    }

    const double LowerPocket = G::ShapeMeadows(400.0, -0.25, 600.0,
        ShoulderHeight, CompressionScale);
    TestEqual(TEXT("Meadow hills preserve the source and add the signed hill once"), LowerPocket, 250.0);
    const double PositiveHill = G::ShapeMeadows(400.0, 0.5, 600.0,
        ShoulderHeight, CompressionScale);
    TestEqual(TEXT("Uncompressed hill adds its signed amplitude once"), PositiveHill, 700.0);
    const double RollingShoulder = G::ShapeMeadows(400.0, 0.5, 600.0, 0.5, 120.0,
        ShoulderHeight, CompressionScale);
    TestEqual(TEXT("Middle-scale rolling relief adds independently before shoulder compression"),
        RollingShoulder, 760.0);
    const double RollingHollow = G::ShapeMeadows(400.0, 0.0, 600.0, -0.5, 120.0,
        ShoulderHeight, CompressionScale);
    TestEqual(TEXT("Rolling trough preserves lower camp-pocket terrain"), RollingHollow, 340.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaMeadowRollingProfileTest,
    "Kalmala.World.Regional.MeadowRollingProfiles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaMeadowRollingProfileTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    const FVector2D Anchors[] = {
        FVector2D(208695.7, -293111.9), // Seed-418 outer dry Meadows baseline.
        FVector2D(-355642.4, 55495.8), // Seed-419 outer dry Meadows baseline.
    };
    const uint64 Seeds[] = {418ull, 419ull};
    const FVector2D Directions[] = {FVector2D(1.0, 0.0), FVector2D(0.0, 1.0)};
    const double Distances[] = {3000.0, 4500.0, 7000.0};

    for (int32 SiteIndex = 0; SiteIndex < UE_ARRAY_COUNT(Anchors); ++SiteIndex)
    {
        const FKalmalaWorldGenerationConfig Config{Seeds[SiteIndex]};
        for (const FVector2D Direction : Directions)
        {
            for (const double Distance : Distances)
            {
                double MinimumHeight = TNumericLimits<double>::Max();
                double MaximumHeight = -TNumericLimits<double>::Max();
                int32 DrySamples = 0;
                double MaximumSlopeDegrees = 0.0;
                for (int32 SampleIndex = 0; SampleIndex <= 10; ++SampleIndex)
                {
                    const double Offset = Distance * (double(SampleIndex) / 10.0 - 0.5);
                    const FVector2D Position = Anchors[SiteIndex] + Direction * Offset;
                    const FKalmalaRegionalSample Sample = G::Sample(Config, Position);
                    TestTrue(TEXT("Rolling profile height is finite"), FMath::IsFinite(Sample.Height));
                    const FKalmalaRegionalSample Repeat = G::Sample(Config, Position);
                    TestEqual(TEXT("Rolling profile is deterministic"), Repeat.Height, Sample.Height);
                    if (Sample.bHasWater) continue;
                    ++DrySamples;
                    TestTrue(TEXT("Dry profile remains within the Meadows blend"), Sample.Weights[0] > 0.0f);
                    MinimumHeight = FMath::Min(MinimumHeight, double(Sample.Height));
                    MaximumHeight = FMath::Max(MaximumHeight, double(Sample.Height));
                    const FVector Normal = FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, Position);
                    const double SlopeDegrees = FMath::RadiansToDegrees(
                        FMath::Acos(FMath::Clamp(double(Normal.Z), -1.0, 1.0)));
                    TestTrue(TEXT("Rolling profile slope is finite"), FMath::IsFinite(SlopeDegrees));
                    MaximumSlopeDegrees = FMath::Max(MaximumSlopeDegrees, SlopeDegrees);
                }
                TestTrue(TEXT("Fixed outer-Meadows profile retains enough dry samples"), DrySamples >= 8);
                TestTrue(FString::Printf(TEXT("%.0f m profile has continuous height variation"), Distance / 100.0),
                    DrySamples >= 8 && MaximumHeight - MinimumHeight > 0.01);
                TestTrue(TEXT("Fixed outer-Meadows profile avoids extreme traversal slopes"),
                    MaximumSlopeDegrees < 45.0);
            }
        }
    }

    TestTrue(TEXT("Meadow rolling noise has an independent domain"),
        FKalmalaRegionalTuning::MeadowRollingSalt != FKalmalaRegionalTuning::MeadowHillSalt
        && FKalmalaRegionalTuning::MeadowRollingSalt != FKalmalaRegionalTuning::ReliefBroadSalt
        && FKalmalaRegionalTuning::MeadowRollingSalt != FKalmalaRegionalTuning::ReliefMiddleSalt
        && FKalmalaRegionalTuning::MeadowRollingSalt != FKalmalaRegionalTuning::ReliefLocalSalt);

    const FKalmalaWorldGenerationConfig Config{418ull};
    const FVector2D PatchEdge(3000.0, 0.0);
    const float WestHeight = FKalmalaTerrainHeightSampler::SampleHeight(Config, PatchEdge - FVector2D(0.5, 0.0));
    const float EastHeight = FKalmalaTerrainHeightSampler::SampleHeight(Config, PatchEdge + FVector2D(0.5, 0.0));
    TestTrue(TEXT("Middle-scale rolling signal has no X patch-edge height step"),
        FMath::Abs(WestHeight - EastHeight) < 5.0f);
    const float SouthHeight = FKalmalaTerrainHeightSampler::SampleHeight(Config, PatchEdge + FVector2D(0.0, 2999.5));
    const float NorthHeight = FKalmalaTerrainHeightSampler::SampleHeight(Config, PatchEdge + FVector2D(0.0, 3000.5));
    TestTrue(TEXT("Middle-scale rolling signal has no Y patch-edge height step"),
        FMath::Abs(SouthHeight - NorthHeight) < 5.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaMeadowFloorDetailTest,
    "Kalmala.World.Regional.MeadowFloorDetail",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaMeadowFloorDetailTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    using L = FKalmalaTerrainPatchLayout;
    using T = FKalmalaRegionalTuning;
    const FKalmalaGenerationPreviewSettings Preview;
    const FKalmalaWorldGenerationConfig Config{418ull};
    const FKalmalaWorldGenerationConfig OtherSeed{419ull};

    TestTrue(TEXT("Documented Meadow floor settings are valid"), Preview.HasValidReliefControls());
    TestEqual(TEXT("Floor scale uses the documented nominal trial"), Preview.MeadowFloorScale, 1200.0);
    TestEqual(TEXT("Floor amplitude uses the documented nominal trial"), Preview.MeadowFloorAmplitude, 20.0);
    TestTrue(TEXT("Floor detail uses at least six collision samples per smallest allowed scale"),
        800.0 / (L::PatchSize / FKalmalaWaterSurfaceMesh::CellsPerSide) >= 6.0);

    const FVector2D Position(208695.7, -293111.9); // Fixed seed-418 outer dry Meadows site.
    const double FloorNoise = G::Noise(Config, T::MeadowFloorSalt, Position, 1.0 / Preview.MeadowFloorScale);
    TestTrue(TEXT("Meadow floor sample is finite and normalized"),
        FMath::IsFinite(FloorNoise) && FMath::Abs(FloorNoise) <= 1.0);
    TestEqual(TEXT("Meadow floor sample repeats for a fixed seed and XY"), FloorNoise,
        G::Noise(Config, T::MeadowFloorSalt, Position, 1.0 / Preview.MeadowFloorScale));
    TestTrue(TEXT("Meadow floor sample varies with world seed"),
        FloorNoise != G::Noise(OtherSeed, T::MeadowFloorSalt, Position, 1.0 / Preview.MeadowFloorScale));
    TestTrue(TEXT("Meadow floor has an independent noise domain"),
        T::MeadowFloorSalt != T::MeadowHillSalt && T::MeadowFloorSalt != T::MeadowRollingSalt
        && T::MeadowFloorSalt != T::ReliefBroadSalt && T::MeadowFloorSalt != T::ReliefMiddleSalt
        && T::MeadowFloorSalt != T::ReliefLocalSalt);

    const double FlatBase = G::ShapeMeadows(400.0, 0.2, 600.0, -0.1, 120.0,
        0.0, 20.0, 1000.0, 3000.0);
    const double FloorHollow = G::ShapeMeadows(400.0, 0.2, 600.0, -0.1, 120.0,
        -0.5, 20.0, 1000.0, 3000.0);
    const double FloorRise = G::ShapeMeadows(400.0, 0.2, 600.0, -0.1, 120.0,
        0.5, 20.0, 1000.0, 3000.0);
    TestTrue(TEXT("Floor hollow adds its signed detail without flattening lower terrain"),
        FMath::IsNearlyEqual(FlatBase - FloorHollow, 10.0, 0.001) && FloorHollow < FlatBase);
    TestTrue(TEXT("Floor rise adds its signed detail before shoulder shaping"),
        FMath::IsNearlyEqual(FloorRise - FlatBase, 10.0, 0.001) && FloorRise > FlatBase);
    const double RaisedBase = G::ShapeMeadows(1400.0, 0.2, 600.0, -0.1, 120.0,
        0.0, 20.0, 1000.0, 3000.0);
    const double RaisedWithFloor = G::ShapeMeadows(1400.0, 0.2, 600.0, -0.1, 120.0,
        0.5, 20.0, 1000.0, 3000.0);
    TestTrue(TEXT("Floor detail remains ordered through shoulder compression"),
        RaisedWithFloor > RaisedBase && RaisedWithFloor - RaisedBase < 10.0);

    const FVector2D PatchOrigin(0.0, 0.0);
    const FVector2D LeftCenter = L::GetPatchCenter(PatchOrigin, 0, 0);
    const FVector2D RightCenter = L::GetPatchCenter(PatchOrigin, 1, 0);
    const FVector2D FromLeft = LeftCenter + FVector2D(L::PatchSize * 0.5 - 0.5, 0.0);
    const FVector2D FromRight = RightCenter - FVector2D(L::PatchSize * 0.5 - 0.5, 0.0);
    TestTrue(TEXT("Floor probes straddle an X patch edge"),
        L::GetPatchCoordinate(PatchOrigin, FromLeft) != L::GetPatchCoordinate(PatchOrigin, FromRight));
    const double LeftSample = G::Noise(Config, T::MeadowFloorSalt, FromLeft, 1.0 / Preview.MeadowFloorScale);
    const double RightSample = G::Noise(Config, T::MeadowFloorSalt, FromRight, 1.0 / Preview.MeadowFloorScale);
    TestTrue(TEXT("Absolute-world floor noise is continuous across the X patch edge"),
        FMath::Abs(RightSample - LeftSample) * Preview.MeadowFloorAmplitude < 0.5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaMeadowHillDomainTest,
    "Kalmala.World.Regional.MeadowHillDomain",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaMeadowHillDomainTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    using T = FKalmalaRegionalTuning;
    const FKalmalaWorldGenerationConfig Config{418ull};
    const FVector2D BaselineAnchor(208695.7, -293111.9); // Seed-418 outer dry Meadows baseline.
    const double Frequency = 1.0 / FKalmalaRegionalTuning::MeadowHillScale;
    double MinimumNoise = TNumericLimits<double>::Max();
    double MaximumNoise = -TNumericLimits<double>::Max();
    for (int32 Y = 0; Y <= 10; ++Y)
    for (int32 X = 0; X <= 10; ++X)
    {
        const FVector2D Position = BaselineAnchor + FVector2D(X * 5000.0 - 25000.0, Y * 5000.0 - 25000.0);
        const double Noise = G::Noise(Config, T::MeadowHillSalt, Position, Frequency);
        TestTrue(TEXT("Meadow hill noise is finite"), FMath::IsFinite(Noise));
        TestTrue(TEXT("Meadow hill signal repeats at the fixed baseline site"),
            Noise == G::Noise(Config, T::MeadowHillSalt, Position, Frequency));
        MinimumNoise = FMath::Min(MinimumNoise, Noise);
        MaximumNoise = FMath::Max(MaximumNoise, Noise);
    }
    TestTrue(TEXT("The fixed 500 m Meadow neighborhood contains both hills and hollows"),
        MinimumNoise < 0.0 && MaximumNoise > 0.0);
    TestTrue(TEXT("The fixed neighborhood spans a useful continuous hill range"),
        MaximumNoise - MinimumNoise > 0.5);
    TestTrue(TEXT("Meadow hills use a domain separate from shared terrain relief"),
        T::MeadowHillSalt != T::ReliefBroadSalt && T::MeadowHillSalt != T::ReliefMiddleSalt
        && T::MeadowHillSalt != T::ReliefLocalSalt);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaSharedTerrainReliefTest,
    "Kalmala.World.Regional.SharedTerrainRelief",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaSharedTerrainReliefTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    using L = FKalmalaTerrainPatchLayout;
    const FKalmalaWorldGenerationConfig Config{418ull};
    const FKalmalaWorldGenerationConfig OtherConfig{419ull};
    const FVector2D Positions[] = {
        FVector2D(-153210.25, 7823.5),
        FVector2D(-10000.0, -3000.0),
        FVector2D(12000.0, -45000.0),
        FVector2D(-185000.0, 276000.0),
    };

    TestTrue(TEXT("Broad, middle and local salts are independent"),
        FKalmalaRegionalTuning::ReliefBroadSalt != FKalmalaRegionalTuning::ReliefMiddleSalt
        && FKalmalaRegionalTuning::ReliefBroadSalt != FKalmalaRegionalTuning::ReliefLocalSalt
        && FKalmalaRegionalTuning::ReliefMiddleSalt != FKalmalaRegionalTuning::ReliefLocalSalt);

    G::ClearHydrologyCache();
    TArray<FKalmalaTerrainReliefSample> Baseline;
    int32 BroadMiddleDifferences = 0;
    int32 MiddleLocalDifferences = 0;
    int32 BroadLocalDifferences = 0;
    int32 SeedDifferences = 0;
    for (const FVector2D Position : Positions)
    {
        const double HeightBefore = G::Sample(Config, Position).Height;
        const FKalmalaTerrainReliefSample Relief = G::SampleSharedRelief(Config, Position);
        Baseline.Add(Relief);
        TestTrue(TEXT("All relief channels are finite and normalized"),
            FMath::IsFinite(Relief.Broad) && FMath::IsFinite(Relief.Middle) && FMath::IsFinite(Relief.Local)
            && FMath::Abs(Relief.Broad) <= 1.0 && FMath::Abs(Relief.Middle) <= 1.0 && FMath::Abs(Relief.Local) <= 1.0);
        const auto Repeated = G::SampleSharedRelief(Config, Position);
        TestEqual(TEXT("Broad relief repeats at an absolute world position"), Repeated.Broad, Relief.Broad);
        TestEqual(TEXT("Middle relief repeats at an absolute world position"), Repeated.Middle, Relief.Middle);
        TestEqual(TEXT("Local relief repeats at an absolute world position"), Repeated.Local, Relief.Local);
        const double HeightAfter = G::Sample(Config, Position).Height;
        TestEqual(TEXT("Sampling the staged relief does not change production height"), HeightAfter, HeightBefore);

        BroadMiddleDifferences += FMath::Abs(Relief.Broad - Relief.Middle) > 0.0001;
        MiddleLocalDifferences += FMath::Abs(Relief.Middle - Relief.Local) > 0.0001;
        BroadLocalDifferences += FMath::Abs(Relief.Broad - Relief.Local) > 0.0001;
        const auto OtherSeed = G::SampleSharedRelief(OtherConfig, Position);
        SeedDifferences += FMath::Abs(OtherSeed.Broad - Relief.Broad) > 0.0001
            || FMath::Abs(OtherSeed.Middle - Relief.Middle) > 0.0001
            || FMath::Abs(OtherSeed.Local - Relief.Local) > 0.0001;
    }
    TestTrue(TEXT("Broad and middle domains produce distinct signals"), BroadMiddleDifferences > 0);
    TestTrue(TEXT("Middle and local domains produce distinct signals"), MiddleLocalDifferences > 0);
    TestTrue(TEXT("Broad and local domains produce distinct signals"), BroadLocalDifferences > 0);
    TestTrue(TEXT("Changing world seed changes the shared relief stack"), SeedDifferences > 0);

    G::ClearHydrologyCache();
    for (int32 Index = UE_ARRAY_COUNT(Positions) - 1; Index >= 0; --Index)
    {
        G::SampleSharedRelief(OtherConfig, Positions[Index]);
        const auto Reversed = G::SampleSharedRelief(Config, Positions[Index]);
        TestEqual(TEXT("Cache reset and reversed query order preserve broad relief"), Reversed.Broad, Baseline[Index].Broad);
        TestEqual(TEXT("Cache reset and reversed query order preserve middle relief"), Reversed.Middle, Baseline[Index].Middle);
        TestEqual(TEXT("Cache reset and reversed query order preserve local relief"), Reversed.Local, Baseline[Index].Local);
    }

    const FVector2D PatchOrigin(-12000.0, -24000.0);
    const FVector2D Patch00 = L::GetPatchCenter(PatchOrigin, 0, 0);
    const FVector2D Patch10 = L::GetPatchCenter(PatchOrigin, 1, 0);
    const FVector2D SharedXFromLeft = Patch00 + FVector2D(L::PatchSize * 0.5, 720.0);
    const FVector2D SharedXFromRight = Patch10 + FVector2D(-L::PatchSize * 0.5, 720.0);
    TestTrue(TEXT("Adjacent X patches resolve the same absolute seam position"),
        (SharedXFromLeft - SharedXFromRight).IsNearlyZero());
    const auto XLeft = G::SampleSharedRelief(Config, SharedXFromLeft - FVector2D(0.5, 0.0));
    const auto XRight = G::SampleSharedRelief(Config, SharedXFromRight + FVector2D(0.5, 0.0));
    TestTrue(TEXT("Broad relief is continuous across an X patch seam"), FMath::Abs(XRight.Broad - XLeft.Broad) < 0.01);
    TestTrue(TEXT("Middle relief is continuous across an X patch seam"), FMath::Abs(XRight.Middle - XLeft.Middle) < 0.01);
    TestTrue(TEXT("Local relief is continuous across an X patch seam"), FMath::Abs(XRight.Local - XLeft.Local) < 0.01);
    TestTrue(TEXT("X seam probes belong to adjacent patches"),
        L::GetPatchCoordinate(PatchOrigin, SharedXFromLeft - FVector2D(0.5, 0.0))
        != L::GetPatchCoordinate(PatchOrigin, SharedXFromRight + FVector2D(0.5, 0.0)));

    const FVector2D Patch01 = L::GetPatchCenter(PatchOrigin, 0, 1);
    const FVector2D SharedYFromSouth = Patch00 + FVector2D(720.0, L::PatchSize * 0.5);
    const FVector2D SharedYFromNorth = Patch01 + FVector2D(720.0, -L::PatchSize * 0.5);
    TestTrue(TEXT("Adjacent Y patches resolve the same absolute seam position"),
        (SharedYFromSouth - SharedYFromNorth).IsNearlyZero());
    const auto YSouth = G::SampleSharedRelief(Config, SharedYFromSouth - FVector2D(0.0, 0.5));
    const auto YNorth = G::SampleSharedRelief(Config, SharedYFromNorth + FVector2D(0.0, 0.5));
    TestTrue(TEXT("Broad relief is continuous across a Y patch seam"), FMath::Abs(YNorth.Broad - YSouth.Broad) < 0.01);
    TestTrue(TEXT("Middle relief is continuous across a Y patch seam"), FMath::Abs(YNorth.Middle - YSouth.Middle) < 0.01);
    TestTrue(TEXT("Local relief is continuous across a Y patch seam"), FMath::Abs(YNorth.Local - YSouth.Local) < 0.01);
    TestTrue(TEXT("Y seam probes belong to adjacent patches"),
        L::GetPatchCoordinate(PatchOrigin, SharedYFromSouth - FVector2D(0.0, 0.5))
        != L::GetPatchCoordinate(PatchOrigin, SharedYFromNorth + FVector2D(0.0, 0.5)));

    G::ClearHydrologyCache();
    AddInfo(FString::Printf(TEXT("Shared relief fixed sites=%d seedDifferences=%d"), UE_ARRAY_COUNT(Positions), SeedDifferences));
    return true;
}

// Search actual spline centers rather than a coarse lattice that can miss a channel.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaHighGroundRiverCenterTest,
    "Kalmala.World.Regional.HighGroundRiverCenter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaHighGroundRiverCenterTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    using T = FKalmalaRegionalTuning;
    const FKalmalaWorldGenerationConfig Config{418ull};
    constexpr int32 SearchRadius = 24;
    for (int32 Y = -SearchRadius; Y <= SearchRadius; ++Y)
    for (int32 X = -SearchRadius; X <= SearchRadius; ++X)
    for (const auto& Segment : G::GetHydrology(Config, FIntPoint(X, Y)))
    {
        if (Segment.bStream) continue;
        const FVector2D Position((Segment.A + Segment.B) * 0.5);
        const auto Fields = FKalmalaWorldFieldSampler::Sample(Config, Position);
        const float SourceHeight = (Fields.Elevation - T::SeaElevation) * 2000.0f;
        if (SourceHeight < 400.0f) continue;
        const auto Sample = G::Sample(Fields);
        // These biome weights are computed before hydrology. Zero lake/mire
        // eligibility excludes all basin support, including its outer rim,
        // without selecting the fixture by its resulting water level or bed.
        if (Sample.Weights[1] + Sample.Weights[3] > 0.0f) continue;

        TestTrue(TEXT("Fixture source terrain is at least four metres above sea"), SourceHeight >= 400.0f);
        TestTrue(TEXT("Generated segment midpoint has full river influence"), Sample.RiverWeight > 0.999f);
        TestEqual(TEXT("Fixture has no lake basin influence"), Sample.BasinWeight, 0.0f);
        TestEqual(TEXT("High-ground river water stays at Z=0"), Sample.WaterLevel, 0.0f);
        TestTrue(TEXT("High-ground river bed is below its water plane"), Sample.Height < Sample.WaterLevel);
        TestTrue(TEXT("Land clamp cannot refill the configured river bed"),
            Sample.Height <= -float(T::RiverBedDepth) + 0.01f);
        TestTrue(TEXT("High-ground river center reports water"), Sample.bHasWater);
        AddInfo(FString::Printf(TEXT("High-ground river seed=418 position=(%.3f,%.3f) source=%.3f bed=%.3f water=%.3f"),
            Position.X, Position.Y, SourceHeight, Sample.Height, Sample.WaterLevel));
        return true;
    }
    AddError(TEXT("No basin-free high-ground river center found in the bounded seed-418 search"));
    return false;
}

// Compare the real sampler against the same land shaping with courses omitted.
// Eligibility is decided before assertions, never by the observed carved height.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRiverBankCrossSectionTest,
    "Kalmala.World.Regional.RiverBankCrossSection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaRiverBankCrossSectionTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    using T = FKalmalaRegionalTuning;
    const FKalmalaWorldGenerationConfig Config{418ull};
    struct FPoint { FVector2D Position; FKalmalaRegionalSample Base; double Influence; };
    constexpr int32 Steps = 64;
    for (int32 Y = -24; Y <= 24; ++Y)
    for (int32 X = -24; X <= 24; ++X)
    for (const auto& Segment : G::GetHydrology(Config, FIntPoint(X, Y)))
    {
        if (Segment.bStream) continue;
        const FVector2D Center((Segment.A + Segment.B) * 0.5);
        const auto CenterBase = G::SampleInternal(FKalmalaWorldFieldSampler::Sample(Config, Center), false, true);
        if (CenterBase.Height < 400 || CenterBase.Weights[1] + CenterBase.Weights[3] > 0) continue;
        const FVector2D Direction = FVector2D(Segment.B - Segment.A).GetSafeNormal();
        if (Direction.IsNearlyZero()) continue;
        const FVector2D Normal(-Direction.Y, Direction.X);
        const double Radius = Segment.Width + T::RiverBankExtent;
        TArray<FPoint> Points;
        bool bEligible = true;
        // Include the entire shoulder plus a substantial unchanged exterior.
        for (int32 I = -Steps; I <= Steps; ++I)
        {
            const FVector2D P = Center + Normal * (1.5 * Radius * I / Steps);
            const auto Fields = FKalmalaWorldFieldSampler::Sample(Config, P);
            const auto Base = G::SampleInternal(Fields, false, true);
            if (Base.Weights[1] + Base.Weights[3] > 0 || Base.Height < 400)
            { bEligible = false; break; }
            double Influence = 0;
            const FIntPoint Cell(FMath::FloorToInt(P.X / T::GridCell), FMath::FloorToInt(P.Y / T::GridCell));
            for (const auto& S : G::GetHydrology(Config, Cell))
            {
                if (S.bStream) continue;
                const FVector2D A(S.A), Delta = FVector2D(S.B) - A;
                const double U = FMath::Clamp(FVector2D::DotProduct(P - A, Delta) / Delta.SizeSquared(), 0.0, 1.0);
                const double Distance = (P - A - Delta * U).Size();
                const double Taper = FMath::Clamp((Distance - S.Width * 0.2)
                    / (S.Width + T::RiverBankExtent - S.Width * 0.2), 0.0, 1.0);
                Influence = FMath::Max(Influence, 1 - Taper * Taper * (3 - 2 * Taper));
            }
            Points.Add({P, Base, Influence});
        }
        if (!bEligible || Points.Num() != 2 * Steps + 1) continue;
        // Avoid neighboring bends/courses that hide the outer edge of either bank.
        if (Points[0].Influence != 0 || Points.Last().Influence != 0) continue;
        bool bMonotone = true;
        for (int32 I = 1; I <= Steps; ++I)
            bMonotone &= Points[Steps - I].Influence <= Points[Steps - I + 1].Influence
                && Points[Steps + I].Influence <= Points[Steps + I - 1].Influence;
        if (!bMonotone) continue;

        int32 Shoulder[2] = {}, Exterior[2] = {};
        for (int32 I = 0; I < Points.Num(); ++I)
        {
            const auto& P = Points[I];
            const auto Actual = G::Sample(Config, P.Position);
            const float Expected = FMath::Lerp(P.Base.Height, -float(T::RiverBedDepth), float(P.Influence));
            TestTrue(TEXT("Both banks follow the continuous cubic shoulder"),
                FMath::IsNearlyEqual(Actual.Height, Expected, 0.02f));
            const int32 Side = I < Steps ? 0 : 1;
            if (P.Influence > 0 && P.Influence < 1) ++Shoulder[Side];
            if (P.Influence == 0)
            {
                ++Exterior[Side];
                TestEqual(TEXT("Beyond either bank terrain is exactly unchanged"), Actual.Height, P.Base.Height);
                TestEqual(TEXT("Beyond either bank water output is unchanged"), Actual.WaterLevel, P.Base.WaterLevel);
                TestEqual(TEXT("Beyond either bank water presence is unchanged"), Actual.bHasWater, P.Base.bHasWater);
            }
            // Tiny physical steps test continuity throughout each bank and its return.
            for (const double Sign : {-1.0, 1.0})
            {
                const auto Nearby = G::Sample(Config, P.Position + Normal * (Sign * 0.01));
                TestTrue(TEXT("Bank and exterior have no height step"), FMath::Abs(Nearby.Height - Actual.Height) < 0.1f);
            }
        }
        for (int32 Side = 0; Side < 2; ++Side)
        {
            TestTrue(TEXT("Each bank includes multiple shoulder samples"), Shoulder[Side] > 8);
            TestTrue(TEXT("Each bank includes multiple unchanged exterior samples"), Exterior[Side] > 4);
        }
        AddInfo(FString::Printf(TEXT("River cross-section seed=418 center=(%.3f,%.3f) radius=%.3f samples=%d"),
            Center.X, Center.Y, Radius, Points.Num()));
        return true;
    }
    AddError(TEXT("No high-ground basin-free two-bank cross-section found in bounded seed-418 search"));
    return false;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRiverSeedRepeatabilityTest,
    "Kalmala.World.Regional.RiverSeedRepeatability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaRiverSeedRepeatabilityTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    using T = FKalmalaRegionalTuning;
    const FKalmalaWorldGenerationConfig Config{418ull}, OtherConfig{419ull};
    TArray<FVector2D> Positions;
    TSet<uint64> Courses;
    G::ClearHydrologyCache();
    // Use actual distinct courses so narrow rivers cannot fall between grid probes.
    for (int32 Y = -24; Y <= 24 && Courses.Num() < 8; ++Y)
    for (int32 X = -24; X <= 24 && Courses.Num() < 8; ++X)
    for (const auto& Segment : G::GetHydrology(Config, FIntPoint(X, Y)))
    {
        if (Segment.bStream || Courses.Contains(Segment.Id)) continue;
        const FVector2D Direction = FVector2D(Segment.B - Segment.A).GetSafeNormal();
        if (Direction.IsNearlyZero()) continue;
        const FVector2D Center((Segment.A + Segment.B) * 0.5);
        if (G::Sample(Config, Center).RiverWeight < 0.999f) continue;
        Courses.Add(Segment.Id);
        const FVector2D Normal(-Direction.Y, Direction.X);
        for (const double Offset : {-1.5, -0.5, 0.0, 0.5, 1.5})
            Positions.Add(Center + Normal * ((Segment.Width + T::RiverBankExtent) * Offset));
        if (Courses.Num() == 8) break;
    }
    if (!TestEqual(TEXT("Bounded search supplies eight distinct river courses"), Courses.Num(), 8)) return false;

    TArray<FKalmalaRegionalSample> Baseline;
    for (const FVector2D& P : Positions) Baseline.Add(G::Sample(Config, P));
    const auto CheckRepeat = [this](const FKalmalaRegionalSample& A, const FKalmalaRegionalSample& B)
    {
        TestEqual(TEXT("Same seed reproduces exact river height"), B.Height, A.Height);
        TestEqual(TEXT("Same seed reproduces exact water level"), B.WaterLevel, A.WaterLevel);
        TestEqual(TEXT("Same seed reproduces water presence"), B.bHasWater, A.bHasWater);
        TestEqual(TEXT("Same seed reproduces river influence"), B.RiverWeight, A.RiverWeight);
    };
    for (int32 I = 0; I < Positions.Num(); ++I)
        CheckRepeat(Baseline[I], G::Sample(Config, Positions[I]));

    G::ClearHydrologyCache();
    int32 HeightChanges = 0, RiverChanges = 0;
    // Reverse traversal after cache disposal and interleave a different seed.
    // Compare identical world positions, not independently relocated fixtures.
    for (int32 I = Positions.Num() - 1; I >= 0; --I)
    {
        const auto Other = G::Sample(OtherConfig, Positions[I]);
        HeightChanges += FMath::Abs(Other.Height - Baseline[I].Height) > 0.01f;
        RiverChanges += FMath::Abs(Other.RiverWeight - Baseline[I].RiverWeight) > 0.001f;
        CheckRepeat(Baseline[I], G::Sample(Config, Positions[I]));
    }
    TestTrue(TEXT("Different seed changes river-area terrain at fixed positions"), HeightChanges > 0);
    TestTrue(TEXT("Different seed moves river influence at fixed positions"), RiverChanges > 0);
    // Sea-level water is deliberately invariant; seed variation must not require
    // its plane to move. Height, wet/dry presence and water level all repeat above.
    AddInfo(FString::Printf(TEXT("River repeatability seeds=418/419 courses=%d samples=%d heightChanges=%d riverChanges=%d"),
        Courses.Num(), Positions.Num(), HeightChanges, RiverChanges));
    G::ClearHydrologyCache();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRiverUnchangedControlsTest,
    "Kalmala.World.Regional.RiverUnchangedControls",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaRiverUnchangedControlsTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    using T = FKalmalaRegionalTuning;
    TestFalse(TEXT("Compiled analytic-basin policy defaults disabled"), T::bEnableAnalyticBasins);
    for (const uint64 Seed : {418ull, 419ull})
    {
        const FKalmalaWorldGenerationConfig Config{Seed};
        int32 Land = 0, Sea = 0, CoastLand = 0, CoastSea = 0, WetlandControls = 0;
        // A bounded 50 m lattice covers both sides of the source coast and the
        // lake-eligible ring. Fixture selection never examines carved outputs.
        for (int32 Y = -120; Y <= 120; ++Y)
        for (int32 X = -120; X <= 120; ++X)
        {
            if (Land >= 8 && Sea >= 8 && CoastLand >= 8 && CoastSea >= 8 && WetlandControls >= 8) break;
            const FVector2D P(X * 5000.0, Y * 5000.0);
            const auto Fields = FKalmalaWorldFieldSampler::Sample(Config, P);
            const auto Base = G::SampleInternal(Fields, false, true);
            const float Source = (Fields.Elevation - T::SeaElevation) * 2000.0f;
            bool bOutside = true;
            const FIntPoint Cell(FMath::FloorToInt(P.X / T::GridCell), FMath::FloorToInt(P.Y / T::GridCell));
            for (const auto& S : G::GetHydrology(Config, Cell))
            {
                const FVector2D A(S.A), Delta = FVector2D(S.B) - A;
                const double U = FMath::Clamp(FVector2D::DotProduct(P - A, Delta) / Delta.SizeSquared(), 0.0, 1.0);
                if ((P - A - Delta * U).Size() <= S.Width + (S.bStream ? 0.0 : T::RiverBankExtent))
                    bOutside = false;
            }
            // Sample basin-eligible land outside courses to prove that wetland
            // labels remain while analytic water and positive basin support do not.
            if (WetlandControls < 8 && bOutside && Fields.Elevation > T::SeaElevation
                && Base.Weights[1] + Base.Weights[3] > 0.0f)
            {
                TestEqual(TEXT("Analytic basin weight is zero on eligible wetland"), Base.BasinWeight, 0.0f);
                TestFalse(TEXT("Eligible wetland without courses has no analytic water depth"), Base.bHasWater);
                TestFalse(TEXT("Eligible wetland has no assigned inland water level"), Base.WaterLevel > 0.0f);
                const auto Actual = G::Sample(Config, P);
                TestEqual(TEXT("Regional output has no analytic basin support"), Actual.BasinWeight, 0.0f);
                TestEqual(TEXT("Eligible wetland has no analytic bowl height change"), Actual.Height, Base.Height);
                TestEqual(TEXT("Eligible wetland has no analytic basin water level"), Actual.WaterLevel, Base.WaterLevel);
                TestEqual(TEXT("Disabling bowls leaves biome identity unchanged"), Actual.Biome, G::SampleBiome(Fields));
                ++WetlandControls;
            }
            if (!bOutside || Base.BasinWeight > 0) continue;
            int32& Count = FMath::Abs(Source) <= 20.0f
                ? (Source > 0 ? CoastLand : CoastSea) : (Source > 0 ? Land : Sea);
            if (Count >= 8) continue;
            const auto Actual = G::Sample(Config, P);
            TestEqual(TEXT("Outside course envelopes terrain is exactly unchanged"), Actual.Height, Base.Height);
            TestEqual(TEXT("Outside course envelopes water level is exactly unchanged"), Actual.WaterLevel, Base.WaterLevel);
            TestEqual(TEXT("Outside course envelopes wet state is unchanged"), Actual.bHasWater, Base.bHasWater);
            TestEqual(TEXT("Source coast retains its land/sea terrain sign"), Actual.Height > 0, Source > 0);
            TestEqual(TEXT("Source coast retains its biome identity"), Actual.Biome, Base.Biome);
            for (int32 I = 0; I < 7; ++I)
                TestEqual(TEXT("Control biome weights remain unchanged"), Actual.Weights[I], Base.Weights[I]);
            ++Count;
        }
        TestEqual(TEXT("Away-from-river land controls exist"), Land, 8);
        TestEqual(TEXT("Away-from-river sea controls exist"), Sea, 8);
        TestEqual(TEXT("Source coast has eight land-side controls within 20 cm"), CoastLand, 8);
        TestEqual(TEXT("Source coast has eight sea-side controls within 20 cm"), CoastSea, 8);
        TestEqual(TEXT("Eight eligible dry-wetland controls exist"), WetlandControls, 8);
        AddInfo(FString::Printf(TEXT("River controls seed=%llu land=%d sea=%d coastLand=%d coastSea=%d wetlandControls=%d"),
            Seed, Land, Sea, CoastLand, CoastSea, WetlandControls));
    }
    G::ClearHydrologyCache();
    return true;
}

// Locate real course/grid intersections; arbitrary seam probes can miss rivers.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRiverHydrologyGridSeamTest,
    "Kalmala.World.Regional.RiverHydrologyGridSeam",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaRiverHydrologyGridSeamTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    using T = FKalmalaRegionalTuning;
    for (const uint64 Seed : {418ull, 419ull})
    {
        const FKalmalaWorldGenerationConfig Config{Seed};
        bool Found[2] = {};
        for (int32 Y = -24; Y <= 24 && !(Found[0] && Found[1]); ++Y)
        for (int32 X = -24; X <= 24 && !(Found[0] && Found[1]); ++X)
        for (const auto& Segment : G::GetHydrology(Config, FIntPoint(X, Y)))
        {
            if (Segment.bStream) continue;
            const FVector2D A(Segment.A), Delta = FVector2D(Segment.B) - A;
            for (int32 Axis = 0; Axis < 2; ++Axis)
            {
                if (Found[Axis]) continue;
                const double Start = Axis == 0 ? A.X : A.Y;
                const double Span = Axis == 0 ? Delta.X : Delta.Y;
                if (FMath::Abs(Span) < 0.001) continue;
                const double Boundary = (FMath::FloorToInt(FMath::Min(Start, Start + Span) / T::GridCell) + 1) * T::GridCell;
                const double U = (Boundary - Start) / Span;
                if (U <= 0 || U >= 1) continue;
                FVector2D Center = A + Delta * U;
                if (Axis == 0) Center.X = Boundary; else Center.Y = Boundary;
                const FVector2D Normal = Axis == 0 ? FVector2D(1, 0) : FVector2D(0, 1);
                // Pre-hydrology biome eligibility excludes lake support without
                // filtering on the expected channel height or water results.
                bool bEligible = true;
                for (const double Offset : {-0.1, 0.0, 0.1})
                {
                    const auto R = G::Sample(Config, Center + Normal * Offset);
                    if (R.Weights[1] + R.Weights[3] > 0) bEligible = false;
                }
                if (!bEligible) continue;
                const auto CellAt = [](FVector2D P)
                { return FIntPoint(FMath::FloorToInt(P.X / T::GridCell), FMath::FloorToInt(P.Y / T::GridCell)); };
                TestTrue(TEXT("River fixture straddles distinct hydrology cells"),
                    CellAt(Center - Normal * 0.01) != CellAt(Center + Normal * 0.01));
                const auto Middle = G::Sample(Config, Center);
                TestTrue(TEXT("Grid seam intersects the river center"), Middle.RiverWeight > 0.999f);
                TestEqual(TEXT("Seam center water is sea level"), Middle.WaterLevel, 0.0f);
                TestTrue(TEXT("Seam center is submerged"), Middle.bHasWater && Middle.Height < Middle.WaterLevel);
                for (const double Epsilon : {0.01, 0.1})
                {
                    G::ClearHydrologyCache();
                    const auto Before = G::Sample(Config, Center - Normal * Epsilon);
                    const auto After = G::Sample(Config, Center + Normal * Epsilon);
                    TestTrue(TEXT("Channel height is continuous across hydrology grid"), FMath::Abs(Before.Height - After.Height) < 0.5f);
                    TestEqual(TEXT("Channel water level agrees across hydrology grid"), Before.WaterLevel, After.WaterLevel);
                    TestEqual(TEXT("Both sides retain sea-level water"), Before.WaterLevel, 0.0f);
                    TestTrue(TEXT("Both sides retain submerged channel water"), Before.bHasWater && After.bHasWater
                        && Before.Height < Before.WaterLevel && After.Height < After.WaterLevel);
                    TestTrue(TEXT("River signal is continuous across hydrology grid"), FMath::Abs(Before.RiverWeight - After.RiverWeight) < 0.001f);
                    G::ClearHydrologyCache();
                    const auto ReverseAfter = G::Sample(Config, Center + Normal * Epsilon);
                    const auto ReverseBefore = G::Sample(Config, Center - Normal * Epsilon);
                    TestEqual(TEXT("Reverse query order reproduces before height"), ReverseBefore.Height, Before.Height);
                    TestEqual(TEXT("Reverse query order reproduces after height"), ReverseAfter.Height, After.Height);
                    TestEqual(TEXT("Reverse query order reproduces before water"), ReverseBefore.WaterLevel, Before.WaterLevel);
                    TestEqual(TEXT("Reverse query order reproduces after water"), ReverseAfter.WaterLevel, After.WaterLevel);
                    TestEqual(TEXT("Reverse query order reproduces before wet flag"), ReverseBefore.bHasWater, Before.bHasWater);
                    TestEqual(TEXT("Reverse query order reproduces after wet flag"), ReverseAfter.bHasWater, After.bHasWater);
                }
                Found[Axis] = true;
                AddInfo(FString::Printf(TEXT("River grid seam seed=%llu axis=%d position=(%.3f,%.3f)"), Seed, Axis, Center.X, Center.Y));
            }
        }
        TestTrue(TEXT("Bounded search finds an X-grid river crossing"), Found[0]);
        TestTrue(TEXT("Bounded search finds a Y-grid river crossing"), Found[1]);
    }
    G::ClearHydrologyCache();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRiverTerrainPatchSeamTest,
    "Kalmala.World.Regional.RiverTerrainPatchSeam",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaRiverTerrainPatchSeamTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    using T = FKalmalaRegionalTuning;
    using L = FKalmalaTerrainPatchLayout;
    constexpr double Size = L::PatchSize;
    constexpr double Half = Size * 0.5;
    constexpr double Step = Size / FKalmalaWaterSurfaceMesh::CellsPerSide;
    for (const uint64 Seed : {418ull, 419ull})
    {
        const FKalmalaWorldGenerationConfig Config{Seed};
        bool Found[2] = {};
        for (int32 Y = -24; Y <= 24 && !(Found[0] && Found[1]); ++Y)
        for (int32 X = -24; X <= 24 && !(Found[0] && Found[1]); ++X)
        for (const auto& Segment : G::GetHydrology(Config, FIntPoint(X, Y)))
        {
            if (Segment.bStream) continue;
            const FVector2D A(Segment.A), Delta = FVector2D(Segment.B) - A;
            for (int32 Axis = 0; Axis < 2; ++Axis)
            {
                if (Found[Axis]) continue;
                const double Start = Axis == 0 ? A.X : A.Y;
                const double Span = Axis == 0 ? Delta.X : Delta.Y;
                if (FMath::Abs(Span) < 0.001) continue;
                // Zero-origin patches have edges at n*Size + Half.
                const double Boundary = (FMath::FloorToInt((FMath::Min(Start, Start + Span) - Half) / Size) + 1) * Size + Half;
                const double U = (Boundary - Start) / Span;
                if (U <= 0 || U >= 1) continue;
                FVector2D Center = A + Delta * U;
                if (Axis == 0) Center.X = Boundary; else Center.Y = Boundary;
                const FVector2D Normal = Axis == 0 ? FVector2D(1, 0) : FVector2D(0, 1);
                const FIntPoint Key = L::GetPatchCoordinate(FVector2D::ZeroVector, Center - Normal * 0.1);
                const FVector2D LeftCenter = L::GetPatchCenter(FVector2D::ZeroVector, Key.X, Key.Y);
                const FVector2D RightCenter = LeftCenter + Normal * Size;
                // Exclude basin support at the two seam lattice endpoints and
                // their neighboring columns, without filtering carved outputs.
                const double Along = Axis == 0 ? Center.Y : Center.X;
                const double AlongStart = FMath::FloorToDouble((Along + Half) / Step) * Step - Half;
                bool bEligible = true;
                for (const double Across : {-Step, 0.0, Step})
                for (const double Tangent : {AlongStart, AlongStart + Step})
                {
                    const FVector2D P = Axis == 0 ? FVector2D(Boundary + Across, Tangent) : FVector2D(Tangent, Boundary + Across);
                    const auto R = G::Sample(Config, P);
                    if (R.Weights[1] + R.Weights[3] > 0) bEligible = false;
                }
                if (!bEligible) continue;
                TestTrue(TEXT("Fixture crosses distinct terrain patches"), Key != L::GetPatchCoordinate(FVector2D::ZeroVector, Center + Normal * 0.1));
                const auto Before = G::Sample(Config, Center - Normal * 0.01);
                const auto After = G::Sample(Config, Center + Normal * 0.01);
                TestTrue(TEXT("Channel height is continuous across patch seam"), FMath::Abs(Before.Height - After.Height) < 0.5f);
                TestTrue(TEXT("Both patch sides retain submerged sea-level channel"), Before.Height < 0 && After.Height < 0
                    && Before.bHasWater && After.bHasWater && Before.WaterLevel == 0 && After.WaterLevel == 0);
                // Explicit origins exercise the actual collision triangle lattice
                // from both adjacent patches, including queries on either side.
                for (const double Offset : {-0.01, 0.0, 0.01})
                {
                    const FVector2D P = Center + Normal * Offset;
                    const auto Left = FKalmalaOceanSampler::Sample(Config, P, LeftCenter - FVector2D(Half, Half));
                    const auto Right = FKalmalaOceanSampler::Sample(Config, P, RightCenter - FVector2D(Half, Half));
                    TestTrue(TEXT("Both patch triangle samples are valid and wet"), Left.IsWater() && Right.IsWater());
                    TestEqual(TEXT("Adjacent patch origins reproduce channel triangle height"), Left.TerrainHeight, Right.TerrainHeight);
                    TestEqual(TEXT("Adjacent patch origins reproduce sea depth"), Left.WaterDepth, Right.WaterDepth);
                }
                const auto Left = FKalmalaWaterSurfaceMesh::BuildPatch(Config, LeftCenter, false);
                const auto Right = FKalmalaWaterSurfaceMesh::BuildPatch(Config, RightCenter, false);
                const auto Edge = [&](const FKalmalaWaterMesh& Mesh, FVector2D Origin)
                {
                    TArray<FVector> Result;
                    for (const FVector& V : Mesh.Vertices)
                    {
                        const FVector World = V + FVector(Origin, 0);
                        const double Across = Axis == 0 ? World.X : World.Y;
                        const double Tangent = Axis == 0 ? World.Y : World.X;
                        if (FMath::Abs(Across - Boundary) < 0.001 && Tangent >= AlongStart - 0.001 && Tangent <= AlongStart + Step + 0.001)
                            Result.AddUnique(World);
                    }
                    return Result;
                };
                const auto LeftEdge = Edge(Left, LeftCenter), RightEdge = Edge(Right, RightCenter);
                TestTrue(TEXT("River crossing supplies nonempty clipped water edges on both patches"), LeftEdge.Num() >= 2 && RightEdge.Num() >= 2);
                TestEqual(TEXT("Shared river edge vertex counts match"), LeftEdge.Num(), RightEdge.Num());
                for (const FVector& V : LeftEdge)
                {
                    TestEqual(TEXT("Clipped river patch edge stays at sea level"), V.Z, 0.0);
                    TestTrue(TEXT("Adjacent river water edges have identical world intersections"), RightEdge.ContainsByPredicate(
                        [&V](const FVector& R) { return V.Equals(R, 0.001); }));
                }
                Found[Axis] = true;
                AddInfo(FString::Printf(TEXT("River patch seam seed=%llu axis=%d position=(%.3f,%.3f) edgeVertices=%d"),
                    Seed, Axis, Center.X, Center.Y, LeftEdge.Num()));
            }
        }
        TestTrue(TEXT("Bounded search finds an X-patch river crossing"), Found[0]);
        TestTrue(TEXT("Bounded search finds a Y-patch river crossing"), Found[1]);
    }
    G::ClearHydrologyCache();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRegionalGenerationTest, "Kalmala.World.Regional.Integrated",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaRegionalGenerationTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    constexpr int32 Side = 161;
    constexpr double Step = 10000;
    TArray<uint8> Previous;
    for (uint64 Seed : {418ull, 419ull})
    {
        FKalmalaWorldGenerationConfig C{Seed};
        TArray<uint8> Biomes;
        int32 Counts[7] = {}, RiverCount = 0, StreamCount = 0, Edges = 0, Different = 0;
        double MaxJump = 0, MaxWeightJump = 0;
        int32 PositiveBasinWeights = 0;
        FVector2D RiverFixture = FVector2D::ZeroVector;
        for (int32 Y = 0; Y < Side; ++Y) for (int32 X = 0; X < Side; ++X)
        {
            const FVector2D P((X - Side / 2) * Step, (Y - Side / 2) * Step);
            auto Fields = FKalmalaWorldFieldSampler::Sample(C, P);
            const auto R = G::Sample(Fields);
            Biomes.Add(R.Biome);
            ++Counts[R.Biome];
            RiverCount += R.RiverWeight > 0;
            if (R.RiverWeight > 0) RiverFixture = P;
            StreamCount += R.StreamWeight > 0;
            PositiveBasinWeights += R.BasinWeight > 0.0f;
            if (!Previous.IsEmpty()) Different += Previous[Biomes.Num() - 1] != R.Biome;
            if (X > 0) Edges += R.Biome != Biomes[Biomes.Num() - 2];
            if (Y > 0) Edges += R.Biome != Biomes[Biomes.Num() - 1 - Side];
            float Total = 0;
            for (float W : R.Weights) { TestTrue(TEXT("Finite normalized biome weight"), FMath::IsFinite(W) && W >= 0 && W <= 1); Total += W; }
            TestTrue(TEXT("Weights sum to one"), FMath::IsNearlyEqual(Total, 1.0f, 0.0001f));
            if (Fields.Elevation < 0.22f) TestEqual(TEXT("Source sea floor stays ocean"), R.Biome, uint8(6));
            if (X % 8 == 0 && Y % 8 == 0)
            {
                Fields.Flora = 0;
                const auto Clearing = G::Sample(Fields);
                Fields.Flora = 1;
                const auto Dense = G::Sample(Fields);
                TestEqual(TEXT("Local Flora cannot fragment any regional biome"), Clearing.Biome, Dense.Biome);
                TestEqual(TEXT("Local Flora does not reshape terrain"), Clearing.Height, Dense.Height);
                const auto East = G::Sample(C, P + FVector2D(0.1, 0));
                MaxJump = FMath::Max(MaxJump, double(FMath::Abs(East.Height - R.Height)));
                for (int32 I = 0; I < 7; ++I) MaxWeightJump = FMath::Max(MaxWeightJump, double(FMath::Abs(East.Weights[I] - R.Weights[I])));
                TestEqual(TEXT("Same identity reproduces shaped height"), G::Sample(C, P).Height, R.Height);
            }
        }
        for (int32 I = 0; I < 7; ++I) TestTrue(FString::Printf(TEXT("Seed %llu contains biome %d"), Seed, I), Counts[I] > 5);
        TestTrue(TEXT("Large area contains rivers and no small streams"), RiverCount > 10 && StreamCount == 0);
        TestEqual(TEXT("Integrated samples contain no analytic basin weights"), PositiveBasinWeights, 0);
        TestTrue(TEXT("Sub-centimetre terrain samples are continuous"), MaxJump < 2);
        TestTrue(TEXT("Biome blends are continuous"), MaxWeightJump < 0.005);
        const double BoundaryDensity = double(Edges) / (2 * Side * (Side - 1));
        TestTrue(TEXT("Biome boundaries do not return to confetti"), BoundaryDensity < 0.18);
        TArray<bool> Visited; Visited.Init(false, Biomes.Num());
        TArray<int32> Areas;
        int32 Small = 0;
        for (int32 Start = 0; Start < Biomes.Num(); ++Start)
        {
            if (Visited[Start]) continue;
            TArray<int32> Queue{Start}; Visited[Start] = true;
            for (int32 Q = 0; Q < Queue.Num(); ++Q)
            {
                const int32 Index = Queue[Q], X = Index % Side, Y = Index / Side;
                const int32 LocalNeighbours[] = { X > 0 ? Index - 1 : -1, X + 1 < Side ? Index + 1 : -1, Y > 0 ? Index - Side : -1, Y + 1 < Side ? Index + Side : -1 };
                for (int32 N : LocalNeighbours) if (N >= 0 && !Visited[N] && Biomes[N] == Biomes[Start]) { Visited[N] = true; Queue.Add(N); }
            }
            Areas.Add(Queue.Num());
            Small += Queue.Num() <= 2;
        }
        Areas.Sort();
        TestTrue(TEXT("At most two percent of broad-world samples form tiny components"), Small < Biomes.Num() / 50);
        AddInfo(FString::Printf(TEXT("Regional seed=%llu coverage=%d,%d,%d,%d,%d,%d,%d boundaries=%.4f components=%d medianCells=%d tiny=%d rivers=%d streams=%d maxHeightJump=%.5f"),
            Seed, Counts[0], Counts[1], Counts[2], Counts[3], Counts[4], Counts[5], Counts[6], BoundaryDensity, Areas.Num(), Areas[Areas.Num()/2], Small, RiverCount, StreamCount, MaxJump));
        if (!Previous.IsEmpty()) TestTrue(TEXT("Different seeds meaningfully move regions"), Different > Biomes.Num() / 4);
        Previous = Biomes;

        // The spatial index must return the same curves regardless of cache/query order.
        const FIntPoint Index(int32(FMath::FloorToInt(RiverFixture.X / FKalmalaRegionalTuning::GridCell)), int32(FMath::FloorToInt(RiverFixture.Y / FKalmalaRegionalTuning::GridCell)));
        const auto A = G::GetHydrology(C, Index);
        TestTrue(TEXT("Spline index comparison is nonempty"), !A.IsEmpty());
        G::ClearHydrologyCache();
        G::GetHydrology(C, Index + FIntPoint(1, 0));
        const auto B = G::GetHydrology(C, Index);
        TestEqual(TEXT("Spline rebuild count"), A.Num(), B.Num());
        for (int32 I = 0; I < FMath::Min(A.Num(), B.Num()); ++I)
        {
            TestEqual(TEXT("Spline identity"), A[I].Id, B[I].Id);
            TestEqual(TEXT("Spline start"), A[I].A, B[I].A);
            TestEqual(TEXT("Spline end"), A[I].B, B[I].B);
            TestTrue(TEXT("No broken spline step"), FVector::Dist(A[I].A, A[I].B) < 500);
        }
        for (int32 I = -4; I < 4; ++I)
        {
            const FVector2D P(I * FKalmalaRegionalTuning::GridCell, 7000);
            const auto West = G::Sample(C, P - FVector2D(0.01, 0));
            const auto East = G::Sample(C, P + FVector2D(0.01, 0));
            TestTrue(TEXT("Hydrology GridCell seam is continuous"), FMath::Abs(West.Height - East.Height) < 0.5);
        }
        // Verify the depth query uses both shaped collision-triangle planes.
        for (const FVector2D Origin : { FVector2D(-3000, -6000), FVector2D(3000, 6000) })
        for (const bool Upper : { false, true })
        {
            const FVector2D P = Origin + FVector2D(Upper ? 250.0 / 3 : 125.0 / 3);
            const float H = (G::Sample(C, Origin + FVector2D(125, 0)).Height
                + G::Sample(C, Origin + FVector2D(0, 125)).Height
                + G::Sample(C, Origin + (Upper ? FVector2D(125) : FVector2D::ZeroVector)).Height) / 3;
            const auto Ocean = FKalmalaOceanSampler::Sample(C, P, Origin);
            TestTrue(TEXT("Depth query uses shaped collision triangles"), FMath::IsNearlyEqual(Ocean.TerrainHeight, H, 0.01f));
            TestEqual(TEXT("Equivalent patch origins preserve shaped depth"), Ocean.TerrainHeight, FKalmalaOceanSampler::Sample(C, P, Origin + FVector2D(3000, 0)).TerrainHeight);
        }
    }
    return true;
}
#endif
