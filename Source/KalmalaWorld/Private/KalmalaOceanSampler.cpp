#include "KalmalaOceanSampler.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaWaterSurfaceMesh.h"
#include "KalmalaWorldPlayerStartResolver.h"

FKalmalaOceanSample FKalmalaOceanSampler::Sample(const FKalmalaWorldGenerationConfig& Config,
    const FVector2D Position)
{
    if (!Config.IsValid() || !FMath::IsFinite(Position.X) || !FMath::IsFinite(Position.Y)) return {};
    static thread_local FKalmalaWorldGenerationConfig CachedConfig;
    static thread_local FVector2D Origin;
    static thread_local bool bHasOrigin = false;
    if (!bHasOrigin || !(CachedConfig == Config))
    {
        Origin = FVector2D(FKalmalaWorldPlayerStartResolver::ResolveStartTransform(Config).GetLocation());
        CachedConfig = Config;
        bHasOrigin = true;
    }
    return Sample(Config, Position, Origin);
}

FKalmalaOceanSample FKalmalaOceanSampler::Sample(const FKalmalaWorldGenerationConfig& Config,
    const FVector2D Position, const FVector2D GridOrigin)
{
    if (!Config.IsValid() || !FMath::IsFinite(Position.X) || !FMath::IsFinite(Position.Y)
        || !FMath::IsFinite(GridOrigin.X) || !FMath::IsFinite(GridOrigin.Y)) return {};
    constexpr double Spacing = FKalmalaTerrainPatchLayout::PatchSize / FKalmalaWaterSurfaceMesh::CellsPerSide;
    const FVector2D Cell = (Position - GridOrigin) / Spacing;
    const FVector2D Base(FMath::FloorToDouble(Cell.X), FMath::FloorToDouble(Cell.Y));
    const double X = Cell.X - Base.X, Y = Cell.Y - Base.Y;
    const bool bUpper = X + Y > 1.0;
    const FVector2D Corner = GridOrigin + Base * Spacing;
    const double East = FKalmalaTerrainHeightSampler::SampleHeight(Config, Corner + FVector2D(Spacing, 0));
    const double North = FKalmalaTerrainHeightSampler::SampleHeight(Config, Corner + FVector2D(0, Spacing));
    const double Opposite = FKalmalaTerrainHeightSampler::SampleHeight(Config,
        Corner + (bUpper ? FVector2D(Spacing, Spacing) : FVector2D::ZeroVector));
    FKalmalaOceanSample Result;
    Result.TerrainHeight = bUpper
        ? East * (1.0 - Y) + North * (1.0 - X) + Opposite * (X + Y - 1.0)
        : East * X + North * Y + Opposite * (1.0 - X - Y);
    Result.WaterDepth = FMath::Max(0.0f, FKalmalaTerrainHeightSampler::SeaLevelWorldHeight - Result.TerrainHeight);
    Result.bIsValid = true;
    return Result;
}
