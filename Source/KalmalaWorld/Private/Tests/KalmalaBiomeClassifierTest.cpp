#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaBiomeClassifier.h"
#include "KalmalaWorldPlayerStartResolver.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaBiomeClassifierTest, "Kalmala.World.Biomes.TerrainSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaBiomeClassifierTest::RunTest(const FString& Parameters)
{
    struct FCase { const TCHAR* Name; FKalmalaWorldFieldSample Fields; EKalmalaBiome Expected; };
    const FCase Cases[] = {
        { TEXT("Submerged cold dense growth is ocean"), { 0.219f, 1, 0, 1 }, EKalmalaBiome::Ocean },
        { TEXT("Exact sea level is land"), { 0.22f, 0.5f, 0.5f, 0.5f }, EKalmalaBiome::Meadows },
        { TEXT("High peaks override wet cold forest"), { 0.781f, 1, 0, 1 }, EKalmalaBiome::ThunderMountains },
        { TEXT("Exact mountain boundary remains upland"), { 0.78f, 0.5f, 0.2f, 1 }, EKalmalaBiome::FreezingTundra },
        { TEXT("Cold upland overrides forest"), { 0.55f, 0.5f, 0.349f, 1 }, EKalmalaBiome::FreezingTundra },
        { TEXT("Mild upland supports forest"), { 0.55f, 0.5f, 0.35f, 1 }, EKalmalaBiome::Elderwood },
        { TEXT("Cold lowland is not tundra"), { 0.3f, 0.5f, 0.1f, 0.5f }, EKalmalaBiome::Meadows },
        { TEXT("Wet temperate lowland is mire"), { 0.3f, 0.8f, 0.28f, 1 }, EKalmalaBiome::MossyMire },
        { TEXT("Cold wet lowland supports lakes"), { 0.3f, 0.8f, 0.279f, 1 }, EKalmalaBiome::ShimmeringLakes },
        { TEXT("Mire upper edge becomes lake country"), { 0.45f, 0.8f, 0.5f, 1 }, EKalmalaBiome::ShimmeringLakes },
        { TEXT("Lake country stops at uplands"), { 0.55f, 0.8f, 0.5f, 1 }, EKalmalaBiome::Elderwood },
        { TEXT("Dense growth on dry ground stays open"), { 0.5f, 0.349f, 0.5f, 1 }, EKalmalaBiome::Meadows },
        { TEXT("Moisture supports dense forest"), { 0.5f, 0.35f, 0.5f, 1 }, EKalmalaBiome::Elderwood },
        { TEXT("Sparse growth stays meadow"), { 0.5f, 0.5f, 0.5f, 0.64f }, EKalmalaBiome::Meadows }
    };
    for (const FCase& Case : Cases) TestEqual(Case.Name, FKalmalaBiomeClassifier::Classify(Case.Fields), Case.Expected);

    FKalmalaWorldFieldSample LegacyCold{ 0.3f, 0.5f, 0.1f, 0.5f, 1 };
    FKalmalaWorldFieldSample LegacyDryForest{ 0.5f, 0.2f, 0.5f, 1, 1 };
    TestEqual(TEXT("Revision one preserves lowland tundra"), FKalmalaBiomeClassifier::Classify(LegacyCold), EKalmalaBiome::FreezingTundra);
    TestEqual(TEXT("Revision one preserves dry forest"), FKalmalaBiomeClassifier::Classify(LegacyDryForest), EKalmalaBiome::Elderwood);

    for (int32 Revision = 1; Revision <= 2; ++Revision)
    {
        const FKalmalaWorldGenerationConfig Config{ 418ull, Revision };
        const FKalmalaWorldGenerationConfig OtherSeed{ 419ull, Revision };
        bool Seen[7] = {};
        bool bDifferentSeed = false;
        for (int32 Y = -24000; Y <= 24000; Y += 500) for (int32 X = -24000; X <= 24000; X += 500)
        {
            const FVector2D Position(X, Y);
            const auto Fields = FKalmalaWorldFieldSampler::Sample(Config, Position);
            const auto Biome = FKalmalaBiomeClassifier::Classify(Fields);
            Seen[static_cast<uint8>(Biome)] = true;
            TestEqual(TEXT("Sampling carries world rule version"), Fields.GeneratorRevision, Revision);
            TestEqual(TEXT("Identical peer identity reproduces biome"), Biome,
                FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, Position)));
            bDifferentSeed |= Biome != FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(OtherSeed, Position));
        }
        for (int32 Index = 0; Index < 7; ++Index)
            TestTrue(FString::Printf(TEXT("Revision %d contains biome %d"), Revision, Index), Seen[Index]);
        TestTrue(TEXT("Changing seed changes biome layout"), bDifferentSeed);
        const auto Start = FKalmalaWorldPlayerStartResolver::ResolveStartTransform(Config);
        TestEqual(TEXT("Both revisions retain a Meadow start"), FKalmalaBiomeClassifier::Classify(
            FKalmalaWorldFieldSampler::Sample(Config, FVector2D(Start.GetLocation()))), EKalmalaBiome::Meadows);
    }
    return true;
}
#endif
