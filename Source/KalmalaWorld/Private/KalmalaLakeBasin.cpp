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
    return FKalmalaRegionalGeneration::Sample(Config, Position).BasinWeight > 0;
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
    double Depth = 0;
    for (int32 I = 0; I < 3; ++I)
    {
        const auto Region = FKalmalaRegionalGeneration::Sample(Config, Origin + FVector2D(Corners[I]) * GridSpacing);
        Depth += (Region.WaterLevel - Region.Height) * Weights[I];
    }
    return Depth > 0;
}
