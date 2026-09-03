#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaBiomeClassifier.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldPlayerStartResolver.h"
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

#endif
