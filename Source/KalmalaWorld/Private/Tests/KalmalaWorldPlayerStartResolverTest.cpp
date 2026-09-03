#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaBiomeClassifier.h"
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

    Config.WorldSeed = 419;
    const FTransform DifferentSeedStart = FKalmalaWorldPlayerStartResolver::ResolveStartTransform(Config);
    TestFalse(TEXT("A different seed resolves to a different player start"), FirstStart.Equals(DifferentSeedStart));
    return true;
}

#endif
