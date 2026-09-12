#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaBiomeClassifier.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaBiomeClassifierTest, "Kalmala.World.Biomes.TerrainSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaBiomeClassifierTest::RunTest(const FString& Parameters)
{
    for (uint64 Seed : {418ull, 419ull})
    {
        const FKalmalaWorldGenerationConfig Config{Seed};
        for (int32 I = 0; I < 512; ++I)
        {
            const double Angle = I * 2.3999632297;
            const FVector2D P = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * (1600000.0 * (I + 1) / 512);
            auto Fields = FKalmalaWorldFieldSampler::Sample(Config, P);
            const auto Biome = FKalmalaBiomeClassifier::Classify(Fields);
            TestEqual(TEXT("Classifier uses current shared rules"), uint8(Biome), FKalmalaRegionalGeneration::Sample(Fields).Biome);
            if (Fields.Elevation <= .22f) TestEqual(TEXT("Sea floor is Ocean"), Biome, EKalmalaBiome::Ocean);
            Fields.Flora = 1 - Fields.Flora;
            TestEqual(TEXT("Local flora does not change broad biome"), FKalmalaBiomeClassifier::Classify(Fields), Biome);
        }
    }
    return true;
}
#endif
