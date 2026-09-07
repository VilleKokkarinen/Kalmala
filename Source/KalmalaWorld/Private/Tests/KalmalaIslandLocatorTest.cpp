#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaIslandLocator.h"
#include "KalmalaWorldPlayerStartResolver.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaIslandLocatorTest, "Kalmala.World.Ocean.IslandLocator",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaIslandLocatorTest::RunTest(const FString& Parameters)
{
    const FKalmalaWorldGenerationConfig Config{418, 4};
    const FVector2D Start(FKalmalaWorldPlayerStartResolver::ResolveStartTransform(Config).GetLocation());
    FVector2D Island;
    TestTrue(TEXT("Seeded terrain resolves a naturally isolated island"), FKalmalaIslandLocator::FindNearest(Config, Start, Island));
    FVector2D Repeated;
    TestTrue(TEXT("Same identity reproduces the island lookup"), FKalmalaIslandLocator::FindNearest(Config, Start, Repeated));
    TestEqual(TEXT("Island X is deterministic"), Island.X, Repeated.X);
    TestEqual(TEXT("Island Y is deterministic"), Island.Y, Repeated.Y);
    const FKalmalaOceanSample IslandSample = FKalmalaOceanSampler::Sample(Config, Island);
    TestTrue(TEXT("Island location is above the sea surface"), IslandSample.bIsValid && IslandSample.TerrainHeight >= 25.0f);
    return true;
}
#endif
