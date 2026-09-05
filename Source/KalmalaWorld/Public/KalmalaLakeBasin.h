#pragma once

#include "KalmalaWorldFieldSampler.h"

/** Presentation-only enclosure check on the terrain triangle lattice. */
struct KALMALAWORLD_API FKalmalaLakeBasin
{
    static constexpr float GridSpacing = 125.0f;
    static constexpr int32 MaxBasinVertices = 8192;
    // Sample returns terrain height and whether this vertex seeds the lake biome.
    static bool Find(const FIntPoint Start, TFunctionRef<TPair<float, bool>(FIntPoint)> Sample,
        TArray<FIntPoint>& WetVertices, int32 Budget = MaxBasinVertices);
    static bool Contains(const FKalmalaWorldGenerationConfig& Config, FVector2D Position, FVector2D GridOrigin);
    static bool IsVisibleWater(const FKalmalaWorldGenerationConfig& Config, FVector2D Position);
};
