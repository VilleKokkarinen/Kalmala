#pragma once

#include "CoreMinimal.h"
#include "KalmalaTerrainHeightSampler.h"

/** Pure layout for a small invisible collision patch around a generated start. */
struct KALMALAWORLD_API FKalmalaTerrainPatchLayout
{
    static constexpr int32 TilesPerSide = 3;
    static constexpr float TileSize = 1000.0f;
    static constexpr float CollisionDepth = 200.0f;

    static FVector GetTileCenter(const FKalmalaWorldGenerationConfig& Config, const FVector2D PatchCenter, const int32 TileX, const int32 TileY)
    {
        const FVector2D TilePosition = PatchCenter + FVector2D(TileX * TileSize, TileY * TileSize);
        return FVector(TilePosition.X, TilePosition.Y, FKalmalaTerrainHeightSampler::SampleHeight(Config, TilePosition) - CollisionDepth);
    }
};
