#include "KalmalaLakeBasin.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaWorldPlayerStartResolver.h"
#include "Misc/ScopeLock.h"

namespace
{
    // Includes the diagonal used by the rendered terrain, not the opposite one.
    const FIntPoint Neighbours[] = {{1,0}, {-1,0}, {0,1}, {0,-1}, {1,-1}, {-1,1}};
}

bool FKalmalaLakeBasin::Find(const FIntPoint Start,
    TFunctionRef<TPair<float, bool>(FIntPoint)> Sample, TArray<FIntPoint>& WetVertices, const int32 Budget)
{
    WetVertices.Reset();
    TSet<FIntPoint> Visited;
    TArray<FIntPoint> Queue = {Start};
    Visited.Add(Start);
    bool bLakeSeed = false;
    for (int32 I = 0; I < Queue.Num(); ++I)
    {
        const auto Value = Sample(Queue[I]);
        if (Value.Key >= FKalmalaShimmeringLakeSampler::WaterSurfaceWorldHeight) continue;
        WetVertices.Add(Queue[I]);
        // Never suspend an inland sheet over a connection to sea level. Large
        // unresolved basins are conservatively omitted, never clipped to a tile.
        if (Value.Key <= 0.0f || WetVertices.Num() > Budget) return false;
        bLakeSeed |= Value.Value;
        for (const FIntPoint Offset : Neighbours)
        {
            const FIntPoint Next = Queue[I] + Offset;
            if (!Visited.Contains(Next)) { Visited.Add(Next); Queue.Add(Next); }
        }
    }
    return bLakeSeed && !WetVertices.IsEmpty();
}

bool FKalmalaLakeBasin::Contains(const FKalmalaWorldGenerationConfig& Config,
    const FVector2D Position, const FVector2D GridOrigin)
{
    if (Config.GeneratorRevision >= 3)
    {
        return FKalmalaRegionalGeneration::Sample(Config, Position).BasinWeight > 0;
    }
    // Cache only completed component decisions. Identity/origin are part of the
    // key; neither query order nor which streaming patch arrives first matters.
    static FCriticalSection Mutex;
    FScopeLock Lock(&Mutex);
    static FKalmalaWorldGenerationConfig CachedConfig;
    static FVector2D CachedOrigin(TNumericLimits<double>::Max());
    static TMap<FIntPoint, bool> Cache;
    const FVector2D Origin(FMath::Fmod(GridOrigin.X, GridSpacing), FMath::Fmod(GridOrigin.Y, GridSpacing));
    if (!(CachedConfig == Config) || !CachedOrigin.Equals(Origin, 0.001) || Cache.Num() > 131072)
    { Cache.Reset(); CachedConfig = Config; CachedOrigin = Origin; }
    const FIntPoint Key(FMath::RoundToInt((Position.X - Origin.X) / GridSpacing), FMath::RoundToInt((Position.Y - Origin.Y) / GridSpacing));
    if (const bool* Known = Cache.Find(Key)) return *Known;
    TArray<FIntPoint> Wet;
    const bool Result = Find(Key, [&](FIntPoint P)
    {
        const FVector2D World = Origin + FVector2D(P.X, P.Y) * GridSpacing;
        const auto Fields = FKalmalaWorldFieldSampler::Sample(Config, World);
        return TPair<float, bool>((Fields.Elevation - FKalmalaTerrainHeightSampler::SeaLevelElevation) * FKalmalaTerrainHeightSampler::WorldUnitsPerElevation,
            FKalmalaBiomeClassifier::Classify(Fields) == EKalmalaBiome::ShimmeringLakes);
    }, Wet);
    for (const FIntPoint P : Wet) Cache.Add(P, Result);
    Cache.Add(Key, Result);
    return Result;
}

bool FKalmalaLakeBasin::IsVisibleWater(const FKalmalaWorldGenerationConfig& Config, const FVector2D Position)
{
    static thread_local FKalmalaWorldGenerationConfig StartConfig;
    static thread_local FVector Start;
    static thread_local bool bHasStart = false;
    if (!bHasStart || !(StartConfig == Config))
    { Start = FKalmalaWorldPlayerStartResolver::ResolveStartTransform(Config).GetLocation(); StartConfig = Config; bHasStart = true; }
    const FVector2D Origin(Start.X, Start.Y);
    const FVector2D Cell = (Position - Origin) / GridSpacing;
    const FIntPoint Base(FMath::FloorToInt(Cell.X), FMath::FloorToInt(Cell.Y));
    const double X = Cell.X - Base.X, Y = Cell.Y - Base.Y;
    const bool Upper = X + Y > 1.0;
    const FIntPoint Corners[] = {Base + FIntPoint(1,0), Base + FIntPoint(0,1), Base + (Upper ? FIntPoint(1,1) : FIntPoint(0,0))};
    const double Weights[] = {Upper ? 1.0-Y : X, Upper ? 1.0-X : Y, Upper ? X+Y-1.0 : 1.0-X-Y};
    if (Config.GeneratorRevision >= 3)
    {
        double Depth = 0;
        for (int32 I = 0; I < 3; ++I)
        {
            const auto Region = FKalmalaRegionalGeneration::Sample(Config, Origin + FVector2D(Corners[I]) * GridSpacing);
            Depth += (Region.WaterLevel - Region.Height) * Weights[I];
        }
        return Depth > 0;
    }
    float Height = 0;
    bool Contained = false;
    for (int32 I=0; I<3; ++I)
    {
        const FVector2D P = Origin + FVector2D(Corners[I].X, Corners[I].Y) * GridSpacing;
        const float H = FKalmalaTerrainHeightSampler::SampleHeight(Config, P);
        Height += H * Weights[I];
        if (H < FKalmalaShimmeringLakeSampler::WaterSurfaceWorldHeight) Contained |= Contains(Config, P, Origin);
    }
    return Height < FKalmalaShimmeringLakeSampler::WaterSurfaceWorldHeight && Contained;
}
