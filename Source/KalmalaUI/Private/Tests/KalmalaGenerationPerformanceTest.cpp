#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaMinimapViewModel.h"
#include "KalmalaMinimapRaster.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaLakeBasin.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaRegionalGeneration.h"
#include "KalmalaWorldPlayerStartResolver.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"
#include "Misc/Crc.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaGenerationPerformanceTest, "Kalmala.UI.Minimap.GenerationPerformance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaGenerationPerformanceTest::RunTest(const FString& Parameters)
{
    for (int32 Revision : {3, 4})
    {
        const FKalmalaWorldGenerationConfig Config{418, Revision};
        const FVector2D Start(FKalmalaWorldPlayerStartResolver::ResolveStartTransform(Config).GetLocation());
        uint32 Digest = 0;
        for (float Radius : {2500.0f, 5000.0f, 10000.0f})
        {
            const double Begin = FPlatformTime::Seconds();
            for (int32 Frame = 0; Frame < 3; ++Frame)
            {
                const auto Samples = UKalmalaMinimapViewModel::BuildTerrainSamples(Config,
                    Start + FVector2D(Frame * 123.25, -Frame * 76.5), Radius, 129);
                TestEqual(TEXT("Full resolution is preserved"), Samples.Num(), 129 * 129);
                for (const auto& S : Samples)
                {
                    Digest = FCrc::MemCrc32(&S.TerrainHeight, sizeof(S.TerrainHeight), Digest);
                    Digest = FCrc::MemCrc32(&S.bIsWater, sizeof(S.bIsWater), Digest);
                    Digest = FCrc::MemCrc32(&S.TerrainColour, sizeof(S.TerrainColour), Digest);
                }
            }
            AddInfo(FString::Printf(TEXT("Generation benchmark revision=%d radius=%.0f meanRefreshMs=%.3f digest=%u"),
                Revision, Radius, (FPlatformTime::Seconds() - Begin) * 1000 / 3, Digest));
        }
        TestEqual(TEXT("Exact pre-optimization terrain, water and colour fingerprint"), Digest,
            Revision == 3 ? 2794663019u : 3461612476u);
    }
    // Compare the batched presentation against the independent collision/water
    // query paths across both triangle orientations and negative coordinates.
    for (int32 Revision : {3, 4}) for (uint64 Seed : {418ull, 419ull})
    {
        const FKalmalaWorldGenerationConfig Config{Seed, Revision};
        const FVector2D Centre(-1234.25, 8765.5);
        const auto Samples = UKalmalaMinimapViewModel::BuildTerrainSamples(Config, Centre, 200000, 33);
        for (const auto& S : Samples)
        {
            const FVector2D P = Centre + S.MapPosition * 200000;
            const auto Ocean = FKalmalaOceanSampler::Sample(Config, P);
            const bool bWater = Ocean.IsWater() || FKalmalaLakeBasin::IsVisibleWater(Config, P);
            const auto Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, P));
            auto Colour = FKalmalaMinimapRaster::SampleBiomeTexture(bWater ? EKalmalaBiome::Ocean : Biome, P);
            if (bWater && Biome == EKalmalaBiome::ShimmeringLakes) Colour *= FLinearColor(1.3f, 1.8f, 1.6f);
            TestEqual(TEXT("Batched height matches collision triangle"), S.TerrainHeight, Ocean.TerrainHeight);
            TestEqual(TEXT("Batched water matches independent water queries"), S.bIsWater, bWater);
            TestEqual(TEXT("Batched colour matches independent biome query"), S.TerrainColour, Colour);
        }
    }
    return true;
}
#endif
