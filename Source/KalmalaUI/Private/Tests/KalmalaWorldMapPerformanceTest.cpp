#if WITH_DEV_AUTOMATION_TESTS

#include "Async/Async.h"
#include "KalmalaMinimapRaster.h"
#include "KalmalaMinimapViewModel.h"
#include "KalmalaWorldGenerationConfig.h"
#include "Misc/AutomationTest.h"
#include "HAL/PlatformTime.h"

namespace KalmalaWorldMapBudget
{
    constexpr int32 TileSamplesPerAxis = 33;
    constexpr int32 MaxCachedTiles = 64;
    constexpr double MaximumTileSeconds = 0.25;
    constexpr double MaximumMinimapSecondsWhileTilesRun = 1.50;

    TArray<FColor> BuildTile(const FKalmalaWorldGenerationConfig& Config, const FIntPoint TileCoordinate)
    {
        const FVector2D Centre = (FVector2D(TileCoordinate) + FVector2D(0.5f)) * 10000.0f;
        return FKalmalaMinimapRaster::BuildPixels(UKalmalaMinimapViewModel::BuildTerrainSamples(
            Config, Centre, FVector2D(5000.0f), FIntPoint(TileSamplesPerAxis, TileSamplesPerAxis)),
            FIntPoint(TileSamplesPerAxis, TileSamplesPerAxis));
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWorldMapPerformanceTest, "Kalmala.UI.WorldMap.PerformanceBudget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWorldMapPerformanceTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 418;
    Config.GeneratorRevision = 4;

    const SIZE_T TilePayloadBytes = KalmalaWorldMapBudget::TileSamplesPerAxis * KalmalaWorldMapBudget::TileSamplesPerAxis * sizeof(FColor);
    const SIZE_T MaximumCachePayloadBytes = KalmalaWorldMapBudget::MaxCachedTiles * TilePayloadBytes;
    TestEqual(TEXT("Expanded-map cache has a bounded CPU pixel payload"), MaximumCachePayloadBytes, SIZE_T(278784));

    const double TileStart = FPlatformTime::Seconds();
    const TArray<FColor> Tile = KalmalaWorldMapBudget::BuildTile(Config, FIntPoint(0, 0));
    const double TileSeconds = FPlatformTime::Seconds() - TileStart;
    TestEqual(TEXT("Expanded-map tile keeps its fixed sample payload"), Tile.Num(), 1089);
    TestTrue(TEXT("Expanded-map tile worker stays within its refresh budget"), TileSeconds <= KalmalaWorldMapBudget::MaximumTileSeconds);

    TArray<TFuture<TArray<FColor>>> TileWorkers;
    for (const FIntPoint Coordinate : { FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(1, 0), FIntPoint(0, -1) })
    {
        TileWorkers.Add(Async(EAsyncExecution::ThreadPool, [Config, Coordinate]() { return KalmalaWorldMapBudget::BuildTile(Config, Coordinate); }));
    }
    const double MinimapStart = FPlatformTime::Seconds();
    const TArray<FColor> MinimapPixels = FKalmalaMinimapRaster::BuildPixels(UKalmalaMinimapViewModel::BuildTerrainSamples(
        Config, FVector2D::ZeroVector, FVector2D(10000.0f), FIntPoint(129, 129)), FIntPoint(129, 129));
    const double MinimapSeconds = FPlatformTime::Seconds() - MinimapStart;
    for (TFuture<TArray<FColor>>& Worker : TileWorkers)
    {
        const TArray<FColor> WorkerPixels = Worker.Get();
        TestEqual(TEXT("Concurrent map worker completes its bounded tile"), WorkerPixels.Num(), 1089);
    }
    TestEqual(TEXT("Companion minimap retains its full local raster while map tiles run"), MinimapPixels.Num(), 129 * 129);
    TestTrue(TEXT("Companion minimap worker stays responsive while expanded-map tiles run"), MinimapSeconds <= KalmalaWorldMapBudget::MaximumMinimapSecondsWhileTilesRun);
    AddInfo(FString::Printf(TEXT("World-map budget: tile=%.3f ms, minimap-with-four-tiles=%.3f ms, cache-pixels=%llu bytes."),
        TileSeconds * 1000.0, MinimapSeconds * 1000.0, static_cast<uint64>(MaximumCachePayloadBytes)));
    return true;
}

#endif
